# Nivel Cisterna

Monitor de nivel de agua para cisterna con ESP32, sensor ultrasonico waterproof JSN-SR04T, dashboard web embebido y reporte a Grafana/InfluxDB. Las alertas (nivel critico, rebalse, sensor caido, sin datos) viven en Grafana — ver [ops/grafana/](ops/grafana/README.md).

## Funcionalidades

- Sensor ultrasonico con filtro mediana y metricas de salud (`sensor_ok`, fallas consecutivas, variacion entre muestras).
- Dashboard web embebido con nivel, volumen, estado del sensor, energia y datos de enlace.
- Reporte a Grafana/InfluxDB por HTTP POST con heartbeat periodico aunque falle la lectura.
- Modo `battery` opcional con deep sleep entre ciclos de medicion.
- Configuracion JSON persistente en SPIFFS con secretos sensibles guardados en NVS.
- WiFi con AP seguro de fallback para provision inicial o contingencia.

## Hardware

| Componente | Pin ESP32 | Notas |
|------------|-----------|-------|
| Sensor Trig | GPIO 5 | JSN-SR04T (modulo + cabezal IP67) |
| Sensor Echo | GPIO 18 | JSN-SR04T (modulo + cabezal IP67) |

## Compilacion

```bash
pio run -e cisterna_dev
pio run -e cisterna_prod
pio run -e cisterna_dev -t upload
pio run -e cisterna_dev -t uploadfs
pio device monitor -b 115200
```

## Configuracion

El proyecto incluye un ejemplo en [data/config.json.example](data/config.json.example). El `data/config.json` real esta gitignored. Los campos sensibles (`wifi_pass`, `grafana.token`, `admin.password`) se aceptan por API, pero al guardarse quedan en NVS y no vuelven a exponerse por `/api/config`.

### Provisioning local con `.env` (recomendado)

Para no commitear secretos al repo, usar variables de entorno y el script de provisioning:

```bash
cp .env.example .env
$EDITOR .env                    # llenar WIFI_PASS, GRAFANA_TOKEN, etc.
./scripts/provision_config.sh   # mergea .env -> data/config.json
pio run -e cisterna_dev -t uploadfs
```

Tanto `.env` como `data/config.json` estan en `.gitignore`. El script solo escribe los campos que estan seteados; lo demas se mantiene del JSON existente.

Variables soportadas: `WIFI_SSID`, `WIFI_PASS`, `GRAFANA_URL`, `GRAFANA_TOKEN`, `ADMIN_USERNAME`, `ADMIN_PASSWORD`.

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
| `/api/status` | GET | Estado actual, salud del sensor, WiFi y heap |
| `/api/config` | GET | Configuracion actual redacted, requiere Basic Auth |
| `/api/config` | POST | Actualiza configuracion y reinicia, requiere Basic Auth |
| `/api/power` | GET/POST | Configuracion de energia (normal/battery), requiere Basic Auth |
| `/restart` | POST | Reinicia el dispositivo, requiere Basic Auth |

## Campos enviados a Grafana

Ejemplo de line protocol:

```text
cisterna,device=cist-01 level=85.2,distance=23.4,volume=1200,sensor_ok=1i,sensor_failures=0i,sensor_spread_cm=0.7,last_success_age_sec=2i,capacity=1700.0,wifi_connected=1i,free_heap=231112i,uptime_sec=7200i
```

Campos utiles para dashboards/alertas:

- `level`, `distance`, `volume`, `capacity`
- `sensor_ok`, `sensor_failures`, `sensor_spread_cm`, `last_success_age_sec`
- `wifi_connected`, `free_heap`, `uptime_sec`

## Alertas

Las alertas se manejan en Grafana, no en el firmware. Ver [ops/grafana/](ops/grafana/README.md) para las 4 reglas provisionadas:

- `cist-low-level`: `level < 15` por 5 min, resolucion a 30 (hysteresis).
- `cist-overflow`: `level > 95` por 1 min, resolucion a 90 (hysteresis).
- `cist-sensor-fault`: `sensor_ok == 0` por 2 min.
- `cist-no-data`: cero puntos en los ultimos 30 min.

Todas notifican al chat de Telegram via el contact point `telegram-cisterna` de Grafana.

## Estructura

```text
include/
  config_manager.h
  debug.h
  grafana.h
  level_sensor.h
  power_manager.h
  secret_manager.h
  tank.h
  version.h
  web_dashboard.h
src/
  main.cpp
data/
  config.json
ops/
  grafana/   # provisioning de alertas
platformio.ini
```
