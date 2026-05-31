#include "pqcap.h"

#include <stdlib.h>

const char *pqcap_strerror(int err) {
  switch (err) {
  case PQCAP_OK:
    return "success";
  case PQCAP_ERR_INVALID_ARG:
    return "invalid argument";
  case PQCAP_ERR_BAD_MAGIC:
    return "bad magic or block type";
  case PQCAP_ERR_BAD_VERSION:
    return "unsupported version";
  case PQCAP_ERR_RANGE:
    return "range validation failed";
  case PQCAP_ERR_IO:
    return "I/O error";
  case PQCAP_ERR_NOMEM:
    return "out of memory";
  case PQCAP_ERR_ALREADY_INDEXED:
    return "capture already contains pqcap metadata";
  case PQCAP_ERR_NOT_FOUND:
    return "pqcap metadata not found";
  default:
    return "unknown error";
  }
}

void pqcap_free(void *ptr) { free(ptr); }
