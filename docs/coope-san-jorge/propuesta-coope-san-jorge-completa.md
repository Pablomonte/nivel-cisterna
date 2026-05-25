# Propuesta Técnica: Sistema de Monitoreo Remoto de Niveles de Cisterna

**Destinatario:** Consejo Directivo  **Cooperativa de Provisión de Obras, Agua Potable y Otros Servicios Villa Parque San Jorge Limitada**  **Localidad:** Molinari, Municipio de Cosquín, Provincia de Córdoba  **Contacto / Teléfono:** *____  ***Expediente / Resolución N°:*** ___*  **Fecha:** Mayo 2026  **Elaboración:** Equipo de desarrollo ONG 501


---


## Resumen ejecutivo

La Cooperativa Villa Parque San Jorge gestiona hoy el abastecimiento de agua mediante recorridos físicos para verificar niveles en sus cisternas y accionar manualmente válvulas y bomba principal. Esta propuesta establece un plan por etapas —de bajo riesgo y baja inversión inicial— para dotar a la cooperativa de monitoreo remoto, alertas automáticas y, progresivamente, automatización del bombeo. El punto de partida es un MVP de sensores (Etapa 1) que permitirá visualizar niveles y recibir alarmas sin desplazarse al sitio. Una vez validado el monitoreo, se contempla la independización de la infraestructura digital actualmente hospedada por AlterMundi como paso transversal hacia la soberanía tecnológica de la cooperativa.


---


## 1. Contexto y diagnóstico

La cooperativa opera con:


- **Dos cisternas** que se llenan alternando la posición de válvulas de derivación.
- **Una bomba principal** de aproximadamente 3 HP.
- **Múltiples pozos** y **una toma en río** como fuentes de captación.
- **Operación manual:** un operario debe desplazarse, observar niveles, mover válvulas y accionar la bomba desde el tablero.

Esta modalidad genera riesgos operativos recurrentes:


- Desbordes por demora en cierre de carga.
- Faltantes por no detección temprana de nivel bajo.
- Golpes de ariete por encendido/apagado brusco de la bomba.
- Desgaste prematuro del motor por funcionamiento continuo al 100 % de potencia.
- Ausencia de registro histórico que permita planificar mantenimientos o negociar tarifas eléctricas.


---


## 2. Objetivo general

Implementar un sistema de telemetría y control escalable que permita a la cooperativa:


1. Conocer el estado de sus cisternas, pozos y válvulas en tiempo real desde un dispositivo móvil.
2. Recibir alertas automáticas ante condiciones críticas (nivel alto, nivel bajo, pozo seco).
3. Automatizar el encendido y apagado de la bomba según niveles programados.
4. Optimizar el consumo energético mediante velocidad variable del motor (VFD).
5. Garantizar la soberanía de sus datos mediante la migración progresiva a infraestructura propia.


---


## 3. Filosofía del proyecto

Ninguna etapa requiere una inversión grande de entrada. Cada fase:


1. Resuelve un problema concreto y medible.
2. Tiene un costo acotado.
3. Genera ahorros u horas liberadas que justifican la siguiente inversión.
4. Produce datos reales para tomar decisiones informadas.


---


## 4. Etapas de implementación


### Etapa 1: Saber qué hay — MVP de monitoreo de niveles

**Duración estimada:** 15 a 30 días desde aprobación.

Se instalan sensores ultrasónicos de nivel en las dos cisternas, conectados a un microcontrolador ESP32 con conectividad WiFi. Los datos se visualizan en un panel web (Grafana) con alarmas por Telegram.

**Notas complementarias:** El dispositivo opera con un perfil de 120 mWh en modo bajo consumo con picos de 0,14 A durante ciclos de lectura y transmisión de datos (intervalo programable). Se proveerá una batería auxiliar para la fase de prueba de concepto, evaluando la viabilidad del sistema durante el período de puesta en marcha.

Se requiere resolver por parte de la cooperativa el suministro eléctrico de este sistema.

**Resultado esperado:** El consejo directivo y el operario pueden ver el nivel de ambas cisternas desde un celular, recibir alertas ante desbordes o vaciamientos, y disponer de un historial de consumo de los primeros 30 días.

**Inversión aproximada:** Ver Anexo A.


---


### Etapa 2: Ver todo — Pozos, válvulas y caudal

**Duración estimada:** 1 mes.

