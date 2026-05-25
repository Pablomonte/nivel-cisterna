# Anexo C: Materiales y costos — Etapa 3 (Automatización on/off)

**Etapa:** Dejar de ir a prender — Comando automático de bomba por niveles  
**Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  
**Dependencia:** Requiere Etapa 1 operativa; recomendada tras Etapa 2.

---

## Descripción técnica resumida

Se intercala un relé industrial de estado sólido o electromecánico (10 A, 250 VCA) en el circuito de comando de la bomba. El ESP32 decide el encendido/apagado según reglas programadas: cisterna baja → arranca; cisterna alta → para; pozo bajo → inhibe arranque. Se agrega un sensor de corriente no invasivo (SCT-013) para confirmar que la bomba efectivamente respondió al comando y detectar fallas (por ejemplo, cavitación o disparo térmico).

---

## Lista de materiales

| Ítem | Especificación técnica mínima | Cantidad | Precio unitario ARS | Precio total ARS |
|---|---|---:|---:|---:|
| Relé industrial | 1 canal, 10 A / 250 VCA, bobina 5 V o 12 V, montaje riel DIN | 1 | $21.000–35.000 | $21.000–35.000 |
| Sensor de corriente no invasivo | SCT-013-030 (30 A) con divisor de voltaje y protección Zener | 1 | $14.000–28.000 | $14.000–28.000 |
| Fuente auxiliar 12 V/1 A | Para bobina del relé si la fuente existente no tiene rail 12 V | 1 | $7.000–12.000 | $7.000–12.000 |
| Borneras y protección | Fusible 6 A, borne tierra, riel DIN 10 cm | 1 lote | $7.000–12.000 | $7.000–12.000 |
| Instalación eléctrica | Conexión en paralelo al circuito de comando de la bomba (sin alterar protecciones motrices), pruebas de seguridad y puesta en marcha | 1 | $70.000–112.000 | $70.000–112.000 |

---

## Totales

| Concepto | Importe ARS |
|---|---|
| Materiales | $49.000–87.000 |
| Mano de obra e instalación | $70.000–112.000 |
| **Total Etapa 3** | **$119.000–199.000** |

---

## Notas para el presupuesto

- La instalación debe ser realizada por electricista matriculado dado que se interviene el tablero de comando existente.
- Se preservan intactas las protecciones térmicas y diferenciales actuales del motor; el relé actúa únicamente sobre el circuito de control (bobina del contactor), nunca en serie con el motor.
- El sensor de corriente permite diferenciar entre "comando enviado pero bomba no arrancó" (falla mecánica o eléctrica) y "bomba consumiendo anormalmente" (obstrucción o cavitación).
