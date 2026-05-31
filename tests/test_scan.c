#include "pqcap.h"

#include <stdio.h>

/* Minimal PCAP-NG: 32-byte SHB + 36-byte EPB (captured_len=4, packet at offset 60). */
static const uint8_t kPcapngOneEpb[] = {
    0x0a, 0x0d, 0x0d, 0x0a, 0x20, 0x00, 0x00, 0x00, 0x4d, 0x3c, 0x2b, 0x1a, 0x01, 0x00,
    0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0xde, 0xad, 0xbe, 0xef, 0x24, 0x00, 0x00, 0x00,
};

int main(void) {
  pqcap_packet_loc_t *locs = NULL;
  size_t count = 0;
  const int err = pqcap_scan_packets(kPcapngOneEpb, sizeof(kPcapngOneEpb), &locs, &count);
  if (err != PQCAP_OK) {
    fprintf(stderr, "scan failed: %s\n", pqcap_strerror(err));
    return 1;
  }
  if (count != 1) {
    fprintf(stderr, "expected 1 packet, got %zu (size=%zu)\n", count, sizeof(kPcapngOneEpb));
    pqcap_free(locs);
    return 1;
  }

  const uint64_t want_offset = 60;
  const uint64_t want_size = 4;

  if (locs[0].offset != want_offset || locs[0].size != want_size) {
    fprintf(stderr, "unexpected loc offset=%llu size=%llu\n",
            (unsigned long long)locs[0].offset, (unsigned long long)locs[0].size);
    pqcap_free(locs);
    return 1;
  }

  pqcap_free(locs);
  return 0;
}
