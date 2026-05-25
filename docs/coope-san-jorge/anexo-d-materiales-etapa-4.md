# Anexo D: Materiales y costos — Etapa 4 (VFD + presión constante)

**Etapa:** Bomba inteligente — Variador de frecuencia y control PID de presión  
**Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  
**Dependencia:** Requiere Etapa 3 operativa; motor caracterizado (monofásico o trifásico, potencia exacta).

---

## Descripción técnica resumida

Se reemplaza el arranque directo por un variador de frecuencia (VFD) acorde al motor existente (~3 HP). El ESP32 cierra un lazo PID leyendo un sensor de presión 4–20 mA en la red de distribución y enviando una referencia de velocidad al VFD (0–10 V o Modbus RTU). La bomba acelera o frena según la demanda, manteniendo la presión estable. Si el cable entre VFD y motor supera los 10 m, se agrega un filtro dV/dt para proteger los devanados.

---

## Lista de materiales

| Ítem | Especificación técnica mínima | Cantidad | Precio unitario ARS | Precio total ARS |
|---|---|---:|---:|---:|
| Variador de frecuencia (VFD) | 3 HP / 2,2 kW, entrada monofásica o trifásica según motor, salida trifásica V/f o vectorial, protección IP20 o IP54 | 1 | $210.000–350.000 | $210.000–350.000 |
| Sensor presión 4–20 mA | Rango 0–10 bar, rosca G 1/2", precisión ±0,5 % FS, con cable 5 m | 1 | $56.000–112.000 | $56.000–112.000 |
| Adaptador ESP32 → VFD | Módulo DAC 0–10 V (MCP4725) o conversor RS485/Modbus (MAX485) según VFD elegido | 1 | $14.000–28.000 | $14.000–28.000 |
| Filtro dV/dt | Para cables motor > 10 m; reactor o filtro pasivo 2,2 kW | 1 | $56.000–84.000 | $56.000–84.000 |
| Gabinete VFD | Estanco IP55 si el VFD no es de encapsulado industrial; ventilación forzada | 1 | $42.000–70.000 | $42.000–70.000 |
| Instalación técnica | Montaje del VFD, cableado potencia/control, puesta en marcha, ajuste de parámetros PID, capacitación al operario | 1 | $140.000–224.000 | $140.000–224.000 |

---

## Totales

| Concepto | Importe ARS |
|---|---|
| Materiales | $378.000–644.000 |
| Mano de obra e instalación | $140.000–224.000 |
| **Total Etapa 4** | **$518.000–868.000** |

---

## Notas para el presupuesto

- El precio del VFD varía significativamente según marca (Chint/Invt/ATV vs. ABB/Siemens). Se recomienda un VFD chino de calidad media (ATO, Chint) con soporte local en Argentina para garantizar repuestos.
- Verificar exactamente si el motor es monofásico o trifásico y su corriente nominal antes de comprar. Un error aquí implica cambio de equipo sin devolución.
- El filtro dV/dt se descarta si el cable motor es inferior a 10 m y el motor es relativamente nuevo (< 10 años).
- La instalación incluye 2 horas de capacitación: ajuste de consigna de presión, interpretación de alarmas del VFD y procedimiento de bypass manual en caso de falla.
