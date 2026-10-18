import test from 'node:test';
import assert from 'node:assert/strict';
import {displayTime,selectedPeriods,configPayload} from '../data/www/domain.js';
test('offline time remains explicit',()=>assert.equal(displayTime(0),'Uhrzeit noch offen'));
test('Berlin timezone rather than browser timezone',()=>assert.match(displayTime(1711924200),/01\.04\.2024.*00:30/));
test('historical period options stay selected, sorted and unique',()=>assert.deepEqual(selectedPeriods(['2025-W01','2025-W01'],['2024-12'],'2024-W52','2025-01'),{weeks:['2025-W01','2024-W52'],months:['2025-01','2024-12']}));
test('blank passwords are not accidentally overwritten',()=>assert.deepEqual(configPayload({ssid:' Office ',wifiPassword:'',apPassword:'',adminPassword:''}),{ssid:'Office'}));
test('open client wifi deliberately clears its password',()=>assert.deepEqual(configPayload({ssid:'Open',clearWifiPassword:true}),{ssid:'Open',wifiPassword:''}));
test('password whitespace is preserved',()=>assert.deepEqual(configPayload({ssid:'Office',wifiPassword:' pass '}),{ssid:'Office',wifiPassword:' pass '}));
