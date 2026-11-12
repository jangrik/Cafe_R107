// Cafe_R107 Shelly script 1.1.5 – Shelly Plug M/S Gen3, Matter must be disabled.
// Set the key shown in Cafe_R107 > Gerät & WLAN. Enable "Run on startup".
// The Shelly joins only the ESP access point. Relay settings are not modified.
let ESP_URL = "http://192.168.4.1/api/shelly";
let API_KEY = "HIER_SHELLY_SCHLUESSEL_EINTRAGEN";
let MAX_EVENTS = 40;
let SAMPLE_MS = 1000;
let GRIND_MIN_MS = 3000;
let GRIND_MAX_MS = 18000;
let FLOW_MIN_MS = 5000;
let FINISH_QUIET_MS = 4000;
let CYCLE_MAX_MS = 180000;
// Initial thresholds derived from the two Philips EP0800 recordings.
// The higher band assumes the heater and grinder are on simultaneously.
let GRIND_LOW_W = 100;
let GRIND_HIGH_W = 250;
let HEATER_GRIND_LOW_W = 1490;
let HEATER_GRIND_HIGH_W = 1700;
let QUIET_W = 30;
// After a validated grinder phase, pumping can occur without heater power.
// The preceding grinder and pause still distinguish this from a rinse.
let FLOW_W = 35;

let deviceInfo = Shelly.getDeviceInfo();
let device = deviceInfo.id;
let boot = "" + Math.floor(Math.random() * 2147483647)
  + "-" + Math.floor(Math.random() * 2147483647);
let sequence = 0;
let queue = [];
let loaded = false;
let storageBusy = false;
let httpBusy = false;
let lastHeartbeat = -10000;
let fault = "";
let lastPower = 0;
let clockEpoch = 0;
let clockAt = 0;
let phase = "idle";
let grindAt = 0;
let grindLast = 0;
let candidateAt = 0;
let quietAt = -1;
let flowConfirmed = false;
let flowActiveMs = 0;
let flowSampleAt = 0;
let recognizedCount = 0;
let lastDecision = "no_cycle_yet";
let lastDecisionAt = 0;
let traceTimes = [];
let tracePowers = [];
let traceCapacity = 180;
let traceNext = 0;
// Keep the phase only for the five samples sent with the next heartbeat.
let recentPhases = [];
let recentNext = 0;

function epochAt(now) {
  let sys = Shelly.getComponentStatus("sys");
  if (sys && typeof sys.unixtime === "number" && sys.unixtime > 1704067200) {
    return sys.unixtime;
  }
  if (clockEpoch) return Math.floor(clockEpoch + (now - clockAt) / 1000);
  return 0;
}

function resetDetection(reason) {
  if (phase !== "idle") {
    lastDecision = reason;
    lastDecisionAt = Shelly.getUptimeMs();
    print("Erkennung", phase, "-> idle:", reason);
  }
  phase = "idle";
  grindAt = 0;
  grindLast = 0;
  quietAt = -1;
  flowConfirmed = false;
  flowActiveMs = 0;
  flowSampleAt = 0;
}

function storeBrew(endAt) {
  let slot = -1;
  for (let i = 0; i < MAX_EVENTS; i++) {
    let used = false;
    for (let j = 0; j < queue.length; j++) {
      if (queue[j].key === "cafe_brew_" + i) used = true;
    }
    if (!used) { slot = i; break; }
  }
  if (slot < 0) {
    fault = "Puffer voll: mindestens ein Bezug konnte nicht gespeichert werden";
    print(fault);
    return;
  }
  sequence++;
  recognizedCount++;
  let epoch = epochAt(Shelly.getUptimeMs());
  // epochAt reads current device time; compensate the quiet confirmation delay.
  if (epoch) epoch -= Math.floor((Shelly.getUptimeMs() - endAt) / 1000);
  let event = {id: boot + "-" + sequence, boot: boot, end: endAt, epoch: epoch};
  queue.push({key: "cafe_brew_" + slot, value: event, saved: false});
  print("Kaffee erkannt", event.id, "Ende ms", endAt);
}

