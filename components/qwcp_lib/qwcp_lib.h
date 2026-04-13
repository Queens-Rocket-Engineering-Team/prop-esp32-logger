#ifndef QWCP_LIB_H
#define QWCP_LIB_H

#include <stdint.h>

//----------------------------------------------------------
// Enums
//----------------------------------------------------------

typedef enum {
    // Return error codes
    QWCP_OK,
    QWCP_NULL_PTR,
    QWCP_NO_MEM,
    QWCP_LEN_MISMATCH,
    QWCP_VERSION_MISMATCH,
    QWCP_INVALID_PACKET_TYPE,
} qwcp_lib_ret;

typedef enum {
    // Server -> Device
    QWCP_PT_ESTOP = 0x00,
    QWCP_PT_DISCOVERY = 0x01,
    QWCP_PT_TIMESYNC = 0x02,
    QWCP_PT_CONTROL = 0x03,
    QWCP_PT_STATUS_REQUEST = 0x04,
    QWCP_PT_STREAM_START = 0x05,
    QWCP_PT_STREAM_STOP = 0x06,
    QWCP_PT_GET_SINGLE = 0x07,
    QWCP_PT_HEARTBEAT = 0x08,
    // Device -> Server
    QWCP_PT_CONFIG = 0x10,
    QWCP_PT_DATA = 0x11,
    QWCP_PT_STATUS = 0x12,
    // All
    QWCP_PT_ACK = 0x13,
    QWCP_PT_NACK = 0x14,
} qwcp_packet_type;

typedef enum {
    // Device status
    QWCP_DS_INACTIVE = 0x00,
    QWCP_DS_ACTIVE = 0x01,
    QWCP_DS_ERROR = 0x02,
    QWCP_DS_CALIBRATING = 0x03,
} qwcp_device_status;

typedef enum {
    // Control state
    QWCP_CS_CLOSED = 0x00,
    QWCP_CS_OPEN = 0x01,
    QWCP_CS_ERROR = 0xFF,
} qwcp_control_state;

typedef enum {
    QWCP_UNIT_VOLTS = 0x00,
    QWCP_UNIT_AMPS = 0x01,
    QWCP_UNIT_CELSIUS = 0x02,
    QWCP_UNIT_FAHRENHEIT = 0x03,
    QWCP_UNIT_KELVIN = 0x04,
    QWCP_UNIT_PSI = 0x05,
    QWCP_UNIT_BAR = 0x06,
    QWCP_UNIT_PASCAL = 0x07,
    QWCP_UNIT_GRAMS = 0x08,
    QWCP_UNIT_KILOGRAMS = 0x09,
    QWCP_UNIT_POUNDS = 0x0A,
    QWCP_UNIT_NEWTONS = 0x0B,
    QWCP_UNIT_SECONDS = 0x0C,
    QWCP_UNIT_MILLISECONDS = 0x0D,
    QWCP_UNIT_HERTZ = 0x0E,
    QWCP_UNIT_OHMS = 0x0F,
    QWCP_UNIT_UNITLESS = 0xFF,
} qwcp_unit;

typedef enum {
    // Packet error codes
    QWCP_ERR_NONE = 0x00,
    QWCP_ERR_UNKNOWN_TYPE = 0x01,
    QWCP_ERR_INVALID_ID = 0x02,
    QWCP_ERR_HARDWARE_FAULT = 0x03,
    QWCP_ERR_BUSY = 0x04,
    QWCP_ERR_NOT_STREAMING = 0x05,
    QWCP_ERR_INVALID_PARAM = 0x06,
} qwcp_err_code;

//----------------------------------------------------------
// Packet sizes
//----------------------------------------------------------

#define QWCP_STREAM_START_DATA_SIZE 2
#define QWCP_CONTROL_DATA_SIZE 2
#define QWCP_ACK_DATA_SIZE 3
#define QWCP_NACK_DATA_SIZE 3

#define QWCP_CONTROL_STATUS_DATA_SIZE 2
#define QWCP_STATUS_DATA_SIZE(control_count) (2 + (QWCP_CONTROL_STATUS_DATA_SIZE * control_count))

#define QWCP_SENSOR_DATA_SIZE 6
#define QWCP_DATA_DATA_SIZE(sensor_count) (1 + (QWCP_SENSOR_DATA_SIZE * sensor_count))

#define QWCP_CONFIG_DATA_SIZE(config_len) (4 + config_len)

#define QWCP_HEADER_SIZE 9

#define QWCP_STREAM_START_PACKET_SIZE (QWCP_HEADER_SIZE + QWCP_STREAM_START_DATA_SIZE)
#define QWCP_CONTROL_PACKET_SIZE (QWCP_HEADER_SIZE + QWCP_CONTROL_DATA_SIZE)
#define QWCP_ACK_PACKET_SIZE (QWCP_HEADER_SIZE + QWCP_ACK_DATA_SIZE)
#define QWCP_NACK_PACKET_SIZE (QWCP_HEADER_SIZE + QWCP_NACK_DATA_SIZE)
#define QWCP_STATUS_PACKET_SIZE(control_count) (QWCP_HEADER_SIZE + QWCP_STATUS_DATA_SIZE(control_count))
#define QWCP_DATA_PACKET_SIZE(sensor_count) (QWCP_HEADER_SIZE + QWCP_DATA_DATA_SIZE(sensor_count))
#define QWCP_CONFIG_PACKET_SIZE(config_len) (QWCP_HEADER_SIZE + QWCP_CONFIG_DATA_SIZE(config_len))

