# Hardware - Nivel Cisterna

## Esquema de Conexión

```
ESP32 DevKit V1
┌──────────────────┐
│              3V3 ├──── VCC Sensor
│              GND ├──── GND Sensor
│           GPIO 5 ├──── TRIG Sensor
│          GPIO 18 ├──── ECHO Sensor (*)
│          GPIO 26 ├──── IN  Relay (opcional)
│              VIN ├──── VCC Relay (5V)
│              GND ├──── GND Relay
└──────────────────┘

(*) JSN-SR04T: si es modelo de 1 pin (modo 2), 
    TRIG y ECHO van al mismo GPIO.
```

## BOM (Bill of Materials)

| Cant | Componente | Referencia |
|------|-----------|------------|
| 1 | ESP32 DevKit V1 | |
| 1 | Sensor ultrasónico waterproof | JSN-SR04T (preferido) o HC-SR04 |
| 1 | Módulo relay 1CH 5V | Opcional, para bomba |
| 1 | Fuente 5V 1A | USB o regulador |
| - | Cableado, caja estanca | Para instalación exterior |

## Notas de Instalación

- El sensor debe montarse en la parte **superior** del tanque, apuntando hacia abajo
- Mantener el sensor **vertical** y libre de obstrucciones
- El JSN-SR04T es resistente al agua (IP67) - preferido para cisterna
- El HC-SR04 NO es waterproof - requiere protección adicional
- Distancia mínima de lectura: ~2 cm (JSN-SR04T) / ~2 cm (HC-SR04)
- Distancia máxima: ~450 cm (JSN-SR04T) / ~400 cm (HC-SR04)
