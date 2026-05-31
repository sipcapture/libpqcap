#include "pqcap.h"

#include "pqcap_internal.h"

#include <stdlib.h>
#include <string.h>

static int append_bytes(uint8_t **buf, size_t *len, size_t *cap, const uint8_t *src,
                        size_t src_len) {
  if (*len + src_len < *len) {
    return PQCAP_ERR_RANGE;
  }
  if (*len + src_len > *cap) {
    size_t new_cap = *cap ? *cap : 256;
    while (new_cap < *len + src_len) {
      new_cap *= 2;
    }
    uint8_t *new_buf = (uint8_t *)realloc(*buf, new_cap);
    if (!new_buf) {
      return PQCAP_ERR_NOMEM;
    }
    *buf = new_buf;
    *cap = new_cap;
  }
  memcpy(*buf + *len, src, src_len);
  *len += src_len;
  return PQCAP_OK;
}

static int append_u32(uint8_t **buf, size_t *len, size_t *cap, uint32_t v) {
  uint8_t tmp[4];
  pqcap_write_u32_le(tmp, v);
  return append_bytes(buf, len, cap, tmp, sizeof(tmp));
}

static int append_u16(uint8_t **buf, size_t *len, size_t *cap, uint16_t v) {
  uint8_t tmp[2];
  pqcap_write_u16_le(tmp, v);
  return append_bytes(buf, len, cap, tmp, sizeof(tmp));
}

static int append_u64(uint8_t **buf, size_t *len, size_t *cap, uint64_t v) {
  uint8_t tmp[8];
  pqcap_write_u64_le(tmp, v);
  return append_bytes(buf, len, cap, tmp, sizeof(tmp));
}

int pqcap_embed(const uint8_t *capture, size_t capture_len, const uint8_t *parquet,
                size_t parquet_len, uint8_t **out, size_t *out_len) {
  if (!capture || !parquet || !out || !out_len) {
    return PQCAP_ERR_INVALID_ARG;
  }

  pqcap_footer_t existing;
  if (pqcap_has_footer(capture, capture_len, &existing) == PQCAP_OK) {
    return PQCAP_ERR_ALREADY_INDEXED;
  }

  uint8_t *body = NULL;
  size_t body_len = 0;
  size_t body_cap = 0;
  int err = PQCAP_OK;

  err = append_u32(&body, &body_len, &body_cap, PQCAP_ENTERPRISE_ID);
  if (err != PQCAP_OK) {
    goto fail;
  }
  err = append_bytes(&body, &body_len, &body_cap, (const uint8_t *)PQCAP_METADATA_MAGIC, 9);
  if (err != PQCAP_OK) {
    goto fail;
  }
  err = append_u16(&body, &body_len, &body_cap, PQCAP_METADATA_VERSION);
  if (err != PQCAP_OK) {
    goto fail;
  }
  err = append_u16(&body, &body_len, &body_cap, 0);
  if (err != PQCAP_OK) {
    goto fail;
  }
  err = append_u64(&body, &body_len, &body_cap, (uint64_t)parquet_len);
  if (err != PQCAP_OK) {
    goto fail;
  }
  err = append_bytes(&body, &body_len, &body_cap, parquet, parquet_len);
  if (err != PQCAP_OK) {
    goto fail;
  }

  const size_t padded_body_len = pqcap_pad4_size(body_len);
  if (padded_body_len > body_len) {
    const size_t pad = padded_body_len - body_len;
    uint8_t zeros[3] = {0, 0, 0};
    err = append_bytes(&body, &body_len, &body_cap, zeros, pad);
    if (err != PQCAP_OK) {
      goto fail;
    }
  }

  const uint32_t custom_len = (uint32_t)(12 + body_len);
  uint8_t *custom_block = NULL;
  size_t custom_len_total = 0;
  size_t custom_cap = 0;

  err = append_u32(&custom_block, &custom_len_total, &custom_cap, PQCAP_CUSTOM_BLOCK_TYPE);
  if (err != PQCAP_OK) {
    goto fail_custom;
  }
  err = append_u32(&custom_block, &custom_len_total, &custom_cap, custom_len);
  if (err != PQCAP_OK) {
    goto fail_custom;
  }
  err = append_bytes(&custom_block, &custom_len_total, &custom_cap, body, body_len);
  if (err != PQCAP_OK) {
    goto fail_custom;
  }
  err = append_u32(&custom_block, &custom_len_total, &custom_cap, custom_len);
  if (err != PQCAP_OK) {
    goto fail_custom;
  }

  const uint64_t parquet_abs_offset =
      (uint64_t)capture_len + 8 + PQCAP_METADATA_HEADER_SIZE;

  pqcap_footer_t footer = {
      .parquet_offset = parquet_abs_offset,
      .parquet_length = (uint64_t)parquet_len,
  };

  uint8_t footer_buf[PQCAP_FOOTER_SIZE];
  err = pqcap_footer_build(&footer, footer_buf);
  if (err != PQCAP_OK) {
    goto fail_custom;
  }

  const size_t total_len = capture_len + custom_len_total + PQCAP_FOOTER_SIZE;
  uint8_t *result = (uint8_t *)malloc(total_len);
  if (!result) {
    err = PQCAP_ERR_NOMEM;
    goto fail_custom;
  }

  memcpy(result, capture, capture_len);
  memcpy(result + capture_len, custom_block, custom_len_total);
  memcpy(result + capture_len + custom_len_total, footer_buf, PQCAP_FOOTER_SIZE);

  free(body);
  free(custom_block);
  *out = result;
  *out_len = total_len;
  return PQCAP_OK;

fail_custom:
  free(custom_block);
fail:
  free(body);
  return err;
}
