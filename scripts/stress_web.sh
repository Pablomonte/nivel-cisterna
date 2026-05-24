#!/usr/bin/env bash
# Stress test del servidor web del ESP32. Dispara N requests concurrentes
# contra los endpoints publicos y reporta status codes + latencias.
#
# Uso:
#   ./scripts/stress_web.sh http://192.168.4.1
#   ./scripts/stress_web.sh http://cisterna-01.local 20
#   ADMIN_USER=admin ADMIN_PASS=cisterna-XXXXXX \
#     ./scripts/stress_web.sh http://192.168.4.1 10
#
# Requiere: curl, awk. Opcional: bc para porcentiles mas precisos.

set -euo pipefail

BASE_URL="${1:-http://192.168.4.1}"
CONCURRENCY="${2:-10}"
ADMIN_USER="${ADMIN_USER:-admin}"
ADMIN_PASS="${ADMIN_PASS:-}"

TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

auth_args=()
if [[ -n "$ADMIN_PASS" ]]; then
    auth_args=(--user "$ADMIN_USER:$ADMIN_PASS")
fi

# $1 = label, $2 = method, $3 = path, $4 = body (opcional)
fire_concurrent() {
    local label="$1" method="$2" path="$3" body="${4-}"
    local outfile="$TMPDIR/${label}.out"
    : > "$outfile"

    echo "[stress] $CONCURRENCY × $method $path"
    for ((i = 0; i < CONCURRENCY; i++)); do
        {
            if [[ -n "$body" ]]; then
                curl -sS -o /dev/null -w '%{http_code} %{time_total}\n' \
                    "${auth_args[@]}" -X "$method" \
                    -H "Content-Type: application/json" \
                    --data "$body" \
                    "${BASE_URL}${path}" || echo "000 0"
            else
                curl -sS -o /dev/null -w '%{http_code} %{time_total}\n' \
                    "${auth_args[@]}" -X "$method" \
                    "${BASE_URL}${path}" || echo "000 0"
            fi
        } >> "$outfile" &
    done
    wait

    awk -v label="$label" '
        { codes[$1]++; times[NR]=$2; total+=$2; n++ }
        END {
            if (n == 0) { print "  (sin respuestas)"; exit }
            asort(times)
            mean = total / n
            p50 = times[int(n * 0.5) + 1]
            p95 = times[int(n * 0.95) + 1]
            max = times[n]
            printf "  n=%d  mean=%.3fs  p50=%.3fs  p95=%.3fs  max=%.3fs\n",
                   n, mean, p50, p95, max
            printf "  status:"
            for (code in codes) printf " %s×%d", code, codes[code]
            print ""
        }
    ' "$outfile"
}

echo "== Stress test contra $BASE_URL (concurrency=$CONCURRENCY) =="
echo

fire_concurrent "status"  GET  "/api/status"
fire_concurrent "root"    GET  "/"
fire_concurrent "scan"    POST "/api/wifi/scan"
fire_concurrent "config"  GET  "/api/config"

echo
echo "[stress] Listo. Codigos esperados:"
echo "  /api/status, /          → 200"
echo "  /api/wifi/scan POST     → 202 (primera) + 202 (las siguientes 'running')"
echo "  /api/config GET         → 200 (con auth) o 401 (sin auth)"
echo "  Cualquier 5xx o timeout indica un problema."
