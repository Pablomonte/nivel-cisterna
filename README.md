# Nivel Cisterna

Sistema de monitoreo de nivel de agua en tanque cisterna para sistema de monitoreo y control integral de cooperativa de servicios barrial.

## Funcionalidades

- **Sensor ultrasónico** (HC-SR04 / JSN-SR04T) para medición de distancia → nivel
- **Dashboard web** embebido con visualización de nivel en tiempo real
- **Reporte a Grafana** via WiFi (InfluxDB line protocol)
- **Control de bomba** con histéresis y timeout de seguridad (opcional)
- **Configuración JSON** persistente via SPIFFS
- **WiFi con AP fallback** para configuración inicial

## Hardware

| Componente | Pin ESP32 | Notas |
|------------|-----------|-------|
| Sensor Trig | GPIO 5 | HC-SR04 / JSN-SR04T |
| Sensor Echo | GPIO 18 | HC-SR04 / JSN-SR04T |
| Relay Bomba | GPIO 26 | Opcional, módulo 1CH |

## Compilación

```bash
# Desarrollo (con debug serial)
pio run -e cisterna_dev

# Producción (sin debug)
pio run -e cisterna_prod

# Flash
pio run -e cisterna_dev -t upload

# Monitor serial
pio device monitor -b 115200
```

## Configuración

Al primer arranque se crea un AP WiFi `cisterna-01`. Conectarse y acceder a `http://192.168.4.1/` para ver el dashboard.

La configuración se edita via API:

```bash
# Ver config actual
curl http://<IP>/api/config

# Actualizar config
curl -X POST http://<IP>/api/config -H 'Content-Type: application/json' -d @config.json
```

## API Endpoints

| Endpoint | Método | Descripción |
|----------|--------|-------------|
| `/` | GET | Dashboard web |
| `/api/status` | GET | Estado actual (JSON) |
| `/api/config` | GET | Configuración actual |
| `/api/config` | POST | Actualizar configuración |
| `/api/pump` | POST | Control de bomba (`?action=on\|off\|auto`) |
| `/restart` | POST | Reiniciar dispositivo |

## Datos a Grafana

```
cisterna,device=cisterna-01 level=85.2,distance=23.4,volume=1200,pump_state=0,pump_runtime=3600
```

## Estructura

```
├── include/
│   ├── config_manager.h    # Carga/guardado config JSON
│   ├── debug.h             # Macros de debug multinivel
│   ├── grafana.h           # HTTP POST a InfluxDB
│   ├── level_sensor.h      # Sensor ultrasónico + filtro mediana
│   ├── pump_controller.h   # Control de bomba con histéresis
│   ├── tank.h              # Geometría y cálculos de tanque
│   ├── version.h           # Versión firmware
│   └── web_dashboard.h     # HTML/CSS/JS embebido
├── src/
│   └── main.cpp            # Setup/loop principal
├── data/
│   └── config.json         # Config por defecto (SPIFFS)
└── platformio.ini
```
