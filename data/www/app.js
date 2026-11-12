import {displayTime,displayChipTime,selectedPeriods,configPayload} from '/domain.js';
const $=s=>document.querySelector(s);
let registry={people:[],cards:[],pending:[]},status={},before=null,oldest=null,pendingCount=-1,refreshing=false,lastSeenRevision=-1,lastTimeReady=false;
function el(tag,text,cls){const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(cls)n.className=cls;return n;}
function notify(text,bad=false){const n=$('#notice');n.textContent=text;n.className=bad?'error':'';n.hidden=false;}
async function api(path,payload){
  const opts={cache:'no-store'};
  if(payload!==undefined){opts.method='POST';opts.headers={'Content-Type':'application/json','X-Cafe-Request':'1'};opts.body=JSON.stringify(payload);}
  const r=await fetch('/api/'+path,opts);
  const d=await r.json().catch(()=>({error:'Antwort des Geräts nicht lesbar'}));
  if(!r.ok)throw new Error(d.error||`HTTP ${r.status}`);return d;
}
async function action(button,fn){button.disabled=true;try{await fn();}catch(e){notify(e.message,true);}finally{button.disabled=false;}}
function empty(target,text,cols=1){target.replaceChildren();const tr=el('tr');const td=el('td',text,'empty');td.colSpan=cols;tr.append(td);target.append(tr);}
function fillOptions(select,items,current){select.replaceChildren();for(const value of items){const o=el('option',value);o.value=value;select.append(o);}if(current)select.value=current;}
function sensorSummary(d){
  if(!d)return 'Keine Diagnose verfügbar';
  const state=d.healthy?'Bereit':(d.error||'Warte auf Messung');
  return `${state} · Wiederherstellungen: ${d.successfulReinitializations}/${d.recoveryAttempts}`;
}
async function loadStatus(){
  status=await api('status');$('#dot').classList.add('ok');$('#connection').textContent=status.staConnected?'Im Büro-WLAN':'Access Point aktiv';$('#version').textContent='Version '+status.version;
  const warnings=[];if(status.lastSeenError)warnings.push(status.lastSeenError);if(status.storageError)warnings.push(status.storageError);if(!status.readerOK)warnings.push('Der RFID-Leser antwortet nicht.');if(!status.epoch)warnings.push('Uhrzeit fehlt. Bitte Browserzeit übernehmen oder WLAN verbinden.');
  if(status.fsTotal && status.fsUsed/status.fsTotal>.85)warnings.push('Speicher fast voll. Bitte sichern und archivieren.');
  $('#health').textContent=warnings.join(' ');$('#health').hidden=!warnings.length;
  $('#pendingBadge').textContent=status.pendingCount;$('#pendingBadge').hidden=!status.pendingCount;
  const info=$('#deviceInfo');info.replaceChildren();
  const entries=[['Access Point',status.apSsid],['AP-Adresse',status.apIP],['WLAN-Modus',status.apOnly?'Nur Access Point':'Access Point + WLAN-Client'],['Gespeicherter Modus',status.configuredApOnly?'Nur Access Point':'Access Point + WLAN-Client'],['WLAN-Client',status.staConnected?`${status.activeSsid||status.ssid} · ${status.staIP}`:'Nicht verbunden'],['Gerätezeit',displayTime(status.epoch)],['LittleFS',`${Math.round(status.fsUsed/1024)} / ${Math.round(status.fsTotal/1024)} KiB`],['TOF',sensorSummary(status.tofDiagnostics)],['RFID',sensorSummary(status.rfidDiagnostics)]];
  for(const [label,value]of entries)info.append(el('dt',label),el('dd',value));
  const clients=$('#apClientList');clients.replaceChildren();
  for(const client of status.apClientList||[]){
    const row=el('tr');row.append(el('td',client.shelly?'Shelly':'WLAN-Client'),el('td',client.mac));
    const cell=el('td');
    if(client.ip && client.ip!=='0.0.0.0'){
      const link=el('a',client.ip);link.href='http://'+client.ip+'/';link.target='_blank';link.rel='noopener';cell.append(link);
    }else cell.textContent='IP noch nicht bekannt';
    row.append(cell);clients.append(row);
  }
  if(!status.apClientList?.length)empty(clients,'Keine AP-Clients verbunden.',3);
  const shelly=status.shelly||{};
  const overview=$('#shellyOverview');
  overview.dataset.online=String(Boolean(shelly.online));
  $('#shellyOverviewState').textContent=shelly.online?'Erkennung aktiv':'Sofortbuchung per Chip aktiv';
  const phases={idle:'Wartet auf Kaffee',grinding:'Mahlen erkannt',waiting:'Wartet auf Brühphase',brewing:'Brühphase erkannt'};
  $('#shellyOverviewPhase').textContent=shelly.online?(phases[shelly.state]||shelly.state||'Bereit'):'Shelly nicht bereit oder nicht erreichbar';
  $('#shellyOverviewPower').textContent=shelly.online && Number.isFinite(shelly.power)?Math.round(shelly.power)+' W':'–';
  $('#shellyOverviewError').textContent=shelly.error||'';
  $('#shellyOverviewError').hidden=!shelly.error;
  $('#shellyKey').textContent=shelly.key||'Nicht verfügbar';
  $('#shellyStatus').textContent=(shelly.online?'Erkennung aktiv':'Sofortbuchung per Chip aktiv')
    +(shelly.device?' · '+shelly.device:'')+(shelly.state?' · '+shelly.state:'')
    +(shelly.online?' · '+Math.round(shelly.power||0)+' W':'')
    +(shelly.error?' · '+shelly.error:'')+' · Offene Bezüge: '+(shelly.openBrews||0);
  const url=new URL(location.href);url.port=status.filemanagerPort;url.pathname='/';url.search='';url.hash='';$('#filemanager').href=url.href;
  if(!$('#configForm').dataset.loaded){
    const f=$('#configForm');f.elements.apOnly.checked=status.configuredApOnly??status.apOnly??false;
    const networks=status.wifiNetworks||[{ssid:status.ssid||''}];
    for(let i=0;i<3;i++)f.elements['ssid'+i].value=networks[i]?.ssid||'';
    f.dataset.loaded='1';
  }
  const revision=status.lastSeenRevision??0,timeReady=Boolean(status.epoch);
  const editing=$('#people').contains(document.activeElement)&&document.activeElement.matches('input,select');
  if(!editing && (pendingCount!==status.pendingCount || lastSeenRevision!==revision || lastTimeReady!==timeReady)){
    await loadRegistry();pendingCount=status.pendingCount;lastSeenRevision=revision;lastTimeReady=timeReady;
  }
}
async function loadReport(){
  const q=new URLSearchParams();if($('#weekSelect').value)q.set('week',$('#weekSelect').value);if($('#monthSelect').value)q.set('month',$('#monthSelect').value);
  const d=await api('report?'+q);const t=d.totals;
  $('#today').textContent=t.today;$('#weekTotal').textContent=t.week;$('#monthTotal').textContent=t.month;$('#total').textContent=t.total;$('#weekLabel').textContent=d.week||'Noch keine Gerätezeit';$('#monthLabel').textContent=d.month||'Noch keine Gerätezeit';
  const periods=selectedPeriods(d.weeks,d.months,d.week,d.month);fillOptions($('#weekSelect'),periods.weeks,d.week);fillOptions($('#monthSelect'),periods.months,d.month);
  const target=$('#summary');target.replaceChildren();
  for(const p of d.people){const row=el('tr');for(const key of ['name','today','week','month','total'])row.append(el('td',p[key]));target.append(row);}
  if(!d.people.length)empty(target,'Noch keine Personen. Unter „Personen & Chips“ die erste Person anlegen.',5);
  $('#undated').hidden=!t.undated;$('#undated').textContent=`${t.undated} Kaffee(s) ohne bekannte Uhrzeit: im Gesamtwert enthalten, nicht in Tages-, Wochen- oder Monatswerten. Bei bereits beendetem Gerätestart ist die Zeit nicht automatisch rekonstruierbar.`;
}
async function loadHistory(){
  const d=await api('history'+(before?'?before='+before:'')),target=$('#history');target.replaceChildren();oldest=null;
  for(const r of d.rows){oldest=r.id;const row=el('tr');row.append(el('td',displayTime(r.epoch)),el('td',r.name),el('td',r.cancelled?'Storniert':'+1 Kaffee'));const td=el('td');
    if(!r.cancelled){const b=el('button','Stornieren','secondary');b.type='button';b.addEventListener('click',()=>action(b,async()=>{if(!confirm(`Buchung #${r.id} für ${r.name} stornieren?`))return;await api('cancel',{id:r.id});await Promise.all([loadReport(),loadHistory()]);notify('Buchung storniert.');}));td.append(b);}row.append(td);target.append(row);}
  if(!d.rows.length)empty(target,'Noch keine Buchungen.',4);$('#older').hidden=!d.more;
}
function personSelect(current){const s=el('select');s.setAttribute('aria-label','Person zuordnen');const blank=el('option','Person auswählen …');blank.value='';s.append(blank);for(const p of registry.people){const o=el('option',p.name);o.value=p.id;s.append(o);}s.value=current||'';return s;}
function chipRow(uid,person,unknown=false){const row=el('div',undefined,'editrow');const identity=el('div',undefined,'chipidentity');identity.append(el('code',uid),el('small',displayChipTime(registry.lastSeen?.[uid]),'chiptime'));row.append(identity);const select=personSelect(person);row.append(select);const assign=el('button',unknown?'Zuordnen':'Ändern');assign.type='button';assign.addEventListener('click',()=>action(assign,async()=>{if(!select.value)throw new Error('Bitte eine Person auswählen.');await api('assign',{uid,person:select.value});await loadRegistry();await refresh();notify('Chip zugeordnet. Der nächste Chipkontakt wird für die Kaffeeerfassung verwendet.');}));row.append(assign);const forget=el('button',unknown?'Verwerfen':'Entfernen','secondary');forget.type='button';forget.addEventListener('click',()=>action(forget,async()=>{if(!confirm('Chip-Zuordnung entfernen? Alte Buchungen bleiben erhalten.'))return;await api('forget',{uid});await loadRegistry();notify('Chip entfernt.');}));row.append(forget);return row;}
async function loadRegistry(){
  registry=await api('registry');const p=$('#pending'),c=$('#cardList'),list=$('#personList');p.replaceChildren();c.replaceChildren();list.replaceChildren();
  if(!registry.pending.length)p.append(el('p','Keine neuen Chips. Zum Anlernen einen Chip vorhalten.','hint'));
  for(const uid of registry.pending)p.append(chipRow(uid,'',true));
  if(!registry.cards.length)c.append(el('p','Noch keine Chips zugeordnet.','hint'));
  for(const card of registry.cards)c.append(chipRow(card.uid,card.person));
  for(const person of registry.people){const form=el('form',undefined,'editrow'),input=el('input');input.value=person.name;input.maxLength=48;input.required=true;input.setAttribute('aria-label','Name von '+person.name);const b=el('button','Speichern','secondary');form.append(input,b);form.addEventListener('submit',e=>{e.preventDefault();action(b,async()=>{await api('person',{id:person.id,name:input.value});await loadRegistry();await Promise.all([loadReport(),loadHistory()]);notify('Name gespeichert.');});});list.append(form);}
}
async function refresh(){if(refreshing)return;refreshing=true;try{await loadStatus();await Promise.all([loadReport(),loadHistory()]);}catch(e){$('#dot').classList.remove('ok');$('#connection').textContent='Verbindung prüfen';notify(e.message,true);}finally{refreshing=false;}}
for(const b of document.querySelectorAll('[data-tab]'))b.addEventListener('click',()=>{for(const t of document.querySelectorAll('.tab'))t.hidden=t.id!==b.dataset.tab;for(const n of document.querySelectorAll('[data-tab]'))n.classList.toggle('active',n===b);});
$('#personForm').addEventListener('submit',e=>{e.preventDefault();const f=e.currentTarget;action(f.querySelector('button'),async()=>{await api('person',{name:f.elements.name.value});f.reset();await loadRegistry();await loadReport();notify('Person angelegt. Jetzt einen Chip zuordnen.');});});
$('#configForm').elements.apOnly.addEventListener('change',async e=>{
  const checkbox=e.currentTarget,requested=checkbox.checked,button=$('#configForm').querySelector('button');
  checkbox.disabled=true;button.disabled=true;
  try {
    await api('config',{apOnly:requested});
    notify(requested?'Gespeichert: Nur Access Point. Die WLAN-Client-Verbindung wird jetzt beendet.':'Gespeichert: WLAN-Client wieder freigegeben. Die WLAN-Verbindung wird auch bei verbundenen AP-Clients wiederhergestellt.');
  } catch(error) {
    checkbox.checked=!requested;notify('Modus nicht bestätigt: '+error.message+'. Bitte Seite neu laden und gespeicherten Modus prüfen.',true);
  } finally {checkbox.disabled=false;button.disabled=false;}
});
$('#configForm').addEventListener('submit',e=>{e.preventDefault();const f=e.currentTarget;action(f.querySelector('button'),async()=>{
  const value=Object.fromEntries(new FormData(f));value.apOnly=f.elements.apOnly.checked;
  for(let i=0;i<3;i++)value['clearWifiPassword'+i]=f.elements['clearWifiPassword'+i].checked;
  const result=await api('config',configPayload(value));
  for(const n of ['wifiPassword0','wifiPassword1','wifiPassword2','apPassword','adminPassword'])f.elements[n].value='';
  for(let i=0;i<3;i++)f.elements['clearWifiPassword'+i].checked=false;
  notify(result.restartRequired?'Gespeichert. WLAN-Modus wird sofort übernommen; für geänderte Zugänge oder Kennwörter bitte neu starten.':'Gespeichert. WLAN-Modus wird sofort übernommen.');
});});
$('#setTime').addEventListener('click',e=>action(e.currentTarget,async()=>{await api('time',{epoch:Math.floor(Date.now()/1000)});await refresh();notify('Browserzeit übernommen.');}));
$('#weekSelect').addEventListener('change',()=>loadReport().catch(e=>notify(e.message,true)));$('#monthSelect').addEventListener('change',()=>loadReport().catch(e=>notify(e.message,true)));
$('#older').addEventListener('click',()=>{before=oldest;loadHistory().catch(e=>notify(e.message,true));});$('#newest').addEventListener('click',()=>{before=null;loadHistory().catch(e=>notify(e.message,true));});
refresh();setInterval(()=>{if(!document.hidden)refresh();},8000);


