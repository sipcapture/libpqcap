# libpqcap examples

Small command-line programs showing common libpqcap workflows. They mirror patterns used in `tests/` and CI.

## Build

Examples are built with the main project:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binaries land in `build/examples/`:

| Program | Purpose |
|---------|---------|
| `convert_pcapng` | Plain PCAP-NG + Parquet metadata → `.pqcapng` |
| `extract_metadata` | Footer-driven Parquet extract from indexed capture |
| `scan_packets` | List `(offset, size)` for each PCAP-NG packet block |

## Run all examples (fixtures)

Uses committed files under `tests/fixtures/` (same as CI):

```bash
bash examples/run_examples.sh
```

Or after building:

```bash
LIBPQCAP_BUILD_DIR=build bash examples/run_examples.sh
```

## Manual usage

Convert the Wireshark [http sample](https://wiki.wireshark.org/uploads/27707187aeb30df68e70c8fb9d614981/http.cap) (as `http.pcapng`):

```bash
./build/examples/convert_pcapng \
  tests/fixtures/http.pcapng \
  tests/fixtures/http_metadata.parquet \
  /tmp/http.pqcapng
```

Extract embedded metadata:

```bash
./build/examples/extract_metadata /tmp/http.pqcapng /tmp/metadata.parquet
```

Scan packet locations (plain or indexed capture):

```bash
./build/examples/scan_packets tests/fixtures/http.pcapng
```

Verify wire readability with tshark (optional, same check as CI):

```bash
tshark -r /tmp/http.pqcapng -Y http -T fields -e frame.number
```

## Notes

- libpqcap does **not** build Parquet rows — supply a `.parquet` file from your indexer (DuckDB, Arrow, etc.). See `scripts/generate_fixtures.sh` for how test metadata is produced.
- Convert is **append-only**: the original capture prefix is unchanged; only metadata blocks and the footer are added at the end.
