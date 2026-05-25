# Anexo A: Materiales y costos — Etapa 1 (MVP Monitoreo)

**Etapa:** Saber qué hay — Sensado de niveles en cisternas  
**Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  
**Validez:** 60 días desde la fecha de emisión

---

## Descripción técnica resumida

Dos sensores ultrasónicos de nivel (rango 0–4 m, salida analógica 4–20 mA o digital UART) se instalan en tapas de visita de las cisternas. Un microcontrolador ESP32-DevKitC con adaptador WiFi recibe las señales, las publica por MQTT hacia el broker de AlterMundi (o local, si ya se ejecutó la independización) y un panel Grafana muestra nivel histórico y alertas. La alimentación es 220 VCA mediante fuente conmutada de 5 V/2 A dentro de gabinete estanco IP65.

---

## Lista de materiales

| Ítem | Especificación técnica mínima | Cantidad | Precio unitario ARS | Precio total ARS |
|---|---|---:|---:|---:|
| Sensor ultrasónico de nivel | Rango 0,1–4 m, IP67, salida 4–20 mA o UART; compatible con alimentación 12–24 V | 2 | $28.000–42.000 | $56.000–84.000 |
| Microcontrolador ESP32 | ESP32-DevKitC o ESP32-WROOM-32U con antena externa; WiFi 2,4 GHz | 1 | $20.000–30.000 | $20.000–30.000 |
| Gabinete estanco | Policarbonato ABS IP65, 200 × 150 × 100 mm, con tornillos y bisagras | 1 | $12.000–18.000 | $12.000–18.000 |
| Fuente conmutada industrial | Entrada 100–240 VCA, salida 5 V/2 A + 12 V/1 A (dual), montaje en riel DIN | 1 | $10.000–15.000 | $10.000–15.000 |
| Borneras y conectores | Borneras tornillo, conectores impermeables M12, cable UTP Cat 5e exterior | 1 lote | $12.000–20.000 | $12.000–20.000 |
| Cable multipar señal | 2 × 1 mm², blindado, para sensor 4–20 mA (15 m totales) | 15 m | $600–1.000/m | $9.000–15.000 |
| Sujeción y accesorios | Abrazaderas, tacos de fijación, silicona de sellado, bridas | 1 lote | $5.000–8.000 | $5.000–8.000 |
| Instalación técnica in situ | Relevamiento, fijación de sensores, cableado, pruebas y capacitación básica al operario | 1 | $70.000–112.000 | $70.000–112.000 |

---

## Totales

| Concepto | Importe ARS |
|---|---|
| Materiales | $112.000–190.000 |
| Mano de obra e instalación | $70.000–112.000 |
| **Total Etapa 1** | **$182.000–280.000** |

---

## Notas para el presupuesto

- Los precios de sensores ultrasónicos varían según stock local versus importación. Se recomienda cotizar alternativas en distribuidores de Córdoba Capital para reducir tiempos de entrega.
- Si una de las cisternas tiene espuma o vapor intenso, evaluar sensor de presión hidrostático sumergible en reemplazo del ultrasónico (incremento estimado: AR$ 40.000–70.000).
- La instalación incluye 1 hora de capacitación al operario para lectura del panel y silenciamiento de alarmas.