const powerChart=$('#powerChart');
const powerSpan=$('#powerSpan'),powerOffset=$('#powerOffset');
const powerSamples=new Map();
const phaseColors={idle:'#777068',grinding:'#b46a1b',waiting:'#7955a0',brewing:'#2d6993'};
const powerClock=new Intl.DateTimeFormat('de-DE',{timeZone:'Europe/Berlin',hour:'2-digit',minute:'2-digit',second:'2-digit'});
let powerSession='',powerNow=0,powerEpoch=0,loadingPower=false;

function drawPowerChart(){
  const span=Number(powerSpan.value)*60,offset=Number(powerOffset.value);
  const end=powerNow-offset,start=end-span+1;
  const width=powerChart.clientWidth,height=powerChart.clientHeight;
  if(!width||!height)return;
  const ratio=window.devicePixelRatio||1;
  powerChart.width=Math.round(width*ratio);
  powerChart.height=Math.round(height*ratio);
  const ctx=powerChart.getContext('2d');
  ctx.scale(ratio,ratio);
  ctx.font='12px sans-serif';
  const left=49,right=width-29,top=14,bottom=height-49;
  const x=second=>left+(second-start)/(span-1)*(right-left);
  let maxPower=0;
  for(let second=start;second<=end;second++)maxPower=Math.max(maxPower,powerSamples.get(second)?.watts??0);
  const step=maxPower<=100?25:maxPower<=500?125:maxPower<=2000?500:2500;
  const limit=Math.max(step*2,Math.ceil(maxPower/step)*step);
  const y=watts=>bottom-watts/limit*(bottom-top);

  ctx.textAlign='right';
  for(let watts=0;watts<=limit;watts+=step){
    const row=y(watts);
    ctx.strokeStyle='#ded5ca';ctx.beginPath();ctx.moveTo(left,row);ctx.lineTo(right,row);ctx.stroke();
    ctx.fillStyle='#55463b';ctx.fillText(watts+' W',left-5,row+4);
  }

  const ticks=width<530?3:5;
  ctx.textAlign='center';
  for(let i=0;i<ticks;i++){
    const second=start+Math.round(i*(span-1)/(ticks-1));
    const label=powerEpoch?powerClock.format((powerEpoch-(powerNow-second))*1000):'−'+(powerNow-second)+' s';
    ctx.fillText(label,Math.min(width-27,Math.max(27,x(second))),bottom+34);
  }

  ctx.save();
  ctx.beginPath();ctx.rect(left,top,right-left+1,bottom-top+16);ctx.clip();
  ctx.strokeStyle='#55463b';ctx.lineWidth=1.35;
  ctx.beginPath();
  let continuous=false,visible=0;
  for(let second=start;second<=end;second++){
    const sample=powerSamples.get(second);
    if(!sample||sample.watts===null){continuous=false;continue;}
    const col=x(second),row=y(sample.watts);
    if(continuous)ctx.lineTo(col,row);
    else {ctx.moveTo(col,row);ctx.lineTo(col+.4,row);}
    continuous=true;visible++;
  }
  ctx.stroke();
  for(let second=start;second<=end;second++){
    const sample=powerSamples.get(second);
    if(!sample)continue;
    const col=x(second);
    if(phaseColors[sample.phase]){
      ctx.fillStyle=phaseColors[sample.phase];
      ctx.fillRect(col,bottom+5,Math.max(1,(right-left)/(span-1)),8);
    }
    if(sample.brew){
      ctx.strokeStyle='#307557';ctx.setLineDash([4,3]);
      ctx.beginPath();ctx.moveTo(col,top);ctx.lineTo(col,bottom+14);ctx.stroke();
      ctx.setLineDash([]);
    }
  }
  ctx.restore();
  $('#powerStatus').textContent=visible
    ? (powerEpoch?'Uhrzeit Berlin':'Zeit relativ zur letzten Aktualisierung')+' · Lücken zeigen fehlende Messwerte.'
    : 'Keine Leistungsmesswerte in diesem Zeitraum.';
}

