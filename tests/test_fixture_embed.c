#include "pqcap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *fixture_path(const char *name) {
  static char path[512];
  const char *root = getenv("LIBPQCAP_FIXTURE_DIR");
  if (!root) {
    root = "tests/fixtures";
  }
  snprintf(path, sizeof(path), "%s/%s", root, name);
  return path;
}

static int expect_ok(const char *label, int err) {
  if (err != PQCAP_OK) {
    fprintf(stderr, "%s: %s\n", label, pqcap_strerror(err));
    return 1;
  }
  return 0;
}

int main(void) {
  uint8_t *capture = NULL;
  size_t capture_len = 0;
  uint8_t *parquet = NULL;
  size_t parquet_len = 0;
  uint8_t *large_parquet = NULL;
  size_t large_parquet_len = 0;
  int failures = 0;

  failures += expect_ok("read capture",
                        pqcap_read_file(fixture_path("udp1.pcapng"), &capture,
                                        &capture_len));
  failures += expect_ok("read minimal parquet",
                        pqcap_read_file(fixture_path("minimal_metadata.parquet"), &parquet,
                                        &parquet_len));
  failures += expect_ok("read large parquet fixture",
                        pqcap_read_file(fixture_path("large_metadata.parquet"), &large_parquet,
                                        &large_parquet_len));

  uint8_t *embedded = NULL;
  size_t embedded_len = 0;
  failures += expect_ok("embed minimal fixture",
                        pqcap_embed(capture, capture_len, parquet, parquet_len, &embedded,
                                    &embedded_len));

  pqcap_footer_t footer;
  failures += expect_ok("parse footer", pqcap_has_footer(embedded, embedded_len, &footer));
  if (footer.parquet_length != (uint64_t)parquet_len) {
    fprintf(stderr, "footer length mismatch: %llu vs %zu\n",
            (unsigned long long)footer.parquet_length, parquet_len);
    failures++;
  }

  uint8_t *extracted = NULL;
  size_t extracted_len = 0;
  failures += expect_ok("extract parquet",
                        pqcap_extract_parquet(embedded, embedded_len, &footer, &extracted,
                                              &extracted_len));
  if (extracted_len != parquet_len || memcmp(extracted, parquet, parquet_len) != 0) {
    fprintf(stderr, "extracted parquet does not match fixture\n");
    failures++;
  }

  pqcap_free(embedded);
  pqcap_free(extracted);
  embedded = NULL;
  extracted = NULL;

  failures += expect_ok("embed large parquet fixture",
                        pqcap_embed(capture, capture_len, large_parquet, large_parquet_len,
                                    &embedded, &embedded_len));
  failures += expect_ok("parse large footer", pqcap_has_footer(embedded, embedded_len, &footer));
  if (footer.parquet_length != (uint64_t)large_parquet_len) {
    fprintf(stderr, "large footer length mismatch\n");
    failures++;
  }
  failures += expect_ok("extract large parquet",
                        pqcap_extract_parquet(embedded, embedded_len, &footer, &extracted,
                                              &extracted_len));
  if (extracted_len != large_parquet_len ||
      memcmp(extracted, large_parquet, large_parquet_len) != 0) {
    fprintf(stderr, "large extracted parquet does not match fixture\n");
    failures++;
  }

  pqcap_free(capture);
  pqcap_free(parquet);
  pqcap_free(large_parquet);
  pqcap_free(embedded);
  pqcap_free(extracted);
  return failures == 0 ? 0 : 1;
}
