#include "pqcap.h"

#include <stdio.h>

static int fail(const char *step, int err) {
  fprintf(stderr, "%s: %s\n", step, pqcap_strerror(err));
  return 1;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr,
            "Usage: %s <input.pqcapng> <output.parquet>\n"
            "\n"
            "Extract embedded Parquet metadata from a pqcap-indexed capture.\n"
            "Uses the 44-byte footer locator when present.\n"
            "\n"
            "Example:\n"
            "  %s out.pqcapng metadata.parquet\n",
            argv[0], argv[0]);
    return 2;
  }

  pqcap_footer_t footer;
  int err = pqcap_read_footer_from_file(argv[1], &footer);
  if (err != PQCAP_OK) {
    return fail("read footer", err);
  }

  uint8_t *file = NULL;
  size_t file_len = 0;
  err = pqcap_read_file(argv[1], &file, &file_len);
  if (err != PQCAP_OK) {
    return fail("read pqcap file", err);
  }

  err = pqcap_validate_range(file_len, &footer);
  if (err != PQCAP_OK) {
    pqcap_free(file);
    return fail("validate footer range", err);
  }

  err = pqcap_extract_parquet_file(argv[1], argv[2]);
  pqcap_free(file);
  if (err != PQCAP_OK) {
    return fail("extract parquet", err);
  }

  printf("extracted metadata from %s -> %s\n", argv[1], argv[2]);
  printf("  metadata_parquet_offset=%llu\n", (unsigned long long)footer.parquet_offset);
  printf("  metadata_parquet_length=%llu\n", (unsigned long long)footer.parquet_length);
  return 0;
}
