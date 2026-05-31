# Test fixtures

## `http.cap` / `http.pcapng`

Source: [Wireshark sample capture — HTTP](https://wiki.wireshark.org/uploads/27707187aeb30df68e70c8fb9d614981/http.cap)

Classic libpcap file containing a simple HTTP request/response conversation (43 frames, including GET requests and `200` responses). Tests use `http.pcapng`, converted from `http.cap` with:

```bash
tshark -r tests/fixtures/http.cap -F pcapng -w tests/fixtures/http.pcapng
```

`http_metadata.parquet` contains one metadata row per scanned EPB (`offset`, `size`, `ts_ns`, `linktype`).

## `udp1.pcapng`

Minimal single-UDP-packet PCAP-NG generated with `text2pcap` (see `scripts/generate_fixtures.sh`).

## Regenerating

```bash
bash scripts/generate_fixtures.sh
```

Requires `duckdb`, `text2pcap`, and `tshark`.
