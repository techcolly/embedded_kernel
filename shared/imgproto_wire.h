#pragma once
#include <stdint.h>

/*
 * Wire-format definitions shared between host (proto/) and firmware (firmware/).
 * No OS dependencies. Safe to include on bare metal.
 */

#define IMG_H           128
#define IMG_W           128
#define MAGIC_LEN       4
#define RGB_BYTES       3
#define PAYLOAD_SIZE    (IMG_H * IMG_W * RGB_BYTES)
#define HEADER_LEN      (MAGIC_LEN + 11)

/* Protocol mode (bits 1-2 of flags) */
#define PMODE_SENDING    (0b00 << 1)
#define PMODE_REQUESTING (0b01 << 1)
#define PMODE_STATUS     (0b10 << 1)
#define PMODE_SYNC       (0b11 << 1)
#define PMODE_MASK       (0b11 << 1)

/* Status codes (bits 3-5 of flags) */
#define STATUS_OK                  (0b000 << 3)
#define STATUS_MALFORMED_PACKET    (0b001 << 3)
#define STATUS_PACKET_TOO_LONG     (0b010 << 3)
#define STATUS_INVALID_FILE_OR_PATH (0b011 << 3)
#define STATUS_REQUEST_DENIED      (0b100 << 3)

/* Color/encryption flags (bits 6-10) */
#define USE_ENC          (1 << 10)
#define USE_OB           (1 << 9)
#define CMODE_NO_R       (1 << 8)
#define CMODE_NO_G       (1 << 7)
#define CMODE_NO_B       (1 << 6)
#define CMODE_GRAYSCALE  (0b111 << 6)
#define MASK_CMODE       (0b111 << 6)

typedef struct __attribute__((packed)) {
    uint8_t  magic[MAGIC_LEN];
    uint8_t  num_chunks;
    uint16_t flags;
    uint32_t payload_len;
    uint16_t filename_len;
    uint16_t path_len;
} WireHeader;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Pixel;

typedef struct {
    Pixel pixels[IMG_H][IMG_W];
} Image;

extern const char MAGIC_BYTES[MAGIC_LEN + 1];
