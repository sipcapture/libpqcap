#include "pqcap.h"

#include <stdio.h>

static int fail(const char *step, int err) {
  fprintf(stderr, "%s: %s\n", step, pqcap_strerror(err));
  return 1;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr,
            "Usage: %s <input.pcapng> <metadata.parquet> <output.pqcapng>\n"
            "\n"
            "Convert a plain PCAP-NG capture to pqcap format.\n"
            "The capture bytes are preserved; metadata and a footer locator are appended.\n"
            "\n"
            "Example:\n"
            "  %s tests/fixtures/http.pcapng tests/fixtures/http_metadata.parquet out.pqcapng\n",
            argv[0], argv[0]);
    return 2;
  }

  uint8_t *capture = NULL;
  size_t capture_len = 0;
  int err = pqcap_read_file(argv[1], &capture, &capture_len);
  if (err != PQCAP_OK) {
    return fail("read capture", err);
  }

  if (!pqcap_is_plain_capture(capture, capture_len)) {
    pqcap_free(capture);
    fprintf(stderr, "input already contains a pqcap footer: %s\n", argv[1]);
    return 1;
  }
  pqcap_free(capture);

  err = pqcap_convert_file(argv[1], argv[2], argv[3]);
  if (err != PQCAP_OK) {
    return fail("pqcap_convert_file", err);
  }

  pqcap_footer_t footer;
  err = pqcap_read_footer_from_file(argv[3], &footer);
  if (err != PQCAP_OK) {
    return fail("read footer", err);
  }

  uint8_t *out = NULL;
  size_t out_len = 0;
  err = pqcap_read_file(argv[3], &out, &out_len);
  if (err != PQCAP_OK) {
    return fail("read output", err);
  }

  err = pqcap_validate_range(out_len, &footer);
  pqcap_free(out);
  if (err != PQCAP_OK) {
    return fail("validate footer range", err);
  }

  printf("converted %s -> %s\n", argv[1], argv[3]);
  printf("  metadata_parquet_offset=%llu\n", (unsigned long long)footer.parquet_offset);
  printf("  metadata_parquet_length=%llu\n", (unsigned long long)footer.parquet_length);
  return 0;
}
