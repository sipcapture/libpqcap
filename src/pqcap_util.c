#include "pqcap_internal.h"

uint16_t pqcap_read_u16_le(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t pqcap_read_u32_le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

uint64_t pqcap_read_u64_le(const uint8_t *p) {
  return (uint64_t)pqcap_read_u32_le(p) |
         ((uint64_t)pqcap_read_u32_le(p + 4) << 32);
}

void pqcap_write_u16_le(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xffu);
  p[1] = (uint8_t)((v >> 8) & 0xffu);
}

void pqcap_write_u32_le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xffu);
  p[1] = (uint8_t)((v >> 8) & 0xffu);
  p[2] = (uint8_t)((v >> 16) & 0xffu);
  p[3] = (uint8_t)((v >> 24) & 0xffu);
}

void pqcap_write_u64_le(uint8_t *p, uint64_t v) {
  pqcap_write_u32_le(p, (uint32_t)(v & 0xffffffffu));
  pqcap_write_u32_le(p + 4, (uint32_t)(v >> 32));
}

size_t pqcap_pad4_size(size_t len) {
  const size_t pad = (4u - (len % 4u)) % 4u;
  return len + pad;
}
