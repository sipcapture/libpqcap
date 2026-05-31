#include "pqcap.h"

#include "pqcap_internal.h"

#include <stdlib.h>
#include <string.h>

static int scan_metadata_block(const uint8_t *data, size_t data_len, uint8_t **out,
                               size_t *out_len) {
  size_t pos = 0;
  while (pos + 12 <= data_len) {
    const uint32_t block_type = pqcap_read_u32_le(data + pos);
    const uint32_t total_len = pqcap_read_u32_le(data + pos + 4);
    if (total_len < 12 || pos + total_len > data_len) {
      break;
    }
    const uint32_t tail_len = pqcap_read_u32_le(data + pos + total_len - 4);
    if (tail_len != total_len) {
      break;
    }

    if (block_type == PQCAP_CUSTOM_BLOCK_TYPE) {
      const uint8_t *body = data + pos + 8;
      const size_t body_len = total_len - 12;
      if (body_len >= 4 + 9 + 2 + 2 + 8) {
        const uint32_t enterprise = pqcap_read_u32_le(body);
        if (enterprise == PQCAP_ENTERPRISE_ID &&
            memcmp(body + 4, PQCAP_METADATA_MAGIC, 9) == 0) {
          const uint64_t parquet_len = pqcap_read_u64_le(body + 17);
          const size_t start = 25;
          const size_t end = start + (size_t)parquet_len;
          if (end <= body_len) {
            uint8_t *copy = (uint8_t *)malloc((size_t)parquet_len);
            if (!copy) {
              return PQCAP_ERR_NOMEM;
            }
            memcpy(copy, body + start, (size_t)parquet_len);
            *out = copy;
            *out_len = (size_t)parquet_len;
            return PQCAP_OK;
          }
        }
      }
    }
    pos += total_len;
  }
  return PQCAP_ERR_NOT_FOUND;
}

int pqcap_extract_parquet(const uint8_t *data, size_t data_len,
                          const pqcap_footer_t *footer, uint8_t **out,
                          size_t *out_len) {
  if (!data || !footer || !out || !out_len) {
    return PQCAP_ERR_INVALID_ARG;
  }

  const int range_err = pqcap_validate_range(data_len, footer);
  if (range_err != PQCAP_OK) {
    return range_err;
  }

  const size_t offset = (size_t)footer->parquet_offset;
  const size_t length = (size_t)footer->parquet_length;
  uint8_t *copy = (uint8_t *)malloc(length);
  if (!copy) {
    return PQCAP_ERR_NOMEM;
  }
  memcpy(copy, data + offset, length);
  *out = copy;
  *out_len = length;
  return PQCAP_OK;
}

int pqcap_extract_parquet_from_buffer(const uint8_t *data, size_t data_len,
                                      uint8_t **out, size_t *out_len) {
  if (!data || !out || !out_len) {
    return PQCAP_ERR_INVALID_ARG;
  }

  if (data_len >= PQCAP_FOOTER_SIZE) {
    pqcap_footer_t footer;
    const int parse_err =
        pqcap_footer_parse(data + data_len - PQCAP_FOOTER_SIZE, &footer);
    if (parse_err == PQCAP_OK) {
      return pqcap_extract_parquet(data, data_len, &footer, out, out_len);
    }
  }

  return scan_metadata_block(data, data_len, out, out_len);
}
