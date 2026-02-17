#include "qret_protocol.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define QRET_PROTOCOL_VERSION 0x02

// Device status
#define DS_INACTIVE 0x00
#define DS_ACTIVE 0x01
#define DS_ERROR 0x02
#define DS_CALIBRATING 0x03

// Control state
#define CS_CLOSED 0x00
#define CS_OPEN 0x01
#define CS_ERROR 0xFF

// Units
#define UNIT_VOLTS 0x00
#define UNIT_AMPS 0x01
#define UNIT_CELSIUS 0x02
#define UNIT_FAHRENHEIT 0x03
#define UNIT_KELVIN 0x04
#define UNIT_PSI 0x05
#define UNIT_BAR 0x06
#define UNIT_PASCAL 0x07
#define UNIT_GRAMS 0x08
#define UNIT_KILOGRAMS 0x09
#define UNIT_POUNDS 0x0A
#define UNIT_NEWTONS 0x0B
#define UNIT_SECONDS 0x0C
#define UNIT_MILLISECONDS 0x0D
#define UNIT_HERTZ 0x0E
#define UNIT_PERCENT 0x0F
#define UNIT_UNITLESS 0xFF

typedef struct {
    const char *unit;
    uint8_t code;
} unit_map_t;

static const unit_map_t UNIT_MAP[] = {
    {"V",   UNIT_VOLTS       },
    {"A",   UNIT_AMPS        },
    {"C",   UNIT_CELSIUS     },
    {"F",   UNIT_FAHRENHEIT  },
    {"K",   UNIT_KELVIN      },
    {"PSI", UNIT_PSI         },
    {"BAR", UNIT_BAR         },
    {"PA",  UNIT_PASCAL      },
    {"g",   UNIT_GRAMS       },
    {"kg",  UNIT_KILOGRAMS   },
    {"lb",  UNIT_POUNDS      },
    {"N",   UNIT_NEWTONS     },
    {"s",   UNIT_SECONDS     },
    {"ms",  UNIT_MILLISECONDS},
    {"Hz",  UNIT_HERTZ       },
    {"%",   UNIT_PERCENT     },
};

static uint8_t _units_to_protocol(const char unit[]);

// Error codes
#define ERR_NONE 0x00
#define ERR_UNKNOWN_TYPE 0x01
#define ERR_INVALID_ID 0x02
#define ERR_HARDWARE_FAULT 0x03
#define ERR_BUSY 0x04
#define ERR_NOT_STREAMING 0x05
#define ERR_INVALID_PARAM 0x06

// Data sizes
#define STATUS_DATA_SIZE 1
#define STREAM_START_DATA_SIZE 2
#define CONTROL_DATA_SIZE 2
#define ACK_DATA_SIZE 3
#define NACK_DATA_SIZE 3
#define TIMESYNC_DATA_SIZE 8

#define HEADER_SIZE 9

#define STATUS_PACKET_SIZE (HEADER_SIZE + STATUS_DATA_SIZE)
#define STREAM_START_PACKET_SIZE (HEADER_SIZE + STREAM_START_DATA_SIZE)
#define CONTROL_PACKET_SIZE (HEADER_SIZE + CONTROL_DATA_SIZE)
#define ACK_PACKET_SIZE (HEADER_SIZE + ACK_DATA_SIZE)
#define NACK_PACKET_SIZE (HEADER_SIZE + NACK_DATA_SIZE)
#define TIMESYNC_PACKET_SIZE (HEADER_SIZE + TIMESYNC_DATA_SIZE)

typedef struct {
    uint8_t protocol_version;
    uint8_t packet_type;
    uint8_t sequence;
    uint16_t data_length;
    uint32_t timestamp;
} protocol_header_t;