Se extiende el monitoreo al pozo activo (nivel) y se agrega sensores de posición en las válvulas de derivación para confirmar su estado (abierta/cerrada). Opcionalmente se instala un sensor de caudal en la línea principal para detectar fugas o roturas.

**Resultado esperado:** Se conoce el nivel del pozo antes de que quede sin agua; se detecta si una válvula quedó en posición incorrecta; se mide el caudal real de entrada y salida.

**Inversión aproximada:** Ver Anexo B.


---


### Etapa 3: Dejar de ir a prender — Automatización on/off

**Duración estimada:** 2 a 3 semanas.

Se incorpora un relé industrial para el comando de la bomba y se programa la lógica de control en el ESP32: arranque automático cuando una cisterna desciende por debajo del 30 %, parada al alcanzar el 90 %, e inhibición de arranque si el pozo está por debajo de su nivel mínimo.

**Resultado esperado:** El operario deja de desplazarse para encender y apagar la bomba; se protege la bomba contra marcha en seco; se pueden programar horarios de preferencia (por ejemplo, no arrancar en horario nocturno si no es urgente).

**Inversión aproximada:** Ver Anexo C.


---


### Etapa 4: Bomba inteligente — VFD y presión constante

**Duración estimada:** 1 mes.

Se instala un variador de frecuencia (VFD) acorde al motor existente (~3 HP) y un sensor de presión en la red de distribución. El ESP32 pasa a enviar una referencia de velocidad al VFD mediante señal 0-10 V o Modbus, cerrando un lazo PID que mantiene la presión estable independientemente de cuántos grifos estén abiertos.

**Resultado esperado:** Ahorro energético del 30 % al 50 % en la factura de luz; presión estable en toda la red; arranques suaves que duplican la vida útil del motor; eliminación definitiva de golpes de ariete.

**Inversión aproximada:** Ver Anexo D.


---


## 5. Anexo transversal: Independización de infraestructura digital

Actualmente, la visualización de datos (Grafana) y la base de datos histórica (InfluxDB) están hospedadas en servidores de la Asociación Civil AlterMundi. Este modelo permitió validar la tecnología sin inversión inicial en servidores, pero condiciona la disponibilidad del sistema a la conectividad con esa infraestructura externa y genera dependencia operativa.

Una vez validado el monitoreo de la Etapa 1 (o en cualquier momento posterior que el Consejo Directivo defina), se propone la migración a un servidor local propio de la cooperativa. Este dispositivo —un minicomputador ARM64 del tipo Raspberry Pi u Orange Pi— ejecuta el mismo stack de código abierto (Mosquitto, InfluxDB 1.8, Grafana) dentro de la red local del predio.

**Ventajas de la independización:**


- **Soberanía de datos:** los registros históricos permanecen físicamente en la cooperativa.
- **Disponibilidad:** el sistema funciona aunque fallen los enlaces a internet o servicios externos.
- **Latencia:** las alertas y comandos locales responden en milisegundos, sin depender de la latencia de red hacia AlterMundi.
- **Escalabilidad autónoma:** agregar nuevos sensores o modificar alarmas no requiere coordinación externa.

**Alcance técnico:** la migración consiste en replicar los contenedores Docker ya probados, reconfigurar los ESP32 para que publiquen en el broker local y transferir los dashboards históricos. No se pierde información si se realiza un backup previo.

**Inversión aproximada:** Ver Anexo E.


---


## 6. Criterios de pasaje entre etapas

El Consejo Directivo aprobará etapa por etapa. Los criterios mínimos para avanzar son:


| De | A | Criterio de pasaje |
|---|---|---|
| Etapa 1 | Etapa 2 o Independización | Dashboard operativo 30 días con datos reales de ambas cisternas y al menos una alerta útil disparada. |
| Etapa 2 | Etapa 3 | Todos los pozos monitoreados y al menos una válvula con sensor de posición funcional. |
| Etapa 3 | Etapa 4 | Automatización on/off operando 15 días sin intervención manual y con registro de horas liberadas. |
| Etapa 4 | — | Medición de consumo eléctrico antes/después con ahorro confirmado ≥ 20 %. |


---


## 7. Beneficios esperados para la cooperativa


