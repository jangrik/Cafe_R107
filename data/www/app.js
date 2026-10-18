import {displayTime,selectedPeriods,configPayload} from '/domain.js';
const $=s=>document.querySelector(s);
let registry={people:[],cards:[],pending:[]},status={},before=null,oldest=null,pendingCount=-1,refreshing=false;
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
async function loadStatus(){
  status=await api('status');$('#dot').classList.add('ok');$('#connection').textContent=status.staConnected?'Im Büro-WLAN':'Access Point aktiv';$('#version').textContent='Version '+status.version;
  const warnings=[];if(status.storageError)warnings.push(status.storageError);if(!status.readerOK)warnings.push('Der RFID-Leser antwortet nicht.');if(!status.epoch)warnings.push('Uhrzeit fehlt. Bitte Browserzeit übernehmen oder WLAN verbinden.');
  if(status.fsTotal && status.fsUsed/status.fsTotal>.85)warnings.push('Speicher fast voll. Bitte sichern und archivieren.');
  $('#health').textContent=warnings.join(' ');$('#health').hidden=!warnings.length;
  $('#pendingBadge').textContent=status.pendingCount;$('#pendingBadge').hidden=!status.pendingCount;
  const info=$('#deviceInfo');info.replaceChildren();
  const entries=[['Access Point',status.apSsid],['AP-Adresse',status.apIP],['WLAN-Client',status.staConnected?`${status.ssid} · ${status.staIP}`:'Nicht verbunden'],['Gerätezeit',displayTime(status.epoch)],['LittleFS',`${Math.round(status.fsUsed/1024)} / ${Math.round(status.fsTotal/1024)} KiB`]];
  for(const [label,value]of entries)info.append(el('dt',label),el('dd',value));
  const url=new URL(location.href);url.port=status.filemanagerPort;url.pathname='/';url.search='';url.hash='';$('#filemanager').href=url.href;
  if(!$('#configForm').dataset.loaded){$('#configForm').elements.ssid.value=status.ssid;$('#configForm').dataset.loaded='1';}
  if(pendingCount!==status.pendingCount){pendingCount=status.pendingCount;await loadRegistry();}
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
function chipRow(uid,person,unknown=false){const row=el('div',undefined,'editrow');row.append(el('code',uid));const select=personSelect(person);row.append(select);const assign=el('button',unknown?'Zuordnen':'Ändern');assign.type='button';assign.addEventListener('click',()=>action(assign,async()=>{if(!select.value)throw new Error('Bitte eine Person auswählen.');await api('assign',{uid,person:select.value});await loadRegistry();await refresh();notify('Chip zugeordnet. Der nächste Chipkontakt bucht einen Kaffee.');}));row.append(assign);const forget=el('button',unknown?'Verwerfen':'Entfernen','secondary');forget.type='button';forget.addEventListener('click',()=>action(forget,async()=>{if(!confirm('Chip-Zuordnung entfernen? Alte Buchungen bleiben erhalten.'))return;await api('forget',{uid});await loadRegistry();notify('Chip entfernt.');}));row.append(forget);return row;}
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
$('#configForm').addEventListener('submit',e=>{e.preventDefault();const f=e.currentTarget;action(f.querySelector('button'),async()=>{const value=Object.fromEntries(new FormData(f));value.clearWifiPassword=f.elements.clearWifiPassword.checked;await api('config',configPayload(value));for(const n of ['wifiPassword','apPassword','adminPassword'])f.elements[n].value='';notify('Gespeichert. Bitte ESP32 per Reset neu starten. Danach gelten die neuen Einstellungen.');});});
$('#setTime').addEventListener('click',e=>action(e.currentTarget,async()=>{await api('time',{epoch:Math.floor(Date.now()/1000)});await refresh();notify('Browserzeit übernommen.');}));
$('#weekSelect').addEventListener('change',()=>loadReport().catch(e=>notify(e.message,true)));$('#monthSelect').addEventListener('change',()=>loadReport().catch(e=>notify(e.message,true)));
$('#older').addEventListener('click',()=>{before=oldest;loadHistory().catch(e=>notify(e.message,true));});$('#newest').addEventListener('click',()=>{before=null;loadHistory().catch(e=>notify(e.message,true));});
refresh();setInterval(()=>{if(!document.hidden)refresh();},8000);
