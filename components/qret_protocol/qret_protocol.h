#ifndef QRET_PROTOCOL_H
#define QRET_PROTOCOL_H

#include <stdint.h>

typedef enum {
    // Return error codes
    PROTOCOL_OK,
    PROTOCOL_ARRAY_LEN_ERR,
    PROTOCOL_NULL_PTR_ERR,
    PROTOCOL_INVALID_PACKET_TYPE,
    PROTOCOL_VERSION_MISMATCH_ERR,
} protocol_err_t;

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
} packet_type_t;

typedef enum {
    // Packet error codes
    ERR_NONE = 0x00,
    ERR_UNKNOWN_TYPE = 0x01,
    ERR_INVALID_ID = 0x02,
    ERR_HARDWARE_FAULT = 0x03,
    ERR_BUSY = 0x04,
    ERR_NOT_STREAMING = 0x05,
    ERR_INVALID_PARAM = 0x06,
} packet_err_t;

// Device status
#define DS_INACTIVE 0x00
#define DS_ACTIVE 0x01
#define DS_ERROR 0x02
#define DS_CALIBRATING 0x03

// Control state
#define CS_CLOSED 0x00
#define CS_OPEN 0x01
#define CS_ERROR 0xFF

// Data sizes
#define STATUS_DATA_SIZE 1
#define STREAM_START_DATA_SIZE 2
#define CONTROL_DATA_SIZE 2
#define ACK_DATA_SIZE 3
#define NACK_DATA_SIZE 3

#define HEADER_SIZE 9

#define STATUS_PACKET_SIZE (HEADER_SIZE + STATUS_DATA_SIZE)
#define STREAM_START_PACKET_SIZE (HEADER_SIZE + STREAM_START_DATA_SIZE)
#define CONTROL_PACKET_SIZE (HEADER_SIZE + CONTROL_DATA_SIZE)
#define ACK_PACKET_SIZE (HEADER_SIZE + ACK_DATA_SIZE)
#define NACK_PACKET_SIZE (HEADER_SIZE + NACK_DATA_SIZE)

typedef struct {
    uint8_t sequence;
    uint32_t ts_offset;
} header_only_packet_t;

protocol_err_t make_header_only_packet(
    uint8_t packet[],
    size_t *packet_len,
    packet_type_t packet_type,
    header_only_packet_t header_only
);

typedef struct {
    uint8_t device_status;
    uint8_t sequence;
    uint32_t ts_offset;
} status_packet_t;

protocol_err_t make_status_packet(
    uint8_t packet[],
    size_t *packet_len,
    status_packet_t status
);

typedef struct {
    uint16_t stream_frequency;
    uint8_t sequence;
    uint32_t ts_offset;
} stream_start_packet_t;

protocol_err_t make_stream_start_packet(
    uint8_t packet[],
    size_t *packet_len,
    stream_start_packet_t stream_start
);

typedef struct {
    uint8_t command_id;
    uint8_t command_state;
    uint8_t sequence;
    uint32_t ts_offset;
} control_packet_t;

protocol_err_t make_control_packet(
    uint8_t packet[],
    size_t *packet_len,
    control_packet_t control
);

typedef struct {
    uint8_t ack_packet_type;
    uint8_t ack_sequence;
    uint8_t sequence;
    uint32_t ts_offset;
} ack_packet_t;

protocol_err_t make_ack_packet(
    uint8_t packet[],
    size_t *packet_len,
    ack_packet_t ack
);

typedef struct {
    uint8_t nack_packet_type;
    uint8_t nack_sequence;
    uint8_t nack_error_code;
    uint8_t sequence;
    uint32_t ts_offset;
} nack_packet_t;

protocol_err_t make_nack_packet(
    uint8_t packet[],
    size_t *packet_len,
    nack_packet_t nack
);

typedef struct {
    uint8_t sensor_id;
    uint8_t unit;
    float value;
} protocol_sensor_data_t;

typedef struct {
    protocol_sensor_data_t *sensor_data;
    uint8_t sensor_count;
    uint8_t sequence;
    uint32_t ts_offset;
} data_packet_t;

protocol_err_t make_data_packet(
    uint8_t packet[],
    size_t *packet_len,
    data_packet_t data
);

typedef struct {
    char *json_config;
    uint32_t json_config_len;
    uint8_t sequence;
    uint32_t ts_offset;
} config_packet_t;

protocol_err_t make_config_packet(
    uint8_t packet[],
    size_t *packet_len,
    config_packet_t config
);

protocol_err_t get_packet_len(
    const uint8_t header[],
    size_t header_len,
    uint16_t *data_len
);

typedef struct {
    packet_type_t packet_type;
    union {
        config_packet_t config;
        data_packet_t data;
        status_packet_t status;
        ack_packet_t ack;
        nack_packet_t nack;
    } payload_data;
} server_payload_t;

protocol_err_t server_parse_packet(
    const uint8_t buffer[],
    size_t buffer_len,
    server_payload_t *payload
);

typedef struct {
    packet_type_t packet_type;
    union {
        header_only_packet_t header_only;
        control_packet_t control;
        stream_start_packet_t stream_start;
        ack_packet_t ack;
        nack_packet_t nack;
    } payload_data;
} client_payload_t;

protocol_err_t client_parse_packet(
    const uint8_t buffer[],
    size_t buffer_len,
    client_payload_t *payload
);

#endif