//----------------------------------------------------------
// Packet info structs
//----------------------------------------------------------

// Generic header struct
typedef struct {
    uint8_t sequence;
    uint32_t timestamp;
} qwcp_header;

// Struct for variable-length control data data in status packet
typedef struct {
    uint8_t control_id;
    uint8_t control_state;
} qwcp_control_data;

// Struct for variable-length sensor data in data packet
typedef struct {
    uint8_t sensor_id;
    uint8_t unit;
    float value;
} qwcp_sensor_data;

// Packet structs

typedef qwcp_header qwcp_header_only_packet;

typedef struct {
    qwcp_header header;
    uint16_t stream_frequency;
} qwcp_stream_start_packet;

typedef struct {
    qwcp_header header;
    uint8_t command_id;
    uint8_t command_state;
} qwcp_control_packet;

typedef struct {
    qwcp_header header;
    uint8_t ack_packet_type;
    uint8_t ack_sequence;
} qwcp_ack_packet;

typedef struct {
    qwcp_header header;
    uint8_t nack_packet_type;
    uint8_t nack_sequence;
    uint8_t nack_error_code;
} qwcp_nack_packet;

// Variable length packets

typedef struct {
    qwcp_header header;
    const qwcp_control_data *control_data;
    uint8_t control_count;
    uint8_t device_status;
} qwcp_status_packet;

typedef struct {
    qwcp_header header;
    const qwcp_sensor_data *sensor_data;
    uint8_t sensor_count;
} qwcp_data_packet;

typedef struct {
    qwcp_header header;
    const char *config_data;
    uint32_t config_data_len;
} qwcp_config_packet;

// Payload tagged unions

typedef struct {
    qwcp_control_data *control_data;
    uint8_t control_data_len;
    qwcp_sensor_data *sensor_data;
    uint8_t sensor_data_len;
    char *config_data;
    uint32_t config_data_len;
} qwcp_server_payload_buffers;

typedef struct {
    qwcp_packet_type packet_type;
    union {
        qwcp_config_packet config;
        qwcp_data_packet data;
        qwcp_status_packet status;
        qwcp_ack_packet ack;
        qwcp_nack_packet nack;
    } payload_data;
} qwcp_server_payload;

typedef struct {
    qwcp_packet_type packet_type;
    union {
        qwcp_header_only_packet header_only;
        qwcp_control_packet control;
        qwcp_stream_start_packet stream_start;
        qwcp_ack_packet ack;
        qwcp_nack_packet nack;
    } payload_data;
} qwcp_client_payload;

//----------------------------------------------------------
// Packet structs to raw bytes:
// Takes a pointer to a packet struct, and
// fills buffer with the encoded packet.
// buffer_len should be a pointer to the length
// of buffer, which will update to the length of
// the packet in the buffer.
//----------------------------------------------------------

qwcp_lib_ret qwcp_encode_header_only(uint8_t buffer[], size_t *buffer_len, qwcp_packet_type packet_type, const qwcp_header_only_packet *header_only);

qwcp_lib_ret qwcp_encode_ack(uint8_t buffer[], size_t *buffer_len, const qwcp_ack_packet *ack);

qwcp_lib_ret qwcp_encode_nack(uint8_t buffer[], size_t *buffer_len, const qwcp_nack_packet *nack);

qwcp_lib_ret qwcp_encode_stream_start(uint8_t buffer[], size_t *buffer_len, const qwcp_stream_start_packet *stream_start);

qwcp_lib_ret qwcp_encode_control(uint8_t buffer[], size_t *buffer_len, const qwcp_control_packet *control);

// Status, data, and config packet structs include pointers to a variable length array.
// When these functions are called, the data at that address must still be intact.

qwcp_lib_ret qwcp_encode_status(uint8_t buffer[], size_t *buffer_len, const qwcp_status_packet *status);

qwcp_lib_ret qwcp_encode_data(uint8_t buffer[], size_t *buffer_len, const qwcp_data_packet *data);

qwcp_lib_ret qwcp_encode_config(uint8_t buffer[], size_t *buffer_len, const qwcp_config_packet *config);

//----------------------------------------------------------
// Raw bytes to packet struct:
// qwcp_decode_client_to_server should be used server-side,
// and qwcp_decode_server_to_client should be used client-side.
// Takes an encoded packet in buffer and decodes into
// payload, as a tagged union.
//----------------------------------------------------------

qwcp_lib_ret qwcp_get_packet_len(uint16_t *data_len, const uint8_t buffer[], size_t buffer_len);

qwcp_lib_ret qwcp_decode_client_to_server(qwcp_server_payload *payload, qwcp_server_payload_buffers *payload_buffers, const uint8_t buffer[], size_t buffer_len);

qwcp_lib_ret qwcp_decode_server_to_client(qwcp_client_payload *payload, const uint8_t buffer[], size_t buffer_len);

#endif