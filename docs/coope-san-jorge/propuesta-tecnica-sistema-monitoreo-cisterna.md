# Propuesta Técnica: Sistema de Monitoreo Remoto de Niveles de Cisterna

**Destinatario:** Consejo Directivo  
**Cooperativa de Provisión de Obras, Agua Potable y Otros Servicios Villa Parque San Jorge Limitada**  
**Localidad:** Molinari, Municipio de Cosquín, Provincia de Córdoba  
**Contacto / Teléfono:** _______________________________  
**Expediente / Resolución N°:** ________________________  
**Fecha:** Mayo 2026  
**Elaboración:** Equipo de desarrollo ONG 501

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

Se instalan sensores ultrasónicos de nivel en las dos cisternas, conectados a un microcontrolador ESP32 con conectividad WiFi. Los datos se visualizan en un panel web (Grafana) con alarmas por Telegram o WhatsApp.

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

<br><br><br>

**Firma Autorizada:** _________________________________________  
**Aclaración:** _______________________________________________  
**Cargo:** ____________________________________________________  
**Lugar y Fecha:** ____________________________________________  