1. **Riesgo bajo:** se inicia con una inversión menor a $280.000. Si no funciona, la pérdida es asumible sin endeudamiento.
2. **Resultados visibles:** en la primera etapa ya se ven los niveles en el celular.
3. **Independencia progresiva:** la cooperativa puede decidir cuándo migrar la infraestructura a su propio equipo.
4. **Datos para gestión:** con registros de consumo, niveles y eventos se pueden planificar obras, negociar tarifas eléctricas o postular a subsidios con información objetiva.
5. **Escalable:** el mismo sistema sirve para futuras ampliaciones de red o nuevas captaciones sin desechar lo anterior.


---


## 8. Próximo paso

Convocar al Consejo Directivo para aprobar la **Etapa 1 (MVP de monitoreo)**. Una vez aprobada, el equipo técnico procederá al relevamiento exacto de las cisternas (altura, acceso, condiciones ambientales) y a la adquisición de sensores. En un plazo de 30 días la cooperativa contará con el primer panel operativo y podrá decidir si avanza hacia la Etapa 2 o prioriza la independización de infraestructura.


---


## Referencias de anexos


- **Anexo A:** Materiales y costos — Etapa 1 (MVP Monitoreo)
- **Anexo B:** Materiales y costos — Etapa 2 (Pozos y válvulas)
- **Anexo C:** Materiales y costos — Etapa 3 (Automatización on/off)
- **Anexo D:** Materiales y costos — Etapa 4 (VFD + presión constante)
- **Anexo E:** Materiales y costos — Independización de infraestructura
- **Anexo F:** Resumen de inversión acumulada y roadmap


---

**Firma Autorizada:** *______***_  Aclaración: ***______  Cargo: _______*  **Lugar y Fecha:** *______*__


# Anexo A: Etapa 1 (MVP Monitoreo)

**Etapa:** Saber qué hay — Sensado de niveles en cisternas  **Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  **Validez:** 15 días desde la fecha de emisión


---


## Descripción técnica resumida

Dos sensores ultrasónicos de nivel se instalan en tapas de visita de las cisternas. Un microcontrolador ESP32 con adaptador WiFi recibe las señales, las publica hacia el servidor de AlterMundi (o local, si ya se ejecutó la independización) y un panel Grafana muestra nivel histórico y alertas.

Se requiere resolver por parte de la cooperativa el suministro eléctrico de este sistema.


---


## Totales


| Concepto | Importe ARS |
|---|---|
| Equipamiento | $182.000–$280.000 |
| Asesoría, instalación y puesta en marcha | $220.000.- |
| **Total Etapa 1** | **$402.000–$500.000** |


# Anexo B: Etapa 2 (Pozos y válvulas)

**Etapa:** Ver todo — Extensión del monitoreo a pozos, válvulas y caudal  **Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  **Dependencia:** Requiere Etapa 1 operativa y validada.


---


## Descripción técnica resumida

Se agrega un sensor de nivel en el pozo activo (sumergible o ultrasónico según profundidad) y sensores de posición (fines de carrera magnéticos o inductivos) en las válvulas de derivación entre cisternas. Opcionalmente se instala un sensor de caudal tipo turbina en la línea de impulsión para detectar fugas y cuantificar el bombeo real.


---


## Totales


| Concepto | Importe ARS |
|---|---|
| Equipamiento | $336.000–$560.000 |
| Asesoría, instalación y puesta en marcha | $480.000.- |
| **Total Etapa 2** | **$816.000–$1.040.000** |


# Anexo C: Etapa 3 (Automatización on/off)

**Etapa:** Dejar de ir a prender — Comando automático de bomba por niveles  **Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  **Dependencia:** Requiere Etapa 1 operativa; recomendada tras Etapa 2.


---


## Descripción técnica resumida

Se intercala un relé industrial en el circuito de comando de la bomba. El microcontrolador decide el encendido/apagado según reglas programadas: cisterna baja → arranca; cisterna alta → para; pozo bajo → inhibe arranque. Se agrega un sensor de corriente no invasivo para confirmar que la bomba efectivamente respondió al comando y detectar fallas (por ejemplo, cavitación o disparo térmico).


---


## Totales


| Concepto | Importe ARS |
|---|---|
| Equipamiento | $119.000–$199.000 |
| Asesoría, instalación y puesta en marcha | $200.000.- |
| **Total Etapa 3** | **$319.000–$399.000** |


# Anexo D: Etapa 4 (VFD + presión constante)

**Etapa:** Bomba inteligente — Variador de frecuencia y control Proporcional Integrativo Derivativo de presión  **Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  **Dependencia:** Requiere Etapa 3 operativa; motor caracterizado (monofásico o trifásico, potencia exacta).