function updatePowerWindow(){
  const max=60-Number(powerSpan.value);
  powerOffset.max=max;
  $('#powerOffsetLabel').hidden=!max;
  $('#powerOffsetValue').textContent=Number(powerOffset.value)?powerOffset.value+' min zurück':'Aktuell';
  drawPowerChart();
}

async function loadPowerHistory(){
  if(loadingPower || document.hidden || $('#power').hidden)return;
  loadingPower=true;
  try{
    const query=new URLSearchParams({session:powerSession,since:powerNow});
    const response=await fetch('/api/power.csv?'+query,{cache:'no-store'});
    if(!response.ok)throw new Error('Messwerte konnten nicht geladen werden.');
    const session=response.headers.get('X-Cafe-Session');
    const now=Number(response.headers.get('X-Cafe-Uptime'));
    const epoch=Number(response.headers.get('X-Cafe-Epoch'));
    if(!session||!Number.isInteger(now))throw new Error('Der ESP liefert keinen Leistungsverlauf.');
    const rows=(await response.text()).trimEnd().split(/\r?\n/);
    if(rows.shift()!=='uptime_s;epoch;leistung_W;phase;kaffee')throw new Error('Messdaten sind nicht lesbar.');
    if(session!==powerSession || now<powerNow)powerSamples.clear();
    powerSession=session;powerNow=now;powerEpoch=epoch;
    for(const row of rows){
      const [second,,watts,phase,brew]=row.split(';');
      powerSamples.set(Number(second),{watts:watts===''?null:Number(watts),phase,brew:brew==='1'});
    }
    for(const second of powerSamples.keys())if(second<now-3599)powerSamples.delete(second);
    drawPowerChart();
  }catch(e){$('#powerStatus').textContent=e.message;}
  finally{loadingPower=false;}
}

powerSpan.addEventListener('change',updatePowerWindow);
powerOffset.addEventListener('input',updatePowerWindow);
document.querySelector('[data-tab="power"]').addEventListener('click',()=>{
  updatePowerWindow();
  loadPowerHistory();
});
window.addEventListener('resize',drawPowerChart);
setInterval(loadPowerHistory,5000);
