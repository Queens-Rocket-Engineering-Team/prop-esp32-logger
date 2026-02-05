import ujson  # type:ignore # ujson and machine are micropython libraries
from machine import (  # type: ignore # machine is a micropython library
    Pin,  # type: ignore # machine is a micropython library
    SoftI2C,  # type: ignore # SoftI2C is a micropython library for I2C communication
)

from ADS112C04 import ADS112C04  # type: ignore # Importing the ADS112 class from ADS112
from Control import Control  # type: ignore # Importing the Valve class from Valve.py
from sensors.Current import Current  # type: ignore
from sensors.LoadCell import LoadCell  # type: ignore
from sensors.PressureTransducer import PressureTransducer  # type: ignore
from sensors.Thermocouple import Thermocouple  # type: ignore # don't need __init__ for micropython


# --------------------- #
#    I2C FUNCTIONS
# --------------------- #

def setupI2C(scl: int, sda: int, freq: int) -> SoftI2C: # Return an I2C bus object
    """Set up the I2C bus with the correct pins and frequency.

    This function doesn't need input parameters because the pins and frequency are set by the hardware configuration,
    and will never need to change.
    The SCL pin is GPIO 6 on the ESP32, and the SDA pin is GPIO 7 on the ESP32.
    """

    # I2C bus 1, SCL pin 16, SDA pin 15, frequency 100kHz
    i2cBus = SoftI2C(scl=Pin(scl), sda=Pin(sda), freq=freq)
    return i2cBus

def makeI2CDevices(softI2CBus: SoftI2C.SoftI2C, addresses: list[int]) -> list[ADS112C04]:
    """Create a list of ADS112 devices on the I2C bus."""
    devices = []
    for addr in addresses:  # Create 4 devices
        device = ADS112C04(softI2CBus, addr)
        devices.append(device)
    return devices

def readConfig(filePath: str) -> dict:  # type: ignore
    try:
        with open(filePath, "r") as file:
            config = ujson.load(file)
            return config
    except Exception as e:
        print(f"Failed to read config file: {e}")
        return {}

def setupDeviceFromConfig(config, adcs):
        """Initialize all devices and sensors from the config file.

        ADC index 0 indicates the sensor is connected directly to the ESP32. Any other index indicates
        connection to an external ADC.

        Returns (sensors_dict, controls_dict, sensor_list, control_list).
        sensor_list is ordered: thermocouples, pressure transducers, load cells, current sensors.
        Index in sensor_list = sensor_id for DATA packets.
        Index in control_list = command_id for CONTROL packets.
        """

        sensors = {}
        controls = {}
        sensor_list = []
        control_list = []

        print(f"Initializing device: {config.get('deviceName', 'Unknown Device')}")
        deviceType = config.get("deviceType", "Unknown")

        if deviceType == "Sensor Monitor":
            sensorInfo = config.get("sensorInfo", {})

            for name, details in sensorInfo.get("thermocouples", {}).items():
                adc = None
                if details["ADCIndex"] > 0 and details["ADCIndex"] <= 4:
                    adc = adcs[details["ADCIndex"] - 1]

                s = Thermocouple(
                    name=name,
                    ADCIndex=details["ADCIndex"],
                    ADC=adc,
                    highPin=details["highPin"],
                    lowPin=details["lowPin"],
                    units=details["units"],
                    thermoType=details["type"],
                )
                sensors[name] = s
                sensor_list.append(s)

            for name, details in sensorInfo.get("pressureTransducers", {}).items():
                adc = None
                if details["ADCIndex"] > 0 and details["ADCIndex"] <= 4:
                    adc = adcs[details["ADCIndex"] - 1]

                s = PressureTransducer(
                    name=name,
                    ADCIndex=details["ADCIndex"],
                    ADC=adc,
                    pinNumber=details["pin"],
                    maxPressure_PSI=details["maxPressure_PSI"],
                    units=details["units"],
                )
                sensors[name] = s
                sensor_list.append(s)

            for name, details in sensorInfo.get("loadCells", {}).items():
                adc = None
                if details["ADCIndex"] > 0 and details["ADCIndex"] <= 4:
                    adc = adcs[details["ADCIndex"] - 1]

                s = LoadCell(
                    name=name,
                    ADCIndex=details["ADCIndex"],
                    ADC=adc,
                    highPin=details["highPin"],
                    lowPin=details["lowPin"],
                    loadRating_N=details["loadRating_N"],
                    excitation_V=details["excitation_V"],
                    sensitivity_vV=details["sensitivity_vV"],
                    units=details["units"],
                )
                sensors[name] = s
                sensor_list.append(s)

            for name, details in sensorInfo.get("current", {}).items():
                adc = None
                if details["ADCIndex"] > 0 and details["ADCIndex"] <= 4:
                    adc = adcs[details["ADCIndex"] - 1]

                s = Current(
                    name=name,
                    ADCIndex=details["ADCIndex"],
                    ADC=adc,
                    pinNumber=details["pin"],
                    shuntResistor_Ohms=details["shuntResistor_Ohms"],
                    csaGain=details["csaGain"],
                )
                sensors[name] = s
                sensor_list.append(s)

            for name, details in config.get("controls", {}).items():
                pin = details.get("pin", None)
                defaultState = details.get("defaultState")

                c = Control(name=name.upper(),
                            controlType=details.get("type").upper(),
                            pin=pin,
                            defaultState=defaultState)
                controls[name.upper()] = c
                control_list.append(c)

            return sensors, controls, sensor_list, control_list

        if deviceType == "Unknown":
            raise ValueError("Device type not specified in config file")

        return {}, {}, [], []
