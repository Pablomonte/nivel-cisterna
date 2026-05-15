# Grafana — Alertas de cisterna

Este directorio reemplaza el `TelegramNotifier` del firmware. La lógica de
alertas vive en Grafana, contra los puntos que el ESP32 publica en InfluxDB
(`cto.cisterna`, tag `device`).

## Reglas creadas

| UID                  | Dispara cuando…                                      | Para              | Resuelve cuando…           |
|----------------------|------------------------------------------------------|-------------------|----------------------------|
| `cist-low-level`     | `level < 15` durante 5 min                           | 5 min             | `level >= 30` (hysteresis) |
| `cist-overflow`      | `level > 95` durante 1 min                           | 1 min             | `level <= 90` (hysteresis) |
| `cist-sensor-fault`  | `sensor_ok == 0` durante 2 min                       | 2 min             | `sensor_ok == 1`           |
| `cist-no-data`       | 0 puntos en los últimos 30 min                       | 30 min            | Vuelven puntos             |

Hysteresis (`unloadEvaluator`) requiere **Grafana 11.3+**. En versiones
anteriores el campo se ignora silenciosamente y la alerta resuelve al instante
en que la condición principal deja de cumplirse.

Notificación: contact point `telegram-cisterna` usa las variables
`${TELEGRAM_BOT_TOKEN}` y `${TELEGRAM_CHAT_ID}` (mismos que tenía el firmware).
Exportarlas via `systemd Environment=` o `/etc/grafana/grafana.ini`
antes de aplicar el provisioning.

---

## Camino A — Provisioning declarativo (recomendado)

### A1. Encontrar el UID de la datasource InfluxDB

```bash
export GRAFANA_URL="http://grafana.altermundi.net:3000"
export GRAFANA_TOKEN="<api token con rol Admin>"

curl -s -H "Authorization: Bearer $GRAFANA_TOKEN" \
  "$GRAFANA_URL/api/datasources" \
  | jq '.[] | select(.type=="influxdb") | {name, uid, url, database}'
```

Anotar el `uid` (string corto tipo `aelm2x7tk` o similar).

### A2. Reemplazar el placeholder en `rules.yaml`

```bash
sed -i "s/<INFLUX_DATASOURCE_UID>/<UID_REAL>/g" \
  ops/grafana/provisioning/alerting/rules.yaml
```

### A3. Subir los archivos al server de Grafana

```bash
scp ops/grafana/provisioning/alerting/*.yaml \
  grafana.altermundi.net:/etc/grafana/provisioning/alerting/
ssh grafana.altermundi.net "sudo systemctl restart grafana-server"
```

(O ajustar paths según la instalación: Docker, Kubernetes, etc.)

### A4. Validar que cargaron

```bash
curl -s -H "Authorization: Bearer $GRAFANA_TOKEN" \
  "$GRAFANA_URL/api/v1/provisioning/alert-rules" \
  | jq '.[] | select(.uid | startswith("cist-")) | {uid, title, condition}'

curl -s -H "Authorization: Bearer $GRAFANA_TOKEN" \
  "$GRAFANA_URL/api/v1/provisioning/contact-points" \
  | jq '.[] | select(.name=="telegram-cisterna")'
```

Las 4 rules deben aparecer.

---

## Camino B — UI step by step

Asumiendo Grafana ≥ 9 con Unified Alerting habilitado.

### B1. Crear contact point Telegram

1. **Alerting → Contact points → New contact point**.
2. Name: `telegram-cisterna`.
3. Integration: `Telegram`.
4. Bot API Token: `$TELEGRAM_BOT_TOKEN` (tomar de `.env` local o gestor de secretos).
5. Chat ID: `$TELEGRAM_CHAT_ID`.
6. Optional Telegram settings → **Message** (paste el template del YAML).
7. **Test** → debe llegar un mensaje al chat. Si no llega: revisar token / chat (este chat es un grupo, el bot tiene que ser miembro).
8. **Save contact point**.

### B2. Crear cada alert rule

Para cada una de las 4 rules en `rules.yaml`:

1. **Alerting → Alert rules → New alert rule**.
2. **Grafana managed alert** (no Mimir/Loki).
3. **A. Query** → datasource = InfluxDB de cisterna. Pegar el `query:` de la rule (modo raw SQL). Format = Time series. Time range = `Last 10m` (o `15m` para no-data).
4. **B. Reduce** → input A, function `last` (o `sum` para no-data).
5. **C. Threshold** → input B, operador y valor según rule. Si Grafana ≥ 11.3, abrir "Recovery threshold" y poner el valor opuesto (30 para low, 90 para overflow).
6. **Condition** = C.
7. **Folder** = `Cisterna` (crear si no existe).
8. **Evaluation group** = `cisterna`, interval = `1m`.
9. **Pending period (for)** = según tabla de arriba.
10. **Labels**: `severity`, `kind` (ver YAML).
11. **Annotations**: `summary`, `description`, `value` (ver YAML).
12. **Notifications** → contact point = `telegram-cisterna`.
13. **Save**.

### B3. Validar end-to-end

- En el panel de la rule, click **Evaluate** → debe mostrar estado (Normal / Pending / Firing).
- Para forzar disparo: temporalmente cambiar el threshold a un valor que se cumpla ahora (ej. low: `< 100`) → guardar → esperar `for:` minutos → verificar mensaje en Telegram → volver al valor original.

---

## Camino C — Validar via API (recomendado en cualquier caso)

Después de provisionar, simular evaluación:

```bash
# Listar rules y su estado actual
curl -s -H "Authorization: Bearer $GRAFANA_TOKEN" \
  "$GRAFANA_URL/api/prometheus/grafana/api/v1/rules" \
  | jq '.data.groups[] | select(.name=="cisterna") | .rules[] | {name, state, health, lastEvaluation}'

# Listar alertas activas
curl -s -H "Authorization: Bearer $GRAFANA_TOKEN" \
  "$GRAFANA_URL/api/prometheus/grafana/api/v1/alerts" \
  | jq '.data.alerts[] | select(.labels.kind | startswith("level") or . == "sensor_fault" or . == "no_data")'
```

Para forzar un test sintético sin tocar las rules, inyectar un punto contra
Influx que dispare la condición y esperar la ventana de evaluación:

```bash
# Tomar el token de escritura desde el .env local o gestor de secretos
export INFLUX_WRITE_TOKEN="$(grep ^GRAFANA_TOKEN ../.env | cut -d= -f2-)"

# Forzar nivel bajo
curl -s -X POST "http://grafana.altermundi.net:8086/write?db=cto" \
  -H "Authorization: ${INFLUX_WRITE_TOKEN}" \
  --data-binary 'cisterna,device=cist-test level=5,sensor_ok=1i'

# Esperar 5 min + 1m de evaluacion -> debe llegar mensaje al chat de Telegram

# Limpiar el dato de test cuando termines (Influx 1.x, POST por DROP):
curl -X POST "http://grafana.altermundi.net:8086/query?db=cto" \
  -H "Authorization: ${INFLUX_WRITE_TOKEN}" \
  --data-urlencode "q=DROP SERIES FROM cisterna WHERE device='cist-test'"
```

---

## Después de validar

Una vez confirmado que las 4 rules disparan y resuelven contra el chat:

1. Eliminar `TelegramNotifier` del firmware (rama separada, tarea #4 del plan).
2. Sacar `telegram.*` del `config.json` y del `.env`.
3. Mantener este README como única fuente de verdad de las alertas.
