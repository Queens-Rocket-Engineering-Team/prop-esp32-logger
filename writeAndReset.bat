mpremote fs cp .\src\main.py :/
mpremote fs cp .\src\setup.py :/
mpremote fs cp .\configs\PandaV3\ESPConfig.json :/

mpremote fs cp .\src\sensors\Thermocouple.py :/sensors/
mpremote fs cp .\src\ADS112C04.py :/

@REM mpremote fs cp .\src\SSDPTools.py :/
@REM mpremote fs cp .\src\TCPTools.py :/
mpremote fs cp .\src\commands.py :/

mpremote reset