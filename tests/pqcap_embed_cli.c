#include "pqcap.h"

#include <stdio.h>

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <capture.pcapng> <metadata.parquet> <output.pqcapng>\n", argv[0]);
    return 2;
  }

  const int err = pqcap_embed_file(argv[1], argv[2], argv[3]);
  if (err != PQCAP_OK) {
    fprintf(stderr, "pqcap_embed_file: %s\n", pqcap_strerror(err));
    return 1;
  }
  return 0;
}
