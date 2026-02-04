# BASE MICROPYTHON BOOT.PY-----------------------------------------------|
# # This is all micropython code to be executed on the esp32 system level and doesn't require a __init__.py file

# This file is executed on every boot (including wake-boot from deep sleep)
#import esp
#esp.osdebug(None)
#import webrepl
#webrepl.start()
#------------------------------------------------------------------------|


import ujson  # type:ignore # noqa: I001# ujson and machine are micropython libraries
import uasyncio as asyncio  # type:ignore # uasyncio is the micropython asyncio library
import socket  # type:ignore # socket is a micropython library

import wifi_tools as wt
import setup
from machine import Pin  # type: ignore # machine is a micropython library

import SSDPTools
import TCPTools
import commands


INIT = 0        # Device is initializing
WAITING = 1     # Device is waiting for a master to connect
READY = 2       # Device has a master connected and is waiting for commands
STREAMING = 3   # Device is streaming data to a master
ERROR = 4       # Device has encountered an error. Will default to WAITING state after error is resolved.

CONFIG_FILE = "ESPConfig.json"
TCP_PORT = 50000  # Port that I've chosen for the TCP server to listen on. This is the port that the master will connect to.

SAFE24_PIN: Pin | None = None  # Pin for the SAFE24 switch, if used. Set to None if not used.
IGNITOR_PIN: Pin | None = None  # Pin for the ignitor switch, if used. Set to None if not used.

UDPRequests = ("SEARCH", # Message received when server is searching for client sensors
               )

TCPRequests = ("SREAD", # Reads a single value from all sensors
               "CREAD", # Continuously reads data from all sensors until STOP received
               "STOP", # Stops continuous reading
               "STAT", # Returns number of sensors and types
               )

# --------------------- #
#    ASYNC FUNCTIONS
# --------------------- #

async def sendConfig(socket: socket.socket,
                     address: str,
                     config: dict) -> None:
    """Send the configuration file to the client."""
    try:
            # Convert the config file to a JSON string so it can have the encode method called on it
            jStringConfig = ujson.dumps(config)
            confString = "CONF" + jStringConfig + "\n" # Add a header to the config file so the client knows what it is receiving

            # Currently TCP block size is 2048 bytes, this can be changed if needed but config files are small right now.
            # This is meant as a warning block of code if we start having larger config files.
            if len(confString) > 2048:
                raise ValueError("ERROR: Config file too large to send in one TCP block!!.")


            socket.sendto(confString.encode("utf-8"), address)
            print(f"Sent conf string: {confString}")

    except Exception as e:
        print(f"Error sending config file: {e}")


# -------------------- #
#   SETUP THE DEVICE
# -------------------- #
state = INIT  # Device is initializing
print("State = INIT")

config = setup.readConfig(CONFIG_FILE)

# Set up the WiFi indicator pin if specified in the config file
WIFI_INDICATOR_PIN_NUM = config.get("wifiIndicatorPin", None)
WIFI_INDICATOR_PIN = Pin(WIFI_INDICATOR_PIN_NUM, Pin.OUT) if WIFI_INDICATOR_PIN_NUM is not None else None

# I2C Setup - Need to establish
i2cSDA = config.get("i2cBus", {}).get("sdaPin")
i2cSCL = config.get("i2cBus", {}).get("sclPin")
i2cFreq = config.get("i2cBus", {}).get("frequency_Hz")
i2cBus = setup.setupI2C(i2cSCL, i2cSDA, i2cFreq)  # Set up the I2C bus

# Scan for all I2C devices on the bus and set them up as ADS112C04 ADCs
devices = i2cBus.scan()
adcs = setup.makeI2CDevices(i2cBus, devices)  # Create a list of ADS112 devices on the I2C bus

print("I2C devices found at following addresses:", [hex(device) for device in devices]) # Print the addresses of the devices found on the bus

# Networking setup
wlan = wt.connectWifi("Hous-fi", "nothomeless")
if wlan and WIFI_INDICATOR_PIN is not None:
    WIFI_INDICATOR_PIN.on()

ipAddress   = wlan.ifconfig()[0]  # Get the IP address of the ESP32
tcpListenerSocket   = TCPTools.createListenerTCPSocket()

state = WAITING  # Device is waiting for a master to connect
print("State = WAITING")

