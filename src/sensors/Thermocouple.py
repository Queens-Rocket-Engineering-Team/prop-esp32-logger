# noqa: INP001 -- Implicit namespace doesn't matter here
from ADS112C04 import ADS112C04  # Importing the ADS112C04 class for external ADCs
from sensors.Sensor import Sensor  # Importing the base Sensor class
import uasyncio as asyncio  # type:ignore # uasyncio is the micropython asyncio library
import time
from math import exp


class Thermocouple(Sensor):
    """
    Class for reading thermocouple data from an ADC.
    """
    ADC: ADS112C04
    def __init__ (self,
                  name: str,
                  ADCIndex: int,
                  ADC: ADS112C04, # ADC must not be None for thermocouples
                  highPin: int,
                  lowPin: int,
                  units: str,
                  thermoType: str,
                  ):

        super().__init__(
            name=name,
            ADCIndex=ADCIndex,
            ADC=ADC,
            highPin=highPin,
            lowPin=lowPin,
            units=units,
        )

        self.pgaGain = 64  # Default PGA gain for the ADC, can be set later.

        if self.units not in ["V", "C"]:
            raise ValueError(f"Invalid units specified: {self.units}. Valid units are 'V' and 'C'.")

        self.type = thermoType
        self.prevTemp = 0

        # Temperature to voltage coefficients

        self.cP = (-1.76e-02,
                   3.89e-02,
                   1.86e-05,
                   -9.95e-08,
                   3.18e-10,
                   -5.61e-13,
                   5.61e-16,
                   -3.20e-19,
                   9.72e-23,
                   -1.21e-26)
        
        self.a = (1.185976e-01,
                  -1.183432e-04,
                  1.269686e+02)
        
        self.cN = (0.00e+00,
                   3.95e-02,
                   2.36e-05,
                   -3.29e-07,
                   -4.99e-09,
                   -6.75e-11,
                   -5.74e-13,
                   -3.11e-15,
                   -1.05e-17,
                   -1.99e-20,
                   -1.63e-23)
        
        # Voltage to temperature coefficients

        self.d1 = (0.0000000E+00,
                   2.5173462E+01,
                   -1.1662878E+00,
                   -1.0833638E+00,
                   -8.9773540E-01,
                   -3.7342377E-01,
                   -8.6632643E-02,
                   -1.0450598E-02,
                   -5.1920577E-04)
        
        self.d2 = (0.000000E+00,
                   2.508355E+01,
                   7.860106E-02,
                   -2.503131E-01,
                   8.315270E-02,
                   -1.228034E-02,
                   9.804036E-04,
                   -4.413030E-05,
                   1.057734E-06,
                   -1.052755E-08)
        
        self.d3 = (-1.318058E+02,
                   4.830222E+01,
                   -1.646031E+00,
                   5.464731E-02,
                   -9.650715E-04,
                   8.802193E-06,
                   -3.110810E-08)


    def takeData(self, unit="DEF") -> float | None: # Currently returns differential voltage reading. DEF for default.
        """Take a reading from the thermocouple and add it to the data list."""
        if self.ADC and self.ADC.pgaGain != self.pgaGain:
            self.ADC.setPGA(self.pgaGain)

        vReading = self._getVoltageReading()
        if vReading is None:
            return None
        CJCTemp = self.ADC.getInternalTemp()

        if unit == "DEF":
            readingUnit = self.units
        else:
            readingUnit = unit

        if readingUnit == "V":
            return vReading
        if readingUnit == "C":
            CJCVoltage = self._convertTemperatureToVoltage(CJCTemp) # Convert C to mV
            thermocoupleVoltage = vReading*1e3 + CJCVoltage
            temperature = self._convertVoltageToTemperature(thermocoupleVoltage)# Conversion from mV to C
            return temperature

        raise ValueError(f"Invalid unit specified: {readingUnit}. Valid units are 'V' and 'C'.")

    def _convertTemperatureToVoltage(self, temperature: float) -> float:
        '''
        Returns the equivalent voltage difference in mV for a given
        thermocouple temperature. This function currently only works
        for K-Type thermocouples.
        Equations from the NIST Temperature Scale Database:
        https://its90.nist.gov/RefFunctions
        '''
        if temperature >= 0:
            voltage = self._polyEval(temperature, self.cP)
            voltage += self.a[0] * exp(self.a[1] * (temperature - self.a[2])**2)
            return voltage
        else:
            voltage = voltage = self._polyEval(temperature, self.cN)
            return voltage
    

    def _convertVoltageToTemperature(self, voltage: float) -> float:
        '''
        Returns the equivalent temperature in C for a
        given thermocouple voltage difference in mV.
        This function currently only works
        for K-Type thermocouples.
        Equations from the NIST Temperature Scale Database:
        https://its90.nist.gov/InvFunctions
        '''
        if voltage < 0:
            temperature = self._polyEval(voltage, self.d1)
            return temperature
        elif voltage < 20.644:
            temperature = self._polyEval(voltage, self.d2)
            return temperature
        else:
            temperature = self._polyEval(voltage, self.d3)
            return temperature


    def _polyEval(self, x: float, coeffs: tuple) -> float:
        '''Hornier's Method'''
        y = 0.0
        for coeff in reversed(coeffs):
            y = y * x + coeff

        return y