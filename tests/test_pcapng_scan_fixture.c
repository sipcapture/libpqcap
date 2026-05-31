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

static int expect_scan_count(const char *label, const char *fixture, size_t want) {
  uint8_t *data = NULL;
  size_t data_len = 0;
  pqcap_packet_loc_t *locs = NULL;
  size_t count = 0;
  int failures = 0;

  failures += expect_ok(label, pqcap_read_file(fixture_path(fixture), &data, &data_len));
  failures += expect_ok(label, pqcap_scan_packets(data, data_len, &locs, &count));
  if (count != want) {
    fprintf(stderr, "%s: expected %zu packets, got %zu\n", label, want, count);
    failures++;
  }

  pqcap_free(locs);
  pqcap_free(data);
  return failures;
}

int main(void) {
  int failures = 0;
  failures += expect_scan_count("udp1.pcapng", "udp1.pcapng", 1);
  failures += expect_scan_count("http.pcapng", "http.pcapng", 43);
  return failures == 0 ? 0 : 1;
}
