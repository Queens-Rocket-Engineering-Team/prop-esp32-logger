import time
import protocol

CONFIG_FILE = "ESPConfig.json"
WATCHDOG_TIMEOUT_MS = 15000

WIFI_SSID = "BELL042"
WIFI_PASSWORD = "177522FF5F55"

class SessionState:
    def __init__(self):
        self.seq = 0
        self.ts_offset = 0
        self.device_status = protocol.DS_ACTIVE

    def next_seq(self):
        s = self.seq
        self.seq = (self.seq + 1) & 0xFF
        return s

time.sleep(0.5) # Allow time for USB to initialize
print('Boot finished')