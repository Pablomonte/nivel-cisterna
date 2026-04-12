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
  <div class="stats">
    <div class="stat">
      <div class="stat-label">Distancia</div>
      <div class="stat-value"><span id="distVal">--</span> <span class="stat-unit">cm</span></div>
    </div>
    <div class="stat">
      <div class="stat-label">Volumen</div>
      <div class="stat-value"><span id="volVal">--</span> <span class="stat-unit">L</span></div>
    </div>
    <div class="stat">
      <div class="stat-label">Capacidad</div>
      <div class="stat-value"><span id="capVal">--</span> <span class="stat-unit">L</span></div>
    </div>
    <div class="stat">
      <div class="stat-label">Ultimo Exito</div>
      <div class="stat-value"><span id="successAgeVal">--</span> <span class="stat-unit">s</span></div>
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
    <div class="status-label">Lecturas fallidas</div>
    <div class="status-value" id="sensorFailures">--</div>
  </div>
  <div class="status-row">
    <div class="status-label">Variacion sensor</div>
    <div class="status-value"><span id="spreadVal">--</span> cm</div>
  </div>
  <div class="status-row">
    <div class="status-label">Bomba</div>
    <div class="status-value">
      <span class="dot" id="pumpDot"></span>
      <span id="pumpState">--</span>
    </div>
  </div>
  <div class="status-row">
    <div class="status-label">Modo bomba</div>
    <div class="status-value" id="pumpMode">--</div>
  </div>
  <div class="status-row">
    <div class="status-label">Runtime bomba</div>
    <div class="status-value"><span id="pumpRuntime">--</span> s</div>
  </div>
  <div class="status-row">
    <div class="status-label">WiFi</div>
    <div class="status-value" id="wifiState">--</div>
  </div>
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
      document.getElementById('volVal').textContent =
        d.volume >= 0 ? Math.round(d.volume) : '--';
      document.getElementById('capVal').textContent =
        d.capacity >= 0 ? Math.round(d.capacity) : '--';
      document.getElementById('successAgeVal').textContent =
        d.last_success_age_sec >= 0 ? d.last_success_age_sec : '--';

      document.getElementById('sensorStatus').textContent =
        d.sensor_ok ? 'OK' : 'ERROR';
      document.getElementById('sensorFailures').textContent =
        d.sensor_failures ?? '--';
      document.getElementById('spreadVal').textContent =
        d.sensor_spread_cm >= 0 ? d.sensor_spread_cm.toFixed(1) : '--';
      setDot('sensorDot', d.sensor_ok ? 'ok' : 'err');

      document.getElementById('pumpState').textContent = d.pump_state || '--';
      document.getElementById('pumpMode').textContent = d.pump_auto_mode ? 'AUTO' : 'MANUAL';
      document.getElementById('pumpRuntime').textContent = d.pump_runtime_sec ?? '--';

      if (d.pump_state === 'timeout') {
        setDot('pumpDot', 'err');
      } else if (d.pump_on) {
        setDot('pumpDot', 'ok');
      } else {
        setDot('pumpDot', 'warn');
      }

      document.getElementById('wifiState').textContent =
        d.wifi_connected ? (d.ip || 'conectado') : (d.wifi_mode || 'sin enlace');

      document.getElementById('deviceName').textContent = d.device || '';
      document.getElementById('fwVersion').textContent = d.version || '';
    })
    .catch(() => {
      setDot('sensorDot', 'err');
      setDot('pumpDot', 'warn');
    });
}

updateData();
setInterval(updateData, 5000);
</script>

</body>
</html>
)rawliteral";

#endif // WEB_DASHBOARD_H
