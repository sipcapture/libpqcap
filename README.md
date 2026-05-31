# libpqcap

Standalone C library for reading and writing the [pqcap](https://github.com/sipcapture/pqcap) capture format: PCAP-NG compatible packet storage with an embedded Parquet metadata index and a fixed 44-byte footer locator for range-read friendly access.

Normative format contract: [pqcap SPEC](https://github.com/sipcapture/pqcap/blob/main/SPEC.md).

## Footer locator vs Parquet payload

The **44-byte footer is only a locator**, not a container for Parquet data and not a size limit on the format.

| Layer | Size | Role |
|-------|------|------|
| Footer block (`PQCAPFTR`) | Fixed 44 bytes | Holds `metadata_parquet_offset` + `metadata_parquet_length` (uint64 each) for tail/range reads |
| Embedded Parquet bytes | Variable (opaque) | Copied verbatim from the writer; may be as large and as feature-rich as any standalone `.parquet` file |

libpqcap treats Parquet as an **opaque byte blob**: row groups, compression, optional columns, and multi-GB files are all determined by your Parquet writer, not by libpqcap. The library never parses or rewrites Parquet — it only wraps, locates, and extracts the exact bytes you provide.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### Testing

libpqcap tests are self-contained: C unit tests, fixture embed/extract checks, and `tshark` wire-readability validation. No pqcap binary or DuckDB dependency is required.

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Cross-testing against the pqcap reference CLI (optional, outside this repo's CI) can be done manually in the [pqcap](https://github.com/sipcapture/pqcap) tree — libpqcap does not invoke it.

### PCAP-NG to pqcap convert (tshark)

Converting plain PCAP-NG to pqcap format must **retain wire readability**. The `pcapng_to_pqcap_convert` test:

1. Asserts input is a plain capture (no pqcap footer yet)
2. Converts with `pqcap_convert_file` / `pqcap_convert_cli`
3. Asserts output is indexed, prefix bytes unchanged, and Parquet extract matches
4. Verifies `tshark`/`capinfos` still decode the same application traffic (`udp`, `http`, GET/`200`)
5. Rejects double-convert on an already-indexed file

```bash
ctest --test-dir build -R pcapng_to_pqcap_convert --output-on-failure
```

Set `LIBPQCAP_SKIP_TSHARK=1` only to skip wire-tool checks locally when tshark is unavailable. CI installs tshark on Linux and macOS.

Regenerate committed Parquet/PCAP-NG fixtures (optional):

```bash
bash scripts/generate_fixtures.sh
```

Install (optional):

```bash
cmake --install build --prefix /usr/local
```

## Quick start

```c
#include "pqcap.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  /* Read footer from an indexed capture */
  pqcap_footer_t footer;
  int err = pqcap_read_footer_from_file("capture.pqcapng", &footer);
  if (err != PQCAP_OK) {
    fprintf(stderr, "footer: %s\n", pqcap_strerror(err));
    return 1;
  }

  /* Embed metadata into a plain capture (in memory) */
  uint8_t *capture = ...;
  size_t capture_len = ...;
  uint8_t *parquet = ...;
  size_t parquet_len = ...;
  uint8_t *out = NULL;
  size_t out_len = 0;

  err = pqcap_embed(capture, capture_len, parquet, parquet_len, &out, &out_len);
  if (err != PQCAP_OK) {
    fprintf(stderr, "embed: %s\n", pqcap_strerror(err));
    return 1;
  }
  pqcap_free(out);
  return 0;
}
```

## API overview

| Function | Purpose |
|----------|---------|
| `pqcap_footer_parse` / `pqcap_footer_build` | Parse or build the 44-byte `PQCAPFTR` footer block |
| `pqcap_read_footer_from_file` | Read footer from the last 44 bytes of a file |
| `pqcap_has_footer` / `pqcap_is_plain_capture` | Detect indexed vs plain captures |
| `pqcap_validate_range` | Overflow-safe footer range checks |
| `pqcap_embed` | Append metadata block + footer to a plain capture |
| `pqcap_embed_file` | Append metadata block + footer to a plain capture (path-based) |
| `pqcap_convert_file` | Convert plain PCAP-NG to pqcap format (alias of `pqcap_embed_file`) |
| `pqcap_extract_parquet` | Slice embedded Parquet bytes using a parsed footer |
| `pqcap_extract_parquet_file` | Extract embedded Parquet to a standalone file |
| `pqcap_extract_parquet_from_buffer` | Footer-first extract with legacy block scan fallback |
| `pqcap_read_file` / `pqcap_write_file` | Whole-file I/O helpers |
| `pqcap_scan_packets` | Scan PCAP-NG EPB (`0x06`) and legacy packet (`0x03`) locations |
| `pqcap_free` | Release buffers returned by the library |
| `pqcap_strerror` | Human-readable error strings |

## Error codes

| Code | Meaning |
|------|---------|
| `PQCAP_OK` | Success |
| `PQCAP_ERR_INVALID_ARG` | NULL pointer or invalid parameter |
| `PQCAP_ERR_BAD_MAGIC` | Block type, enterprise ID, or magic mismatch |
| `PQCAP_ERR_BAD_VERSION` | Unsupported footer/metadata version |
| `PQCAP_ERR_RANGE` | Invalid offset/length or overflow |
| `PQCAP_ERR_IO` | File I/O failure |
| `PQCAP_ERR_NOMEM` | Allocation failure |
| `PQCAP_ERR_ALREADY_INDEXED` | Capture already contains pqcap metadata |
| `PQCAP_ERR_NOT_FOUND` | No pqcap metadata present |

## Submodule workflow (pqcap basecamp)

The [pqcap](https://github.com/sipcapture/pqcap) repository tracks this library as a git submodule:

```bash
git clone --recurse-submodules https://github.com/sipcapture/pqcap.git
cd pqcap/libpqcap
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

Develop, commit, and push from inside `libpqcap/`; bump the submodule pointer in pqcap when releasing a new libpqcap revision.

## Status

v0.1 — footer parse/build, metadata embed/extract, PCAP-NG packet offset scan, file helpers, and self-contained fixture/integration tests (tshark wire compatibility). Parquet schema construction and L4 decode are out of scope; use your own Parquet writer and join with `pqcap_scan_packets` offsets.

## License

MIT — see [LICENSE](LICENSE).
