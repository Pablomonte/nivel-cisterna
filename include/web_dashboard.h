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
  :root {
    --bg:           #f1f5f9;
    --bg-elevated:  #ffffff;
    --bg-soft:      #f8fafc;
    --border:       #e2e8f0;
    --border-soft:  #f1f5f9;
    --text:         #0f172a;
    --text-muted:   #475569;
    --text-subtle:  #94a3b8;
    --accent:       #0284c7;
    --accent-soft:  #e0f2fe;
    --accent-hover: #0369a1;
    --ok:           #16a34a;
    --ok-soft:      #dcfce7;
    --warn:         #d97706;
    --warn-soft:    #fef3c7;
    --err:          #dc2626;
    --err-soft:     #fee2e2;
    --shadow-card:  0 1px 3px rgba(15, 23, 42, 0.06), 0 1px 2px rgba(15, 23, 42, 0.04);
    --radius:       12px;
  }

  * { margin: 0; padding: 0; box-sizing: border-box; }

  body {
    font-family: "Segoe UI", system-ui, -apple-system, sans-serif;
    background: var(--bg);
    color: var(--text);
    line-height: 1.5;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 1rem;
  }

  h1 {
    font-size: 1.35rem;
    font-weight: 700;
    color: var(--text);
    margin-bottom: 1rem;
    text-align: center;
  }

  .card {
    background: var(--bg-elevated);
    border-radius: var(--radius);
    padding: 1rem;
    margin-bottom: 1rem;
    width: 100%;
    max-width: 440px;
    border: 1px solid var(--border);
    box-shadow: var(--shadow-card);
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
    border: 3px solid var(--border);
    border-radius: 0 0 14px 14px;
    border-top: none;
    position: relative;
    overflow: hidden;
    background: var(--bg-soft);
  }

  .tank-top {
    width: 138px;
    height: 8px;
    background: var(--border);
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

  .water.low { background: linear-gradient(180deg, #f87171 0%, #dc2626 100%); }
  .water.mid { background: linear-gradient(180deg, #38bdf8 0%, #0284c7 100%); }
  .water.high { background: linear-gradient(180deg, #4ade80 0%, #16a34a 100%); }

  .level-text {
    font-size: 2.4rem;
    font-weight: 700;
    margin-top: 0.75rem;
    color: var(--text);
  }

  .level-text span {
    font-size: 1.1rem;
    color: var(--text-muted);
  }

  .uncalibrated-badge {
    display: none;
    margin-top: 0.6rem;
    padding: 0.4rem 0.8rem;
    background: var(--warn-soft);
    border: 1px solid var(--warn);
    border-radius: 999px;
    color: var(--warn);
    font-size: 0.82rem;
    font-weight: 600;
    text-decoration: none;
  }

  .uncalibrated-badge.show { display: inline-block; }

  .stats {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 0.8rem;
  }

  .stat {
    background: var(--bg-soft);
    border: 1px solid var(--border-soft);
    border-radius: 10px;
    padding: 0.9rem;
    text-align: center;
  }

  .stat-label {
    font-size: 0.72rem;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--text-muted);
    margin-bottom: 0.35rem;
  }

  .stat-value {
    font-size: 1.25rem;
    font-weight: 600;
    color: var(--text);
  }

  .stat-unit {
    font-size: 0.8rem;
    color: var(--text-muted);
  }

  .status-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.65rem 0;
    border-top: 1px solid var(--border-soft);
  }

  .status-row:first-child {
    border-top: none;
    padding-top: 0;
  }

  .status-label {
    font-size: 0.85rem;
    color: var(--text-muted);
  }

  .status-value {
    font-size: 0.95rem;
    font-weight: 600;
    color: var(--text);
    display: flex;
    align-items: center;
    gap: 0.45rem;
  }

  .dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--border);
  }

  .dot.ok { background: var(--ok); box-shadow: 0 0 0 4px var(--ok-soft); }
  .dot.warn { background: var(--warn); box-shadow: 0 0 0 4px var(--warn-soft); }
  .dot.err { background: var(--err); box-shadow: 0 0 0 4px var(--err-soft); }

  .device-name {
    color: var(--text-muted);
    font-size: 0.8rem;
    text-align: center;
    margin-top: 0.25rem;
  }

  .footer {
    margin-top: auto;
    padding-top: 0.75rem;
    color: var(--text-subtle);
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
    border-radius: var(--radius);
    border: 1px solid var(--accent);
    background: var(--accent);
    color: #ffffff;
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
    <div class="level-text"><span id="levelVal">--</span><span id="levelUnit">%</span></div>
    <a class="uncalibrated-badge" id="uncalBadge" href="/config">Sin calibrar — configurar</a>
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
  <a class="button-link" href="/config">Configuracion</a>
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
      const distance = d.distance ?? -1;
      const calibrated = d.tank_calibrated !== false && level >= 0;
      const water = document.getElementById('water');
      const levelVal = document.getElementById('levelVal');
      const levelUnit = document.getElementById('levelUnit');
      const uncalBadge = document.getElementById('uncalBadge');

      if (calibrated) {
        water.style.height = level + '%';
        water.className = 'water ' + (level < 20 ? 'low' : level > 80 ? 'high' : 'mid');
        levelVal.textContent = level.toFixed(1);
        levelUnit.textContent = '%';
        uncalBadge.classList.remove('show');
      } else {
        // Sin calibrar: tank vacio y el numero grande muestra distancia bruta.
        water.style.height = '0%';
        water.className = 'water mid';
        levelVal.textContent = distance >= 0 ? distance.toFixed(1) : '--';
        levelUnit.textContent = distance >= 0 ? ' cm' : '';
        uncalBadge.classList.add('show');
      }

      document.getElementById('distVal').textContent =
        distance >= 0 ? distance.toFixed(1) : '--';

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
    --bg:           #f1f5f9;
    --bg-elevated:  #ffffff;
    --bg-soft:      #f8fafc;
    --border:       #e2e8f0;
    --border-soft:  #f1f5f9;
    --text:         #0f172a;
    --muted:        #475569;
    --text-subtle:  #94a3b8;
    --accent:       #0284c7;
    --accent-soft:  #e0f2fe;
    --accent-hover: #0369a1;
    --ok:           #16a34a;
    --ok-soft:      #dcfce7;
    --warn:         #d97706;
    --warn-soft:    #fef3c7;
    --err:          #dc2626;
    --err-soft:     #fee2e2;
    --shadow-card:  0 1px 3px rgba(15, 23, 42, 0.06), 0 1px 2px rgba(15, 23, 42, 0.04);
    --radius:       12px;
    /* alias para CSS heredado del topbar */
    --bg-top:       var(--bg);
  }

  body {
    font-family: "Segoe UI", system-ui, -apple-system, sans-serif;
    background: var(--bg);
    color: var(--text);
    line-height: 1.5;
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
    color: var(--text);
    margin-bottom: 0.35rem;
    font-weight: 700;
  }

  .subtle {
    color: var(--muted);
    font-size: 0.92rem;
    line-height: 1.45;
  }

  .card {
    background: var(--bg-elevated);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 1rem;
    margin-bottom: 1rem;
    box-shadow: var(--shadow-card);
  }

  .card h2 {
    font-size: 1rem;
    margin-bottom: 0.85rem;
    color: var(--text);
    font-weight: 700;
  }

  .status-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 0.8rem;
  }

  .status-item {
    background: var(--bg-soft);
    border: 1px solid var(--border-soft);
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
    color: var(--text);
    font-size: 0.9rem;
    font-weight: 600;
  }

  input[type="text"],
  input[type="password"],
  input[type="number"],
  select {
    width: 100%;
    border: 1px solid var(--border);
    border-radius: var(--radius);
    background: var(--bg-elevated);
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
    border-color: var(--accent);
    box-shadow: 0 0 0 3px var(--accent-soft);
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
    background: var(--bg-soft);
    border: 1px solid var(--border);
    color: var(--muted);
    font-size: 0.85rem;
    font-weight: 500;
    padding: 0.4rem 0.6rem;
    border-radius: 8px;
    cursor: pointer;
    line-height: 1;
  }

  .pwd-toggle:hover {
    color: var(--accent);
    border-color: var(--accent);
  }

  .hidden {
    display: none !important;
  }

  .inline-option {
    display: flex;
    align-items: center;
    gap: 0.55rem;
    color: var(--text);
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
    border-radius: var(--radius);
    padding: 0.9rem 1rem;
    font-size: 0.95rem;
    font-weight: 600;
    cursor: pointer;
    text-decoration: none;
    display: inline-flex;
    align-items: center;
    justify-content: center;
  }

  .primary {
    background: var(--accent);
    color: #ffffff;
  }

  .primary:hover {
    background: var(--accent-hover);
  }

  .primary:disabled {
    background: var(--text-subtle);
    cursor: not-allowed;
  }

  .secondary,
  .nav-link {
    background: var(--bg-elevated);
    color: var(--text);
    border: 1px solid var(--border);
  }

  .secondary:hover,
  .nav-link:hover {
    border-color: var(--accent);
    color: var(--accent);
  }

  .message {
    display: none;
    margin-top: 0.5rem;
    padding: 0.8rem 0.9rem;
    border-radius: var(--radius);
    font-size: 0.9rem;
    line-height: 1.4;
  }

  .message.show { display: block; }
  .message.ok { background: var(--ok-soft); color: var(--ok); border: 1px solid var(--ok); }
  .message.err { background: var(--err-soft); color: var(--err); border: 1px solid var(--err); }
  .message.warn { background: var(--warn-soft); color: var(--warn); border: 1px solid var(--warn); }

  .network-list {
    display: grid;
    gap: 0.7rem;
  }

  .network-item {
    width: 100%;
    text-align: left;
    background: var(--bg-elevated);
    border: 1px solid var(--border);
    color: var(--text);
  }

  .network-item:hover {
    border-color: var(--accent);
  }

  .network-item.selected {
    border-color: var(--accent);
    box-shadow: 0 0 0 2px var(--accent-soft);
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

  .badge.ok { background: var(--ok-soft); color: var(--ok); }
  .badge.warn { background: var(--warn-soft); color: var(--warn); }

  @media (max-width: 640px) {
    .status-grid {
      grid-template-columns: 1fr;
    }

    .actions > * {
      width: 100%;
    }
  }

  .topbar {
    position: sticky;
    top: 0;
    z-index: 10;
    background: var(--bg);
    padding: 0.9rem 0 0;
    margin: -1rem -1rem 1rem;
    padding-left: 1rem;
    padding-right: 1rem;
    border-bottom: 1px solid var(--border);
  }

  .topbar-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 0.6rem;
    margin-bottom: 0.6rem;
  }

  .topbar-title {
    font-size: 1.05rem;
    font-weight: 700;
    color: var(--text);
    letter-spacing: 0.01em;
  }

  .topbar-back {
    color: var(--muted);
    text-decoration: none;
    font-size: 0.88rem;
    font-weight: 600;
    padding: 0.35rem 0.6rem;
    border-radius: 8px;
    border: 1px solid var(--border);
    background: var(--bg-elevated);
  }

  .topbar-back:hover {
    color: var(--accent);
    border-color: var(--accent);
  }

  .tabbar {
    display: flex;
    gap: 0.2rem;
    overflow-x: auto;
    scrollbar-width: none;
    margin: 0 -1rem;
    padding: 0 1rem;
  }

  .tabbar::-webkit-scrollbar { display: none; }

  .tab {
    flex: 0 0 auto;
    background: transparent;
    border: none;
    color: var(--muted);
    padding: 0.7rem 1rem;
    font-size: 0.92rem;
    font-weight: 600;
    cursor: pointer;
    border-bottom: 2px solid transparent;
    border-radius: 0;
    white-space: nowrap;
  }

  .tab[aria-selected="true"] {
    color: var(--accent);
    border-bottom-color: var(--accent);
  }

  .tab:hover {
    color: var(--text);
  }

  [role="tabpanel"][hidden] {
    display: none;
  }
</style>
</head>
<body>
<div class="shell">
  <div class="topbar">
    <div class="topbar-row">
      <div class="topbar-title">Cisterna · Configuracion</div>
      <a class="topbar-back" href="/">← Dashboard</a>
    </div>
    <div class="tabbar" role="tablist">
      <button class="tab" role="tab" data-target="panel-wifi" aria-selected="true">WiFi</button>
      <button class="tab" role="tab" data-target="panel-sensor" aria-selected="false">Calibracion</button>
      <button class="tab" role="tab" data-target="panel-power" aria-selected="false">Energia</button>
      <button class="tab" role="tab" data-target="panel-admin" aria-selected="false">Admin</button>
    </div>
  </div>

  <section id="panel-wifi" role="tabpanel" aria-labelledby="tab-wifi">
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
      </div>
    </form>
    <div class="message" id="formMessage"></div>

    <div class="subtle" style="margin: 1rem 0 0.6rem;">Redes detectadas</div>
    <div class="network-list" id="networkList">
      <div class="subtle">Todavia no se escanearon redes.</div>
    </div>
    <div class="message" id="scanMessage"></div>
  </div>
  </section>

  <section id="panel-sensor" role="tabpanel" aria-labelledby="tab-sensor" hidden>
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
        <input id="emptyDistance" name="emptyDistance" type="number" step="0.1" min="1" max="400" placeholder="ej. 145" required>
        <div class="hint">Distancia que mide el sensor cuando el tanque esta vacio (0%).</div>
      </div>
      <div>
        <label for="fullDistance">Distancia "tanque lleno" (cm)</label>
        <input id="fullDistance" name="fullDistance" type="number" step="0.1" min="1" max="400" placeholder="ej. 10" required>
        <div class="hint">Distancia que mide el sensor cuando el tanque esta lleno (100%). Debe ser menor que la de vacio.</div>
      </div>
      <div>
        <label for="sensorOffset">Offset del sensor (cm)</label>
        <input id="sensorOffset" name="sensorOffset" type="number" step="0.1" min="-50" max="50" required>
        <div class="hint">Correccion fija sumada a cada lectura. Usalo para compensar montaje del sensor (-50 a +50).</div>
      </div>
      <div class="actions">
        <button class="primary" type="submit">Guardar calibracion</button>
        <button class="secondary" type="button" id="clearCalBtn" hidden>Borrar calibracion</button>
      </div>
    </form>
    <div class="message" id="sensorCalMessage"></div>
  </div>
  </section>

  <section id="panel-power" role="tabpanel" aria-labelledby="tab-power" hidden>
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
  </section>

  <section id="panel-admin" role="tabpanel" aria-labelledby="tab-admin" hidden>
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
  </section>
</div>

<script>
// Tab switcher: activa el panel correspondiente al hash de la URL.
// Mantiene compatibilidad con browser back/forward usando replaceState.
const TAB_HASHES = ['wifi', 'sensor', 'power', 'admin'];

function activateTab(hash) {
  const target = TAB_HASHES.includes(hash) ? hash : 'wifi';
  document.querySelectorAll('.tab').forEach(btn => {
    const isActive = btn.dataset.target === 'panel-' + target;
    btn.setAttribute('aria-selected', isActive ? 'true' : 'false');
  });
  document.querySelectorAll('[role="tabpanel"]').forEach(panel => {
    panel.hidden = panel.id !== 'panel-' + target;
  });
  if (location.hash !== '#' + target) {
    history.replaceState(null, '', '#' + target);
  }
}

document.querySelectorAll('.tab').forEach(btn => {
  btn.addEventListener('click', () => {
    const target = btn.dataset.target.replace(/^panel-/, '');
    activateTab(target);
    window.scrollTo({ top: 0, behavior: 'instant' });
  });
});

window.addEventListener('hashchange', () => {
  activateTab(location.hash.replace(/^#/, ''));
});

activateTab(location.hash.replace(/^#/, ''));

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
  // Muestra vacio si esta sin calibrar (0) en vez de "0" literal — el
  // placeholder del input gui­a al usuario.
  if (data.empty_distance_cm !== undefined) {
    elements.emptyDistance.value = data.empty_distance_cm > 0 ? data.empty_distance_cm : '';
  }
  if (data.full_distance_cm !== undefined) {
    elements.fullDistance.value = data.full_distance_cm > 0 ? data.full_distance_cm : '';
  }
  if (data.offset_cm !== undefined) elements.sensorOffset.value = data.offset_cm;
  elements.liveDistance.textContent = formatLive(data.current_distance_cm);
  elements.liveLevel.textContent = formatLive(data.current_level_pct);

  const clearBtn = document.getElementById('clearCalBtn');
  if (clearBtn) {
    const calibrated = data.empty_distance_cm > 0 &&
                       data.full_distance_cm > 0 &&
                       data.empty_distance_cm > data.full_distance_cm;
    clearBtn.hidden = !calibrated;
  }
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

document.getElementById('clearCalBtn').addEventListener('click', async (event) => {
  if (!confirm('Borrar la calibracion del tanque?\n\nEl dashboard volvera a mostrar la distancia bruta hasta que vuelvas a calibrar.')) {
    return;
  }
  const btn = event.currentTarget;
  clearMessage(elements.sensorCalMessage);
  showMessage(elements.sensorCalMessage, 'warn', 'Borrando calibracion...');
  btn.disabled = true;

  try {
    const response = await fetch('/api/sensor/calibrate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ tank: { empty_distance_cm: 0, full_distance_cm: 0 } })
    });
    const text = await response.text();
    if (!response.ok) throw new Error(text || 'No se pudo borrar la calibracion.');
    showMessage(elements.sensorCalMessage, 'ok', 'Calibracion borrada. Dashboard mostrando distancia bruta.');
    loadSensorCal();
  } catch (error) {
    showMessage(elements.sensorCalMessage, 'err', error.message || 'No se pudo borrar la calibracion.');
  } finally {
    btn.disabled = false;
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
