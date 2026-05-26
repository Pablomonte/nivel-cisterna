# Sistema de Monitoreo y Gestión de Agua Potable para Cooperativas de Servicios

---

## Contexto

Las cooperativas de agua de pequeñas y medianas localidades operan infraestructura crítica —cisternas, pozos, bombas, válvulas— con recursos limitados de personal y sin herramientas de visibilidad en tiempo real. El resultado habitual es una gestión reactiva: se actúa cuando algo falla, no antes.

Este documento presenta una solución de telemetría y control por etapas, diseñada para organizaciones que necesitan mejorar su operación sin grandes inversiones iniciales y sin depender de conocimiento técnico externo para el día a día.

---

## Problemática típica

- **Sin datos históricos:** No hay registro de niveles, caudales ni consumo eléctrico. Las decisiones operativas se toman "a ojo".
- **Operación reactiva:** Los desbordamientos, faltantes y fallas de bomba se detectan cuando ya ocurrieron, no con anticipación.
- **Visitas innecesarias al campo:** El personal debe trasladarse físicamente para constatar niveles o verificar el estado de la bomba.
- **Riesgo de golpe de ariete y desgaste prematuro:** Arranques y paradas manuales sin criterio técnico acortan la vida útil de los equipos.
- **Falta de información para la gestión:** Sin datos, es difícil justificar inversiones, negociar tarifas o solicitar subsidios.
- **Dependencia de terceros para la infraestructura digital:** Muchas cooperativas no tienen servidores propios; los datos quedan en sistemas ajenos.

---

## Filosofía del proyecto

El sistema está diseñado bajo tres principios:

1. **Cada etapa resuelve un problema concreto y medible**, generando resultados visibles antes de pasar a la siguiente.
2. **El costo de entrada es bajo.** La primera etapa requiere una inversión menor y produce datos reales en pocas semanas.
3. **La soberanía de datos es un objetivo, no un lujo.** Desde el diseño, el sistema contempla que la cooperativa opere su propia infraestructura digital cuando esté lista para hacerlo.

---

## Etapas de implementación

### Etapa 1 · MVP Monitoreo de cisternas

**Qué se instala:** Sensores de nivel ultrasónicos en las cisternas principales, conectados a un controlador de bajo consumo con conectividad WiFi.

**Qué se obtiene:**
- Panel de visualización en tiempo real (nivel porcentual y volumen estimado por cisterna)
- Alertas automáticas por desbordamiento, nivel crítico y falla de sensor (vía Telegram u otro canal de mensajería)
- Primeros 30 días de datos históricos para tomar decisiones informadas

**Inversión estimada:** USD 290–360 (materiales + instalación)

**Duración:** 15–30 días desde aprobación hasta sistema operativo

---

### Etapa 2 · Monitoreo de pozos, válvulas y caudal

**Qué se instala:** Sensores de nivel en pozos de captación, sensores de posición en válvulas de desvío y, opcionalmente, un medidor de caudal en la línea principal.

**Qué se obtiene:**
- Visibilidad completa del ciclo hídrico: desde la captación hasta la cisterna
- Detección temprana de fugas o caídas de rendimiento en pozos
- Historial de maniobras de válvulas para auditoría y análisis

**Inversión estimada:** USD 580–740

**Duración:** 3–4 semanas

---

### Etapa 3 · Automatización on/off de bomba

**Qué se instala:** Módulo de control que activa y detiene la bomba automáticamente según los niveles configurados, más un sensor de corriente no invasivo para validar que la bomba efectivamente arrancó o se detuvo.

**Qué se obtiene:**
- Eliminación de las visitas para maniobras de bomba en condiciones normales
- Control de desbordamiento y nivel mínimo sin intervención manual
- Registro de horas de funcionamiento y alertas de falla eléctrica

**Nota de seguridad:** La automatización actúa sobre el circuito de mando (bobina del contactor), no sobre la alimentación del motor. Las protecciones eléctricas existentes (térmico, diferencial) permanecen intactas. La instalación debe realizarla un electricista matriculado.

**Inversión estimada:** USD 230–285

**Duración:** 2–3 semanas

---

### Etapa 4 · Bomba inteligente con variador de frecuencia

**Qué se instala:** Variador de frecuencia (VFD) en la bomba principal y sensor de presión en la línea de impulsión. El sistema regula la velocidad del motor para mantener presión constante.

**Qué se obtiene:**
- Reducción del consumo eléctrico estimada en 20–40 % respecto al funcionamiento on/off
- Presión estable en la red, eliminando golpes de ariete
- Mayor vida útil del motor y los caños
- Datos de consumo eléctrico para justificar la inversión ante organismos de financiamiento

**Inversión estimada:** USD 820–1.070

**Duración:** 3–4 semanas (incluye parametrización y capacitación del operador)

---

### Etapa T · Independización de infraestructura digital

