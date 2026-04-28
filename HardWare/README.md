# Hardware - Nivel Cisterna

## Sensor: JSN-SR04T (waterproof)

Sensor ultrasonico industrial con cabezal IP67 separado por cable de ~2.5 m,
ideal para cisternas y depositos. El modulo de control queda fuera del agua y
solo el transductor sumergible mira hacia la superficie del liquido.

- Voltaje: 5 V (VCC) / logica 5 V tolerada por GPIO ESP32 a traves de divisor
  recomendado en ECHO si se va a operar mucho tiempo, opcional para pruebas.
- Rango util: ~25 cm a 450 cm.
- Angulo de cono: ~50 grados.
- Misma interfaz Trig/Echo que el HC-SR04 (driver compartido).

## Esquema de Conexión

```
ESP32 DevKit V1
┌──────────────────┐
│              VIN ├──── VCC Modulo JSN-SR04T (5V)
│              GND ├──── GND Modulo JSN-SR04T
│           GPIO 5 ├──── TRIG Modulo JSN-SR04T
│          GPIO 18 ├──── ECHO Modulo JSN-SR04T (*)
│          GPIO 26 ├──── IN  Relay (opcional)
│              VIN ├──── VCC Relay (5V)
│              GND ├──── GND Relay
└──────────────────┘

Cabezal sumergible <── cable ── Modulo JSN-SR04T

(*) Si se usa el modulo en modo 1-pin (variantes con jumper para
    comunicacion serial), TRIG y ECHO no aplican y se debe usar el
    driver UART correspondiente. Este firmware usa modo Trig/Echo.
```

## BOM (Bill of Materials)

| Cant | Componente | Referencia |
|------|-----------|------------|
| 1 | ESP32 DevKit V1 | |
| 1 | Sensor ultrasonico waterproof | JSN-SR04T (modulo + cabezal IP67) |
| 1 | Modulo relay 1CH 5V | Opcional, para bomba |
| 1 | Fuente 5V 1A | USB o regulador |
| - | Cableado, caja estanca | Para instalacion exterior |

## Notas de Instalacion

- El cabezal sumergible va montado en la parte **superior** del tanque,
  apuntando hacia abajo, por encima de la maxima posible altura del agua.
- Mantener el sensor **vertical** y libre de obstrucciones.
- El cabezal del JSN-SR04T es resistente al agua (IP67) - apto para cisterna.
- Distancia minima de lectura: ~25 cm (zona ciega del transductor).
- Distancia maxima: ~450 cm.
- El modulo de control debe quedar **fuera del agua** en una caja estanca.