static protocol_err_t _pack_header(uint8_t header[],
                                   size_t header_len,
                                   protocol_header_t header_data) {

    if (header == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (header_len != HEADER_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    header[0] = header_data.protocol_version;
    header[1] = header_data.packet_type;
    header[2] = header_data.sequence;

    header[3] = (uint8_t)(header_data.data_length >> 8);
    header[4] = (uint8_t)header_data.data_length;

    header[5] = (uint8_t)(header_data.timestamp >> 24);
    header[6] = (uint8_t)(header_data.timestamp >> 16);
    header[7] = (uint8_t)(header_data.timestamp >> 8);
    header[8] = (uint8_t)header_data.timestamp;

    return PROTOCOL_OK;
}

static protocol_err_t _unpack_header(const uint8_t header[],
                                     size_t header_len,
                                     protocol_header_t *header_data) {

    if (header == NULL || header_data == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (header_len != HEADER_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }
    if (header[0] != QRET_PROTOCOL_VERSION) {
        return PROTOCOL_VERSION_MISMATCH_ERR;
    }

    header_data->protocol_version = header[0];
    header_data->packet_type = header[1];
    header_data->sequence = header[2];

    header_data->data_length =
        ((uint16_t)header[3] << 8) | ((uint16_t)header[4]);

    header_data->timestamp = ((uint32_t)header[5] << 24) |
                             ((uint32_t)header[6] << 16) |
                             ((uint32_t)header[7] << 8) | ((uint32_t)header[8]);

    return PROTOCOL_OK;
}

static bool _is_packet_header_only(protocol_packet_type_t packet_type);

protocol_err_t make_header_only_packet(uint8_t packet[],
                                       size_t packet_len,
                                       protocol_packet_type_t packet_type,
                                       header_only_packet_t header_only) {

    if (packet == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len != HEADER_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }
    if (!_is_packet_header_only(packet_type)) {
        return PROTOCOL_INVALID_PACKET_TYPE;
    }

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = packet_type,
        .sequence = header_only.sequence,
        .data_length = HEADER_SIZE,
        .timestamp = header_only.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }
    return PROTOCOL_OK;
}

protocol_err_t make_status_packet(uint8_t packet[],
                                  size_t packet_len,
                                  status_packet_t status) {

    if (packet == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len != STATUS_PACKET_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_STATUS,
        .sequence = status.sequence,
        .data_length = STATUS_PACKET_SIZE,
        .timestamp = status.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }

    packet[HEADER_SIZE + 0] = status.device_status;

    return PROTOCOL_OK;
}

protocol_err_t make_stream_start_packet(uint8_t packet[],
                                        size_t packet_len,
                                        stream_start_packet_t stream_start) {

    if (packet == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len != STREAM_START_PACKET_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_STREAM_START,
        .sequence = stream_start.sequence,
        .data_length = STREAM_START_PACKET_SIZE,
        .timestamp = stream_start.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }

    packet[HEADER_SIZE + 0] = (uint8_t)(stream_start.stream_frequency >> 8);
    packet[HEADER_SIZE + 1] = (uint8_t)stream_start.stream_frequency;

    return PROTOCOL_OK;
}

protocol_err_t make_control_packet(uint8_t packet[],
                                   size_t packet_len,
                                   control_packet_t control) {

    if (packet == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len != CONTROL_PACKET_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_CONTROL,
        .sequence = control.sequence,
        .data_length = CONTROL_PACKET_SIZE,
        .timestamp = control.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }

    packet[HEADER_SIZE + 0] = control.command_id;
    packet[HEADER_SIZE + 1] = control.command_state;

    return PROTOCOL_OK;
}

protocol_err_t make_ack_packet(uint8_t packet[],
                               size_t packet_len,
                               ack_packet_t ack) {

    if (packet == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len != ACK_PACKET_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_ACK,
        .sequence = ack.sequence,
        .data_length = ACK_PACKET_SIZE,
        .timestamp = ack.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }

    packet[HEADER_SIZE + 0] = ack.ack_packet_type;
    packet[HEADER_SIZE + 1] = ack.ack_sequence;
    packet[HEADER_SIZE + 2] = ERR_NONE;

    return PROTOCOL_OK;
}

protocol_err_t make_nack_packet(uint8_t packet[],
                                size_t packet_len,
                                nack_packet_t nack) {

    if (packet == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len != NACK_PACKET_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_NACK,
        .sequence = nack.sequence,
        .data_length = NACK_PACKET_SIZE,
        .timestamp = nack.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }

    packet[HEADER_SIZE + 0] = nack.nack_packet_type;
    packet[HEADER_SIZE + 1] = nack.nack_sequence;
    packet[HEADER_SIZE + 2] = nack.nack_error_code;

    return PROTOCOL_OK;
}

protocol_err_t make_timesync_packet(uint8_t packet[],
                                    size_t packet_len,
                                    timesync_packet_t timesync) {

    if (packet == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len != TIMESYNC_PACKET_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_TIMESYNC,
        .sequence = timesync.sequence,
        .data_length = TIMESYNC_PACKET_SIZE,
        .timestamp = timesync.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }

    packet[HEADER_SIZE + 0] = (uint8_t)(timesync.server_time_ms >> 56);
    packet[HEADER_SIZE + 1] = (uint8_t)(timesync.server_time_ms >> 48);
    packet[HEADER_SIZE + 2] = (uint8_t)(timesync.server_time_ms >> 40);
    packet[HEADER_SIZE + 3] = (uint8_t)(timesync.server_time_ms >> 32);
    packet[HEADER_SIZE + 4] = (uint8_t)(timesync.server_time_ms >> 24);
    packet[HEADER_SIZE + 5] = (uint8_t)(timesync.server_time_ms >> 16);
    packet[HEADER_SIZE + 6] = (uint8_t)(timesync.server_time_ms >> 8);
    packet[HEADER_SIZE + 7] = (uint8_t)timesync.server_time_ms;

    return PROTOCOL_OK;
}

protocol_err_t make_data_packet(uint8_t packet[],
                                size_t *packet_len,
                                data_packet_t data) {

    if (packet == NULL || packet_len == NULL || data.sensor_data == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    const size_t data_size = 1 + (6 * data.sensor_count);
    if (*packet_len < HEADER_SIZE + data_size) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }
    *packet_len = HEADER_SIZE + data_size;

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_DATA,
        .sequence = data.sequence,
        .data_length = HEADER_SIZE + data_size,
        .timestamp = data.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }

    packet[HEADER_SIZE + 0] = data.sensor_count;

    for (size_t i = 0; i < data.sensor_count; i++) {
        size_t offset = HEADER_SIZE + 1 + (6 * i);
        packet[offset + 0] = data.sensor_data[i].sensor_id;
        packet[offset + 1] = data.sensor_data[i].unit;
        // Float to bytes
        uint32_t value_bytes;
        memcpy(&value_bytes, &data.sensor_data[i].value, sizeof(uint32_t));
        packet[offset + 2] = (uint8_t)(value_bytes >> 24);
        packet[offset + 3] = (uint8_t)(value_bytes >> 16);
        packet[offset + 4] = (uint8_t)(value_bytes >> 8);
        packet[offset + 5] = (uint8_t)(value_bytes);
    }
    return PROTOCOL_OK;
}

protocol_err_t make_config_packet(uint8_t packet[],
                                  size_t *packet_len,
                                  config_packet_t config) {

    if (packet == NULL || packet_len == NULL || config.json_config == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    uint32_t packet_data_len = config.json_config_len;
    if (config.json_config_len > 0 &&
        config.json_config[config.json_config_len - 1] == '\0') {
        packet_data_len--; // Remove null terminator from json_config
    }
    if (*packet_len < (HEADER_SIZE + 4 + packet_data_len)) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }
    *packet_len = HEADER_SIZE + (4 + packet_data_len);

    protocol_err_t err;

    protocol_header_t header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_CONFIG,
        .sequence = config.sequence,
        .data_length = HEADER_SIZE + (4 + packet_data_len),
        .timestamp = config.ts_offset,
    };

    err = _pack_header(packet, HEADER_SIZE, header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }

    packet[HEADER_SIZE + 0] = (uint8_t)(packet_data_len >> 24);
    packet[HEADER_SIZE + 1] = (uint8_t)(packet_data_len >> 16);
    packet[HEADER_SIZE + 2] = (uint8_t)(packet_data_len >> 8);
    packet[HEADER_SIZE + 3] = (uint8_t)packet_data_len;

    memcpy(packet + (HEADER_SIZE + 4), config.json_config, packet_data_len);

    return PROTOCOL_OK;
}

