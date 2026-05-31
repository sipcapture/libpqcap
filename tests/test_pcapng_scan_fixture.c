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
  int failures = 0;

  failures += expect_ok("read udp1 capture",
                        pqcap_read_file(fixture_path("udp1.pcapng"), &capture, &capture_len));

  pqcap_packet_loc_t *locs = NULL;
  size_t count = 0;
  failures += expect_ok("scan udp1 capture",
                        pqcap_scan_packets(capture, capture_len, &locs, &count));
  if (count == 0) {
    fprintf(stderr, "expected at least one packet in udp1.pcapng\n");
    failures++;
  }

  pqcap_free(locs);
  pqcap_free(capture);
  return failures == 0 ? 0 : 1;
}