---


## Descripción técnica resumida

Se reemplaza el arranque directo por un variador de frecuencia (VFD) acorde al motor existente (~3 HP). El microcontrolador cierra un lazo PID leyendo un sensor de presión en la red de distribución y enviando una referencia de velocidad al variador. La bomba acelera o frena según la demanda, manteniendo la presión estable.


---


## Totales


| Concepto | Importe ARS |
|---|---|
| Materiales | $518.000–$868.000 |
| Mano de obra e instalación | $630.000.- |
| **Total Etapa 4** | **$1.148.000–$1.498.000** |


---


## Notas para el presupuesto


- El precio del VFD varía significativamente según marca (Chint/Invt/ATV vs. ABB/Siemens).
- Verificar exactamente si el motor es monofásico o trifásico y su corriente nominal antes de comprar.


# Anexo E: Independización de infraestructura digital

**Anexo transversal:** Migración de Grafana e InfluxDB desde AlterMundi a servidor local propio  **Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD  **Dependencia:** Recomendada tras validar la Etapa 1; puede ejecutarse en cualquier momento.


---


## Descripción técnica resumida

Se adquiere un minicomputador para ejecutar localmente el stack de telemetría: base de datos de series temporales y panel de visualización. Los datos históricos se respaldan y transfieren; los microcontroladores se reconfiguran para publicar en el servidor local.


---


## Totales


| Concepto | Importe ARS |
|---|---|
| Equipamiento | $175.000–$304.000 |
| Implementación y migración | $110.000.- |
| **Total Independización** | **$285.000–$414.000** |


# Anexo F: Resumen de inversión acumulada y roadmap

**Cotización:** Mayo 2026 — Tasa estimada: AR$ 1.400 / USD


---


## Tabla resumen por etapa


| Etapa | Concepto | Inversión ARS | Acumulado ARS | Plazo estimado |
|---|---|---|---|---|
| 1 | MVP Monitoreo (2 cisternas) | $402.000–$500.000 | $402.000–$500.000 | 15–30 días |
| 2 | Pozos, válvulas y caudal | $816.000–$1.040.000 | $1.218.000–$1.540.000 | 1 mes |
| 3 | Automatización on/off | $319.000–$399.000 | $1.537.000–$1.939.000 | 2–3 semanas |
| 4 | VFD + presión constante | $1.148.000–$1.498.000 | $2.685.000–$3.437.000 | 1 mes |
| **T** | **Independización infraestructura** | **$285.000–$414.000** | *acumulado parcial* | — |


---


## Inversión mínima y máxima según escenario


| Escenario | Etapas incluidas | Inversión total ARS |
|---|---|---|
| Mínimo viable (Etapa 1 + independización) | 1 + T | $687.000–$914.000 |
| Completo con automatización (1 + 2 + 3 + T) | 1, 2, 3, T | $1.822.000–$2.353.000 |
| Completo optimizado (todas las etapas) | 1, 2, 3, 4, T | $2.970.000–$3.851.000 |


---


## Roadmap visual


```
Mes 1-2    Etapa 1: MVP Monitoreo → validación operativa
Mes 3-4    Independización (paralelo o post Etapa 1)
Mes 4-5    Etapa 2: Pozos y válvulas
Mes 6-7    Etapa 3: Automatización on/off
Mes 8-12   Etapa 4: VFD (postulación a fondos con datos de 6 meses)
Año 2      Escalamiento a otras captaciones / replicación
```


---


## Fuentes de financiamiento sugeridas

Con datos reales de consumo y ahorro (generados desde la Etapa 1), la cooperativa puede postular a:


- **GEF Small Grants Programme (UNDP):** hasta USD 50.000 para eficiencia energética y cambio climático.
- **FONCyT PCE:** proyectos concertados academia–ONG (requiere carta de institución académica).
- **Programas provinciales/municipales:** subsidios a cooperativas de servicios públicos.
- **Fondos rotatorios de la cooperativa:** amortización mensual con el ahorro energético validado.


---


## Nota sobre variación de costos


- Los rangos reflejan incertidumbre de importación, logística y disponibilidad de modelos específicos.
- Se recomienda recotizar los ítems de mayor valor (VFD, sensores de caudal, SBC) antes de cada aprobación de etapa.
- La tasa de AR$ 1.400 / USD debe revisarse si el plazo entre aprobación y compra supera los 90 días.