> Esta etapa puede ejecutarse en paralelo con la Etapa 1 si la cooperativa prefiere soberanía de datos desde el inicio, o puede diferirse hasta validar el sistema.

**Contexto:** Durante las primeras etapas, el sistema de visualización y alertas (panel de control, base de datos histórica) utiliza herramientas disponibles en la nube con capa de uso gratuita. Esta dependencia de servicios externos es **deliberadamente transitoria**: reduce el costo de entrada y permite validar el sistema sin comprometer inversión en infraestructura propia antes de tener datos reales.

**Qué se instala:** Un servidor de pequeño formato (mini-PC de bajo consumo, tipo Raspberry Pi o similar) en las instalaciones de la cooperativa, con el stack de visualización y almacenamiento funcionando localmente.

**Qué se obtiene:**
- Los datos pasan a ser propiedad exclusiva de la cooperativa: sin cuotas de servicio, sin dependencia de terceros
- Retención histórica ilimitada (configurable según capacidad de almacenamiento)
- Acceso a los datos desde la red local, incluso sin internet
- Capacidad de expandir el sistema a futuras instalaciones sin costos adicionales de software

**Inversión estimada:** USD 200–300

---

## Criterios de avance entre etapas

Avanzar a la siguiente etapa no requiere que la anterior sea "perfecta", sino que haya demostrado valor tangible:

| Transición | Condición de avance |
|---|---|
| Etapa 1 → Etapa 2 o T | Panel operativo durante 30 días con datos reales y al menos 1 alerta útil disparada |
| Etapa 2 → Etapa 3 | Todos los pozos monitoreados y al menos 1 sensor de válvula funcional |
| Etapa 3 → Etapa 4 | Automatización on/off funcionando 15 días sin intervención manual; horas liberadas registradas |
| Etapa 4 → escala | Consumo eléctrico medido antes/después con al menos 20 % de ahorro confirmado |

---

## Inversión estimada acumulada

Los rangos son orientativos y deben actualizarse con cotización al momento de aprobación de cada etapa.

| Etapa | Inversión (USD) | Acumulado (USD) |
|---|---|---|
| 1 · MVP Monitoreo | 290–360 | 290–360 |
| 2 · Pozos, válvulas y caudal | 580–740 | 870–1.100 |
| 3 · Automatización on/off | 230–285 | 1.100–1.385 |
| 4 · VFD + presión constante | 820–1.070 | 1.920–2.455 |
| T · Independización digital | 200–300 | — |

**Escenarios de inversión:**

| Escenario | Etapas incluidas | Total estimado (USD) |
|---|---|---|
| Mínimo viable | 1 + T | 490–660 |
| Monitoreo completo + autonomía | 1 + 2 + T | 1.070–1.400 |
| Control total + soberanía digital | 1 + 2 + 3 + T | 1.300–1.685 |
| Óptimo energético completo | 1 + 2 + 3 + 4 + T | 2.120–2.755 |

---

## Beneficios esperados

- **Bajo riesgo de entrada:** La primera etapa tiene costo accesible y resultados visibles en semanas.
- **Operación más eficiente:** Reducción de visitas de campo, detección temprana de fallas, menor desgaste de equipos.
- **Datos para la gestión:** Historial que respalda decisiones de inversión, negociación de tarifas y solicitud de subsidios.
- **Soberanía progresiva:** La cooperativa gana autonomía sobre su infraestructura digital a medida que avanza.
- **Escalabilidad:** El sistema puede extenderse a nuevas captaciones, cisternas o instalaciones sin rediseño.

---

## Fuentes de financiamiento sugeridas

Una vez disponibles los datos de la Etapa 1 (consumo, niveles históricos, horas de bomba), la cooperativa cuenta con evidencia concreta para acceder a:

- **GEF Small Grants Programme (PNUD):** hasta USD 50.000 para eficiencia energética y resiliencia climática
- **FONCyT — Proyectos de Cooperación Especial:** proyectos academia-organización social con carta de intención institucional
- **Programas provinciales y municipales:** subsidios para cooperativas de servicios públicos con proyectos de mejora operativa
- **Fondos rotativos cooperativos:** amortización mensual a partir del ahorro energético validado en Etapa 4

---

## Próximos pasos

1. **Reunión de relevamiento:** visita técnica para relevar la instalación actual (cisternas, pozos, tablero eléctrico, conectividad).
2. **Cotización específica:** presupuesto detallado con materiales y mano de obra para la Etapa 1, ajustado a las condiciones reales del sitio.
3. **Aprobación de la Etapa 1:** decisión del Consejo Directivo para iniciar con la inversión mínima y obtener los primeros datos reales.
4. **Instalación y puesta en marcha:** en 15 a 30 días desde la aprobación, el sistema estará operativo.

---

*Los rangos de inversión están expresados en USD para independizarse de la volatilidad cambiaria. Deben convertirse y cotizarse en moneda local al momento de cada aprobación.*
