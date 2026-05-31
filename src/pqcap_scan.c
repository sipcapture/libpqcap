#include "pqcap.h"

#include "pqcap_internal.h"

#include <stdlib.h>

int pqcap_scan_packets(const uint8_t *data, size_t data_len, pqcap_packet_loc_t **out,
                       size_t *out_count) {
  if (!data || !out || !out_count) {
    return PQCAP_ERR_INVALID_ARG;
  }

  size_t capacity = 0;
  size_t count = 0;
  pqcap_packet_loc_t *locs = NULL;
  size_t pos = 0;

  while (pos + 12 <= data_len) {
    const uint32_t block_type = pqcap_read_u32_le(data + pos);
    const uint32_t total_len = pqcap_read_u32_le(data + pos + 4);
    if (total_len < 12 || pos + total_len > data_len) {
      break;
    }

    if (block_type == 0x00000006u && total_len >= 32) {
      const uint32_t captured_len = pqcap_read_u32_le(data + pos + 8 + 12);
      if (count == capacity) {
        size_t new_cap = capacity ? capacity * 2 : 16;
        pqcap_packet_loc_t *new_locs =
            (pqcap_packet_loc_t *)realloc(locs, new_cap * sizeof(*new_locs));
        if (!new_locs) {
          free(locs);
          return PQCAP_ERR_NOMEM;
        }
        locs = new_locs;
        capacity = new_cap;
      }
      locs[count].offset = (uint64_t)(pos + 28);
      locs[count].size = (uint64_t)captured_len;
      count++;
    } else if (block_type == 0x00000003u && total_len >= 16) {
      const uint32_t pkt_len = pqcap_read_u32_le(data + pos + 8);
      if (count == capacity) {
        size_t new_cap = capacity ? capacity * 2 : 16;
        pqcap_packet_loc_t *new_locs =
            (pqcap_packet_loc_t *)realloc(locs, new_cap * sizeof(*new_locs));
        if (!new_locs) {
          free(locs);
          return PQCAP_ERR_NOMEM;
        }
        locs = new_locs;
        capacity = new_cap;
      }
      locs[count].offset = (uint64_t)(pos + 12);
      locs[count].size = (uint64_t)pkt_len;
      count++;
    }

    pos += total_len;
  }

  *out = locs;
  *out_count = count;
  return PQCAP_OK;
}
