#include "pqcap.h"

#include <stdio.h>
#include <string.h>

static const uint8_t kCapture[] = {
    0x0a, 0x0d, 0x0d, 0x0a, /* SHB magic */
    0x1c, 0x00, 0x00, 0x00, /* block length */
};

static const uint8_t kParquet[] = {'P', 'A', 'R', '1', 0x01, 0x02, 0x03};

int main(void) {
  uint8_t *embedded = NULL;
  size_t embedded_len = 0;
  int err = pqcap_embed(kCapture, sizeof(kCapture), kParquet, sizeof(kParquet),
                        &embedded, &embedded_len);
  if (err != PQCAP_OK) {
    fprintf(stderr, "embed failed: %s\n", pqcap_strerror(err));
    return 1;
  }

  if (memcmp(embedded, kCapture, sizeof(kCapture)) != 0) {
    fprintf(stderr, "capture prefix mismatch\n");
    pqcap_free(embedded);
    return 1;
  }

  pqcap_footer_t footer;
  err = pqcap_has_footer(embedded, embedded_len, &footer);
  if (err != PQCAP_OK) {
    fprintf(stderr, "has_footer failed: %s\n", pqcap_strerror(err));
    pqcap_free(embedded);
    return 1;
  }

  err = pqcap_validate_range(embedded_len, &footer);
  if (err != PQCAP_OK) {
    fprintf(stderr, "validate_range failed: %s\n", pqcap_strerror(err));
    pqcap_free(embedded);
    return 1;
  }

  uint8_t *extracted = NULL;
  size_t extracted_len = 0;
  err = pqcap_extract_parquet(embedded, embedded_len, &footer, &extracted, &extracted_len);
  if (err != PQCAP_OK) {
    fprintf(stderr, "extract failed: %s\n", pqcap_strerror(err));
    pqcap_free(embedded);
    return 1;
  }

  if (extracted_len != sizeof(kParquet) ||
      memcmp(extracted, kParquet, sizeof(kParquet)) != 0) {
    fprintf(stderr, "extracted parquet mismatch\n");
    pqcap_free(extracted);
    pqcap_free(embedded);
    return 1;
  }

  uint8_t *fallback = NULL;
  size_t fallback_len = 0;
  err = pqcap_extract_parquet_from_buffer(embedded, embedded_len, &fallback, &fallback_len);
  if (err != PQCAP_OK || fallback_len != sizeof(kParquet) ||
      memcmp(fallback, kParquet, sizeof(kParquet)) != 0) {
    fprintf(stderr, "extract_parquet_from_buffer failed\n");
    pqcap_free(fallback);
    pqcap_free(extracted);
    pqcap_free(embedded);
    return 1;
  }

  uint8_t *embedded_copy = embedded;
  size_t embedded_copy_len = embedded_len;
  err = pqcap_embed(embedded_copy, embedded_copy_len, kParquet, sizeof(kParquet), &embedded,
                    &embedded_len);
  if (err != PQCAP_ERR_ALREADY_INDEXED) {
    fprintf(stderr, "expected ALREADY_INDEXED on double embed, got %s\n", pqcap_strerror(err));
    pqcap_free(fallback);
    pqcap_free(extracted);
    pqcap_free(embedded);
    return 1;
  }

  pqcap_free(fallback);
  pqcap_free(extracted);
  pqcap_free(embedded);
  return 0;
}
