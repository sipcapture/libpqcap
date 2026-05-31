#include "pqcap.h"

#include "pqcap_internal.h"

#include <string.h>

int pqcap_footer_parse(const uint8_t buf[PQCAP_FOOTER_SIZE], pqcap_footer_t *out) {
  if (!buf || !out) {
    return PQCAP_ERR_INVALID_ARG;
  }

  const uint32_t block_type = pqcap_read_u32_le(buf + 0);
  const uint32_t block_len = pqcap_read_u32_le(buf + 4);
  const uint32_t tail_len = pqcap_read_u32_le(buf + PQCAP_FOOTER_SIZE - 4);
  if (block_type != PQCAP_FOOTER_BLOCK_TYPE || block_len != PQCAP_FOOTER_SIZE ||
      tail_len != PQCAP_FOOTER_SIZE) {
    return PQCAP_ERR_BAD_MAGIC;
  }

  const uint32_t enterprise = pqcap_read_u32_le(buf + 8);
  if (enterprise != PQCAP_ENTERPRISE_ID) {
    return PQCAP_ERR_BAD_MAGIC;
  }
  if (memcmp(buf + 12, PQCAP_FOOTER_MAGIC, 8) != 0) {
    return PQCAP_ERR_BAD_MAGIC;
  }

  const uint16_t version = pqcap_read_u16_le(buf + 20);
  if (version > PQCAP_FOOTER_VERSION) {
    return PQCAP_ERR_BAD_VERSION;
  }

  out->parquet_offset = pqcap_read_u64_le(buf + 24);
  out->parquet_length = pqcap_read_u64_le(buf + 32);
  return PQCAP_OK;
}

int pqcap_footer_build(const pqcap_footer_t *in, uint8_t buf[PQCAP_FOOTER_SIZE]) {
  if (!in || !buf) {
    return PQCAP_ERR_INVALID_ARG;
  }

  pqcap_write_u32_le(buf + 0, PQCAP_FOOTER_BLOCK_TYPE);
  pqcap_write_u32_le(buf + 4, PQCAP_FOOTER_SIZE);
  pqcap_write_u32_le(buf + 8, PQCAP_ENTERPRISE_ID);
  memcpy(buf + 12, PQCAP_FOOTER_MAGIC, 8);
  pqcap_write_u16_le(buf + 20, PQCAP_FOOTER_VERSION);
  pqcap_write_u16_le(buf + 22, 0);
  pqcap_write_u64_le(buf + 24, in->parquet_offset);
  pqcap_write_u64_le(buf + 32, in->parquet_length);
  pqcap_write_u32_le(buf + 40, PQCAP_FOOTER_SIZE);
  return PQCAP_OK;
}

int pqcap_validate_range(uint64_t file_size, const pqcap_footer_t *footer) {
  if (!footer) {
    return PQCAP_ERR_INVALID_ARG;
  }
  if (footer->parquet_length == 0) {
    return PQCAP_ERR_RANGE;
  }
  if (footer->parquet_offset > file_size) {
    return PQCAP_ERR_RANGE;
  }
  if (footer->parquet_length > file_size - footer->parquet_offset) {
    return PQCAP_ERR_RANGE;
  }
  if (footer->parquet_offset + footer->parquet_length < footer->parquet_offset) {
    return PQCAP_ERR_RANGE;
  }
  return PQCAP_OK;
}
