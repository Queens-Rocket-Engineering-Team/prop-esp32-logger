#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "qret_protocol.h"

#define QRET_PROTOCOL_VERSION 2

// Internal header struct
typedef struct {
    uint8_t protocol_version;
    uint8_t packet_type;
    uint8_t sequence;
    uint16_t packet_length;
    uint32_t timestamp;
} qret_protocol_header;

// Encode a header_data struct into the buffer
static qret_protocol_ret s_pack_header(uint8_t buffer[], size_t header_len, const qret_protocol_header *header_data) {
    if (buffer == NULL || header_data == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (header_len != HEADER_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }

    buffer[0] = header_data->protocol_version;
    buffer[1] = header_data->packet_type;
    buffer[2] = header_data->sequence;

    buffer[3] = (uint8_t)(header_data->packet_length >> 8);
    buffer[4] = (uint8_t)header_data->packet_length;

    buffer[5] = (uint8_t)(header_data->timestamp >> 24);
    buffer[6] = (uint8_t)(header_data->timestamp >> 16);
    buffer[7] = (uint8_t)(header_data->timestamp >> 8);
    buffer[8] = (uint8_t)header_data->timestamp;

    return PROTOCOL_OK;
}

// Take a buffer which contains the encoded header, and fill out the header_data struct
static qret_protocol_ret s_unpack_header(const uint8_t buffer[], size_t header_len, qret_protocol_header *header_data) {
    if (buffer == NULL || header_data == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (header_len != HEADER_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    if (buffer[0] != QRET_PROTOCOL_VERSION) {
        return PROTOCOL_VERSION_MISMATCH_ERR;
    }

    header_data->protocol_version = buffer[0];
    header_data->packet_type = buffer[1];
    header_data->sequence = buffer[2];

    header_data->packet_length = ((uint16_t)buffer[3] << 8) | ((uint16_t)buffer[4]);

    header_data->timestamp = ((uint32_t)buffer[5] << 24) | ((uint32_t)buffer[6] << 16) | ((uint32_t)buffer[7] << 8) |
                             ((uint32_t)buffer[8]);

    return PROTOCOL_OK;
}

static inline bool s_is_packet_header_only(qret_packet_type packet_type) {
    switch (packet_type) {
    case PT_ESTOP:
    case PT_DISCOVERY:
    case PT_TIMESYNC:
    case PT_STREAM_STOP:
    case PT_GET_SINGLE:
    case PT_HEARTBEAT:
    case PT_STATUS_REQUEST:
        return true;
    default:
        return false;
    }
}

qret_protocol_ret make_header_only_packet(uint8_t buffer[], size_t *packet_len, qret_packet_type packet_type, const qret_header_only_packet *header_only) {
    if (buffer == NULL || packet_len == NULL || header_only == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (!s_is_packet_header_only(packet_type)) {
        return PROTOCOL_INVALID_PACKET_TYPE_ERR;
    }
    if (*packet_len < HEADER_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    *packet_len = HEADER_SIZE;

    qret_protocol_ret ret;

    const qret_protocol_header header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = packet_type,
        .sequence = header_only->sequence,
        .packet_length = HEADER_SIZE,
        .timestamp = header_only->timestamp,
    };

    ret= s_pack_header(buffer, HEADER_SIZE, &header_data);
    if (ret != PROTOCOL_OK) {
        return ret;
    }
    return PROTOCOL_OK;
}

qret_protocol_ret make_ack_packet(uint8_t buffer[], size_t *packet_len, const qret_ack_packet *ack) {
    if (buffer == NULL || packet_len == NULL || ack == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (*packet_len < ACK_PACKET_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    *packet_len = ACK_PACKET_SIZE;

    qret_protocol_ret ret;

    const qret_protocol_header header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_ACK,
        .sequence = ack->header.sequence,
        .packet_length = ACK_PACKET_SIZE,
        .timestamp = ack->header.timestamp,
    };

    ret= s_pack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }

    buffer[HEADER_SIZE + 0] = ack->ack_packet_type;
    buffer[HEADER_SIZE + 1] = ack->ack_sequence;
    buffer[HEADER_SIZE + 2] = ERR_NONE;

    return PROTOCOL_OK;
}

qret_protocol_ret make_nack_packet(uint8_t buffer[], size_t *packet_len, const qret_nack_packet *nack) {
    if (buffer == NULL || packet_len == NULL || nack == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (*packet_len < NACK_PACKET_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    *packet_len = NACK_PACKET_SIZE;

    qret_protocol_ret ret;

    const qret_protocol_header header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_NACK,
        .sequence = nack->header.sequence,
        .packet_length = NACK_PACKET_SIZE,
        .timestamp = nack->header.timestamp,
    };

    ret= s_pack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }

    buffer[HEADER_SIZE + 0] = nack->nack_packet_type;
    buffer[HEADER_SIZE + 1] = nack->nack_sequence;
    buffer[HEADER_SIZE + 2] = nack->nack_error_code;

    return PROTOCOL_OK;
}

qret_protocol_ret make_stream_start_packet(uint8_t buffer[], size_t *packet_len, const qret_stream_start_packet *stream_start) {
    if (buffer == NULL || packet_len == NULL || stream_start == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (*packet_len < STREAM_START_PACKET_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    *packet_len = STREAM_START_PACKET_SIZE;

    qret_protocol_ret ret;

    const qret_protocol_header header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_STREAM_START,
        .sequence = stream_start->header.sequence,
        .packet_length = STREAM_START_PACKET_SIZE,
        .timestamp = stream_start->header.timestamp,
    };

    ret= s_pack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }

    buffer[HEADER_SIZE + 0] = (uint8_t)(stream_start->stream_frequency >> 8);
    buffer[HEADER_SIZE + 1] = (uint8_t)stream_start->stream_frequency;

    return PROTOCOL_OK;
}

qret_protocol_ret make_control_packet(uint8_t buffer[], size_t *packet_len, const qret_control_packet *control) {
    if (buffer == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (*packet_len < CONTROL_PACKET_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    *packet_len = CONTROL_PACKET_SIZE;

    qret_protocol_ret ret;

    const qret_protocol_header header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_CONTROL,
        .sequence = control->header.sequence,
        .packet_length = CONTROL_PACKET_SIZE,
        .timestamp = control->header.timestamp,
    };

    ret= s_pack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }

    buffer[HEADER_SIZE + 0] = control->command_id;
    buffer[HEADER_SIZE + 1] = control->command_state;

    return PROTOCOL_OK;
}

qret_protocol_ret make_status_packet(uint8_t buffer[], size_t *packet_len, const qret_status_packet *status) {
    if (buffer == NULL || packet_len == NULL || status == NULL || status->control_data == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (*packet_len < STATUS_PACKET_SIZE(status->control_count)) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    *packet_len = STATUS_PACKET_SIZE(status->control_count);

    qret_protocol_ret ret;

    const qret_protocol_header header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_STATUS,
        .sequence = status->header.sequence,
        .packet_length = STATUS_PACKET_SIZE(status->control_count),
        .timestamp = status->header.timestamp,
    };

    ret= s_pack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }

    buffer[HEADER_SIZE + 0] = status->device_status;
    buffer[HEADER_SIZE + 1] = status->control_count;

    for (size_t i = 0; i < status->control_count; i++) {
        size_t offset = STATUS_PACKET_SIZE(i);
        buffer[offset + 0] = status->control_data[i].control_id;
        buffer[offset + 1] = status->control_data[i].control_state;
    }
    return PROTOCOL_OK;
}

qret_protocol_ret make_data_packet(uint8_t buffer[], size_t *packet_len, const qret_data_packet *data) {
    if (buffer == NULL || packet_len == NULL || data == NULL || data->sensor_data == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (*packet_len < DATA_PACKET_SIZE(data->sensor_count)) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    *packet_len = DATA_PACKET_SIZE(data->sensor_count);

    qret_protocol_ret ret;

    const qret_protocol_header header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_DATA,
        .sequence = data->header.sequence,
        .packet_length = DATA_PACKET_SIZE(data->sensor_count),
        .timestamp = data->header.timestamp,
    };

    ret= s_pack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }

    buffer[HEADER_SIZE + 0] = data->sensor_count;

    for (size_t i = 0; i < data->sensor_count; i++) {
        size_t offset = DATA_PACKET_SIZE(i);
        buffer[offset + 0] = data->sensor_data[i].sensor_id;
        buffer[offset + 1] = data->sensor_data[i].unit;
        // Float to bytes
        uint32_t value_bytes;
        memcpy(&value_bytes, &data->sensor_data[i].value, sizeof(uint32_t));
        buffer[offset + 2] = (uint8_t)(value_bytes >> 24);
        buffer[offset + 3] = (uint8_t)(value_bytes >> 16);
        buffer[offset + 4] = (uint8_t)(value_bytes >> 8);
        buffer[offset + 5] = (uint8_t)(value_bytes);
    }
    return PROTOCOL_OK;
}

qret_protocol_ret make_config_packet(uint8_t buffer[], size_t *packet_len, const qret_config_packet *config) {
    if (buffer == NULL || packet_len == NULL || config == NULL || config->json_config == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    uint32_t packet_data_len = config->json_config_len;
    if (config->json_config_len > 0 && config->json_config[config->json_config_len - 1] == '\0') {
        packet_data_len--; // Remove null terminator from json_config
    }
    if (*packet_len < CONFIG_PACKET_SIZE(packet_data_len)) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }
    *packet_len = CONFIG_PACKET_SIZE(packet_data_len);

    qret_protocol_ret ret;

    const qret_protocol_header header_data = {
        .protocol_version = QRET_PROTOCOL_VERSION,
        .packet_type = PT_CONFIG,
        .sequence = config->header.sequence,
        .packet_length = HEADER_SIZE + (4 + packet_data_len),
        .timestamp = config->header.timestamp,
    };

    ret= s_pack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }

    buffer[HEADER_SIZE + 0] = (uint8_t)(packet_data_len >> 24);
    buffer[HEADER_SIZE + 1] = (uint8_t)(packet_data_len >> 16);
    buffer[HEADER_SIZE + 2] = (uint8_t)(packet_data_len >> 8);
    buffer[HEADER_SIZE + 3] = (uint8_t)packet_data_len;

    memcpy(buffer + (HEADER_SIZE + 4), config->json_config, packet_data_len);

    return PROTOCOL_OK;
}

qret_protocol_ret get_packet_len(const uint8_t buffer[], size_t buffer_len, uint16_t *data_len) {
    if (buffer == NULL || data_len == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (buffer_len < HEADER_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }

    qret_protocol_ret ret;
    qret_protocol_header header_data = {0};

    ret= s_unpack_header(buffer, buffer_len, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }

    *data_len = header_data.packet_length;
    return PROTOCOL_OK;
}

qret_protocol_ret server_parse_packet(const uint8_t buffer[], size_t buffer_len, qret_server_payload *payload) {
    if (buffer == NULL || payload == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (buffer_len < HEADER_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }

    qret_protocol_ret ret;
    qret_protocol_header header_data = {0};

    ret= s_unpack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }
    if (buffer_len < header_data.packet_length) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }

    // Tag the packet type
    payload->packet_type = header_data.packet_type;

    switch (header_data.packet_type) {
    case PT_ACK:
        if (header_data.packet_length != ACK_PACKET_SIZE) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        if (buffer[HEADER_SIZE + 2] != ERR_NONE) {
            return PROTOCOL_INVALID_PACKET_TYPE_ERR;
        }
        payload->payload_data.ack.ack_packet_type = buffer[HEADER_SIZE + 0];
        payload->payload_data.ack.ack_sequence = buffer[HEADER_SIZE + 1];
        payload->payload_data.ack.header.sequence = header_data.sequence;
        payload->payload_data.ack.header.timestamp = header_data.timestamp;
        break;
    case PT_NACK:
        if (header_data.packet_length != NACK_PACKET_SIZE) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        payload->payload_data.nack.nack_packet_type = buffer[HEADER_SIZE + 0];
        payload->payload_data.nack.nack_sequence = buffer[HEADER_SIZE + 1];
        payload->payload_data.nack.nack_error_code = buffer[HEADER_SIZE + 2];
        payload->payload_data.nack.header.sequence = header_data.sequence;
        payload->payload_data.nack.header.timestamp = header_data.timestamp;
        break;
    case PT_STATUS:
        uint8_t control_count = buffer[HEADER_SIZE + 1];
        if (header_data.packet_length != STATUS_PACKET_SIZE(control_count)) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        payload->payload_data.status.device_status = buffer[HEADER_SIZE + 0];
        payload->payload_data.status.control_count = buffer[HEADER_SIZE + 1];

        for (size_t i = 0; i < control_count; i++) {
            size_t offset = STATUS_PACKET_SIZE(i);
            payload->payload_data.status.control_data[i].control_id = buffer[offset + 0];
            payload->payload_data.status.control_data[i].control_state = buffer[offset + 1];
        }
        payload->payload_data.status.header.sequence = header_data.sequence;
        payload->payload_data.status.header.timestamp = header_data.timestamp;
        break;
    case PT_DATA:
        uint8_t sensor_count = buffer[HEADER_SIZE + 0];
        if (header_data.packet_length != DATA_PACKET_SIZE(sensor_count)) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        payload->payload_data.data.sensor_count = sensor_count;

        for (size_t i = 0; i < sensor_count; i++) {
            size_t offset = DATA_PACKET_SIZE(i);
            payload->payload_data.data.sensor_data[i].sensor_id = buffer[offset + 0];
            payload->payload_data.data.sensor_data[i].unit = buffer[offset + 1];
            // Bytes to float
            uint32_t value_bytes = (uint32_t)(buffer[offset + 2] << 24) | (uint32_t)(buffer[offset + 3] << 16) |
                                   (uint32_t)(buffer[offset + 4] << 8) | (uint32_t)(buffer[offset + 5]);
            memcpy(&payload->payload_data.data.sensor_data[i].value, &value_bytes, sizeof(uint32_t));
        }
        payload->payload_data.data.header.sequence = header_data.sequence;
        payload->payload_data.data.header.timestamp = header_data.timestamp;
        break;
    case PT_CONFIG:
        uint32_t data_len = (uint32_t)(buffer[HEADER_SIZE + 0] << 24) | (uint32_t)(buffer[HEADER_SIZE + 1] << 16) |
                            (uint32_t)(buffer[HEADER_SIZE + 2] << 8) | (uint32_t)buffer[HEADER_SIZE + 3];
        if (header_data.packet_length != CONFIG_PACKET_SIZE(data_len)) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        payload->payload_data.config.json_config_len = data_len;

        payload->payload_data.config.json_config = (char *)(buffer + CONFIG_PACKET_SIZE(0));

        break;
    default:
        return PROTOCOL_INVALID_PACKET_TYPE_ERR;
    }

    return PROTOCOL_OK;
}

qret_protocol_ret client_parse_packet(const uint8_t buffer[], size_t buffer_len, qret_client_payload *payload) {
    if (buffer == NULL || payload == NULL) {
        return PROTOCOL_NULL_PTR_ERR;
    }
    if (buffer_len < HEADER_SIZE) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }

    qret_protocol_ret ret;
    qret_protocol_header header_data = {0};

    ret= s_unpack_header(buffer, HEADER_SIZE, &header_data);
    if (ret!= PROTOCOL_OK) {
        return ret;
    }
    if (buffer_len < header_data.packet_length) {
        return PROTOCOL_BUFFER_LEN_ERR;
    }

    // Tag the packet type
    payload->packet_type = header_data.packet_type;

    switch (header_data.packet_type) {
    case PT_ESTOP:
    case PT_DISCOVERY:
    case PT_TIMESYNC:
    case PT_STREAM_STOP:
    case PT_GET_SINGLE:
    case PT_HEARTBEAT:
    case PT_STATUS_REQUEST:
        if (header_data.packet_length != HEADER_SIZE) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        payload->payload_data.header_only.sequence = header_data.sequence;
        payload->payload_data.header_only.timestamp = header_data.timestamp;
        break;
    case PT_ACK:
        if (header_data.packet_length != ACK_PACKET_SIZE) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        if (buffer[HEADER_SIZE + 2] != ERR_NONE) {
            return PROTOCOL_INVALID_PACKET_TYPE_ERR;
        }
        payload->payload_data.ack.ack_packet_type = buffer[HEADER_SIZE + 0];
        payload->payload_data.ack.ack_sequence = buffer[HEADER_SIZE + 1];
        payload->payload_data.ack.header.sequence = header_data.sequence;
        payload->payload_data.ack.header.timestamp = header_data.timestamp;
        break;
    case PT_NACK:
        if (header_data.packet_length != NACK_PACKET_SIZE) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        payload->payload_data.nack.nack_packet_type = buffer[HEADER_SIZE + 0];
        payload->payload_data.nack.nack_sequence = buffer[HEADER_SIZE + 1];
        payload->payload_data.nack.nack_error_code = buffer[HEADER_SIZE + 2];
        payload->payload_data.nack.header.sequence = header_data.sequence;
        payload->payload_data.nack.header.timestamp = header_data.timestamp;
        break;
    case PT_STREAM_START:
        if (header_data.packet_length != STREAM_START_PACKET_SIZE) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        payload->payload_data.stream_start.stream_frequency = ((uint16_t)buffer[HEADER_SIZE + 0] << 8) |
                                                              (uint16_t)buffer[HEADER_SIZE + 1];
        payload->payload_data.stream_start.header.sequence = header_data.sequence;
        payload->payload_data.stream_start.header.timestamp = header_data.timestamp;
        break;
    case PT_CONTROL:
        if (header_data.packet_length != CONTROL_PACKET_SIZE) {
            return PROTOCOL_BUFFER_LEN_ERR;
        }
        payload->payload_data.control.command_id = buffer[HEADER_SIZE + 0];
        payload->payload_data.control.command_state = buffer[HEADER_SIZE + 1];
        payload->payload_data.control.header.sequence = header_data.sequence;
        payload->payload_data.control.header.timestamp = header_data.timestamp;
        break;
    default:
        return PROTOCOL_INVALID_PACKET_TYPE_ERR;
    }

    return PROTOCOL_OK;
}
