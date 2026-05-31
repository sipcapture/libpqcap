#include "pqcap.h"

#include "pqcap_internal.h"

#include <stdio.h>
#include <stdlib.h>

int pqcap_read_file(const char *path, uint8_t **out, size_t *out_len) {
  if (!path || !out || !out_len) {
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

  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return PQCAP_ERR_IO;
  }

  uint8_t *buf = NULL;
  if (size > 0) {
    buf = (uint8_t *)malloc((size_t)size);
    if (!buf) {
      fclose(f);
      return PQCAP_ERR_NOMEM;
    }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
      free(buf);
      fclose(f);
      return PQCAP_ERR_IO;
    }
  }

  fclose(f);
  *out = buf;
  *out_len = (size_t)size;
  return PQCAP_OK;
}

int pqcap_write_file(const char *path, const uint8_t *data, size_t data_len) {
  if (!path || (data_len > 0 && !data)) {
    return PQCAP_ERR_INVALID_ARG;
  }

  FILE *f = fopen(path, "wb");
  if (!f) {
    return PQCAP_ERR_IO;
  }

  if (data_len > 0 && fwrite(data, 1, data_len, f) != data_len) {
    fclose(f);
    return PQCAP_ERR_IO;
  }

  if (fclose(f) != 0) {
    return PQCAP_ERR_IO;
  }
  return PQCAP_OK;
}

int pqcap_embed_file(const char *capture_path, const char *parquet_path,
                     const char *output_path) {
  if (!capture_path || !parquet_path || !output_path) {
    return PQCAP_ERR_INVALID_ARG;
  }

  uint8_t *capture = NULL;
  size_t capture_len = 0;
  uint8_t *parquet = NULL;
  size_t parquet_len = 0;
  uint8_t *embedded = NULL;
  size_t embedded_len = 0;
  int err = PQCAP_OK;

  err = pqcap_read_file(capture_path, &capture, &capture_len);
  if (err != PQCAP_OK) {
    goto done;
  }

  err = pqcap_read_file(parquet_path, &parquet, &parquet_len);
  if (err != PQCAP_OK) {
    goto done;
  }

  err = pqcap_embed(capture, capture_len, parquet, parquet_len, &embedded, &embedded_len);
  if (err != PQCAP_OK) {
    goto done;
  }

  err = pqcap_write_file(output_path, embedded, embedded_len);

done:
  pqcap_free(capture);
  pqcap_free(parquet);
  pqcap_free(embedded);
  return err;
}

int pqcap_extract_parquet_file(const char *pqcap_path, const char *parquet_out_path) {
  if (!pqcap_path || !parquet_out_path) {
    return PQCAP_ERR_INVALID_ARG;
  }

  uint8_t *data = NULL;
  size_t data_len = 0;
  uint8_t *parquet = NULL;
  size_t parquet_len = 0;
  int err = pqcap_read_file(pqcap_path, &data, &data_len);
  if (err != PQCAP_OK) {
    return err;
  }

  err = pqcap_extract_parquet_from_buffer(data, data_len, &parquet, &parquet_len);
  if (err != PQCAP_OK) {
    pqcap_free(data);
    return err;
  }

  err = pqcap_write_file(parquet_out_path, parquet, parquet_len);
  pqcap_free(data);
  pqcap_free(parquet);
  return err;
}

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
