#!/usr/bin/env bash
# Shared helpers for validating pqcap convert output with tshark.
convert_pcapng_to_pqcap() {
  local plain="$1"
  local metadata="$2"
  local converted="$3"

  python3 "${ROOT}/tests/lib/pqcap_file_checks.py" assert-plain "${plain}"
  "${CONVERT_CLI}" "${plain}" "${metadata}" "${converted}"
  python3 "${ROOT}/tests/lib/pqcap_file_checks.py" assert-indexed "${converted}"
}

validate_converted_readability() {
  local plain="$1"
  local converted="$2"
  local app_filter="$3"
  local label="$4"

  tshark_must_read "${plain}"
  tshark_must_read "${converted}"

  local plain_app converted_app
  plain_app="$(tshark_count "${plain}" "${app_filter}")"
  converted_app="$(tshark_count "${converted}" "${app_filter}")"
  [[ "${plain_app}" =~ ^[0-9]+$ && "${converted_app}" =~ ^[0-9]+$ ]] || {
    echo "FAIL: invalid ${label} packet count from tshark"
    return 1
  }
  (( plain_app > 0 )) || {
    echo "FAIL: plain ${label} capture has no decodable ${app_filter} packets"
    return 1
  }
  [[ "${converted_app}" == "${plain_app}" ]] || {
    echo "FAIL: ${label} ${app_filter} count changed after convert (plain=${plain_app}, converted=${converted_app})"
    return 1
  }

  python3 "${ROOT}/tests/lib/pqcap_file_checks.py" assert-prefix "${plain}" "${converted}"

  if command -v capinfos >/dev/null 2>&1; then
    capinfos "${converted}" >/dev/null 2>&1 || {
      echo "FAIL: capinfos rejected converted ${label} file"
      return 1
    }
  fi

  echo "PASS ${label} convert readability (${app_filter} packets=${plain_app}, prefix preserved)"
}

validate_embed_tshark() {
  local plain="$1"
  local metadata="$2"
  local converted="$3"
  local app_filter="$4"
  local label="$5"

  [[ -f "${plain}" ]] || {
    echo "FAIL: missing plain capture ${plain}"
    return 1
  }
  [[ -f "${metadata}" ]] || {
    echo "FAIL: missing metadata ${metadata}"
    return 1
  }

  convert_pcapng_to_pqcap "${plain}" "${metadata}" "${converted}"
  validate_converted_readability "${plain}" "${converted}" "${app_filter}" "${label}"
}

tshark_count() {
  local file="$1"
  local filter="${2:-}"
  if [[ -n "${filter}" ]]; then
    tshark -r "${file}" -Y "${filter}" -T fields -e frame.number 2>/dev/null | wc -l | tr -d ' '
  else
    tshark -r "${file}" -T fields -e frame.number 2>/dev/null | wc -l | tr -d ' '
  fi
}

tshark_must_read() {
  local file="$1"
  tshark -r "${file}" -T fields -e frame.number >/dev/null 2>&1 || {
    echo "FAIL: tshark cannot read ${file}"
    return 1
  }
}
