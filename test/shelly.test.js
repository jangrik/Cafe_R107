import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const source=fs.readFileSync(new URL('../shelly/coffee-detector.js',import.meta.url),'utf8');
function detector() {
  let now=0,power=0;
  const kv={},timers=[],endpoints={};
  const context=vm.createContext({
    Math,JSON,print:()=>{},
    Timer:{set:(ms,repeat,fn)=>{timers.push(fn);return timers.length;}},
    HTTPServer:{registerEndpoint:(name,fn)=>{endpoints[name]=fn;}},
    Shelly:{
      getDeviceInfo:()=>({id:'shellyplugmg3-test'}),
      getCurrentScriptId:()=>1,getUptimeMs:()=>now,
      getComponentStatus:name=>name==='switch:0'?{apower:power}:{unixtime:null},
      call:(method,args,callback)=>{
        if(method==='KVS.GetMany')callback({items:{},total:0},0);
        else if(method==='KVS.Set'){kv[args.key]=args.value;callback({},0);}
        else if(method==='HTTP.Request')callback(null,1);
        else throw Error(method);
      }
    }
  });
  vm.runInContext(source,context);
  return {
    sample(t,p){now=t;power=p;timers[0]();},kv,context,
    request(name){const response={send(){}};endpoints[name]({},response);return response;}
  };
}
test('detects grinder alone and grinder plus heater, ignores hot water and flushing',()=>{
  const d=detector();
  let now=1000;
  function samples(power,count){for(let i=0;i<count;i++){d.sample(now,power);now+=1000;}}
  samples(1400,12);samples(55,8);samples(1,5); // startup and flushing
  samples(130,7);samples(2,5);samples(1400,20);samples(1,5);
  samples(1,10);
  samples(1530,7);samples(219,1);samples(2,5);samples(1450,25);samples(1,5);
  samples(1400,7);samples(34,1);samples(1440,100);samples(1,10); // hot water
  samples(130,1);samples(1,5);samples(1400,20);samples(1,5); // isolated spike
  assert.equal(Object.keys(d.kv).length,2);
});
test('a second warm coffee is recognized without sustained heater power',()=>{
  const d=detector();
  let now=1000;
  function samples(power,count){for(let i=0;i<count;i++){d.sample(now,power);now+=1000;}}
  // First coffee with heating, then a warm coffee driven by the pump alone.
  samples(130,7);samples(2,5);samples(1450,20);samples(1,5);
  samples(1,5);
  samples(130,7);samples(3,5);samples(55,20);samples(1,5);
  assert.equal(Object.keys(d.kv).length,2);
  // A rinse and hot water without a grinder must not create another coffee.
  samples(55,15);samples(1,5);samples(1450,20);samples(1,5);
  assert.equal(Object.keys(d.kv).length,2);
});
test('short pump bursts after grinding do not confirm a coffee',()=>{
  const d=detector();
  let now=1000;
  function samples(power,count){for(let i=0;i<count;i++){d.sample(now,power);now+=1000;}}
  samples(130,7);samples(3,5);samples(55,3);samples(1,5);
  assert.equal(Object.keys(d.kv).length,0);
});
test('warm grinder cycles continue directly into heating without a quiet pause',()=>{
  const d=detector();
  let now=1000;
  function samples(power,count){for(let i=0;i<count;i++){d.sample(now,power);now+=1000;}}
  samples(1530,7);samples(1000,30);samples(1,5);
  assert.equal(Object.keys(d.kv).length,1);
  samples(1530,7);samples(1400,10);samples(1000,20);
  assert.equal(JSON.parse(d.request('status').body).state,'brewing');
  samples(1,5);
  assert.equal(Object.keys(d.kv).length,2);
  assert.equal(JSON.parse(d.request('status').body).lastDecision,'brew_confirmed');
  // Heating and a rinse without a grinder still do not confirm another coffee.
  samples(1400,20);samples(55,10);samples(1,5);
  assert.equal(Object.keys(d.kv).length,2);
});
test('isolated flow bursts do not become a full brew just through elapsed time',()=>{
  const d=detector();
  let now=1000;
  function samples(power,count){for(let i=0;i<count;i++){d.sample(now,power);now+=1000;}}
  samples(130,7);samples(2,3);samples(55,1);samples(1,3);
  samples(55,1);samples(1,5);
  assert.equal(Object.keys(d.kv).length,0);
  assert.equal(JSON.parse(d.request('status').body).lastDecision,'flow_too_short');
});
test('diagnostic CSV retains only the last 180 samples, including missing values',()=>{
  const d=detector();
  for(let i=1;i<=200;i++)d.sample(i*1000,1);
  d.sample(201000,null);
  const csv=d.request('trace').body.trimEnd().split('\r\n');
  assert.equal(csv.length,181);
  assert.equal(csv[0],'uptime_ms;leistung_W');
  assert.equal(csv[1],'22000;1');
  assert.equal(csv.at(-1),'201000;');
});
test('sampling interruption discards an incomplete candidate',()=>{
  const d=detector();
  for(let i=0;i<7;i++)d.sample(i*1000+1000,130);
  d.sample(15000,1);
  for(let i=16;i<40;i++)d.sample(i*1000,1400);
  for(let i=40;i<46;i++)d.sample(i*1000,1);
  assert.equal(Object.keys(d.kv).length,0);
});

test('queued event survives restart and is deleted only after matching ESP acknowledgement',()=>{
  let now=100000,tick,mode='unreachable',deleted=0;
  const saved={id:'previous-1',boot:'previous',end:5000,epoch:0};
  let httpCallback;
  const sourceWithKey=source.replace('HIER_SHELLY_SCHLUESSEL_EINTRAGEN";','test-key";');
  vm.runInNewContext(sourceWithKey,{
    Math,JSON,print:()=>{},
    Timer:{set:(ms,repeat,fn)=>{tick=fn;}},
    HTTPServer:{registerEndpoint(){}},
    Shelly:{
      getDeviceInfo:()=>({id:'shellyplugmg3-test'}),getCurrentScriptId:()=>1,getUptimeMs:()=>now,
      getComponentStatus:name=>name==='switch:0'?{apower:1}:{unixtime:null},
      call:(method,args,callback)=>{
        if(method==='KVS.GetMany')callback({items:{cafe_brew_0:{value:saved}},total:1},0);
        else if(method==='HTTP.Request'){
          assert.equal(args.method,'POST');assert.equal(args.headers['X-Cafe-Shelly'],'test-key');
          const payload=JSON.parse(args.body);
          if(payload.type==='heartbeat')callback({code:200,body:'{"ok":true,"epoch":0}'},0);
          else {
            assert.equal(payload.event,'previous-1');assert.equal(payload.ageMs,-1);
            if(mode==='unreachable')callback(null,1);
            else if(mode==='wrong')callback({code:200,body:'{"ok":true,"event":"another"}'},0);
            else httpCallback=callback;
          }
        } else if(method==='KVS.Delete'){deleted++;callback({},0);}
        else throw Error(method);
      }
    }
  });
  tick();now+=1000;tick();assert.equal(deleted,0);
  mode='wrong';now+=1000;tick();assert.equal(deleted,0);
  mode='ack';now+=1000;tick();assert.equal(deleted,0);
  httpCallback({code:200,body:'{"ok":true,"event":"previous-1"}'},0);
  assert.equal(deleted,1);
});
