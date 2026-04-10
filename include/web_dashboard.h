#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

/**
 * Embedded HTML/CSS/JS for the cisterna dashboard.
 * Single-page app that auto-refreshes data via /api/status JSON endpoint.
 */

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
    font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
    background: #0f172a;
    color: #e2e8f0;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 1rem;
  }

  h1 {
    font-size: 1.4rem;
    font-weight: 600;
    color: #38bdf8;
    margin-bottom: 1rem;
    text-align: center;
  }

  .card {
    background: #1e293b;
    border-radius: 12px;
    padding: 1.2rem;
    margin-bottom: 1rem;
    width: 100%;
    max-width: 420px;
    border: 1px solid #334155;
  }

  /* Tank gauge visualization */
  .tank-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 1rem 0;
  }

  .tank {
    width: 120px;
    height: 200px;
    border: 3px solid #475569;
    border-radius: 0 0 12px 12px;
    border-top: none;
    position: relative;
    overflow: hidden;
    background: #0f172a;
  }

  .tank-top {
    width: 136px;
    height: 8px;
    background: #475569;
    border-radius: 4px 4px 0 0;
  }

  .water {
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    background: linear-gradient(to top, #0369a1, #38bdf8);
    transition: height 1s ease;
    border-radius: 0 0 9px 9px;
  }

  .water.low { background: linear-gradient(to top, #dc2626, #f87171); }
  .water.mid { background: linear-gradient(to top, #0369a1, #38bdf8); }
  .water.high { background: linear-gradient(to top, #059669, #34d399); }

  .level-text {
    font-size: 2.4rem;
    font-weight: 700;
    margin-top: 0.8rem;
    color: #f1f5f9;
  }

  .level-text span { font-size: 1.2rem; color: #94a3b8; }

  /* Stats grid */
  .stats {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.8rem;
  }

  .stat {
    background: #0f172a;
    border-radius: 8px;
    padding: 0.8rem;
    text-align: center;
  }

  .stat-label {
    font-size: 0.7rem;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: #64748b;
    margin-bottom: 0.3rem;
  }

  .stat-value {
    font-size: 1.3rem;
    font-weight: 600;
    color: #e2e8f0;
  }

  .stat-unit { font-size: 0.8rem; color: #94a3b8; }

  /* Pump status */
  .pump-status {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 0.6rem 0;
  }

  .pump-indicator {
    display: inline-block;
    width: 12px;
    height: 12px;
    border-radius: 50%;
    margin-right: 0.5rem;
    background: #475569;
  }

  .pump-indicator.on { background: #22c55e; box-shadow: 0 0 8px #22c55e88; }
  .pump-indicator.off { background: #475569; }
  .pump-indicator.timeout { background: #ef4444; box-shadow: 0 0 8px #ef444488; }

  .sensor-error {
    text-align: center;
    padding: 1rem;
    color: #f87171;
    font-weight: 600;
  }

  .label { color: #94a3b8; font-size: 0.85rem; }
  .device-name { color: #64748b; font-size: 0.75rem; text-align: center; margin-top: 0.5rem; }

  .btn {
    background: #334155;
    color: #e2e8f0;
    border: 1px solid #475569;
    border-radius: 6px;
    padding: 0.4rem 1rem;
    cursor: pointer;
    font-size: 0.8rem;
  }
  .btn:hover { background: #475569; }

  .footer {
    margin-top: auto;
    padding-top: 1rem;
    color: #475569;
    font-size: 0.7rem;
    text-align: center;
  }
</style>
</head>
<body>

<h1>🏗️ Cisterna Monitor</h1>

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
      <div class="stat-label">Sensor</div>
      <div class="stat-value"><span id="sensorStatus">--</span></div>
    </div>
  </div>
</div>

<div class="card" id="pumpCard" style="display:none">
  <div class="pump-status">
    <div>
      <span class="pump-indicator" id="pumpDot"></span>
      <span class="label">Bomba: </span>
      <strong id="pumpState">--</strong>
    </div>
    <div>
      <button class="btn" onclick="pumpAction('on')">ON</button>
      <button class="btn" onclick="pumpAction('off')">OFF</button>
      <button class="btn" onclick="pumpAction('auto')">AUTO</button>
    </div>
  </div>
</div>

<div class="device-name" id="deviceName"></div>
<div class="footer">nivel-cisterna <span id="fwVersion"></span></div>

<script>
function updateData() {
  fetch('/api/status')
    .then(r => r.json())
    .then(d => {
      // Level gauge
      const level = d.level ?? -1;
      const water = document.getElementById('water');
      const levelVal = document.getElementById('levelVal');

      if (level >= 0) {
        water.style.height = level + '%';
        levelVal.textContent = level.toFixed(1);
        water.className = 'water ' + (level < 20 ? 'low' : level > 80 ? 'high' : 'mid');
      } else {
        levelVal.textContent = '--';
      }

      // Stats
      document.getElementById('distVal').textContent = 
        d.distance >= 0 ? d.distance.toFixed(1) : '--';
      document.getElementById('volVal').textContent = 
        d.volume >= 0 ? Math.round(d.volume) : '--';
      document.getElementById('capVal').textContent = 
        d.capacity ? Math.round(d.capacity) : '--';
      document.getElementById('sensorStatus').textContent = 
        d.sensor_ok ? '✓' : '✗';
      document.getElementById('sensorStatus').style.color = 
        d.sensor_ok ? '#22c55e' : '#ef4444';

      // Pump
      if (d.pump_enabled) {
        document.getElementById('pumpCard').style.display = 'block';
        const dot = document.getElementById('pumpDot');
        dot.className = 'pump-indicator ' + (d.pump_on ? 'on' : d.pump_state === 'timeout' ? 'timeout' : 'off');
        document.getElementById('pumpState').textContent = d.pump_state || 'off';
      }

      // Device info
      document.getElementById('deviceName').textContent = d.device || '';
      document.getElementById('fwVersion').textContent = d.version || '';
    })
    .catch(() => {});
}

function pumpAction(action) {
  fetch('/api/pump?action=' + action, { method: 'POST' }).then(() => updateData());
}

updateData();
setInterval(updateData, 5000);
</script>

</body>
</html>
)rawliteral";

#endif // WEB_DASHBOARD_H
