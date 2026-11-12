#include "web_setup_page.h"

#include <pgmspace.h>

const char kSetupPageHtml[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Fermentatiebox setup</title>
<style>
  body { font-family: sans-serif; margin: 0; padding: 1.5rem; background: #111; color: #eee; }
  h1 { font-size: 1.3rem; margin: 0 0 1rem; }
  .controls { display: flex; flex-wrap: wrap; gap: 0.6rem; align-items: center; margin-bottom: 0.8rem; }
  button {
    background: #222; color: #eee; border: 1px solid #444; border-radius: 4px;
    padding: 0.5rem 1rem; font-size: 0.95rem; cursor: pointer;
  }
  button:disabled { opacity: 0.5; cursor: default; }
  label { display: flex; align-items: center; gap: 0.35rem; cursor: pointer; font-size: 0.95rem; }
  input[type=number] {
    width: 4.5rem; background: #222; color: #eee; border: 1px solid #444;
    border-radius: 4px; padding: 0.4rem;
  }
  .status { color: #999; font-size: 0.9rem; margin-bottom: 0.6rem; min-height: 1.2em; }
  .photo-wrap { position: relative; display: inline-block; max-width: 100%; }
  #photo { max-width: 100%; display: none; border-radius: 4px; cursor: zoom-in; }
  #grid { position: absolute; top: 0; left: 0; pointer-events: none; display: none; }
  .meta { color: #999; font-size: 0.85rem; margin-top: 0.4rem; }
  #fullsize {
    display: none; position: fixed; inset: 0; background: rgba(0,0,0,0.9);
    z-index: 10; align-items: center; justify-content: center; cursor: zoom-out;
  }
  #fullsize img { max-width: 100%; max-height: 100%; }
</style>
</head>
<body>
<h1>Fermentatiebox - setup</h1>
<div class="controls">
  <button id="photo-btn">Take test photo</button>
  <label><input type="checkbox" id="live-toggle"> Live (every 4 s)</label>
  <label><input type="checkbox" id="grid-toggle"> Grid overlay</label>
  <button id="reference-btn" disabled>Save as reference</button>
</div>
<div class="controls">
  <label>Flash brightness
    <select id="brightness-select">
      <option value="0">0% (off)</option>
      <option value="10">10%</option>
      <option value="15">15%</option>
      <option value="25">25%</option>
      <option value="50">50%</option>
      <option value="100">100%</option>
    </select>
  </label>
  <button id="use-for-schedule-btn">Use for schedule</button>
  <span class="status" id="schedule-status"></span>
</div>
<div class="controls">
  <label>White balance
    <select id="wb-select">
      <option value="0">Auto</option>
      <option value="1">Sunny</option>
      <option value="2">Cloudy</option>
      <option value="3">Office</option>
      <option value="4">Home</option>
    </select>
  </label>
  <label>Exposure
    <select id="aec-select">
      <option value="auto">Auto</option>
      <option value="manual">Manual</option>
    </select>
  </label>
  <input type="number" id="aec-value" min="0" max="1200" step="10" value="300">
  <label>Gain
    <select id="agc-select">
      <option value="auto">Auto</option>
      <option value="manual">Manual</option>
    </select>
  </label>
  <input type="number" id="agc-value" min="0" max="30" value="0">
  <button id="use-camera-settings-btn">Use for schedule</button>
  <span class="status" id="camera-settings-status"></span>
</div>
<div class="status" id="status"></div>
<div class="photo-wrap">
  <img id="photo" alt="test photo">
  <canvas id="grid"></canvas>
</div>
<div class="meta" id="meta"></div>
<div id="fullsize"><img id="fullsize-img" alt="test photo (full size)"></div>

<script>
(function () {
  var photoBtn = document.getElementById('photo-btn');
  var referenceBtn = document.getElementById('reference-btn');
  var liveToggle = document.getElementById('live-toggle');
  var gridToggle = document.getElementById('grid-toggle');
  var statusEl = document.getElementById('status');
  var metaEl = document.getElementById('meta');
  var photo = document.getElementById('photo');
  var grid = document.getElementById('grid');
  var fullsize = document.getElementById('fullsize');
  var fullsizeImg = document.getElementById('fullsize-img');
  var brightnessSelect = document.getElementById('brightness-select');
  var useForScheduleBtn = document.getElementById('use-for-schedule-btn');
  var scheduleStatusEl = document.getElementById('schedule-status');
  var wbSelect = document.getElementById('wb-select');
  var aecSelect = document.getElementById('aec-select');
  var aecValue = document.getElementById('aec-value');
  var agcSelect = document.getElementById('agc-select');
  var agcValue = document.getElementById('agc-value');
  var useCameraSettingsBtn = document.getElementById('use-camera-settings-btn');
  var cameraSettingsStatusEl = document.getElementById('camera-settings-status');
  var liveTimer = null;
  var hasPhoto = false;

  fetch('/api/flash/brightness').then(function (r) {
    return r.ok ? r.json() : null;
  }).then(function (d) {
    if (d && d.percent !== undefined) {
      brightnessSelect.value = String(d.percent);
    }
  }).catch(function () {});

  function updateExposureGainVisibility() {
    aecValue.style.display = aecSelect.value === 'manual' ? 'inline-block' : 'none';
    agcValue.style.display = agcSelect.value === 'manual' ? 'inline-block' : 'none';
  }

  fetch('/api/camera/settings').then(function (r) {
    return r.ok ? r.json() : null;
  }).then(function (d) {
    if (!d) return;
    wbSelect.value = String(d.whiteBalance);
    aecSelect.value = d.exposureAuto ? 'auto' : 'manual';
    aecValue.value = String(d.exposureValue);
    agcSelect.value = d.gainAuto ? 'auto' : 'manual';
    agcValue.value = String(d.gainValue);
    updateExposureGainVisibility();
  }).catch(function () {});
  updateExposureGainVisibility();
  aecSelect.addEventListener('change', updateExposureGainVisibility);
  agcSelect.addEventListener('change', updateExposureGainVisibility);

  function drawGrid() {
    if (!gridToggle.checked || photo.clientWidth === 0) {
      grid.style.display = 'none';
      return;
    }
    var w = photo.clientWidth, h = photo.clientHeight;
    grid.width = w;
    grid.height = h;
    grid.style.display = 'block';
    var ctx = grid.getContext('2d');
    ctx.clearRect(0, 0, w, h);
    ctx.strokeStyle = 'rgba(255,255,255,0.6)';
    ctx.lineWidth = 1;
    [w / 3, 2 * w / 3].forEach(function (x) {
      ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
    });
    [h / 3, 2 * h / 3].forEach(function (y) {
      ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
    });
    ctx.strokeStyle = 'rgba(255,80,80,0.9)';
    ctx.beginPath(); ctx.moveTo(w / 2 - 10, h / 2); ctx.lineTo(w / 2 + 10, h / 2); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(w / 2, h / 2 - 10); ctx.lineTo(w / 2, h / 2 + 10); ctx.stroke();
  }

  function takePhoto() {
    photoBtn.disabled = true;
    statusEl.textContent = 'Taking photo...';
    var brightnessUsed = brightnessSelect.value;
    fetch('/api/photo?brightness=' + encodeURIComponent(brightnessUsed)).then(function (r) {
      if (!r.ok) {
        var retryAfter = r.headers.get('Retry-After');
        return r.text().then(function (t) {
          throw new Error((t || ('error ' + r.status)) + (retryAfter ? ' (retry in ' + retryAfter + 's)' : ''));
        });
      }
      brightnessUsed = r.headers.get('X-Flash-Brightness-Percent') || brightnessUsed;
      var wbUsed = r.headers.get('X-White-Balance') || '?';
      var aecUsed = r.headers.get('X-Exposure') || '?';
      var agcUsed = r.headers.get('X-Gain') || '?';
      return r.blob().then(function (blob) {
        return { blob: blob, wbUsed: wbUsed, aecUsed: aecUsed, agcUsed: agcUsed };
      });
    }).then(function (result) {
      var blob = result.blob;
      photo.onload = drawGrid;
      photo.src = URL.createObjectURL(blob);
      photo.style.display = 'block';
      hasPhoto = true;
      referenceBtn.disabled = false;
      metaEl.textContent = 'Taken at ' + new Date().toLocaleTimeString() +
        ' - ' + (blob.size / 1024).toFixed(1) + ' KB - flash ' + brightnessUsed + '%' +
        ' - wb ' + result.wbUsed + ' - exposure ' + result.aecUsed + ' - gain ' + result.agcUsed;
      statusEl.textContent = '';
    }).catch(function (e) {
      statusEl.textContent = 'Error: ' + e.message;
    }).finally(function () {
      photoBtn.disabled = false;
    });
  }

  photoBtn.addEventListener('click', takePhoto);
  gridToggle.addEventListener('change', drawGrid);
  window.addEventListener('resize', drawGrid);

  liveToggle.addEventListener('change', function () {
    if (liveToggle.checked) {
      takePhoto();
      liveTimer = setInterval(takePhoto, 4000);
    } else if (liveTimer) {
      clearInterval(liveTimer);
      liveTimer = null;
    }
  });

  photo.addEventListener('click', function () {
    if (!hasPhoto) return;
    fullsizeImg.src = photo.src;
    fullsize.style.display = 'flex';
  });
  fullsize.addEventListener('click', function () {
    fullsize.style.display = 'none';
  });

  useForScheduleBtn.addEventListener('click', function () {
    useForScheduleBtn.disabled = true;
    scheduleStatusEl.textContent = 'Saving...';
    fetch('/api/flash/brightness?percent=' + encodeURIComponent(brightnessSelect.value), {
      method: 'POST'
    }).then(function (r) {
      if (!r.ok) {
        return r.text().then(function (t) { throw new Error(t || ('error ' + r.status)); });
      }
      return r.json();
    }).then(function (d) {
      scheduleStatusEl.textContent = 'Schedule now uses ' + d.percent + '%';
    }).catch(function (e) {
      scheduleStatusEl.textContent = 'Error: ' + e.message;
    }).finally(function () {
      useForScheduleBtn.disabled = false;
    });
  });

  useCameraSettingsBtn.addEventListener('click', function () {
    useCameraSettingsBtn.disabled = true;
    cameraSettingsStatusEl.textContent = 'Saving...';
    var params = 'whiteBalance=' + encodeURIComponent(wbSelect.value) +
      '&exposureAuto=' + (aecSelect.value === 'auto') +
      '&exposureValue=' + encodeURIComponent(aecValue.value) +
      '&gainAuto=' + (agcSelect.value === 'auto') +
      '&gainValue=' + encodeURIComponent(agcValue.value);
    fetch('/api/camera/settings?' + params, { method: 'POST' }).then(function (r) {
      if (!r.ok) {
        return r.text().then(function (t) { throw new Error(t || ('error ' + r.status)); });
      }
      return r.json();
    }).then(function () {
      cameraSettingsStatusEl.textContent = 'Schedule updated';
    }).catch(function (e) {
      cameraSettingsStatusEl.textContent = 'Error: ' + e.message;
    }).finally(function () {
      useCameraSettingsBtn.disabled = false;
    });
  });

  referenceBtn.addEventListener('click', function () {
    referenceBtn.disabled = true;
    statusEl.textContent = 'Saving reference photo...';
    fetch('/api/photo/reference', { method: 'POST' }).then(function (r) {
      if (!r.ok) {
        return r.text().then(function (t) { throw new Error(t || ('error ' + r.status)); });
      }
      return r.json();
    }).then(function (d) {
      statusEl.textContent = 'Saved as ' + d.path;
    }).catch(function (e) {
      statusEl.textContent = 'Error: ' + e.message;
    }).finally(function () {
      referenceBtn.disabled = false;
    });
  });
})();
</script>
</body>
</html>
)HTML";
