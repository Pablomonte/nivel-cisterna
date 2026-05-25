# Anexo E: Materiales y costos — Independización de infraestructura digital

**Anexo transversal:** Migración de Grafana e InfluxDB desde AlterMundi a servidor local propio  
**Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  
**Dependencia:** Recomendada tras validar la Etapa 1; puede ejecutarse en paralelo con Etapa 2 o 3.

---

## Descripción técnica resumida

Se adquiere un minicomputador ARM64 (Raspberry Pi 4/5 u Orange Pi 3B) para ejecutar localmente el stack de telemetría: broker MQTT (Mosquitto), base de datos de series temporales (InfluxDB 1.8) y panel de visualización (Grafana). Los datos históricos se respaldan y transfieren; los ESP32 se reconfiguran para publicar en el broker local. El equipo se aloja en un gabinete seco con estabilización de tensión. La conectividad puede ser WiFi o Ethernet según disponibilidad en el predio.

---

## Lista de materiales

| Ítem | Especificación técnica mínima | Cantidad | Precio unitario ARS | Precio total ARS |
|---|---|---:|---:|---:|
| SBC ARM64 | Raspberry Pi 4 (4 GB) o Pi 5 (4 GB); alternativa: Orange Pi 3B (8 GB) con fuente incluida | 1 | $84.000–140.000 | $84.000–140.000 |
| Almacenamiento | MicroSD 64 GB U3 (clase A2) o SSD USB 3.0 120 GB para mayor durabilidad | 1 | $21.000–42.000 | $21.000–42.000 |
| Fuente de alimentación | Oficial USB-C 5 V/3 A (si no viene con el SBC); con protección sobretensión | 1 | $14.000–21.000 | $14.000–21.000 |
| Gabinete protegido | Caja plástica con ventilación pasiva, soporte pared, bridas; opción estanca si el lugar es húmedo | 1 | $21.000–35.000 | $21.000–35.000 |
| Estabilizador/UPS | Estabilizador de línea 500 VA o mini-UPS 12 V para SBC (evita corrupción de SD por cortes) | 1 | $28.000–56.000 | $28.000–56.000 |
| Cable Ethernet Cat 5e | 5 m, conectores RJ45 moldeados, para conexión a switch/router local | 1 | $7.000–10.000 | $7.000–10.000 |
| Instalación y migración | Instalación del SO, despliegue Docker, traspaso de dashboards, backup de datos históricos, reconfiguración de ESP32 y capacitación básica de administración | 1 | $70.000–112.000 | $70.000–112.000 |

---

## Totales

| Concepto | Importe ARS |
|---|---|
| Materiales | $175.000–304.000 |
| Mano de obra e instalación | $70.000–112.000 |
| **Total Independización** | **$245.000–416.000** |

---

## Notas para el presupuesto

- Se recomienda el Orange Pi 3B (8 GB) como alternativa de mejor relación costo/rendimiento en Argentina, siempre que se verifique disponibilidad de imágenes Docker para ARM64.
- La retención de datos se configura a 90 días para no saturar el almacenamiento; dashboards y configuraciones se respaldan automáticamente en copia semanal.
- Si el predio no tiene conectividad WiFi estable en la ubicación del gabinete, evaluar un par de antenas direccionales o un repetidor (costo adicional ~AR$ 50.000–90.000).
- La migración puede hacerse en horario de bajo consumo para no perder datos del día; se estima una ventana de 4 horas de trabajo.
