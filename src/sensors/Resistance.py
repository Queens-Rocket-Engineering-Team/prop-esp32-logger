from ADS112C04 import ADS112C04
from sensors.Sensor import Sensor
import time


class Resistance(Sensor):
    """Class for taking resistance readings with IDAC from an ADS112C04 ADC."""

    ADC: ADS112C04
    def __init__(self,
                 name: str,
                 ADCIndex: int,
                 ADC: ADS112C04,  # ADC must not be None for thermocouples
                 pinNumber: int,
                 injectedCurrent: int, # Injected current must be specific numbers listed in the ADS112C04 datasheet (in uA)
    ):
        super().__init__(
            name=name,
            ADCIndex=ADCIndex,
            ADC=ADC,
            highPin=pinNumber,
            lowPin=-1,
            units="Ohms",
        )
        self.pgaGain = -1 # Single-ended
        self.injectedCurrent = injectedCurrent

    def takeData(self, unit: str = "DEF") -> float:
        """Take a reading from the resistance sense and add it to the data list."""
        if self.ADC and self.ADC.pgaGain != self.pgaGain:
            self.ADC.setPGA(self.pgaGain)

        self.ADC.setCurrentSource(1, self.highPin, self.injectedCurrent)
        time.sleep(0.1)
        vReading = self._getVoltageReading()

        try:
            if unit == "DEF":
                readingUnit = self.units
            else:
                readingUnit = unit

            if readingUnit == "Ohms":
                resistance = self._convertVoltageToResistance(vReading)
                return resistance

            if readingUnit == "V":
                return vReading

            raise ValueError(f"Invalid unit specified: {readingUnit}. Valid units are 'Ohms' and 'V'.")
        finally:
            self.ADC.setCurrentSource(1, self.highPin, 0)


    def _convertVoltageToResistance(self, voltage: float) -> float:
        """Convert the voltage reading to resistance in ohms."""
        print(voltage)
        rShort = 47.91259768
        resistance = voltage/(self.injectedCurrent*1e-6) - rShort # Subtract ~47 ohms from RC input filter
        return resistance