function detect(now, power) {
  if (phase !== "idle" && now - candidateAt > CYCLE_MAX_MS) resetDetection("cycle_timeout");
  let grinder = (power >= GRIND_LOW_W && power <= GRIND_HIGH_W)
    || (power >= HEATER_GRIND_LOW_W && power <= HEATER_GRIND_HIGH_W);
  if (phase === "idle") {
    if (grinder) {
      phase = "grinding";grindAt = now;grindLast = now;candidateAt = now;
    }
    return;
  }
  if (phase === "grinding") {
    if (grinder) grindLast = now;
    if (now - grindAt > GRIND_MAX_MS) { resetDetection("grinding_too_long");return; }
    if (!grinder && now - grindLast >= 1000) {
      if (grindLast - grindAt < GRIND_MIN_MS) {
        resetDetection("grinding_too_short");
      } else if (power < QUIET_W) {
        phase = "waiting";quietAt = now;
      } else if (power >= FLOW_W && power < HEATER_GRIND_LOW_W) {
        // A warm machine may keep heating when the grinder stops.
        // A validated grinder phase can lead straight into pumping/heating.
        phase = "brewing";flowSampleAt = now;quietAt = -1;
        print("Erkennung grinding -> brewing: Heizung/Pumpe bleibt aktiv");
      } else if (now - grindLast > 3000) {
        resetDetection("grinder_transition_rejected");
      }
    }
    return;
  }
  if (phase === "waiting") {
    if (now - grindLast > 25000) {resetDetection("no_flow_after_grinding");return;}
    // A low phase followed by sustained pump operation, with optional heating.
    if (power >= FLOW_W && now - quietAt >= 1000) {
      phase = "brewing";flowSampleAt = now;quietAt = -1;
    }
    return;
  }
  if (phase === "brewing") {
    // Count active samples rather than elapsed time between isolated bursts.
    if (power >= FLOW_W) {
      let elapsed = now - flowSampleAt;
      if (elapsed > SAMPLE_MS) elapsed = SAMPLE_MS;
      flowActiveMs += elapsed;
    }
    flowSampleAt = now;
    if (flowActiveMs >= FLOW_MIN_MS) flowConfirmed = true;
    if (power < 20) {
      if (quietAt < 0) quietAt = now;
      if (now - quietAt >= FINISH_QUIET_MS) {
        if (flowConfirmed) {
          storeBrew(quietAt);
          resetDetection("brew_confirmed");
        } else if (now - grindLast <= 25000) {
          // A brief heater/pump pulse followed by a pre-infusion pause
          // can precede the actual flow, as in the recorded small coffee.
          phase = "waiting";flowActiveMs = 0;flowSampleAt = 0;
          lastDecision = "preinfusion_pause";lastDecisionAt = now;
          print("Erkennung: Vorbruehpause, wartet auf Fortsetzung");
        } else resetDetection("flow_too_short");
      }
    } else quietAt = -1;
  }
}

function loadQueue(offset) {
  Shelly.call("KVS.GetMany", {match: "cafe_brew_*", offset: offset}, function (result, error) {
    if (error || !result || !result.items) {
      fault = "KVS-Lesen fehlgeschlagen";print(fault);return;
    }
    let count = 0;
    for (let key in result.items) {
      let value = result.items[key].value;
      if (!value || typeof value.id !== "string" || typeof value.end !== "number"
          || typeof value.boot !== "string" || typeof value.epoch !== "number"
          || queue.length >= MAX_EVENTS) {
        fault = "Ungueltiger Kaffee-Puffer";print(fault);return;
      }
      queue.push({key: key, value: value, saved: true});
      count++;
    }
    if (offset + count < result.total) {
      if (!count) {fault = "KVS-Seite leer";return;}
      loadQueue(offset + count);
    } else {
      loaded = true;
      print("Cafe_R107 bereit, gepufferte Bezüge:", queue.length);
    }
  });
}

function persistNext() {
  if (storageBusy || !loaded) return;
  for (let i = 0; i < queue.length; i++) {
    if (!queue[i].saved) {
      let item = queue[i];
      storageBusy = true;
      Shelly.call("KVS.Set", {key: item.key, value: item.value}, function (result, error) {
        storageBusy = false;
        if (error) {fault = "KVS-Schreiben fehlgeschlagen";print(fault);return;}
        item.saved = true;
        if (fault === "KVS-Schreiben fehlgeschlagen") fault = "";
      });
      return;
    }
  }
}

