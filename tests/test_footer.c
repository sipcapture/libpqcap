#include "pqcap.h"

#include <stdio.h>
#include <string.h>

static int expect_eq_int(const char *label, int got, int want) {
  if (got != want) {
    fprintf(stderr, "%s: expected %d, got %d (%s)\n", label, want, got,
            pqcap_strerror(got));
    return 1;
  }
  return 0;
}

static int expect_footer_roundtrip(uint64_t offset, uint64_t length) {
  pqcap_footer_t in = {.parquet_offset = offset, .parquet_length = length};
  uint8_t buf[PQCAP_FOOTER_SIZE];
  pqcap_footer_t out;

  if (pqcap_footer_build(&in, buf) != PQCAP_OK) {
    fprintf(stderr, "footer build failed\n");
    return 1;
  }
  if (pqcap_footer_parse(buf, &out) != PQCAP_OK) {
    fprintf(stderr, "footer parse failed\n");
    return 1;
  }
  if (out.parquet_offset != offset || out.parquet_length != length) {
    fprintf(stderr, "footer roundtrip mismatch\n");
    return 1;
  }
  return 0;
}

int main(void) {
  int failures = 0;

  failures += expect_footer_roundtrip(100, 42);
  failures += expect_footer_roundtrip(0xffffffffULL, 0x100000000ULL);

  uint8_t bad_magic[PQCAP_FOOTER_SIZE] = {0};
  pqcap_footer_t ignored;
  failures += expect_eq_int("bad magic", pqcap_footer_parse(bad_magic, &ignored),
                           PQCAP_ERR_BAD_MAGIC);

  uint8_t good[PQCAP_FOOTER_SIZE];
  pqcap_footer_t built_in = {.parquet_offset = 57, .parquet_length = 11};
  pqcap_footer_build(&built_in, good);
  good[12] = 'X';
  failures += expect_eq_int("corrupt magic", pqcap_footer_parse(good, &ignored),
                           PQCAP_ERR_BAD_MAGIC);

  pqcap_footer_t footer = {.parquet_offset = 10, .parquet_length = 5};
  failures += expect_eq_int("valid range", pqcap_validate_range(20, &footer), PQCAP_OK);
  failures +=
      expect_eq_int("short file", pqcap_validate_range(10, &footer), PQCAP_ERR_RANGE);
  footer.parquet_length = 0;
  failures +=
      expect_eq_int("zero length", pqcap_validate_range(100, &footer), PQCAP_ERR_RANGE);

  return failures == 0 ? 0 : 1;
}
