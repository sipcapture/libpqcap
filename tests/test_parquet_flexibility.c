#include "pqcap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect_ok(const char *label, int err) {
  if (err != PQCAP_OK) {
    fprintf(stderr, "%s: %s\n", label, pqcap_strerror(err));
    return 1;
  }
  return 0;
}

static int expect_u64(const char *label, uint64_t got, uint64_t want) {
  if (got != want) {
    fprintf(stderr, "%s: expected %llu, got %llu\n", label, (unsigned long long)want,
            (unsigned long long)got);
    return 1;
  }
  return 0;
}

int main(void) {
  int failures = 0;

  /* Footer locator fields are uint64 — not capped at 4 GiB or footer size. */
  const uint64_t big_offset = 1024ULL * 1024ULL;
  const uint64_t big_length = 5ULL * 1024ULL * 1024ULL * 1024ULL;
  pqcap_footer_t footer = {
      .parquet_offset = big_offset,
      .parquet_length = big_length,
  };
  const uint64_t file_size = big_offset + big_length;
  failures += expect_ok("validate large parquet range",
                        pqcap_validate_range(file_size, &footer));

  uint8_t block[PQCAP_FOOTER_SIZE];
  pqcap_footer_t parsed;
  failures += expect_ok("footer build large", pqcap_footer_build(&footer, block));
  failures += expect_ok("footer parse large", pqcap_footer_parse(block, &parsed));
  failures += expect_u64("large offset roundtrip", parsed.parquet_offset, big_offset);
  failures += expect_u64("large length roundtrip", parsed.parquet_length, big_length);

  /* Embedded Parquet bytes are opaque — any length the caller supplies is preserved. */
  uint8_t capture[] = {0x0a, 0x0d, 0x0d, 0x0a, 0x1c, 0x00, 0x00, 0x00};
  uint8_t *parquet = NULL;
  const size_t parquet_len = 256 * 1024;
  parquet = (uint8_t *)malloc(parquet_len);
  if (!parquet) {
    fprintf(stderr, "alloc parquet\n");
    return 1;
  }
  for (size_t i = 0; i < parquet_len; i++) {
    parquet[i] = (uint8_t)(i & 0xffu);
  }
  memcpy(parquet, "PAR1", 4);
  memcpy(parquet + parquet_len - 4, "PAR1", 4);

  uint8_t *embedded = NULL;
  size_t embedded_len = 0;
  failures += expect_ok("embed large opaque parquet",
                        pqcap_embed(capture, sizeof(capture), parquet, parquet_len,
                                    &embedded, &embedded_len));

  pqcap_footer_t out_footer;
  failures += expect_ok("has footer", pqcap_has_footer(embedded, embedded_len, &out_footer));
  failures += expect_u64("footer length matches payload", out_footer.parquet_length,
                         (uint64_t)parquet_len);
  failures += expect_ok("range ok", pqcap_validate_range(embedded_len, &out_footer));

  uint8_t *extracted = NULL;
  size_t extracted_len = 0;
  failures +=
      expect_ok("extract large parquet",
                pqcap_extract_parquet(embedded, embedded_len, &out_footer, &extracted,
                                      &extracted_len));
  if (extracted_len != parquet_len || memcmp(extracted, parquet, parquet_len) != 0) {
    fprintf(stderr, "large parquet byte mismatch\n");
    failures++;
  }

  free(parquet);
  pqcap_free(embedded);
  pqcap_free(extracted);
  return failures == 0 ? 0 : 1;
}
