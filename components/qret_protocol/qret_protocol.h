#pragma once

#include <stdint.h>

//----------------------------------------------------------
// enums
//----------------------------------------------------------

typedef enum {
    // Return error codes
    PROTOCOL_OK,
    PROTOCOL_ARRAY_LEN_ERR,
    PROTOCOL_NULL_PTR_ERR,
    PROTOCOL_INVALID_PACKET_TYPE,
    PROTOCOL_VERSION_MISMATCH_ERR,
} qret_protocol_ret;

typedef enum {
    // Server -> Device
    PT_ESTOP = 0x00,
    PT_DISCOVERY = 0x01,
    PT_TIMESYNC = 0x02,
    PT_CONTROL = 0x03,
    PT_STATUS_REQUEST = 0x04,
    PT_STREAM_START = 0x05,
    PT_STREAM_STOP = 0x06,
    PT_GET_SINGLE = 0x07,
    PT_HEARTBEAT = 0x08,
    // Device -> Server
    PT_CONFIG = 0x10,
    PT_DATA = 0x11,
    PT_STATUS = 0x12,
    // All
    PT_ACK = 0x13,
    PT_NACK = 0x14,
} qret_packet_type;

typedef enum {
    // Device status
    DS_INACTIVE = 0x00,
    DS_ACTIVE = 0x01,
    DS_ERROR = 0x02,
    DS_CALIBRATING = 0x03,
} qret_device_status;

typedef enum {
    // Control state
    CS_CLOSED = 0x00,
    CS_OPEN = 0x01,
    CS_ERROR = 0xFF,
} qret_control_state;

typedef enum {
    UNIT_VOLTS = 0x00,
    UNIT_AMPS = 0x01,
    UNIT_CELSIUS = 0x02,
    UNIT_FAHRENHEIT = 0x03,
    UNIT_KELVIN = 0x04,
    UNIT_PSI = 0x05,
    UNIT_BAR = 0x06,
    UNIT_PASCAL = 0x07,
    UNIT_GRAMS = 0x08,
    UNIT_KILOGRAMS = 0x09,
    UNIT_POUNDS = 0x0A,
    UNIT_NEWTONS = 0x0B,
    UNIT_SECONDS = 0x0C,
    UNIT_MILLISECONDS = 0x0D,
    UNIT_HERTZ = 0x0E,
    UNIT_OHMS = 0x0F,
    UNIT_UNITLESS = 0xFF,
} qret_unit;

typedef enum {
    // Packet error codes
    ERR_NONE = 0x00,
    ERR_UNKNOWN_TYPE = 0x01,
    ERR_INVALID_ID = 0x02,
    ERR_HARDWARE_FAULT = 0x03,
    ERR_BUSY = 0x04,
    ERR_NOT_STREAMING = 0x05,
    ERR_INVALID_PARAM = 0x06,
} qret_packet_err;

//----------------------------------------------------------
// data sizes
//----------------------------------------------------------

#define STREAM_START_DATA_SIZE 2
#define CONTROL_DATA_SIZE 2
#define ACK_DATA_SIZE 3
#define NACK_DATA_SIZE 3

#define CONTROL_STATUS_DATA_SIZE 2
#define STATUS_DATA_SIZE(control_count) (2 + (CONTROL_STATUS_DATA_SIZE * control_count))

#define SENSOR_DATA_SIZE 6
#define DATA_DATA_SIZE(sensor_count) (1 + (SENSOR_DATA_SIZE * sensor_count))

#define CONFIG_DATA_SIZE(config_len) (4 + config_len)

#define HEADER_SIZE 9

#define STREAM_START_PACKET_SIZE (HEADER_SIZE + STREAM_START_DATA_SIZE)
#define CONTROL_PACKET_SIZE (HEADER_SIZE + CONTROL_DATA_SIZE)
#define ACK_PACKET_SIZE (HEADER_SIZE + ACK_DATA_SIZE)
#define NACK_PACKET_SIZE (HEADER_SIZE + NACK_DATA_SIZE)
#define STATUS_PACKET_SIZE(control_count) (HEADER_SIZE + STATUS_DATA_SIZE(control_count))
#define DATA_PACKET_SIZE(sensor_count) (HEADER_SIZE + DATA_DATA_SIZE(sensor_count))
#define CONFIG_PACKET_SIZE(config_len) (HEADER_SIZE + CONFIG_DATA_SIZE(config_len))

