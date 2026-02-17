#ifndef QRET_PROTOCOL_H
#define QRET_PROTOCOL_H

#include <stdint.h>

typedef enum {
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
    PT_ACK = 0x13,
    PT_NACK = 0x14,
} protocol_packet_type_t;

typedef struct {
    uint8_t sequence;
    uint32_t ts_offset;
} header_only_packet_t;

protocol_err_t make_header_only_packet(uint8_t packet[],
                                       size_t packet_len,
                                       protocol_packet_type_t packet_type,
                                       header_only_packet_t header_only);

typedef struct {
    uint8_t device_status;
    uint8_t sequence;
    uint32_t ts_offset;
} status_packet_t;

protocol_err_t make_status_packet(uint8_t packet[],
                                  size_t packet_len,
                                  status_packet_t status);

typedef struct {
    uint16_t stream_frequency;
    uint8_t sequence;
    uint32_t ts_offset;
} stream_start_packet_t;

protocol_err_t make_stream_start_packet(uint8_t packet[],
                                        size_t packet_len,
                                        stream_start_packet_t stream_start);

typedef struct {
    uint8_t command_id;
    uint8_t command_state;
    uint8_t sequence;
    uint32_t ts_offset;
} control_packet_t;

protocol_err_t make_control_packet(uint8_t packet[],
                                   size_t packet_len,
                                   control_packet_t control);

typedef struct {
    uint8_t ack_packet_type;
    uint8_t ack_sequence;
    uint8_t sequence;
    uint32_t ts_offset;
} ack_packet_t;

protocol_err_t make_ack_packet(uint8_t packet[],
                               size_t packet_len,
                               ack_packet_t ack);

typedef struct {
    uint8_t nack_packet_type;
    uint8_t nack_sequence;
    uint8_t nack_error_code;
    uint8_t sequence;
    uint32_t ts_offset;
} nack_packet_t;

protocol_err_t make_nack_packet(uint8_t packet[],
                                size_t packet_len,
                                nack_packet_t nack);

typedef struct {
    uint64_t server_time_ms;
    uint8_t sequence;
    uint32_t ts_offset;
} timesync_packet_t;

protocol_err_t make_timesync_packet(uint8_t packet[],
                                    size_t packet_len,
                                    timesync_packet_t timesync);

typedef struct {
    uint8_t sensor_id;
    uint8_t unit;
    float value;
} protocol_sensor_data_t;

typedef struct {
    protocol_sensor_data_t sensor_data[UINT8_MAX];
    uint8_t sensor_count;
    uint8_t sequence;
    uint32_t ts_offset;
} data_packet_t;

protocol_err_t make_data_packet(uint8_t packet[],
                                size_t *packet_len,
                                data_packet_t data);

typedef struct {
    const char *json_config;
    uint32_t json_config_len;
    uint8_t sequence;
    uint32_t ts_offset;
} config_packet_t;

protocol_err_t make_config_packet(uint8_t packet[],
                                  size_t *packet_len,
                                  config_packet_t config);

typedef struct {
    protocol_packet_type_t packet_type;
    union {
        config_packet_t config;
        data_packet_t data;
        status_packet_t status;
        ack_packet_t ack;
        nack_packet_t nack;
    } payload_data;
} server_payload_t;

protocol_err_t server_parse_packet(const uint8_t packet[],
                                   size_t packet_len,
                                   server_payload_t *payload);

typedef struct {
    protocol_packet_type_t packet_type;
    union {
        header_only_packet_t header_only;
        timesync_packet_t timesync;
        control_packet_t control;
        stream_start_packet_t stream_start;
    } payload_data;
} client_payload_t;

protocol_err_t client_parse_packet(const uint8_t packet[],
                                   size_t packet_len,
                                   client_payload_t *payload);

#endif