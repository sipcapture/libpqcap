#ifndef PQCAP_H
#define PQCAP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PQCAP_FOOTER_SIZE 44

#define PQCAP_OK 0
#define PQCAP_ERR_INVALID_ARG -1
#define PQCAP_ERR_BAD_MAGIC -2
#define PQCAP_ERR_BAD_VERSION -3
#define PQCAP_ERR_RANGE -4
#define PQCAP_ERR_IO -5
#define PQCAP_ERR_NOMEM -6
#define PQCAP_ERR_ALREADY_INDEXED -7
#define PQCAP_ERR_NOT_FOUND -8

typedef struct {
  uint64_t parquet_offset;
  uint64_t parquet_length;
} pqcap_footer_t;

typedef struct {
  uint64_t offset;
  uint64_t size;
} pqcap_packet_loc_t;

int pqcap_footer_parse(const uint8_t buf[PQCAP_FOOTER_SIZE], pqcap_footer_t *out);
int pqcap_footer_build(const pqcap_footer_t *in, uint8_t buf[PQCAP_FOOTER_SIZE]);

int pqcap_read_footer_from_file(const char *path, pqcap_footer_t *out);
int pqcap_has_footer(const uint8_t *data, size_t data_len, pqcap_footer_t *out);
int pqcap_is_plain_capture(const uint8_t *data, size_t data_len);

int pqcap_validate_range(uint64_t file_size, const pqcap_footer_t *footer);

int pqcap_extract_parquet(const uint8_t *data, size_t data_len,
                          const pqcap_footer_t *footer, uint8_t **out,
                          size_t *out_len);
int pqcap_extract_parquet_from_buffer(const uint8_t *data, size_t data_len,
                                      uint8_t **out, size_t *out_len);

int pqcap_embed(const uint8_t *capture, size_t capture_len, const uint8_t *parquet,
                size_t parquet_len, uint8_t **out, size_t *out_len);

int pqcap_read_file(const char *path, uint8_t **out, size_t *out_len);
int pqcap_write_file(const char *path, const uint8_t *data, size_t data_len);

int pqcap_embed_file(const char *capture_path, const char *parquet_path, const char *output_path);
int pqcap_extract_parquet_file(const char *pqcap_path, const char *parquet_out_path);

int pqcap_scan_packets(const uint8_t *data, size_t data_len,
                       pqcap_packet_loc_t **out, size_t *out_count);

void pqcap_free(void *ptr);

const char *pqcap_strerror(int err);

#ifdef __cplusplus
}
#endif

#endif /* PQCAP_H */