protocol_err_t server_parse_packet(const uint8_t packet[],
                            size_t packet_len,
                            server_payload_t *payload) {

    if (packet == NULL || payload == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len < HEADER_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    protocol_err_t err;
    protocol_header_t header_data = {0};

    err = _unpack_header(packet, HEADER_SIZE, &header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }
    if (packet_len != header_data.data_length) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    // Tag the packet type
    payload->packet_type = header_data.packet_type;

    switch (header_data.packet_type) {
    case PT_STATUS:
        if (header_data.data_length != STATUS_PACKET_SIZE) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        payload->payload_data.status.device_status = packet[HEADER_SIZE + 0];
        payload->payload_data.status.sequence = header_data.sequence;
        payload->payload_data.status.ts_offset = header_data.timestamp;
        break;
    case PT_ACK:
        if (header_data.data_length != ACK_PACKET_SIZE) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        if (packet[HEADER_SIZE + 2] != ERR_NONE) {
            return PROTOCOL_INVALID_PACKET_TYPE;
        }
        payload->payload_data.ack.ack_packet_type = packet[HEADER_SIZE + 0];
        payload->payload_data.ack.ack_sequence = packet[HEADER_SIZE + 1];
        payload->payload_data.ack.sequence = header_data.sequence;
        payload->payload_data.ack.ts_offset = header_data.timestamp;
        break;
    case PT_NACK:
        if (header_data.data_length != NACK_PACKET_SIZE) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        payload->payload_data.nack.nack_packet_type = packet[HEADER_SIZE + 0];
        payload->payload_data.nack.nack_sequence = packet[HEADER_SIZE + 1];
        payload->payload_data.nack.nack_error_code = packet[HEADER_SIZE + 2];
        payload->payload_data.nack.sequence = header_data.sequence;
        payload->payload_data.nack.ts_offset = header_data.timestamp;
        break;
    case PT_DATA:
        uint8_t count = packet[HEADER_SIZE + 0];
        if (header_data.data_length != HEADER_SIZE + 1 + (6 * count)) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        payload->payload_data.data.sensor_count = count;

        for (size_t i = 0; i < count; i++) {
            size_t offset = HEADER_SIZE + 1 + (6 * i);
            payload->payload_data.data.sensor_data[i].sensor_id =
                packet[offset + 0];
            payload->payload_data.data.sensor_data[i].unit = packet[offset + 1];
            // Bytes to float
            uint32_t value_bytes = (uint32_t)(packet[offset + 2] << 24) |
                                   (uint32_t)(packet[offset + 3] << 16) |
                                   (uint32_t)(packet[offset + 4] << 8) |
                                   (uint32_t)packet[offset + 5];
            memcpy(&payload->payload_data.data.sensor_data[i].value,
                   &value_bytes, sizeof(uint32_t));
        }
        payload->payload_data.data.sequence = header_data.sequence;
        payload->payload_data.data.ts_offset = header_data.timestamp;
        break;
    case PT_CONFIG:
        uint32_t data_len = (uint32_t)(packet[HEADER_SIZE + 0] << 24) |
                            (uint32_t)(packet[HEADER_SIZE + 1] << 16) |
                            (uint32_t)(packet[HEADER_SIZE + 2] << 8) |
                            (uint32_t)packet[HEADER_SIZE + 3];
        if (header_data.data_length != HEADER_SIZE + (4 + data_len)) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        payload->payload_data.config.json_config_len = data_len;

        payload->payload_data.config.json_config =
            (const char *)(packet + (HEADER_SIZE + 4));

        break;
    default:
        return PROTOCOL_INVALID_PACKET_TYPE;
    }

    return PROTOCOL_OK;
}

protocol_err_t client_parse_packet(const uint8_t packet[],
                            size_t packet_len,
                            client_payload_t *payload) {

    if (packet == NULL || payload == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (packet_len < HEADER_SIZE) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    protocol_err_t err;
    protocol_header_t header_data = {0};

    err = _unpack_header(packet, HEADER_SIZE, &header_data);
    if (err != PROTOCOL_OK) {
        return err;
    }
    if (packet_len != header_data.data_length) {
        return PROTOCOL_ARRAY_LEN_ERR;
    }

    // Tag the packet type
    payload->packet_type = header_data.packet_type;

    switch (header_data.packet_type) {
    case PT_ESTOP:
    case PT_DISCOVERY:
    case PT_STREAM_STOP:
    case PT_GET_SINGLE:
    case PT_HEARTBEAT:
    case PT_STATUS_REQUEST:
        if (header_data.data_length != HEADER_SIZE) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        payload->payload_data.header_only.sequence = header_data.sequence;
        payload->payload_data.header_only.ts_offset = header_data.timestamp;
        break;
    case PT_STREAM_START:
        if (header_data.data_length != STREAM_START_PACKET_SIZE) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        payload->payload_data.stream_start.stream_frequency =
            ((uint16_t)packet[HEADER_SIZE + 0] << 8) |
            (uint16_t)packet[HEADER_SIZE + 1];
        payload->payload_data.stream_start.sequence = header_data.sequence;
        payload->payload_data.stream_start.ts_offset = header_data.timestamp;
        break;
    case PT_CONTROL:
        if (header_data.data_length != CONTROL_PACKET_SIZE) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        payload->payload_data.control.command_id = packet[HEADER_SIZE + 0];
        payload->payload_data.control.command_state = packet[HEADER_SIZE + 1];
        payload->payload_data.control.sequence = header_data.sequence;
        payload->payload_data.control.ts_offset = header_data.timestamp;
        break;
    case PT_TIMESYNC:
        if (header_data.data_length != TIMESYNC_PACKET_SIZE) {
            return PROTOCOL_ARRAY_LEN_ERR;
        }
        payload->payload_data.timesync.server_time_ms =
            ((uint64_t)packet[HEADER_SIZE + 0] << 56) |
            ((uint64_t)packet[HEADER_SIZE + 1] << 48) |
            ((uint64_t)packet[HEADER_SIZE + 2] << 40) |
            ((uint64_t)packet[HEADER_SIZE + 3] << 32) |
            ((uint64_t)packet[HEADER_SIZE + 4] << 24) |
            ((uint64_t)packet[HEADER_SIZE + 5] << 16) |
            ((uint64_t)packet[HEADER_SIZE + 6] << 8) |
            (uint64_t)packet[HEADER_SIZE + 7];
        payload->payload_data.timesync.sequence = header_data.sequence;
        payload->payload_data.timesync.ts_offset = header_data.timestamp;
        break;
    default:
        return PROTOCOL_INVALID_PACKET_TYPE;
    }

    return PROTOCOL_OK;
}

static uint8_t _units_to_protocol(const char unit[]) {
    if (unit == NULL) {
        return UNIT_UNITLESS;
    }

    for (size_t i = 0; i < sizeof(UNIT_MAP) / sizeof(UNIT_MAP[0]); i++) {
        if (strcmp(unit, UNIT_MAP[i].unit) == 0) {
            return UNIT_MAP[i].code;
        }
    }
    return UNIT_UNITLESS;
}

static bool _is_packet_header_only(protocol_packet_type_t packet_type) {
    switch (packet_type) {
    case PT_ESTOP:
    case PT_DISCOVERY:
    case PT_STREAM_STOP:
    case PT_GET_SINGLE:
    case PT_HEARTBEAT:
    case PT_STATUS_REQUEST:
        return true;
    default:
        return false;
    }
}