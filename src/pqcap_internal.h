#ifndef PQCAP_INTERNAL_H
#define PQCAP_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#define PQCAP_CUSTOM_BLOCK_TYPE 0x00000BADu
#define PQCAP_FOOTER_BLOCK_TYPE 0x00000BEEu
#define PQCAP_ENTERPRISE_ID 0x51584950u
#define PQCAP_METADATA_MAGIC "PQCAPMETA"
#define PQCAP_FOOTER_MAGIC "PQCAPFTR"
#define PQCAP_METADATA_HEADER_SIZE 25u
#define PQCAP_FOOTER_VERSION 1u
#define PQCAP_METADATA_VERSION 1u

uint16_t pqcap_read_u16_le(const uint8_t *p);
uint32_t pqcap_read_u32_le(const uint8_t *p);
uint64_t pqcap_read_u64_le(const uint8_t *p);

void pqcap_write_u16_le(uint8_t *p, uint16_t v);
void pqcap_write_u32_le(uint8_t *p, uint32_t v);
void pqcap_write_u64_le(uint8_t *p, uint64_t v);

size_t pqcap_pad4_size(size_t len);

#endif /* PQCAP_INTERNAL_H */
