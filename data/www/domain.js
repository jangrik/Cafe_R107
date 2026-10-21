export function displayTime(epoch) {
  return epoch ? new Intl.DateTimeFormat('de-DE',{timeZone:'Europe/Berlin',day:'2-digit',month:'2-digit',year:'numeric',hour:'2-digit',minute:'2-digit'}).format(new Date(epoch*1000)) : 'Uhrzeit noch offen';
}
export function selectedPeriods(weeks,months,week,month) {
  return {weeks:[...new Set([week,...weeks].filter(Boolean))].sort().reverse(),months:[...new Set([month,...months].filter(Boolean))].sort().reverse()};
}
export function configPayload(form) {
  if(Object.hasOwn(form,'apOnly')) {
    const wifiNetworks=Array.from({length:3},(_,i)=>{
      const n={ssid:(form['ssid'+i]||'').trim()};
      if(form['clearWifiPassword'+i])n.password='';
      else if(form['wifiPassword'+i])n.password=form['wifiPassword'+i];
      return n;
    });
    const d={apOnly:Boolean(form.apOnly),wifiNetworks};
    if(form.apPassword)d.apPassword=form.apPassword;
    if(form.adminPassword)d.adminPassword=form.adminPassword;
    return d;
  }
  const d={ssid:form.ssid.trim()};
  if(form.clearWifiPassword)d.wifiPassword='';
  else if(form.wifiPassword)d.wifiPassword=form.wifiPassword;
  if(form.apPassword)d.apPassword=form.apPassword;
  if(form.adminPassword)d.adminPassword=form.adminPassword;
  return d;
}

export function displayChipTime(epoch) {
  if(epoch===undefined || epoch===null)return '—';
  if(!epoch)return 'Uhrzeit unbekannt';
  return new Intl.DateTimeFormat('de-DE',{
    timeZone:'Europe/Berlin',day:'2-digit',month:'2-digit',year:'numeric',
    hour:'2-digit',minute:'2-digit',second:'2-digit'
  }).format(new Date(epoch*1000));
}

