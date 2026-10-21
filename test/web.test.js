import test from 'node:test';
import assert from 'node:assert/strict';
import {displayTime,displayChipTime,selectedPeriods,configPayload} from '../data/www/domain.js';
test('offline time remains explicit',()=>assert.equal(displayTime(0),'Uhrzeit noch offen'));
test('Berlin timezone rather than browser timezone',()=>assert.match(displayTime(1711924200),/01\.04\.2024.*00:30/));
test('historical period options stay selected, sorted and unique',()=>assert.deepEqual(selectedPeriods(['2025-W01','2025-W01'],['2024-12'],'2024-W52','2025-01'),{weeks:['2025-W01','2024-W52'],months:['2025-01','2024-12']}));
test('blank passwords are not accidentally overwritten',()=>assert.deepEqual(configPayload({ssid:' Office ',wifiPassword:'',apPassword:'',adminPassword:''}),{ssid:'Office'}));
test('open client wifi deliberately clears its password',()=>assert.deepEqual(configPayload({ssid:'Open',clearWifiPassword:true}),{ssid:'Open',wifiPassword:''}));
test('password whitespace is preserved',()=>assert.deepEqual(configPayload({ssid:'Office',wifiPassword:' pass '}),{ssid:'Office',wifiPassword:' pass '}));

test('chip timestamps show date and seconds in Berlin time',()=>assert.match(displayChipTime(1711924201),/01\.04\.2024.*00:30:01/));
test('old chip without recorded contact shows a dash',()=>assert.equal(displayChipTime(undefined),'—'));
test('recorded contact without clock remains explicitly undated',()=>assert.equal(displayChipTime(0),'Uhrzeit unbekannt'));


test('AP-only keeps all three network slots and does not send blank passwords',()=>{
  assert.deepEqual(configPayload({apOnly:true,ssid0:' Office ',ssid1:'Home',ssid2:'',wifiPassword0:'',wifiPassword1:''}),{
    apOnly:true,wifiNetworks:[{ssid:'Office'},{ssid:'Home'},{ssid:''}]
  });
});
test('each slot independently preserves, sets or deliberately clears passwords',()=>{
  assert.deepEqual(configPayload({apOnly:false,ssid0:'A',ssid1:'B',ssid2:'C',wifiPassword0:' pass ',clearWifiPassword1:true,wifiPassword2:''}),{
    apOnly:false,wifiNetworks:[{ssid:'A',password:' pass '},{ssid:'B',password:''},{ssid:'C'}]
  });
});
test('turning AP-only off retains the saved SSIDs',()=>{
  const form={apOnly:true,ssid0:'A',ssid1:'B',ssid2:'C'};
  const before=configPayload(form);form.apOnly=false;const after=configPayload(form);
  assert.deepEqual(after.wifiNetworks,before.wifiNetworks);assert.equal(after.apOnly,false);
});
