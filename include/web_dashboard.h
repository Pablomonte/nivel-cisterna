#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Cisterna Monitor</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }

  body {
    font-family: "Segoe UI", system-ui, -apple-system, sans-serif;
    background:
      radial-gradient(circle at top, rgba(56, 189, 248, 0.12), transparent 35%),
      linear-gradient(180deg, #020617 0%, #0f172a 100%);
    color: #e2e8f0;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 1rem;
  }

  h1 {
    font-size: 1.35rem;
    font-weight: 700;
    color: #7dd3fc;
    margin-bottom: 1rem;
    text-align: center;
  }

  .card {
    background: rgba(15, 23, 42, 0.86);
    border-radius: 14px;
    padding: 1rem;
    margin-bottom: 1rem;
    width: 100%;
    max-width: 440px;
    border: 1px solid rgba(148, 163, 184, 0.2);
    box-shadow: 0 18px 36px rgba(2, 6, 23, 0.35);
    backdrop-filter: blur(8px);
  }

  .tank-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 0.5rem 0 1rem;
  }

  .tank {
    width: 122px;
    height: 204px;
    border: 3px solid #475569;
    border-radius: 0 0 14px 14px;
    border-top: none;
    position: relative;
    overflow: hidden;
    background: rgba(2, 6, 23, 0.9);
  }

  .tank-top {
    width: 138px;
    height: 8px;
    background: #475569;
    border-radius: 4px 4px 0 0;
  }

  .water {
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    background: linear-gradient(180deg, #38bdf8 0%, #0369a1 100%);
    transition: height 0.8s ease;
  }

  .water.low { background: linear-gradient(180deg, #fb7185 0%, #dc2626 100%); }
  .water.mid { background: linear-gradient(180deg, #38bdf8 0%, #0369a1 100%); }
  .water.high { background: linear-gradient(180deg, #6ee7b7 0%, #059669 100%); }

  .level-text {
    font-size: 2.4rem;
    font-weight: 700;
    margin-top: 0.75rem;
    color: #f8fafc;
  }

  .level-text span {
    font-size: 1.1rem;
    color: #94a3b8;
  }

  .stats {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 0.8rem;
  }

  .stat {
    background: rgba(2, 6, 23, 0.9);
    border-radius: 10px;
    padding: 0.9rem;
    text-align: center;
  }

  .stat-label {
    font-size: 0.72rem;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: #64748b;
    margin-bottom: 0.35rem;
  }

  .stat-value {
    font-size: 1.25rem;
    font-weight: 600;
    color: #e2e8f0;
  }

  .stat-unit {
    font-size: 0.8rem;
    color: #94a3b8;
  }

  .status-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.65rem 0;
    border-top: 1px solid rgba(148, 163, 184, 0.12);
  }

  .status-row:first-child {
    border-top: none;
    padding-top: 0;
  }

  .status-label {
    font-size: 0.85rem;
    color: #94a3b8;
  }

  .status-value {
    font-size: 0.95rem;
    font-weight: 600;
    color: #e2e8f0;
    display: flex;
    align-items: center;
    gap: 0.45rem;
  }

  .dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: #475569;
  }

  .dot.ok { background: #22c55e; box-shadow: 0 0 8px rgba(34, 197, 94, 0.4); }
  .dot.warn { background: #f59e0b; box-shadow: 0 0 8px rgba(245, 158, 11, 0.4); }
  .dot.err { background: #ef4444; box-shadow: 0 0 8px rgba(239, 68, 68, 0.4); }

  .device-name {
    color: #94a3b8;
    font-size: 0.8rem;
    text-align: center;
    margin-top: 0.25rem;
  }

  .footer {
    margin-top: auto;
    padding-top: 0.75rem;
    color: #475569;
    font-size: 0.72rem;
    text-align: center;
  }

  .actions {
    display: flex;
    justify-content: center;
    margin-bottom: 1rem;
    width: 100%;
    max-width: 440px;
  }

  .button-link {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 100%;
    padding: 0.9rem 1rem;
    border-radius: 12px;
    border: 1px solid rgba(56, 189, 248, 0.25);
    background: linear-gradient(180deg, rgba(14, 165, 233, 0.18), rgba(2, 132, 199, 0.1));
    color: #e0f2fe;
    text-decoration: none;
    font-weight: 600;
  }
</style>
</head>
<body>

<h1>Cisterna Monitor</h1>

<div class="card">
  <div class="tank-container">
    <div class="tank-top"></div>
    <div class="tank">
      <div class="water mid" id="water" style="height: 0%"></div>
    </div>
    <div class="level-text"><span id="levelVal">--</span><span>%</span></div>
  </div>
</div>

<div class="card">
  <div class="stats" style="grid-template-columns: 1fr;">
    <div class="stat">
      <div class="stat-label">Distancia</div>
      <div class="stat-value"><span id="distVal">--</span> <span class="stat-unit">cm</span></div>
    </div>
  </div>
</div>

<div class="card">
  <div class="status-row">
    <div class="status-label">Sensor</div>
    <div class="status-value">
      <span class="dot" id="sensorDot"></span>
      <span id="sensorStatus">--</span>
    </div>
  </div>
  <div class="status-row">
    <div class="status-label">WiFi</div>
    <div class="status-value" id="wifiState">--</div>
  </div>
  <div class="status-row">
    <div class="status-label">Energia</div>
    <div class="status-value" id="powerMode">--</div>
  </div>
</div>

<div class="actions">
  <a class="button-link" href="/wifi">Configuracion</a>
</div>

<div class="device-name" id="deviceName"></div>
<div class="footer">nivel-cisterna <span id="fwVersion"></span></div>

<script>
function setDot(id, state) {
  const dot = document.getElementById(id);
  dot.className = 'dot ' + state;
}

function updateData() {
  fetch('/api/status')
    .then(r => r.json())
    .then(d => {
      const level = d.level ?? -1;
      const water = document.getElementById('water');

      if (level >= 0) {
        water.style.height = level + '%';
        water.className = 'water ' + (level < 20 ? 'low' : level > 80 ? 'high' : 'mid');
        document.getElementById('levelVal').textContent = level.toFixed(1);
      } else {
        water.style.height = '0%';
        water.className = 'water low';
        document.getElementById('levelVal').textContent = '--';
      }

      document.getElementById('distVal').textContent =
        d.distance >= 0 ? d.distance.toFixed(1) : '--';

      document.getElementById('sensorStatus').textContent =
        d.sensor_ok ? 'OK' : 'ERROR';
      setDot('sensorDot', d.sensor_ok ? 'ok' : 'err');

      document.getElementById('wifiState').textContent =
        d.wifi_connected ? (d.ip || 'conectado') : (d.wifi_mode || 'sin enlace');

      document.getElementById('deviceName').textContent = d.device || '';
      document.getElementById('fwVersion').textContent = d.version || '';

      const pm = d.power_mode === 'battery'
        ? 'Bateria (ciclo #' + (d.boot_count ?? '--') + ')'
        : 'Normal';
      document.getElementById('powerMode').textContent = pm;
    })
    .catch(() => {
      setDot('sensorDot', 'err');
    });
}

updateData();
setInterval(updateData, 5000);
</script>

</body>
</html>
)rawliteral";

const char WIFI_ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Configuracion del dispositivo</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }

  :root {
    --bg-top: #04111d;
    --bg-bottom: #0b1f33;
    --panel: rgba(7, 23, 40, 0.88);
    --panel-border: rgba(148, 163, 184, 0.22);
    --text: #e2e8f0;
    --muted: #94a3b8;
    --accent: #38bdf8;
    --accent-strong: #0ea5e9;
    --ok: #22c55e;
    --warn: #f59e0b;
    --err: #ef4444;
  }

  body {
    font-family: "Segoe UI", system-ui, -apple-system, sans-serif;
    background:
      radial-gradient(circle at top left, rgba(56, 189, 248, 0.18), transparent 35%),
      linear-gradient(180deg, var(--bg-top) 0%, var(--bg-bottom) 100%);
    color: var(--text);
    min-height: 100vh;
    padding: 1rem;
  }

  .shell {
    width: 100%;
    max-width: 720px;
    margin: 0 auto;
  }

  .header {
    margin-bottom: 1rem;
  }

  h1 {
    font-size: 1.6rem;
    color: #bae6fd;
    margin-bottom: 0.35rem;
  }

  .subtle {
    color: var(--muted);
    font-size: 0.92rem;
    line-height: 1.45;
  }

  .card {
    background: var(--panel);
    border: 1px solid var(--panel-border);
    border-radius: 16px;
    padding: 1rem;
    margin-bottom: 1rem;
    box-shadow: 0 18px 36px rgba(2, 6, 23, 0.28);
    backdrop-filter: blur(8px);
  }

  .card h2 {
    font-size: 1rem;
    margin-bottom: 0.85rem;
    color: #e0f2fe;
  }

  .status-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 0.8rem;
  }

  .status-item {
    background: rgba(2, 6, 23, 0.45);
    border-radius: 12px;
    padding: 0.85rem;
    min-height: 72px;
  }

  .status-label {
    color: var(--muted);
    font-size: 0.72rem;
    letter-spacing: 0.05em;
    text-transform: uppercase;
    margin-bottom: 0.35rem;
  }

  .status-value {
    font-size: 1rem;
    font-weight: 600;
    word-break: break-word;
  }

  form {
    display: grid;
    gap: 0.9rem;
  }

  label {
    display: block;
    margin-bottom: 0.42rem;
    color: #cbd5e1;
    font-size: 0.9rem;
    font-weight: 600;
  }

  input[type="text"],
  input[type="password"],
  input[type="number"],
  select {
    width: 100%;
    border: 1px solid rgba(148, 163, 184, 0.24);
    border-radius: 12px;
    background: rgba(2, 6, 23, 0.72);
    color: var(--text);
    padding: 0.9rem 0.95rem;
    font-size: 0.96rem;
    outline: none;
    appearance: none;
  }

  input[type="text"]:focus,
  input[type="password"]:focus,
  input[type="number"]:focus,
  select:focus {
    border-color: rgba(56, 189, 248, 0.75);
    box-shadow: 0 0 0 3px rgba(56, 189, 248, 0.15);
  }

  .hint {
    color: var(--muted);
    font-size: 0.82rem;
    line-height: 1.4;
    margin-top: 0.35rem;
  }

  .pwd-wrap {
    position: relative;
  }

  .pwd-wrap input {
    padding-right: 3rem;
  }

  .pwd-toggle {
    position: absolute;
    right: 0.4rem;
    top: 50%;
    transform: translateY(-50%);
    background: transparent;
    border: 1px solid rgba(148, 163, 184, 0.18);
    color: var(--muted);
    font-size: 0.85rem;
    font-weight: 500;
    padding: 0.4rem 0.6rem;
    border-radius: 10px;
    cursor: pointer;
    line-height: 1;
  }

  .pwd-toggle:hover {
    color: var(--text);
    border-color: rgba(56, 189, 248, 0.4);
  }

  .hidden {
    display: none !important;
  }

  .inline-option {
    display: flex;
    align-items: center;
    gap: 0.55rem;
    color: #cbd5e1;
    font-size: 0.88rem;
  }

  .actions {
    display: flex;
    flex-wrap: wrap;
    gap: 0.75rem;
  }

  button,
  .nav-link {
    border: none;
    border-radius: 12px;
    padding: 0.9rem 1rem;
    font-size: 0.95rem;
    font-weight: 700;
    cursor: pointer;
    text-decoration: none;
    display: inline-flex;
    align-items: center;
    justify-content: center;
  }

  .primary {
    background: linear-gradient(180deg, var(--accent) 0%, var(--accent-strong) 100%);
    color: #082f49;
  }

  .secondary,
  .nav-link {
    background: rgba(15, 23, 42, 0.88);
    color: #dbeafe;
    border: 1px solid rgba(148, 163, 184, 0.2);
  }

  .message {
    display: none;
    margin-top: 0.5rem;
    padding: 0.8rem 0.9rem;
    border-radius: 12px;
    font-size: 0.9rem;
    line-height: 1.4;
  }

  .message.show { display: block; }
  .message.ok { background: rgba(34, 197, 94, 0.12); color: #bbf7d0; border: 1px solid rgba(34, 197, 94, 0.25); }
  .message.err { background: rgba(239, 68, 68, 0.12); color: #fecaca; border: 1px solid rgba(239, 68, 68, 0.25); }
  .message.warn { background: rgba(245, 158, 11, 0.12); color: #fde68a; border: 1px solid rgba(245, 158, 11, 0.25); }

  .network-list {
    display: grid;
    gap: 0.7rem;
  }

  .network-item {
    width: 100%;
    text-align: left;
    background: rgba(2, 6, 23, 0.62);
    border: 1px solid rgba(148, 163, 184, 0.16);
    color: var(--text);
  }

  .network-item.selected {
    border-color: rgba(56, 189, 248, 0.7);
    box-shadow: 0 0 0 2px rgba(56, 189, 248, 0.12);
  }

  .network-item strong {
    display: block;
    font-size: 1rem;
    margin-bottom: 0.25rem;
  }

  .network-meta {
    display: flex;
    flex-wrap: wrap;
    gap: 0.6rem;
    color: var(--muted);
    font-size: 0.82rem;
  }

  .badge {
    display: inline-flex;
    align-items: center;
    border-radius: 999px;
    padding: 0.18rem 0.55rem;
    font-size: 0.76rem;
    font-weight: 700;
  }

  .badge.ok { background: rgba(34, 197, 94, 0.14); color: #bbf7d0; }
  .badge.warn { background: rgba(245, 158, 11, 0.14); color: #fde68a; }

  @media (max-width: 640px) {
    .status-grid {
      grid-template-columns: 1fr;
    }

    .actions > * {
      width: 100%;
    }
  }
</style>
</head>
<body>
<div class="shell">
  <div class="header">
    <h1>Configuracion del dispositivo</h1>
    <div class="subtle">Estado en vivo, conexion WiFi, calibracion del sensor, modo de energia y acceso admin del equipo.</div>
  </div>

  <div class="card">
    <h2>Estado actual</h2>
    <div class="status-grid">
      <div class="status-item">
        <div class="status-label">Dispositivo / AP</div>
        <div class="status-value" id="statusDevice">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Modo</div>
        <div class="status-value" id="statusMode">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Red conectada</div>
        <div class="status-value" id="statusConnected">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">IP activa</div>
        <div class="status-value" id="statusIp">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">SSID configurado</div>
        <div class="status-value" id="statusConfigured">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Password WiFi guardada</div>
        <div class="status-value" id="statusPassword">--</div>
      </div>
    </div>
  </div>

  <div class="card">
    <h2>Red WiFi</h2>
    <div class="subtle" style="margin-bottom: 0.9rem;">Configura el nombre del dispositivo y la red a la que se conecta. El nombre tambien se usa como SSID del AP del equipo.</div>
    <form id="wifiForm">
      <div>
        <label for="deviceName">Nombre del dispositivo / SSID del AP</label>
        <input id="deviceName" name="deviceName" type="text" maxlength="31" placeholder="cisterna-01">
        <div class="hint">Maximo 31 caracteres. Se usa en el dashboard y como SSID del AP del equipo.</div>
      </div>

      <div>
        <label for="wifiSsid">Red WiFi a conectar</label>
        <input id="wifiSsid" name="wifiSsid" type="text" maxlength="32" placeholder="MiWiFi">
        <div class="hint">Puedes escribirlo manualmente o tocar una red del escaneo. Si lo dejas vacio, el equipo queda solo en modo AP.</div>
      </div>

      <div>
        <label for="wifiPass">Contraseña WiFi</label>
        <div class="pwd-wrap">
          <input id="wifiPass" name="wifiPass" type="password" placeholder="Dejar vacio para conservar la actual" minlength="8" maxlength="63">
          <button type="button" class="pwd-toggle" data-target="wifiPass" aria-label="Mostrar contraseña">Ver</button>
        </div>
        <div class="hint">8-63 caracteres (WPA2). Dejar vacio conserva la actual. Las redes abiertas se detectan automaticamente del escaneo.</div>
      </div>

      <div class="actions">
        <button class="primary" type="submit">Guardar y reiniciar</button>
        <button class="secondary" type="button" id="scanButton">Escanear redes</button>
        <a class="nav-link" href="/">Volver al dashboard</a>
      </div>
    </form>
    <div class="message" id="formMessage"></div>

    <div class="subtle" style="margin: 1rem 0 0.6rem;">Redes detectadas</div>
    <div class="network-list" id="networkList">
      <div class="subtle">Todavia no se escanearon redes.</div>
    </div>
    <div class="message" id="scanMessage"></div>
  </div>

  <div class="card">
    <h2>Calibracion del sensor</h2>
    <div class="subtle" style="margin-bottom: 0.9rem;">
      Lectura actual: <strong id="liveDistance">--</strong> cm
      (nivel <strong id="liveLevel">--</strong>%).
      Ajusta los topes y el offset; los cambios se aplican sin reiniciar.
    </div>
    <form id="sensorCalForm">
      <div>
        <label for="emptyDistance">Distancia "tanque vacio" (cm)</label>
        <input id="emptyDistance" name="emptyDistance" type="number" step="0.1" min="1" max="400" required>
        <div class="hint">Distancia que mide el sensor cuando el tanque esta vacio (0%).</div>
      </div>
      <div>
        <label for="fullDistance">Distancia "tanque lleno" (cm)</label>
        <input id="fullDistance" name="fullDistance" type="number" step="0.1" min="0" max="400" required>
        <div class="hint">Distancia que mide el sensor cuando el tanque esta lleno (100%). Debe ser menor que la de vacio.</div>
      </div>
      <div>
        <label for="sensorOffset">Offset del sensor (cm)</label>
        <input id="sensorOffset" name="sensorOffset" type="number" step="0.1" min="-50" max="50" required>
        <div class="hint">Correccion fija sumada a cada lectura. Usalo para compensar montaje del sensor (-50 a +50).</div>
      </div>
      <div class="actions">
        <button class="primary" type="submit">Guardar calibracion</button>
      </div>
    </form>
    <div class="message" id="sensorCalMessage"></div>
  </div>

  <div class="card">
    <h2>Modo de energia</h2>
    <div class="subtle" style="margin-bottom: 0.9rem;">
      Controla como gestiona la energia el dispositivo. El cambio se aplica al reiniciar.
    </div>
    <form id="powerForm">
      <div>
        <label for="powerMode">Modo</label>
        <select id="powerMode" name="powerMode">
          <option value="normal">Normal — siempre encendido (corriente electrica)</option>
          <option value="battery">Bateria — ciclos de deep sleep (~10µA dormido)</option>
        </select>
        <div class="hint" id="modeHintNormal">
          El dispositivo permanece activo, el servidor web esta disponible en todo momento y el WiFi se mantiene conectado. Reconexion automatica cada cierta cantidad de segundos si pierde la red.
        </div>
        <div class="hint" id="modeHintBattery" style="display:none;">
          Al despertar: lee el sensor, conecta WiFi (con timeout), envia datos a Grafana, luego abre el servidor web durante la ventana configurada. Despues duerme profundamente hasta el proximo ciclo. Solo consume corriente al estar activo.
        </div>
      </div>

      <div id="batteryFields" style="display:none; gap:0.9rem;">
        <div>
          <label for="sleepInterval">Intervalo de sleep (segundos)</label>
          <input id="sleepInterval" name="sleepInterval" type="number" min="30" max="86400" step="1">
          <div class="hint">Tiempo que duerme entre ciclos. 300 = 5 min. Minimo 30, maximo 86400 (24 h).</div>
        </div>
        <div>
          <label for="webWindow">Ventana web tras despertar (segundos)</label>
          <input id="webWindow" name="webWindow" type="number" min="10" max="3600" step="1">
          <div class="hint">Cuantos segundos permanece activo el servidor web luego de enviar datos. Minimo 10.</div>
        </div>
        <div>
          <label for="wifiTimeout">Timeout de conexion WiFi (ms)</label>
          <input id="wifiTimeout" name="wifiTimeout" type="number" min="5000" max="60000" step="500">
          <div class="hint">Tiempo maximo de espera para conectar a la red antes de continuar sin WiFi. 5000–60000 ms.</div>
        </div>
      </div>

      <div id="normalFields">
        <div>
          <label for="wifiRetry">Intervalo de reintento WiFi (segundos)</label>
          <input id="wifiRetry" name="wifiRetry" type="number" min="10" max="3600" step="1">
          <div class="hint">Cada cuantos segundos reintenta conectar a la red si se desconecta. 30 = recuperacion rapida.</div>
        </div>
      </div>

      <div class="actions">
        <button class="primary" type="submit">Guardar y reiniciar</button>
      </div>
    </form>
    <div class="message" id="powerMessage"></div>
  </div>

  <div class="card">
    <h2>Acceso admin</h2>
    <div class="subtle" style="margin-bottom: 0.9rem;">Cambia la contraseña usada por el dashboard y la API. Si no hay una configurada, se usa la derivada del MAC (cisterna-XXXXXX).</div>
    <form id="adminPwdForm">
      <div id="currentPwdRow">
        <label for="currentAdminPass">Contraseña actual</label>
        <div class="pwd-wrap">
          <input id="currentAdminPass" type="password" autocomplete="current-password" minlength="8" maxlength="64">
          <button type="button" class="pwd-toggle" data-target="currentAdminPass" aria-label="Mostrar contraseña">Ver</button>
        </div>
        <div class="hint">Si nunca configuraste una, dejala vacia (se acepta solo en el primer cambio).</div>
      </div>
      <div>
        <label for="newAdminPass">Nueva contraseña</label>
        <div class="pwd-wrap">
          <input id="newAdminPass" type="password" autocomplete="new-password" minlength="8" maxlength="64" required>
          <button type="button" class="pwd-toggle" data-target="newAdminPass" aria-label="Mostrar contraseña">Ver</button>
        </div>
        <div class="hint">8 a 64 caracteres.</div>
      </div>
      <div>
        <label for="confirmAdminPass">Confirmar nueva</label>
        <div class="pwd-wrap">
          <input id="confirmAdminPass" type="password" autocomplete="new-password" minlength="8" maxlength="64" required>
          <button type="button" class="pwd-toggle" data-target="confirmAdminPass" aria-label="Mostrar contraseña">Ver</button>
        </div>
      </div>
      <div class="actions">
        <button class="primary" type="submit">Cambiar contraseña</button>
      </div>
    </form>
    <div class="message" id="adminPwdMessage"></div>
  </div>
</div>

<script>
const elements = {
  deviceName: document.getElementById('deviceName'),
  wifiSsid: document.getElementById('wifiSsid'),
  wifiPass: document.getElementById('wifiPass'),
  scanButton: document.getElementById('scanButton'),
  networkList: document.getElementById('networkList'),
  formMessage: document.getElementById('formMessage'),
  scanMessage: document.getElementById('scanMessage'),
  statusDevice: document.getElementById('statusDevice'),
  statusMode: document.getElementById('statusMode'),
  statusConnected: document.getElementById('statusConnected'),
  statusIp: document.getElementById('statusIp'),
  statusConfigured: document.getElementById('statusConfigured'),
  statusPassword: document.getElementById('statusPassword'),
  adminPwdForm: document.getElementById('adminPwdForm'),
  adminPwdMessage: document.getElementById('adminPwdMessage'),
  currentPwdRow: document.getElementById('currentPwdRow'),
  currentAdminPass: document.getElementById('currentAdminPass'),
  newAdminPass: document.getElementById('newAdminPass'),
  confirmAdminPass: document.getElementById('confirmAdminPass'),
  sensorCalForm: document.getElementById('sensorCalForm'),
  sensorCalMessage: document.getElementById('sensorCalMessage'),
  emptyDistance: document.getElementById('emptyDistance'),
  fullDistance: document.getElementById('fullDistance'),
  sensorOffset: document.getElementById('sensorOffset'),
  liveDistance: document.getElementById('liveDistance'),
  liveLevel: document.getElementById('liveLevel')
};

document.querySelectorAll('.pwd-toggle').forEach((btn) => {
  btn.addEventListener('click', () => {
    const targetId = btn.dataset.target;
    const input = document.getElementById(targetId);
    if (!input) return;
    if (input.type === 'password') {
      input.type = 'text';
      btn.textContent = 'Ocultar';
    } else {
      input.type = 'password';
      btn.textContent = 'Ver';
    }
  });
});

let selectedSsid = '';
let scanPollToken = 0;
let scanStartedAt = 0;
const openSsids = new Set();

function showMessage(target, tone, text) {
  target.className = 'message show ' + tone;
  target.textContent = text;
}

function clearMessage(target) {
  target.className = 'message';
  target.textContent = '';
}

function normalizeNetworks(networks) {
  const strongestBySsid = new Map();
  (networks || []).forEach((network) => {
    const ssid = network.ssid || '';
    if (!ssid) return;
    const current = strongestBySsid.get(ssid);
    if (!current || (network.rssi || -999) > (current.rssi || -999)) {
      strongestBySsid.set(ssid, network);
    }
  });

  return Array.from(strongestBySsid.values()).sort((a, b) => (b.rssi || -999) - (a.rssi || -999));
}

function renderNetworks(networks) {
  const list = normalizeNetworks(networks);

  openSsids.clear();
  list.forEach((network) => {
    if (network.ssid && !network.secure) {
      openSsids.add(network.ssid);
    }
  });

  if (!list.length) {
    elements.networkList.innerHTML = '<div class="subtle">No se encontraron redes visibles.</div>';
    return;
  }

  elements.networkList.innerHTML = list.map((network) => {
    const ssid = network.ssid || '';
    const safeSsid = ssid
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
    const selected = ssid === (selectedSsid || elements.wifiSsid.value) ? ' selected' : '';
    const badges = [
      network.connected ? '<span class="badge ok">Conectada</span>' : '',
      network.configured ? '<span class="badge warn">Configurada</span>' : '',
      network.secure ? '<span class="badge warn">Segura</span>' : '<span class="badge ok">Abierta</span>'
    ].join('');

    return `
      <button class="network-item${selected}" type="button" data-ssid="${safeSsid}">
        <strong>${safeSsid}</strong>
        <div class="network-meta">
          <span>RSSI ${network.rssi ?? '--'} dBm</span>
          <span>Canal ${network.channel ?? '--'}</span>
          ${badges}
        </div>
      </button>
    `;
  }).join('');

  elements.networkList.querySelectorAll('.network-item').forEach((button) => {
    button.addEventListener('click', () => {
      selectedSsid = button.dataset.ssid || '';
      elements.wifiSsid.value = selectedSsid;
      renderNetworks(list);
    });
  });
}

function fillStatus(settings) {
  const configuredSsid = settings.wifi_ssid || '';
  const connectedSsid = settings.connected_ssid || '';

  elements.statusDevice.textContent = settings.device_name || '--';
  elements.statusMode.textContent = settings.wifi_mode || '--';
  elements.statusConnected.textContent = connectedSsid || 'Sin enlace';
  elements.statusIp.textContent = settings.ip || '--';
  elements.statusConfigured.textContent = configuredSsid || 'Sin configurar';
  elements.statusPassword.textContent = settings.wifi_pass_configured ? 'Si' : 'No';

  elements.deviceName.value = settings.device_name || '';
  elements.wifiSsid.value = configuredSsid;
  selectedSsid = configuredSsid;

  if (settings.admin_password_configured) {
    elements.currentPwdRow.classList.remove('hidden');
    elements.currentAdminPass.required = true;
  } else {
    elements.currentPwdRow.classList.add('hidden');
    elements.currentAdminPass.required = false;
    elements.currentAdminPass.value = '';
  }
}

async function loadSettings() {
  clearMessage(elements.formMessage);

  const response = await fetch('/api/wifi/settings', { cache: 'no-store' });
  if (!response.ok) {
    throw new Error('No se pudo cargar la configuracion WiFi');
  }

  const data = await response.json();
  fillStatus(data);
}

async function scanNetworks() {
  clearMessage(elements.scanMessage);
  showMessage(elements.scanMessage, 'warn', 'Escaneando redes cercanas...');
  const originalScanLabel = elements.scanButton.textContent;
  elements.scanButton.disabled = true;
  elements.scanButton.textContent = 'Escaneando...';
  const currentToken = ++scanPollToken;
  scanStartedAt = Date.now();

  try {
    const startResponse = await fetch('/api/wifi/scan', {
      method: 'POST',
      cache: 'no-store'
    });

    if (!startResponse.ok && startResponse.status !== 202) {
      throw new Error(await startResponse.text() || 'No se pudo iniciar el escaneo WiFi.');
    }

    await pollScanResult(currentToken, 0);
  } catch (error) {
    showMessage(elements.scanMessage, 'err', error.message || 'No se pudo escanear redes.');
  } finally {
    if (currentToken === scanPollToken) {
      elements.scanButton.disabled = false;
      elements.scanButton.textContent = originalScanLabel;
    }
  }
}

async function pollScanResult(token, attempt) {
  if (token !== scanPollToken) return;

  const elapsedMs = Date.now() - scanStartedAt;
  if (elapsedMs > 35000) {
    throw new Error('El escaneo tardo demasiado y se cancelo.');
  }

  try {
    const response = await fetch('/api/wifi/scan', { cache: 'no-store' });
    const text = await response.text();
    let data = {};

    try {
      data = text ? JSON.parse(text) : {};
    } catch (_) {
      data = {};
    }

    if (!response.ok) {
      throw new Error(text || 'No se pudo consultar el escaneo WiFi.');
    }

    if (data.status === 'running' || data.status === 'idle') {
      const elapsed = data.started_ms_ago ? Math.round(data.started_ms_ago / 1000) : Math.round(elapsedMs / 1000);
      const channelText = data.current_channel ? ` canal ${data.current_channel}` : '';
      showMessage(elements.scanMessage, 'warn', `Escaneando redes cercanas... ${elapsed}s${channelText}`);
      await delay(900);
      return pollScanResult(token, attempt + 1);
    }

    if (data.status === 'failed') {
      const reason = data.error === 'timeout'
        ? 'El escaneo excedio el tiempo esperado.'
        : data.error === 'channel_scan_failed'
        ? 'Fallo el escaneo de uno de los canales WiFi.'
        : data.error === 'scan_start_failed'
        ? 'No se pudo iniciar el escaneo WiFi.'
        : (data.error || 'El escaneo WiFi fallo.');
      throw new Error(reason);
    }

    renderNetworks(data.networks || []);
    showMessage(elements.scanMessage, 'ok', `Escaneo completo: ${data.count ?? 0} redes detectadas.`);
  } catch (error) {
    if (attempt < 25) {
      await delay(1000);
      return pollScanResult(token, attempt + 1);
    }
    throw error;
  }
}

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

document.getElementById('wifiForm').addEventListener('submit', async (event) => {
  event.preventDefault();
  clearMessage(elements.formMessage);

  const submitBtn = event.target.querySelector('button[type="submit"]');
  const payload = {
    device_name: elements.deviceName.value.trim(),
    wifi_ssid: elements.wifiSsid.value.trim()
  };

  if (elements.wifiPass.value.length > 0) {
    payload.wifi_pass = elements.wifiPass.value;
  } else if (payload.wifi_ssid && openSsids.has(payload.wifi_ssid)) {
    payload.wifi_pass = '';
  }

  showMessage(elements.formMessage, 'warn', 'Guardando configuracion y reiniciando el equipo...');
  if (submitBtn) submitBtn.disabled = true;

  try {
    const response = await fetch('/api/wifi/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    if (!response.ok) {
      throw new Error(await response.text() || 'No se pudo guardar la configuracion.');
    }

    const message = await response.text();
    showMessage(elements.formMessage, 'ok', message || 'Configuracion guardada. Reiniciando...');
  } catch (error) {
    showMessage(elements.formMessage, 'err', error.message || 'No se pudo guardar la configuracion.');
    if (submitBtn) submitBtn.disabled = false;
  }
});

elements.scanButton.addEventListener('click', scanNetworks);

elements.adminPwdForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  clearMessage(elements.adminPwdMessage);

  const next = elements.newAdminPass.value;
  const confirm = elements.confirmAdminPass.value;

  if (next.length < 8 || next.length > 64) {
    showMessage(elements.adminPwdMessage, 'err', 'La nueva contraseña debe tener entre 8 y 64 caracteres.');
    return;
  }
  if (next !== confirm) {
    showMessage(elements.adminPwdMessage, 'err', 'La confirmacion no coincide.');
    return;
  }

  const payload = {
    current: elements.currentAdminPass.value,
    new: next
  };

  const submitBtn = event.target.querySelector('button[type="submit"]');
  showMessage(elements.adminPwdMessage, 'warn', 'Actualizando contraseña admin...');
  if (submitBtn) submitBtn.disabled = true;

  try {
    const response = await fetch('/api/admin/password', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    const text = await response.text();
    if (!response.ok) {
      throw new Error(text || 'No se pudo cambiar la contraseña.');
    }

    showMessage(elements.adminPwdMessage, 'ok', text + ' En el proximo acceso el navegador pedira la nueva contraseña.');
    elements.adminPwdForm.reset();
    document.querySelectorAll('.pwd-toggle').forEach((btn) => { btn.textContent = 'Ver'; });
    loadSettings().catch(() => {});
  } catch (error) {
    showMessage(elements.adminPwdMessage, 'err', error.message || 'No se pudo cambiar la contraseña.');
  } finally {
    if (submitBtn) submitBtn.disabled = false;
  }
});

function formatLive(value) {
  if (value === null || value === undefined || isNaN(value) || value < 0) return '--';
  return Number(value).toFixed(1);
}

function fillSensorCal(data) {
  if (data.empty_distance_cm !== undefined) elements.emptyDistance.value = data.empty_distance_cm;
  if (data.full_distance_cm !== undefined) elements.fullDistance.value = data.full_distance_cm;
  if (data.offset_cm !== undefined) elements.sensorOffset.value = data.offset_cm;
  elements.liveDistance.textContent = formatLive(data.current_distance_cm);
  elements.liveLevel.textContent = formatLive(data.current_level_pct);
}

async function loadSensorCal() {
  try {
    const response = await fetch('/api/sensor/calibrate', { cache: 'no-store' });
    if (!response.ok) throw new Error('No se pudo cargar la calibracion del sensor.');
    fillSensorCal(await response.json());
  } catch (error) {
    showMessage(elements.sensorCalMessage, 'err', error.message || 'No se pudo cargar la calibracion.');
  }
}

async function refreshLiveReading() {
  try {
    const response = await fetch('/api/sensor/calibrate', { cache: 'no-store' });
    if (!response.ok) return;
    const data = await response.json();
    elements.liveDistance.textContent = formatLive(data.current_distance_cm);
    elements.liveLevel.textContent = formatLive(data.current_level_pct);
  } catch (_) { /* ignorar fallos transitorios */ }
}

elements.sensorCalForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  clearMessage(elements.sensorCalMessage);

  const empty = parseFloat(elements.emptyDistance.value);
  const full = parseFloat(elements.fullDistance.value);
  const offset = parseFloat(elements.sensorOffset.value);

  if (isNaN(empty) || isNaN(full) || isNaN(offset)) {
    showMessage(elements.sensorCalMessage, 'err', 'Completa todos los campos con numeros validos.');
    return;
  }
  if (empty <= full) {
    showMessage(elements.sensorCalMessage, 'err', 'La distancia "vacio" debe ser mayor que la de "lleno".');
    return;
  }
  if (offset < -50 || offset > 50) {
    showMessage(elements.sensorCalMessage, 'err', 'El offset debe estar entre -50 y 50 cm.');
    return;
  }

  const payload = {
    tank: { empty_distance_cm: empty, full_distance_cm: full },
    sensor: { offset_cm: offset }
  };

  const submitBtn = event.target.querySelector('button[type="submit"]');
  showMessage(elements.sensorCalMessage, 'warn', 'Guardando calibracion...');
  if (submitBtn) submitBtn.disabled = true;

  try {
    const response = await fetch('/api/sensor/calibrate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    const text = await response.text();
    if (!response.ok) throw new Error(text || 'No se pudo guardar la calibracion.');
    showMessage(elements.sensorCalMessage, 'ok', text || 'Calibracion guardada.');
    refreshLiveReading();
  } catch (error) {
    showMessage(elements.sensorCalMessage, 'err', error.message || 'No se pudo guardar la calibracion.');
  } finally {
    if (submitBtn) submitBtn.disabled = false;
  }
});

loadSensorCal();
setInterval(refreshLiveReading, 5000);

loadSettings().catch((error) => {
  showMessage(elements.formMessage, 'err', error.message || 'No se pudo cargar el estado actual.');
});

// Power config
const powerEl = {
  form: document.getElementById('powerForm'),
  mode: document.getElementById('powerMode'),
  hintNormal: document.getElementById('modeHintNormal'),
  hintBattery: document.getElementById('modeHintBattery'),
  batteryFields: document.getElementById('batteryFields'),
  normalFields: document.getElementById('normalFields'),
  sleepInterval: document.getElementById('sleepInterval'),
  webWindow: document.getElementById('webWindow'),
  wifiTimeout: document.getElementById('wifiTimeout'),
  wifiRetry: document.getElementById('wifiRetry'),
  message: document.getElementById('powerMessage')
};

function applyPowerModeUi(mode) {
  const isBattery = mode === 'battery';
  powerEl.hintNormal.style.display = isBattery ? 'none' : '';
  powerEl.hintBattery.style.display = isBattery ? '' : 'none';
  powerEl.batteryFields.style.display = isBattery ? 'grid' : 'none';
  powerEl.normalFields.style.display = isBattery ? 'none' : '';
}

powerEl.mode.addEventListener('change', () => applyPowerModeUi(powerEl.mode.value));

async function loadPowerConfig() {
  try {
    const response = await fetch('/api/power', { cache: 'no-store' });
    if (!response.ok) throw new Error('No se pudo cargar la config de energia.');
    const d = await response.json();
    powerEl.mode.value = d.mode || 'normal';
    powerEl.sleepInterval.value = d.sleep_interval_sec ?? 300;
    powerEl.webWindow.value = d.web_window_sec ?? 60;
    powerEl.wifiTimeout.value = d.wifi_timeout_ms ?? 20000;
    powerEl.wifiRetry.value = d.wifi_retry_interval_sec ?? 30;
    applyPowerModeUi(d.mode || 'normal');
  } catch (error) {
    showMessage(powerEl.message, 'err', error.message || 'Error cargando config de energia.');
  }
}

powerEl.form.addEventListener('submit', async (event) => {
  event.preventDefault();
  clearMessage(powerEl.message);

  const mode = powerEl.mode.value;
  const payload = {
    mode,
    sleep_interval_sec: parseInt(powerEl.sleepInterval.value, 10),
    web_window_sec: parseInt(powerEl.webWindow.value, 10),
    wifi_timeout_ms: parseInt(powerEl.wifiTimeout.value, 10),
    wifi_retry_interval_sec: parseInt(powerEl.wifiRetry.value, 10)
  };

  const submitBtn = event.target.querySelector('button[type="submit"]');
  showMessage(powerEl.message, 'warn', 'Guardando y reiniciando...');
  if (submitBtn) submitBtn.disabled = true;

  try {
    const response = await fetch('/api/power', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    const text = await response.text();
    if (!response.ok) throw new Error(text || 'No se pudo guardar.');
    showMessage(powerEl.message, 'ok', text || 'Configuracion guardada. Reiniciando...');
  } catch (error) {
    showMessage(powerEl.message, 'err', error.message || 'Error al guardar.');
    if (submitBtn) submitBtn.disabled = false;
  }
});

loadPowerConfig();
</script>
</body>
</html>
)rawliteral";

#endif // WEB_DASHBOARD_H
