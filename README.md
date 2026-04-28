# Nivel Cisterna

Monitor de nivel de agua para cisterna con ESP32, sensor ultrasonico waterproof JSN-SR04T, dashboard web embebido, reporte a Grafana/InfluxDB, alertas Telegram y control opcional de bomba.

## Funcionalidades

- Sensor ultrasonico con filtro mediana y metricas de salud (`sensor_ok`, fallas consecutivas, variacion entre muestras).
- Dashboard web embebido con nivel, volumen, estado del sensor, estado de la bomba y datos de enlace.
- Reporte a Grafana/InfluxDB por HTTP POST con heartbeat periodico aunque falle la lectura.
- Control de bomba opcional con histeresis, timeout de seguridad, modo manual y metricas para timeline/alertas.
- Notificaciones Telegram directas desde el ESP32 para nivel critico, timeout de bomba, error de sensor y recuperacion.
- Configuracion JSON persistente en SPIFFS con secretos sensibles guardados en NVS.
- WiFi con AP seguro de fallback para provision inicial o contingencia.

## Hardware

| Componente | Pin ESP32 | Notas |
|------------|-----------|-------|
| Sensor Trig | GPIO 5 | JSN-SR04T (modulo + cabezal IP67) |
| Sensor Echo | GPIO 18 | JSN-SR04T (modulo + cabezal IP67) |
| Relay Bomba | GPIO 26 | Opcional, modulo 1CH |

## Compilacion

```bash
pio run -e cisterna_dev
pio run -e cisterna_prod
pio run -e cisterna_dev -t upload
pio run -e cisterna_dev -t uploadfs
pio device monitor -b 115200
```

## Configuracion

El proyecto incluye un ejemplo en [data/config.json.example](data/config.json.example). El `data/config.json` real esta gitignored. Los campos sensibles (`wifi_pass`, `grafana.token`, `admin.password`, `telegram.bot_token`) se aceptan por API, pero al guardarse quedan en NVS y no vuelven a exponerse por `/api/config`.

### Provisioning local con `.env` (recomendado)

Para no commitear secretos al repo, usar variables de entorno y el script de provisioning:

```bash
cp .env.example .env
$EDITOR .env                    # llenar TELEGRAM_BOT_TOKEN, etc.
./scripts/provision_config.sh   # mergea .env -> data/config.json
pio run -e cisterna_dev -t uploadfs
```

Tanto `.env` como `data/config.json` estan en `.gitignore`. El script solo escribe los campos que estan seteados; lo demas se mantiene del JSON existente. Si `telegram.enabled=true`, el script exige `TELEGRAM_BOT_TOKEN` y `TELEGRAM_CHAT_ID`.

Variables soportadas: `TELEGRAM_ENABLED`, `TELEGRAM_BOT_TOKEN`, `TELEGRAM_CHAT_ID`, `WIFI_SSID`, `WIFI_PASS`, `GRAFANA_URL`, `GRAFANA_TOKEN`, `ADMIN_USERNAME`, `ADMIN_PASSWORD`.

En el primer arranque, o si no hay `wifi_ssid`, el equipo levanta un AP WPA2 con:

- SSID: `cisterna-01` o el `device_name` configurado
- Usuario admin: `admin` por defecto
- Password AP/admin: `cisterna-XXXXXX` derivado de los ultimos 6 hex del MAC, salvo que luego se configure `admin.password`

La configuracion se consulta y actualiza con autenticacion Basic:

```bash
curl -u admin:<password> http://<IP>/api/config

curl -u admin:<password> \
  -X POST http://<IP>/api/config \
  -H 'Content-Type: application/json' \
  -d @data/config.json
```

## API

| Endpoint | Metodo | Descripcion |
|----------|--------|-------------|
| `/` | GET | Dashboard web |
| `/api/status` | GET | Estado actual, salud del sensor, bomba, WiFi y heap |
| `/api/config` | GET | Configuracion actual redacted, requiere Basic Auth |
| `/api/config` | POST | Actualiza configuracion y reinicia, requiere Basic Auth |
| `/api/pump?action=on|off|auto` | POST | Control manual/auto de bomba, requiere Basic Auth |
| `/restart` | POST | Reinicia el dispositivo, requiere Basic Auth |

## Campos enviados a Grafana

Ejemplo de line protocol:

```text
cisterna,device=cisterna-01 level=85.2,distance=23.4,volume=1200,sensor_ok=1i,sensor_failures=0i,sensor_spread_cm=0.7,last_success_age_sec=2i,capacity=1700.0,wifi_connected=1i,free_heap=231112i,uptime_sec=7200i,pump_enabled=1i,pump_on=0i,pump_state_code=0i,pump_runtime_sec=3600i,pump_auto_mode=1i
```

Campos utiles para dashboards/alertas:

- `level`, `distance`, `volume`, `capacity`
- `sensor_ok`, `sensor_failures`, `sensor_spread_cm`, `last_success_age_sec`
- `pump_enabled`, `pump_on`, `pump_state_code`, `pump_runtime_sec`, `pump_auto_mode`
- `wifi_connected`, `free_heap`, `uptime_sec`

## Sugerencia de alertas

- Nivel critico: `level <= 15` con `sensor_ok == 1`
- Nivel bajo: `level <= 30` con `sensor_ok == 1`
- Sensor erratico: `sensor_spread_cm` alto o `sensor_failures > 0`
- Device offline: ausencia de serie o `last_success_age_sec` creciendo fuera del intervalo esperado
- Bomba timeout: `pump_state_code == 4`

## Estructura

```text
include/
  config_manager.h
  debug.h
  grafana.h
  level_sensor.h
  pump_controller.h
  secret_manager.h
  tank.h
  telegram_notifier.h
  version.h
  web_dashboard.h
src/
  main.cpp
data/
  config.json
platformio.ini
```
