import uasyncio as asyncio  # type:ignore
import ujson  # type:ignore
from machine import Pin  # type: ignore
import time

import commands
import protocol
import setup
import SSDPTools
import TCPTools
import wifi_tools as wt
from boot import *

def run():
    # -------------------- #
    #   SETUP THE DEVICE
    # -------------------- #

    config = setup.readConfig(CONFIG_FILE)

    WIFI_INDICATOR_PIN_NUM = config.get("wifiIndicatorPin", None)
    WIFI_INDICATOR_PIN = Pin(WIFI_INDICATOR_PIN_NUM, Pin.OUT) if WIFI_INDICATOR_PIN_NUM is not None else None

    i2cSDA = config.get("i2cBus", {}).get("sdaPin")
    i2cSCL = config.get("i2cBus", {}).get("sclPin")
    i2cFreq = config.get("i2cBus", {}).get("frequency_Hz")
    i2cBus = setup.setupI2C(i2cSCL, i2cSDA, i2cFreq)

    devices = i2cBus.scan()
    adcs = setup.makeI2CDevices(i2cBus, devices)
    print("I2C devices found at:", [hex(d) for d in devices])

    wlan = wt.connectWifi(WIFI_SSID, WIFI_PASSWORD)
    if wlan and WIFI_INDICATOR_PIN is not None:
        WIFI_INDICATOR_PIN.on()

    config_json = ujson.dumps(config).encode("utf-8")

    sensor_list, control_list = setup.setupDeviceFromConfig(config, adcs)
    
    try:
        asyncio.run(main(sensor_list, control_list, adcs, config_json))
    except KeyboardInterrupt:
        print("Stopped.")


async def main(sensor_list, control_list, adcs, config_json):

    for adc in adcs:
        adc.updatingInternalTemp = True
        asyncio.create_task(adc._updateInternalTemp()) # Update the internal temperature at boot
        adc.prevInternalTemp_ms = time.ticks_ms() #type: ignore

    while True:
        # Phase 1: Discover server via SSDP
        print("Discovering server...")
        server_ip = await SSDPTools.discoverServer()

        # Phase 2: Connect to server
        print(f"Connecting to {server_ip}:{TCPTools.TCP_PORT}...")
        try:
            sock = TCPTools.connectToServer(server_ip)
        except OSError as e:
            print(f"Connection to server failed: {e}")
            await asyncio.sleep(1)
            continue

        print("Connected.")
        state = SessionState()

        # Phase 3: Send CONFIG as first packet
        sock.sendall(protocol.make_config(state.next_seq(), state.ts_offset, config_json))
        print("CONFIG sent.")

        # Phase 4: Packet processing loop
        try:
            while True:
                packet = await asyncio.wait_for_ms(TCPTools.recvPacket(sock),
                                                   WATCHDOG_TIMEOUT_MS)
                ptype, pseq, pts, payload = packet

                if ptype == protocol.PT_ESTOP:
                    print("ESTOP received!")
                    commands.executeEstop(control_list)
                    commands.stopStream()

                elif ptype == protocol.PT_TIMESYNC:
                    # We store the server time offset to generate timestamps in the server's time base
                    state.ts_offset = pts - time.ticks_ms() # type: ignore # MicroPython ticks_ms returns an int32
                    sock.sendall(protocol.make_ack(state.next_seq(), state.ts_offset, ptype, pseq))

                elif ptype == protocol.PT_CONTROL:
                    command_id, command_state = protocol.parse_control(payload)
                    err = commands.executeControl(control_list, command_id, command_state)
                    if err == protocol.ERR_NONE:
                        sock.sendall(protocol.make_ack(state.next_seq(), state.ts_offset, ptype, pseq))
                    else:
                        sock.sendall(protocol.make_nack(state.next_seq(), state.ts_offset, ptype, pseq, err))

                elif ptype == protocol.PT_STATUS_REQUEST:
                    sock.sendall(protocol.make_status(state.next_seq(), state.ts_offset, state.device_status))

                elif ptype == protocol.PT_STREAM_START:
                    freq = protocol.parse_stream_start(payload)
                    commands.startStream(sensor_list, sock, freq, state)
                    sock.sendall(protocol.make_ack(state.next_seq(), state.ts_offset, ptype, pseq))

                elif ptype == protocol.PT_STREAM_STOP:
                    commands.stopStream()
                    sock.sendall(protocol.make_ack(state.next_seq(), state.ts_offset, ptype, pseq))

                elif ptype == protocol.PT_GET_SINGLE:
                    readings = commands.readAllSensors(sensor_list)
                    sock.sendall(protocol.make_data(state.next_seq(), state.ts_offset, readings))

                elif ptype == protocol.PT_HEARTBEAT:
                    sock.sendall(protocol.make_ack(state.next_seq(), state.ts_offset, ptype, pseq))

                elif ptype in (protocol.PT_ACK, protocol.PT_NACK):
                    pass  # Server acknowledgments, ignore

                else:
                    sock.sendall(protocol.make_nack(
                        state.next_seq(), state.ts_offset, ptype, pseq, protocol.ERR_UNKNOWN_TYPE))

        except (asyncio.TimeoutError, OSError) as e:
            print(f"Connection lost: {e}")
            commands.stopStream()
            try:
                sock.close()
            except Exception:
                pass
            await asyncio.sleep(1)
            continue

run()