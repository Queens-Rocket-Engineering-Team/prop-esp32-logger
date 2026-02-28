import struct
import time


VERSION = 0x02

# Packet types - Server -> Device
PT_ESTOP          = 0x00
PT_DISCOVERY      = 0x01
PT_TIMESYNC       = 0x02
PT_CONTROL        = 0x03
PT_STATUS_REQUEST = 0x04
PT_STREAM_START   = 0x05
PT_STREAM_STOP    = 0x06
PT_GET_SINGLE     = 0x07
PT_HEARTBEAT      = 0x08

# Packet types - Device -> Server
PT_CONFIG = 0x10
PT_DATA   = 0x11
PT_STATUS = 0x12
PT_ACK    = 0x13
PT_NACK   = 0x14

# Device status
DS_INACTIVE    = 0x00
DS_ACTIVE      = 0x01
DS_ERROR       = 0x02
DS_CALIBRATING = 0x03

# Control state
CS_CLOSED = 0x00
CS_OPEN   = 0x01
CS_ERROR  = 0xFF

# Units
UNIT_VOLTS        = 0x00
UNIT_AMPS         = 0x01
UNIT_CELSIUS      = 0x02
UNIT_FAHRENHEIT   = 0x03
UNIT_KELVIN       = 0x04
UNIT_PSI          = 0x05
UNIT_BAR          = 0x06
UNIT_PASCAL        = 0x07
UNIT_GRAMS        = 0x08
UNIT_KILOGRAMS    = 0x09
UNIT_POUNDS       = 0x0A
UNIT_NEWTONS      = 0x0B
UNIT_SECONDS      = 0x0C
UNIT_MILLISECONDS = 0x0D
UNIT_HERTZ        = 0x0E
UNIT_OHMS      = 0x0F
UNIT_UNITLESS     = 0xFF

UNIT_MAP = {
    "V":    UNIT_VOLTS,
    "A":    UNIT_AMPS,
    "C":    UNIT_CELSIUS,
    "F":    UNIT_FAHRENHEIT,
    "K":    UNIT_KELVIN,
    "PSI":  UNIT_PSI,
    "BAR":  UNIT_BAR,
    "PA":   UNIT_PASCAL,
    "g":    UNIT_GRAMS,
    "kg":   UNIT_KILOGRAMS,
    "lb":   UNIT_POUNDS,
    "N":    UNIT_NEWTONS,
    "s":    UNIT_SECONDS,
    "ms":   UNIT_MILLISECONDS,
    "Hz":   UNIT_HERTZ,
    "Ohm":    UNIT_OHMS,
}

# Error codes
ERR_NONE          = 0x00
ERR_UNKNOWN_TYPE  = 0x01
ERR_INVALID_ID    = 0x02
ERR_HARDWARE_FAULT = 0x03
ERR_BUSY          = 0x04
ERR_NOT_STREAMING = 0x05
ERR_INVALID_PARAM = 0x06

HEADER_SIZE = 9
HEADER_FMT = ">BBBHI"


# The session time offset is stored by the device to convert its local time into the server's time base. This is the
# offset that is used to generate every packet's timestamp, as it is added to the device's local time.

def get_timestamp(ts_offset):
    return (time.ticks_ms() + ts_offset) & 0xFFFFFFFF # type: ignore # MicroPython ticks_ms returns an int32


def pack_header(ptype, seq, length, timestamp):
    return struct.pack(HEADER_FMT, VERSION, ptype, seq, length, timestamp)


def make_config(seq, ts_offset, json_bytes):
    json_len = len(json_bytes)
    length = HEADER_SIZE + 4 + json_len
    header = pack_header(PT_CONFIG, seq, length, get_timestamp(ts_offset))
    return header + struct.pack(">I", json_len) + json_bytes


def make_data(seq, ts_offset, readings):
    count = len(readings)
    length = HEADER_SIZE + 1 + 6 * count
    fmt = ">BBBHI" + "B" + "BBf" * count
    args = [VERSION, PT_DATA, seq, length, get_timestamp(ts_offset), count]
    for sensor_id, unit_enum, value in readings:
        args.append(sensor_id)
        args.append(unit_enum)
        args.append(value)
    return struct.pack(fmt, *args)


def make_status(seq, ts_offset, device_status):
    length = HEADER_SIZE + 1
    header = pack_header(PT_STATUS, seq, length, get_timestamp(ts_offset))
    return header + struct.pack(">B", device_status)


def make_ack(seq, ts_offset, ack_ptype, ack_seq):
    length = HEADER_SIZE + 3
    header = pack_header(PT_ACK, seq, length, get_timestamp(ts_offset))
    return header + struct.pack(">BBB", ack_ptype, ack_seq, ERR_NONE)


def make_nack(seq, ts_offset, nack_ptype, nack_seq, error_code):
    length = HEADER_SIZE + 3
    header = pack_header(PT_NACK, seq, length, get_timestamp(ts_offset))
    return header + struct.pack(">BBB", nack_ptype, nack_seq, error_code)


def parse_control(payload):
    command_id, command_state = struct.unpack(">BB", payload)
    return command_id, command_state


def parse_stream_start(payload):
    frequency_hz = struct.unpack(">H", payload)[0]
    return frequency_hz


def parse_timesync(payload):
    server_time_ms = struct.unpack(">Q", payload)[0]
    return server_time_ms
