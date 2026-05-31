#include "pqcap.h"

#include "pqcap_internal.h"

#include <stdio.h>
#include <stdlib.h>

int pqcap_has_footer(const uint8_t *data, size_t data_len, pqcap_footer_t *out) {
  if (!data || !out) {
    return PQCAP_ERR_INVALID_ARG;
  }
  if (data_len < PQCAP_FOOTER_SIZE) {
    return PQCAP_ERR_NOT_FOUND;
  }
  return pqcap_footer_parse(data + data_len - PQCAP_FOOTER_SIZE, out);
}

int pqcap_is_plain_capture(const uint8_t *data, size_t data_len) {
  pqcap_footer_t footer;
  if (pqcap_has_footer(data, data_len, &footer) == PQCAP_OK) {
    return 0;
  }
  return 1;
}

int pqcap_read_footer_from_file(const char *path, pqcap_footer_t *out) {
  if (!path || !out) {
    return PQCAP_ERR_INVALID_ARG;
  }

  FILE *f = fopen(path, "rb");
  if (!f) {
    return PQCAP_ERR_IO;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return PQCAP_ERR_IO;
  }

  long size = ftell(f);
  if (size < 0) {
    fclose(f);
    return PQCAP_ERR_IO;
  }
  if ((uint64_t)size < PQCAP_FOOTER_SIZE) {
    fclose(f);
    return PQCAP_ERR_NOT_FOUND;
  }

  if (fseek(f, size - (long)PQCAP_FOOTER_SIZE, SEEK_SET) != 0) {
    fclose(f);
    return PQCAP_ERR_IO;
  }

  uint8_t buf[PQCAP_FOOTER_SIZE];
  if (fread(buf, 1, PQCAP_FOOTER_SIZE, f) != PQCAP_FOOTER_SIZE) {
    fclose(f);
    return PQCAP_ERR_IO;
  }
  fclose(f);

  return pqcap_footer_parse(buf, out);
}
