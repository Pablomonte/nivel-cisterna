#!/usr/bin/env bash
# Mergea credenciales desde variables de entorno (o .env) hacia data/config.json.
# Las claves vacias se ignoran: solo se sobrescriben los campos para los que se
# proveyo un valor. config.json esta en .gitignore, asi que los secretos nunca
# salen del checkout.
#
# Uso:
#   ./scripts/provision_config.sh           # carga ./.env si existe
#   ./scripts/provision_config.sh ruta.env  # carga otro archivo
#   TELEGRAM_BOT_TOKEN=... ./scripts/provision_config.sh
#
# Requiere: jq

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

ENV_FILE="${1:-.env}"
CONFIG="data/config.json"
TEMPLATE="data/config.json.example"

if ! command -v jq >/dev/null 2>&1; then
    echo "[provision] jq no esta instalado (apt install jq)" >&2
    exit 1
fi

if [[ -f "$ENV_FILE" ]]; then
    echo "[provision] cargando $ENV_FILE"
    set -a
    # shellcheck disable=SC1090
    source "$ENV_FILE"
    set +a
else
    echo "[provision] $ENV_FILE no existe, usando solo variables del shell"
fi

if [[ ! -f "$CONFIG" ]]; then
    if [[ -f "$TEMPLATE" ]]; then
        echo "[provision] $CONFIG no existe, copiando desde $TEMPLATE"
        cp "$TEMPLATE" "$CONFIG"
    else
        echo "[provision] no hay $CONFIG ni $TEMPLATE para inicializar" >&2
        exit 1
    fi
fi

# Comienza con el JSON existente.
JSON="$(cat "$CONFIG")"

# Helper: si la variable esta seteada y no vacia, asigna en el JSON.
assign_string() {
    local path="$1"; local value="$2"
    if [[ -n "$value" ]]; then
        JSON="$(jq --arg v "$value" "$path = \$v" <<<"$JSON")"
    fi
}

assign_bool() {
    local path="$1"; local value="$2"
    if [[ -n "$value" ]]; then
        local lower="${value,,}"
        local b="false"
        [[ "$lower" == "true" || "$lower" == "1" || "$lower" == "yes" ]] && b="true"
        JSON="$(jq --argjson v "$b" "$path = \$v" <<<"$JSON")"
    fi
}

ensure_object() {
    local path="$1"
    JSON="$(jq "if $path == null then $path = {} else . end" <<<"$JSON")"
}

# Top-level WiFi
assign_string '.wifi_ssid' "${WIFI_SSID:-}"
assign_string '.wifi_pass' "${WIFI_PASS:-}"

# Telegram
ensure_object '.telegram'
assign_bool   '.telegram.enabled'   "${TELEGRAM_ENABLED:-}"
assign_string '.telegram.bot_token' "${TELEGRAM_BOT_TOKEN:-}"
assign_string '.telegram.chat_id'   "${TELEGRAM_CHAT_ID:-}"

# Grafana
ensure_object '.grafana'
assign_string '.grafana.url'   "${GRAFANA_URL:-}"
assign_string '.grafana.token' "${GRAFANA_TOKEN:-}"

# Admin
ensure_object '.admin'
assign_string '.admin.username' "${ADMIN_USERNAME:-}"
assign_string '.admin.password' "${ADMIN_PASSWORD:-}"

# Validacion: si telegram.enabled es true, exigir bot_token y chat_id.
if [[ "$(jq -r '.telegram.enabled // false' <<<"$JSON")" == "true" ]]; then
    bt="$(jq -r '.telegram.bot_token // ""' <<<"$JSON")"
    ci="$(jq -r '.telegram.chat_id // ""' <<<"$JSON")"
    if [[ -z "$bt" || -z "$ci" ]]; then
        echo "[provision] telegram.enabled=true pero falta bot_token o chat_id" >&2
        exit 2
    fi
fi

# Escritura atomica.
TMP="${CONFIG}.tmp"
jq '.' <<<"$JSON" >"$TMP"
mv "$TMP" "$CONFIG"

echo "[provision] $CONFIG actualizado. Subir con: pio run -e cisterna_dev -t uploadfs"