//----------------------------------------------------------
// packet structs
//----------------------------------------------------------

typedef struct {
    uint8_t sequence;
    uint32_t ts_offset;
} qret_header_only_packet;

typedef struct {
    uint8_t control_id;
    uint8_t control_state;
} qret_control_status;

typedef struct {
    qret_control_status *control_data;
    uint8_t control_count;
    uint8_t device_status;
    uint8_t sequence;
    uint32_t ts_offset;
} qret_status_packet;

typedef struct {
    uint16_t stream_frequency;
    uint8_t sequence;
    uint32_t ts_offset;
} qret_stream_start_packet;

typedef struct {
    uint8_t command_id;
    uint8_t command_state;
    uint8_t sequence;
    uint32_t ts_offset;
} qret_control_packet;

typedef struct {
    uint8_t ack_packet_type;
    uint8_t ack_sequence;
    uint8_t sequence;
    uint32_t ts_offset;
} qret_ack_packet;

typedef struct {
    uint8_t nack_packet_type;
    uint8_t nack_sequence;
    uint8_t nack_error_code;
    uint8_t sequence;
    uint32_t ts_offset;
} qret_nack_packet;

typedef struct {
    uint8_t sensor_id;
    uint8_t unit;
    float value;
} qret_sensor_data;

typedef struct {
    qret_sensor_data *sensor_data;
    uint8_t sensor_count;
    uint8_t sequence;
    uint32_t ts_offset;
} qret_data_packet;

typedef struct {
    const char *json_config;
    uint32_t json_config_len;
    uint8_t sequence;
    uint32_t ts_offset;
} qret_config_packet;

typedef struct {
    qret_packet_type packet_type;
    union {
        qret_config_packet config;
        qret_data_packet data;
        qret_status_packet status;
        qret_ack_packet ack;
        qret_nack_packet nack;
    } payload_data;
} qret_server_payload;

typedef struct {
    qret_packet_type packet_type;
    union {
        qret_header_only_packet header_only;
        qret_control_packet control;
        qret_stream_start_packet stream_start;
        qret_ack_packet ack;
        qret_nack_packet nack;
    } payload_data;
} qret_client_payload;

//----------------------------------------------------------
// packet struct to raw bytes
//----------------------------------------------------------

qret_protocol_ret make_header_only_packet(
    uint8_t packet[],
    size_t *packet_len,
    qret_packet_type packet_type,
    const qret_header_only_packet *header_only
);

qret_protocol_ret make_status_packet(uint8_t packet[], size_t *packet_len, const qret_status_packet *status);

qret_protocol_ret make_stream_start_packet(
    uint8_t packet[],
    size_t *packet_len,
    const qret_stream_start_packet *stream_start
);

qret_protocol_ret make_control_packet(uint8_t packet[], size_t *packet_len, const qret_control_packet *control);

qret_protocol_ret make_ack_packet(uint8_t packet[], size_t *packet_len, const qret_ack_packet *ack);

qret_protocol_ret make_nack_packet(uint8_t packet[], size_t *packet_len, const qret_nack_packet *nack);

qret_protocol_ret make_data_packet(uint8_t packet[], size_t *packet_len, const qret_data_packet *data);

qret_protocol_ret make_config_packet(uint8_t packet[], size_t *packet_len, const qret_config_packet *config);

//----------------------------------------------------------
// raw bytes to packet struct
//----------------------------------------------------------

qret_protocol_ret get_packet_len(const uint8_t header[], size_t header_len, uint16_t *data_len);

qret_protocol_ret server_parse_packet(const uint8_t buffer[], size_t buffer_len, qret_server_payload *payload);

qret_protocol_ret client_parse_packet(const uint8_t buffer[], size_t buffer_len, qret_client_payload *payload);