def run() -> None:
    """Run the main event loop."""

    try:
        # Start the main event loop with the initial state
        asyncio.run(main(state))
    except KeyboardInterrupt:
        print("Server stopped gracefully...")


async def main(state: int) -> None:
    global tcpListenerSocket  # noqa: PLW0603

    sensors, controls = setup.setupDeviceFromConfig(config, adcs)  # Initialize sensors from config file

    # Set up some variables for the server state
    clientSock = None

    # Fire off the SSDP listener task to send out pings if it receives a discovery message.
    ssdpTask = asyncio.create_task(SSDPTools.waitForDiscovery())

    # Let whatever tripped an error state set an error message for the error state event loop to catch.
    errorMessage = ""

    print("Starting server...")

    while True:

        try:

            # ------------- #
            # WAITING STATE #
            # ------------- #
            try:
                if state == WAITING:
                    print("Waiting for a master to connect...")

                    # Ensure we have a listening socket
                    if tcpListenerSocket is None:
                        tcpListenerSocket = TCPTools.createListenerTCPSocket()

                    try:
                        # Wait for incoming TCP connection
                        clientSock, clientAddr = await TCPTools.waitForConnection(tcpListenerSocket)

                        # Send config to newly connected client
                        jStringConfig = ujson.dumps(config)
                        confString = "CONF" + jStringConfig + "\n"
                        clientSock.sendall(confString.encode("utf-8"))
                        print(f"Sent config to client at {clientAddr}")

                        state = READY
                        print("State = READY")

                    except OSError as e:
                        errorMessage = f"Network error: {e}"
                        state = ERROR
                        continue
                    except Exception as e:
                        errorMessage = f"Error in WAITING state: {e}"
                        state = ERROR
                        continue
            except Exception as e:
                state = ERROR
                errorMessage = f"Unexpected error in waiting state: {e}"
                continue

            # ------------- #
            #  READY STATE  #
            # ------------- #
            if state == READY and clientSock is not None:

                try:
                    cmds = await TCPTools.waitForCommand(clientSock)

                    for cmd in cmds:
                        if not cmd:
                            errorMessage = "Empty message received. Server closed connection or there was an error."
                            state = ERROR
                            continue

                        response: str = ""  # Reset response for each command

                        print(f"Received command: {cmd}")
                        cmdParts = cmd.split(" ")
                        print(f"Command parts: {cmdParts}")
                        if cmdParts[0] == "GETS": response = await commands.gets(sensors)  # Get a single reading from each sensor
                        if cmdParts[0] == "STREAM": commands.strm(sensors, clientSock, cmdParts[1:])  # Start streaming data from sensors
                        if cmdParts[0] == "STOP": response = commands.stopStrm()  # Stop streaming data from sensors
                        if cmdParts[0] == "CONTROL": response = commands.actuateControl(controls, cmdParts[1:])  # Open or close a control
                        if cmdParts[0] == "STATUS" : response = commands.getStatus(controls)

                        if response:
                            message = f"{cmdParts[0]} {response}\n"
                            clientSock.sendall(message.encode("utf-8"))
                            print(f"Sent response: {message}")

                except TCPTools.ConnectionClosedError:
                    state = ERROR
                    errorMessage = "Connection closed by client."
                    continue

            # ------------- #
            #  ERROR STATE  #
            # ------------- #

            # The error state will handle all the potential cleanup so we can just reset the state to WAITING afterwards.
            # Any error that we are catching should probably trigger an error state, so we can reset the device and try again.
            if state == ERROR:
                print(f"ERROR STATE: {errorMessage}\nResetting to WAITING state.")
                if clientSock:
                    clientSock.close()
                    clientSock = None

                # Reset the sockets and variables to kill any existing connections or attempts to connect.
                clientSock = None

                # Kill stream task if it exists
                commands.stopStrm()

                state = WAITING
                errorMessage = ""  # Reset the error message

                await asyncio.sleep(0)  # Yield to event loop

        except KeyboardInterrupt:
            print("Server stopped by user.")
            if clientSock:
                clientSock.close()
            ssdpTask.cancel()  # Cancel the SSDP task
            if tcpListenerSocket:
                tcpListenerSocket.close()  # Close the TCP listener socket
            break
