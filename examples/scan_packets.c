#include "pqcap.h"

#include <stdio.h>

static int fail(const char *step, int err) {
  fprintf(stderr, "%s: %s\n", step, pqcap_strerror(err));
  return 1;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr,
            "Usage: %s <capture.pcapng|pqcapng>\n"
            "\n"
            "Scan PCAP-NG packet blocks and print (offset, size) locations.\n"
            "Works on plain captures and pqcap-indexed files (capture prefix unchanged).\n"
            "\n"
            "Example:\n"
            "  %s tests/fixtures/http.pcapng\n",
            argv[0], argv[0]);
    return 2;
  }

  uint8_t *data = NULL;
  size_t data_len = 0;
  int err = pqcap_read_file(argv[1], &data, &data_len);
  if (err != PQCAP_OK) {
    return fail("read capture", err);
  }

  pqcap_packet_loc_t *locs = NULL;
  size_t count = 0;
  err = pqcap_scan_packets(data, data_len, &locs, &count);
  pqcap_free(data);
  if (err != PQCAP_OK) {
    return fail("scan packets", err);
  }

  printf("packets in %s: %zu\n", argv[1], count);
  for (size_t i = 0; i < count; i++) {
    printf("  [%zu] offset=%llu size=%llu\n", i, (unsigned long long)locs[i].offset,
           (unsigned long long)locs[i].size);
  }

  pqcap_free(locs);
  return 0;
}
