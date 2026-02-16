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
    // Packet types - Server -> Device
    PT_ESTOP,
    PT_DISCOVERY,
    PT_TIMESYNC,
    PT_CONTROL,
    PT_STATUS_REQUEST,
    PT_STREAM_START,
    PT_STREAM_STOP,
    PT_GET_SINGLE,
    PT_HEARTBEAT,
    // Packet types - Device -> Server
    PT_CONFIG,
    PT_DATA,
    PT_STATUS,
    PT_ACK,
    PT_NACK,
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
    uint8_t sequence;
    uint32_t ts_offset;
} data_packet_t;

protocol_err_t make_data_packet(uint8_t packet[],
                                size_t *packet_len,
                                protocol_sensor_data_t sensor_data[],
                                uint8_t sensor_data_len,
                                data_packet_t data);

typedef struct {
    uint8_t sequence;
    uint32_t ts_offset;
} config_packet_t;

protocol_err_t make_config_packet(uint8_t packet[],
                                  size_t packet_len,
                                  const char json_config[],
                                  size_t json_config_len,
                                  config_packet_t config);

typedef struct {
    protocol_packet_type_t packet_type;
    union {
        header_only_packet_t header_only;
        status_packet_t status;
        stream_start_packet_t stream_start;
        control_packet_t control;
        ack_packet_t ack;
        nack_packet_t nack;
        timesync_packet_t timesync;
        data_packet_t data;
        config_packet_t config;
    } payload_data;
} protocol_payload_t;

protocol_err_t parse_packet(const uint8_t packet[],
                            size_t packet_len,
                            protocol_payload_t *payload);

#endif