function sendMessage(now) {
  if (httpBusy || API_KEY === "HIER_SHELLY_SCHLUESSEL_EINTRAGEN") return;
  let heartbeat = now - lastHeartbeat >= 5000;
  let item = null;
  if (!heartbeat) {
    for (let i = 0; i < queue.length; i++) {
      if (queue[i].saved && !queue[i].deleting) {item = queue[i];break;}
    }
    if (item === null) return;
  }
  let payload = {type: "heartbeat", device: device, mac: deviceInfo.mac, healthy: loaded && !fault && queue.length < MAX_EVENTS,
    power: lastPower, state: phase, error: fault};
  if (heartbeat) {
    let samples = [];
    let count = traceTimes.length;
    if (count > 5) count = 5;
    for (let i = count; i > 0; i--) {
      let index = (traceNext + traceCapacity - i) % traceCapacity;
      let watts = tracePowers[index];
      if (watts !== null && watts >= 0 && watts <= 10000) watts = Math.round(watts);
      else watts = null;
      samples.push([now - traceTimes[index], watts, recentPhases[(recentNext + 5 - i) % 5]]);
    }
    payload.samples = samples;
  }
  if (item !== null) {
    let event = item.value;
    let age = event.boot === boot ? now - event.end : -1;
    let epoch = epochAt(now);
    if (age < 0 && event.epoch && epoch) age = Math.max(0, (epoch - event.epoch) * 1000);
    payload = {type: "brew", device: device, event: event.id, epoch: event.epoch, ageMs: age};
  }
  httpBusy = true;
  if (heartbeat) lastHeartbeat = now;
  Shelly.call("HTTP.Request", {method: "POST", url: ESP_URL, timeout: 2,
    headers: {"Content-Type": "application/json", "X-Cafe-Shelly": API_KEY},
    body: JSON.stringify(payload)}, function (result, error) {
    httpBusy = false;
    if (error || !result || result.code !== 200) return;
    let reply;
    try {reply = JSON.parse(result.body);} catch (e) {return;}
    if (!reply.ok) return;
    if (reply.epoch > 1704067200) {clockEpoch = reply.epoch;clockAt = Shelly.getUptimeMs();}
    if (item === null || reply.event !== item.value.id || storageBusy) return;
    storageBusy = true;item.deleting = true;
    Shelly.call("KVS.Delete", {key: item.key}, function (result, error) {
      storageBusy = false;item.deleting = false;
      if (error) {print("Bestaetigter Bezug wird erneut gesendet");return;}
      for (let i = 0; i < queue.length; i++) {
        if (queue[i].key === item.key) {queue.splice(i, 1);break;}
      }
    });
  });
}

let previousSample = 0;
Timer.set(SAMPLE_MS, true, function () {
  let now = Shelly.getUptimeMs();
  let status = Shelly.getComponentStatus("switch:0");
  if (previousSample && now - previousSample > 2500) resetDetection("sample_gap");
  previousSample = now;
  if (status && typeof status.apower === "number") {
    lastPower = status.apower;
    if (loaded) detect(now, lastPower);
    if (fault === "Leistungsmessung fehlt") fault = "";
  } else {
    resetDetection("power_unavailable");fault = "Leistungsmessung fehlt";
  }
  traceTimes[traceNext] = now;
  tracePowers[traceNext] = status && typeof status.apower === "number" ? status.apower : null;
  let phaseCode = 1;
  if (phase === "grinding") phaseCode = 2;
  else if (phase === "waiting") phaseCode = 3;
  else if (phase === "brewing") phaseCode = 4;
  recentPhases[recentNext] = phaseCode;
  recentNext = (recentNext + 1) % 5;
  traceNext = (traceNext + 1) % traceCapacity;
  persistNext();
  sendMessage(now);
});

HTTPServer.registerEndpoint("status", function (request, response) {
  response.code = 200;
  response.headers = [["Content-Type", "application/json"]];
  response.body = JSON.stringify({version: "1.1.5", device: device, ready: loaded,
    state: phase, power: lastPower, queued: queue.length, error: fault,
    recognized: recognizedCount, lastDecision: lastDecision, lastDecisionAtMs: lastDecisionAt,
    configured: API_KEY !== "HIER_SHELLY_SCHLUESSEL_EINTRAGEN"});
  response.send();
});
HTTPServer.registerEndpoint("trace", function (request, response) {
  let csv = "uptime_ms;leistung_W\r\n";
  let oldest = traceTimes.length === traceCapacity ? traceNext : 0;
  for (let i = 0; i < traceTimes.length; i++) {
    let index = (oldest + i) % traceCapacity;
    csv += traceTimes[index] + ";";
    if (tracePowers[index] !== null) csv += Math.round(tracePowers[index] * 10) / 10;
    csv += "\r\n";
  }
  response.code = 200;
  response.headers = [
    ["Content-Type", "text/csv; charset=utf-8"],
    ["Content-Disposition", "attachment; filename=cafe-r107-trace.csv"],
    ["Cache-Control", "no-store"]
  ];
  response.body = csv;
  response.send();
});
loadQueue(0);
