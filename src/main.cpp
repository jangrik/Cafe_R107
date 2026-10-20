#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <ESPFMfGK.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include "Domain.h"
#include "DisplayLogic.h"
#include "Backlight.h"
#include "HealthLogic.h"
#include <ArduinoOTA.h>

namespace pins {
constexpr uint8_t RFID_CS=25, RFID_RST=22, LCD_CS=26, LCD_DC=27, LCD_RST=33, BUZZER=32;
constexpr uint8_t LCD_BL=21, TOF_SDA=16, TOF_SCL=17, TOF_XSHUT=4;
}
constexpr uint16_t BG=0x1083, CREAM=0xFF18, GOLD=0xED68;
constexpr uint16_t BUZZER_FREQUENCY=2500, BUZZER_MS=200;
constexpr uint8_t BUZZER_CHANNEL=0;
constexpr uint16_t FM_PORT=8080;
constexpr size_t MAX_PEOPLE=50, MAX_CARDS=100, MAX_PENDING=30;
WebServer web(80);
ESPFMfGK filemgr(FM_PORT);
MFRC522 rfid(pins::RFID_CS,pins::RFID_RST);
Adafruit_ST7789 lcd(&SPI,pins::LCD_CS,pins::LCD_DC,pins::LCD_RST);

struct Person { String id, name; };
struct Card { String uid, person; };
struct Pending { String uid; };
struct Anchor { int64_t epoch=0; uint64_t uptime=0; };
struct LastSeen {String session;int64_t epoch=0;uint64_t uptime=0,sequence=0;};
struct SensorDiagnostics {
  bool healthy=false;uint32_t checks=0,attempts=0,successes=0,checkedAt=0,recoveredAt=0;
  String error,lastError,lastRecoveryReason;
};
SensorDiagnostics tofDiagnostics,rfidDiagnostics;
cafe::HealthSchedule healthSchedule;
uint8_t readerVersion=0,readerTxControl=0;
cafe::ReaderFailures readerFailures;
MFRC522::StatusCode readerWakeStatus=MFRC522::STATUS_TIMEOUT;
MFRC522::StatusCode readerSelectStatus=MFRC522::STATUS_TIMEOUT;
bool readerPolled=false,readerSelected=false,readerHasUid=false,readerReadFault=false;
uint32_t readerUidAt=0,readerPolls=0,readerUidReads=0,readerProbeAt=0;
String readerLastUid;
uint32_t tofInitAt=0;
cafe::TofRecovery tofRecovery;
String tofOriginalError,tofInitError,tofInitStage;
bool tofFaultActive=false;
uint32_t tofHardwareResets=0;
uint8_t tofLastI2cStatus=0;
bool tofInitTimeout=false;
std::map<String,LastSeen> lastSeen;
uint64_t lastSeenSequence=0;
String lastSeenError;
bool lastSeenWritable=true;
struct Config { String ssid, wifiPassword, apPassword, adminPassword; } cfg;
std::vector<Person> people;
std::vector<Card> cards;
std::vector<Pending> pending;
std::map<String,Anchor> anchors;
std::set<uint32_t> cancellations;
String session, storageError;
bool mounted=false, journalOK=true, readerOK=false, fmReady=false, ntpStarted=false;
uint32_t lastId=0;
uint32_t lastPoll=0, lastPresence=0, lastAccepted=0, displayAt=0, lastWifiRetry=0;
bool latched=false, acceptedOnce=false;
enum class Screen { Boot, Idle, Result, Ranking };
Screen currentScreen=Screen::Boot;
String lastNetworkLine, lastCupTime;
uint64_t lastSteamSecond=UINT64_MAX;
cafe::RankingCycle rankingCycle;
cafe::BacklightTimer backlightTimer;
cafe::PresenceGate presenceGate;
VL53L0X tof;
bool backlightOn=true, tofOK=false;
bool tofHasSample=false,tofSampleValid=false;
cafe::RestartRequest restartRequest;
uint16_t tofDistance=0;
uint32_t lastTofPoll=0,lastTofResult=0;

void serviceBacklight() {
  bool on=backlightTimer.on(millis());
  if(on!=backlightOn) {
    backlightOn=on;
    digitalWrite(pins::LCD_BL,on?HIGH:LOW);
  }
}
void setupTof(bool hardwareReset=false) {
  bool wasInitialized=tofOK;
  tofOK=false;tofSampleValid=false;tofDistance=0;
  tofInitError="";tofInitTimeout=false;tofInitStage="bus_begin";
  presenceGate=cafe::PresenceGate();
  Wire.end();
  if(hardwareReset) {
    // Never drive HIGH: release to the module's own XSHUT pull-up.
    digitalWrite(pins::TOF_XSHUT,LOW);
    pinMode(pins::TOF_XSHUT,OUTPUT);delay(10);
    pinMode(pins::TOF_XSHUT,INPUT);delay(10);
    ++tofHardwareResets;
    tof=VL53L0X(); // Discard library state belonging to the previous sensor boot.
  } else pinMode(pins::TOF_XSHUT,INPUT);
  tofInitAt=millis();
  Wire.begin(pins::TOF_SDA,pins::TOF_SCL,100000);
  Wire.setTimeOut(10);
  tof.setBus(&Wire);tof.setTimeout(200);
  tof.timeoutOccurred();
  auto failed=[](const char* stage) {
    tofInitStage=stage;tofInitError=stage;
    tofLastI2cStatus=tof.last_status;tofInitTimeout=tof.timeoutOccurred();
    tofDiagnostics.healthy=false;tofDiagnostics.error="reinitialization_failed";
    Serial.println("TOF init failed: "+tofInitError);
  };
  tofInitStage="address_probe";
  Wire.beginTransmission(0x29);tofLastI2cStatus=Wire.endTransmission();
  if(tofLastI2cStatus!=0) {tofInitError="address_probe";return;}
  if(wasInitialized && !hardwareReset) {
    tof.stopContinuous();
    if(tof.last_status!=0) {failed("stop_continuous");return;}
  }
  tofInitStage="sensor_init";
  if(!tof.init()) {failed("sensor_init");return;}
  tofInitStage="timing_budget";
  if(!tof.setMeasurementTimingBudget(33000) || tof.last_status!=0) {
    failed("timing_budget");return;
  }
  tofInitStage="start_continuous";
  tof.startContinuous(100);
  bool timedOut=tof.timeoutOccurred();
  if(tof.last_status!=0 || timedOut) {
    failed("start_continuous");tofInitTimeout=tofInitTimeout || timedOut;return;
  }
  tofOK=true;tofLastI2cStatus=0;tofInitStage="waiting_for_measurement";
  tof.setTimeout(20);
  // Preserve lastTofResult for API age; recovery uses its own first-sample state.
  tofDiagnostics.healthy=false;tofDiagnostics.error="waiting_for_measurement";
  Serial.println("VL53L0X bereit (SDA16/SCL17, XSHUT4, Naehe 80 cm)");
}
void serviceTof() {
  uint32_t now=millis();
  if(!tofOK || uint32_t(now-lastTofPoll)<100)return;
  lastTofPoll=now;
  uint8_t ready=tof.readReg(VL53L0X::RESULT_INTERRUPT_STATUS);
  tofLastI2cStatus=tof.last_status;
  bool valid=false;uint16_t mm=0;
  if(tof.last_status==0 && (ready&0x07)) {
    // Read only a completed continuous measurement, never start/wait for a shot.
    mm=tof.readRangeContinuousMillimeters();
    tofLastI2cStatus=tof.last_status;
    bool transportOK=!tof.timeoutOccurred() && tofLastI2cStatus==0;
    valid=transportOK && mm>=20 && mm<=2000;
    tofDistance=valid?mm:0;tofSampleValid=valid;
    if(transportOK) {
      lastTofResult=now;tofHasSample=true;
      if(tofRecovery.measurement())++tofDiagnostics.successes;
      tofFaultActive=false;tofInitStage="running";
      tofDiagnostics.healthy=true;tofDiagnostics.error="";
    } else {
      tofDiagnostics.healthy=false;tofDiagnostics.error="i2c_measurement_error";
    }
  } else {
    if(uint32_t(now-lastTofResult)<1000)return;
    tofDistance=0;tofSampleValid=false;
  }
  if(presenceGate.update(valid,mm)) {
    backlightTimer.approach(now);
    serviceBacklight();
  }
}

void beginRecovery(SensorDiagnostics& d,const String& reason) {
  ++d.attempts;d.recoveredAt=millis();d.lastRecoveryReason=reason;
  d.lastError=reason;d.error=reason;d.healthy=false;
}
bool probeReader() {
  digitalWrite(pins::LCD_CS,HIGH);
  uint8_t versions[3],antennas[3];
  for(unsigned i=0;i<3;++i) {
    versions[i]=rfid.PCD_ReadRegister(MFRC522::VersionReg);
    antennas[i]=rfid.PCD_ReadRegister(MFRC522::TxControlReg)&0x03;
  }
  readerTxControl=antennas[2];readerProbeAt=millis();
  return cafe::readerProbeOK(readerVersion,versions,3) &&
      cafe::readerProbeOK(0x03,antennas,3);
}
void recoverReader(const String& reason) {
  beginRecovery(rfidDiagnostics,reason);
  Serial.println("RFID recovery: "+reason);
  digitalWrite(pins::LCD_CS,HIGH);
  pinMode(pins::RFID_RST,OUTPUT);digitalWrite(pins::RFID_RST,LOW);delay(2);
  digitalWrite(pins::RFID_RST,HIGH);delay(50);rfid.PCD_Init();
  // Retain the boot reference unless startup never produced a plausible version.
  if(!cafe::readerVersionPlausible(readerVersion))
    readerVersion=rfid.PCD_ReadRegister(MFRC522::VersionReg);
  readerOK=probeReader();readerFailures.observe(false);readerReadFault=false;
  rfidDiagnostics.healthy=readerOK;
  if(readerOK) {++rfidDiagnostics.successes;rfidDiagnostics.error="";}
  else rfidDiagnostics.error="reinitialization_failed";
  // Preserve latched and cooldown: a held chip must not create another booking.
}
void serviceDeviceHealth() {
  uint32_t now=millis();if(!healthSchedule.takeIfDue(now))return;
  ++tofDiagnostics.checks;tofDiagnostics.checkedAt=now;
  Wire.beginTransmission(0x29);tofLastI2cStatus=Wire.endTransmission();
  bool ack=tofLastI2cStatus==0;
  String reason=cafe::tofHealthError(tofOK,ack,tofHasSample && !tofRecovery.pending(),
      uint32_t(now-lastTofResult),uint32_t(now-tofInitAt));
  if(reason.length()) {
    if(!tofFaultActive) {tofOriginalError=reason;tofFaultActive=true;}
    beginRecovery(tofDiagnostics,reason);
    bool hardwareReset=tofRecovery.begin();
    Serial.println("TOF recovery: "+reason+(hardwareReset?" (XSHUT)":" (software)"));
    setupTof(hardwareReset);
    if(!tofOK)tofDiagnostics.error="reinitialization_failed";
  } else if(!tofRecovery.pending() && tofHasSample) {
    tofDiagnostics.healthy=true;tofDiagnostics.error="";
  }

  ++rfidDiagnostics.checks;rfidDiagnostics.checkedAt=millis();
  bool registersOK=probeReader();
  if(readerOK && registersOK && !readerReadFault) {
    rfidDiagnostics.healthy=true;rfidDiagnostics.error="";return;
  }
  String readerReason=!readerOK?"not_initialized":
      !registersOK?"register_or_antenna_mismatch":"repeated_read_errors";
  recoverReader(readerReason);
  // No ESP.restart(): clock, network, registry and journal stay alive.
}

uint64_t uptimeMs() { return static_cast<uint64_t>(esp_timer_get_time()/1000); }
int64_t epochNow() { int64_t t=time(nullptr); return cafe::validTime(t) ? t : 0; }
String randomHex() { char b[17]; snprintf(b,sizeof(b),"%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random()); return b; }
String apName() { return "Cafe-R107-"+WiFi.macAddress().substring(12,14)+WiFi.macAddress().substring(15,17); }
String personName(const String& id) { for(const auto& p:people) if(p.id==id) return p.name; return "Unbekannte Person"; }
String personFor(const String& uid) { for(const auto& c:cards) if(c.uid==uid) return c.person; return ""; }
bool personExists(const String& id) { for(const auto& p:people) if(p.id==id) return true; return false; }

// Core 2.0.17 is pinned in platformio.ini. Hardware PWM runs while the loop handles HTTP.
bool buzzerReady=false, buzzerOn=false;
esp_timer_handle_t buzzerCutoff=nullptr;
void cutoffTone(void*) { ledcWrite(BUZZER_CHANNEL,0); }
void armCutoff(uint16_t ms) {
  if(buzzerCutoff) {esp_timer_stop(buzzerCutoff);esp_timer_start_once(buzzerCutoff,uint64_t(ms)*1000);}
}
uint8_t beeps=0; uint16_t beepFrequency=0, beepDuration=0; uint32_t beepAt=0;
void toneHz(uint16_t hz) { if(buzzerReady) ledcWriteTone(BUZZER_CHANNEL,hz); }
void beep(uint8_t count,uint16_t frequency,uint16_t duration) {
  if(!buzzerReady) return;
  beeps=count; beepFrequency=frequency; beepDuration=duration; buzzerOn=true;
  toneHz(frequency); armCutoff(duration); beepAt=millis();
}
void serviceBuzzer() {
  if(!beeps) return;
  uint32_t now=millis();
  if(buzzerOn && uint32_t(now-beepAt)>=beepDuration) {
    toneHz(0); buzzerOn=false; --beeps; beepAt=now;
  } else if(!buzzerOn && uint32_t(now-beepAt)>=100) {
    toneHz(beepFrequency); armCutoff(beepDuration); buzzerOn=true; beepAt=now;
  }
}

void center(const String& s,int y,uint8_t size,uint16_t color) {
  lcd.setTextSize(size); lcd.setTextColor(color);
  int16_t x,y0; uint16_t w,h; lcd.getTextBounds(s,0,0,&x,&y0,&w,&h);
  lcd.setCursor((240-int(w))/2-x,y); lcd.print(s);
}
// Built-in GFX font is ASCII: transliterate common German letters only for LCD.
String lcdText(String s) {
  s.replace("ä","ae"); s.replace("ö","oe"); s.replace("ü","ue");
  s.replace("Ä","Ae"); s.replace("Ö","Oe"); s.replace("Ü","Ue"); s.replace("ß","ss");
  if(s.length()>18) s=s.substring(0,17)+".";
  return s;
}
void frame() {
  digitalWrite(pins::RFID_CS,HIGH);
  lcd.fillScreen(BG); lcd.fillRect(0,0,240,49,0x3104);
  center("Cafe R107",14,3,CREAM); lcd.drawFastHLine(18,49,204,GOLD);
}
String networkLine() { return WiFi.status()==WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }
// Redraw only the clock/steam regions, avoiding a full-screen flash each second.
String cupTime() { return cafe::dateKey(epochNow(),"%H:%M").c_str(); }
void drawCupTime() {
  digitalWrite(pins::RFID_CS,HIGH);
  lcd.fillRect(64,105,94,35,BG);
  lastCupTime=cupTime();
  if(lastCupTime.length()) {
    lcd.setTextSize(3); lcd.setTextColor(CREAM);
    lcd.setCursor(66,112); lcd.print(lastCupTime);
  }
}
void drawSteam() {
  digitalWrite(pins::RFID_CS,HIGH);
  lcd.fillRect(73,59,83,29,BG);
  lastSteamSecond=uptimeMs()/1000;
  static const int8_t sway[]={-3,0,3,0};
  uint8_t phase=lastSteamSecond%4;
  for(int i=0;i<3;i++) {
    int x=88+i*24, bend=sway[(phase+i)%4];
    lcd.drawLine(x,85,x+bend,78,CREAM);
    lcd.drawLine(x+bend,78,x-bend,70,CREAM);
    lcd.drawLine(x-bend,70,x,62,CREAM);
  }
}
void idleScreen() {
  frame();
  // Wide enough for a five-character HH:MM at text size 3.
  lcd.drawRoundRect(54,92,114,62,9,CREAM);
  lcd.drawRoundRect(167,104,23,35,8,CREAM);
  lcd.drawFastHLine(44,160,155,GOLD);
  drawCupTime(); drawSteam();
  center("Zeit fuer Kaffee?",173,2,CREAM);
  center(storageError.length() ? "Speicher pruefen!" : (readerOK ? "Bitte Chip vorhalten" : "RFID nicht bereit"),197,2,storageError.length()?ST77XX_RED:GOLD);
  lastNetworkLine=networkLine(); center(lastNetworkLine,224,1,CREAM);
  currentScreen=Screen::Idle;
}

void message(const String& title,const String& name,const String& detail1, const String& detail2, bool success) {
  frame(); uint16_t c=success?0x77AE:GOLD;
  center(title,75,2,c); center(lcdText(name),112,2,CREAM); 
  center(detail1,145,2,GOLD);
  center(detail2,175,2,GOLD);
  center("Cafe R107",211,1,CREAM); currentScreen=Screen::Result; displayAt=millis();
}

// Atomic replacement within LittleFS: never format on mount or parsing errors.
bool saveJson(const char* path,JsonDocument& doc) {
  String tmp=String(path)+".tmp";
  File f=LittleFS.open(tmp,"w");
  if(!f) return false;
  size_t expected=measureJson(doc), written=serializeJson(doc,f);
  f.flush(); f.close();
  if(written!=expected) { LittleFS.remove(tmp); return false; }
  if(!LittleFS.rename(tmp,path)) { LittleFS.remove(tmp); return false; }
  return true;
}
bool saveLastSeen() {
  if(!mounted || !lastSeenWritable)return false;
  JsonDocument d;d["schema"]=1;d["sequence"]=lastSeenSequence;
  auto entries=d["entries"].to<JsonArray>();
  for(const auto& item:lastSeen) {
    auto o=entries.add<JsonObject>();o["uid"]=item.first;
    o["session"]=item.second.session;o["epoch"]=item.second.epoch;
    o["uptime"]=item.second.uptime;o["sequence"]=item.second.sequence;
  }
  if(d.overflowed())return false;
  return saveJson("/lastseen.json",d);
}
bool loadLastSeen() {
  if(!LittleFS.exists("/lastseen.json"))return true; // Existing installations migrate lazily.
  File f=LittleFS.open("/lastseen.json","r");JsonDocument d;
  if(!f || deserializeJson(d,f) || d["schema"]!=1 || !d["entries"].is<JsonArray>() ||
     d["entries"].size()>MAX_CARDS+MAX_PENDING || !d["sequence"].is<uint64_t>())return false;
  std::map<String,LastSeen> loaded;
  uint64_t sequence=d["sequence"].as<uint64_t>();
  for(JsonObject o:d["entries"].as<JsonArray>()) {
    String uid=o["uid"]|"";LastSeen seen;
    if(!uid.length() || !o["session"].is<String>() || !o["epoch"].is<int64_t>() ||
       !o["uptime"].is<uint64_t>() || !o["sequence"].is<uint64_t>() || loaded.count(uid))return false;
    seen.session=o["session"].as<String>();seen.epoch=o["epoch"].as<int64_t>();
    seen.uptime=o["uptime"].as<uint64_t>();seen.sequence=o["sequence"].as<uint64_t>();
    if(seen.sequence>sequence || (seen.epoch && !cafe::validTime(seen.epoch)))return false;
    loaded[uid]=seen;
  }
  lastSeen=loaded;lastSeenSequence=sequence;return true;
}
void rememberChipContact(const String& uid) {
  if(!lastSeen.count(uid) && lastSeen.size()>=MAX_CARDS+MAX_PENDING) {
    // Keep current chip assignments preferentially; bound memory even for stray tags.
    auto victim=lastSeen.end();
    for(auto it=lastSeen.begin();it!=lastSeen.end();++it) {
      if(personFor(it->first).length())continue;
      if(victim==lastSeen.end() || it->second.sequence<victim->second.sequence)victim=it;
    }
    if(victim==lastSeen.end())victim=lastSeen.begin();
    lastSeen.erase(victim);
  }
  LastSeen seen;seen.session=session;seen.epoch=epochNow();seen.uptime=uptimeMs();
  seen.sequence=++lastSeenSequence;lastSeen[uid]=seen;
  // One atomic snapshot per new chip contact, never per polling cycle.
  if(saveLastSeen())lastSeenError="";
  else lastSeenError="Chip-Zeitstempel nicht dauerhaft gespeichert (lastseen.json pruefen)";
}
int64_t lastSeenEpoch(const LastSeen& seen) {
  auto it=anchors.find(seen.session);Anchor anchor;
  if(it!=anchors.end())anchor=it->second;
  return cafe::anchoredEpoch(seen.epoch,seen.uptime,anchor.epoch,anchor.uptime);
}

bool saveConfig() {
  JsonDocument d; d["schema"]=1; d["ssid"]=cfg.ssid; d["wifiPassword"]=cfg.wifiPassword;
  d["apPassword"]=cfg.apPassword; d["adminPassword"]=cfg.adminPassword;
  return saveJson("/config.json",d);
}
bool saveRegistry() {
  JsonDocument d; d["schema"]=1;
  JsonArray ps=d["people"].to<JsonArray>();
  for(const auto& p:people) { JsonObject o=ps.add<JsonObject>(); o["id"]=p.id; o["name"]=p.name; }
  JsonArray cs=d["cards"].to<JsonArray>();
  for(const auto& c:cards) { JsonObject o=cs.add<JsonObject>(); o["uid"]=c.uid; o["person"]=c.person; }
  JsonArray us=d["pending"].to<JsonArray>();
  for(const auto& u:pending) us.add(u.uid);
  return saveJson("/registry.json",d);
}
bool loadConfig() {
  if(!LittleFS.exists("/config.json")) return saveConfig();
  File f=LittleFS.open("/config.json","r"); JsonDocument d;
  if(!f || deserializeJson(d,f) || d["schema"]!=1) return false;
  if(!d["ssid"].is<String>() || !d["wifiPassword"].is<String>() || !d["apPassword"].is<String>() || !d["adminPassword"].is<String>()) return false;
  Config next;
  next.ssid=d["ssid"].as<String>(); next.wifiPassword=d["wifiPassword"].as<String>();
  next.apPassword=d["apPassword"].as<String>(); next.adminPassword=d["adminPassword"].as<String>();
  if(next.ssid.length()>32 || next.wifiPassword.length()>63 || next.apPassword.length()<8 || next.apPassword.length()>63 || next.adminPassword.length()<8 || next.adminPassword.length()>63) return false;
  cfg=next; return true;
}
bool loadRegistry() {
  if(!LittleFS.exists("/registry.json")) return saveRegistry();
  File f=LittleFS.open("/registry.json","r"); JsonDocument d;
  if(!f || deserializeJson(d,f) || d["schema"]!=1) return false;
  if(!d["people"].is<JsonArray>() || !d["cards"].is<JsonArray>() || !d["pending"].is<JsonArray>()) return false;
  if(d["people"].size()>MAX_PEOPLE || d["cards"].size()>MAX_CARDS || d["pending"].size()>MAX_PENDING) return false;
  std::set<String> ids,uids;
  for(JsonObject o:d["people"].as<JsonArray>()) {
    String id=o["id"].as<String>(), name=o["name"].as<String>();
    if(!id.length() || !name.length() || !ids.insert(id).second) return false;
    people.push_back({id,name});
  }
  for(JsonObject o:d["cards"].as<JsonArray>()) {
    String uid=o["uid"].as<String>(), id=o["person"].as<String>();
    if(!uid.length() || !personExists(id) || !uids.insert(uid).second) return false;
    cards.push_back({uid,id});
  }
  for(JsonVariant v:d["pending"].as<JsonArray>()) {
    String uid=v.as<String>(); if(!uid.length() || !uids.insert(uid).second) return false;
    pending.push_back({uid});
  }
  return true;
}

// Append-only journal: clock anchors and cancellations never rewrite coffee records.
// A damaged line blocks further bookings; preserve the file for recovery via filemanager.
bool scanJournal(const std::function<void(JsonDocument&)>& visit) {
  if(!LittleFS.exists("/journal.ndjson")) return true;
  File f=LittleFS.open("/journal.ndjson","r"); if(!f) return false;
  while(f.available()) {
    String line; line.reserve(320); bool newline=false;
    while(f.available()) { char ch=f.read(); if(ch=='\n') {newline=true; break;} if(line.length()>=1024) return false; line+=ch; }
    JsonDocument d;
    if(!newline || !line.length() || deserializeJson(d,line)) return false;
    String kind=d["kind"].as<String>();
    if(!d["id"].is<uint32_t>() || d["id"].as<uint32_t>()==0 || (kind!="coffee" && kind!="cancel" && kind!="clock")) return false;
    visit(d); serviceBuzzer(); delay(0);
  }
  return true;
}
bool appendRecord(JsonDocument& d) {
  if(!mounted || !journalOK || storageError.length()) return false;
  if(LittleFS.totalBytes()-LittleFS.usedBytes()<16384) { storageError="LittleFS voll: bitte sichern und archivieren"; return false; }
  d["id"]=lastId+1;
  File f=LittleFS.open("/journal.ndjson","a"); if(!f) {storageError="Journal kann nicht geoeffnet werden";return false;}
  size_t expected=measureJson(d), n=serializeJson(d,f); size_t end=f.print('\n'); f.flush(); f.close();
  if(n!=expected || end!=1) { journalOK=false; storageError="Journal-Schreibfehler"; return false; }
  ++lastId; return true;
}
void loadJournal() {
  uint32_t prev=0; bool valid=true;
  journalOK=scanJournal([&](JsonDocument& d) {
    uint32_t id=d["id"]; if(id<=prev) valid=false; prev=id; lastId=std::max(lastId,id);
    String kind=d["kind"].as<String>();
    if(kind=="clock") {
      Anchor a; a.epoch=d["epoch"].as<int64_t>(); a.uptime=d["uptime"].as<uint64_t>();
      if(!cafe::validTime(a.epoch) || !d["session"].is<String>()) valid=false;
      else anchors[d["session"].as<String>()]=a;
    } else if(kind=="cancel") {
      uint32_t target=d["target"]|0U; if(!target || target>=id) valid=false;
      cancellations.insert(target);
    } else if(!d["person"].is<String>() || !d["session"].is<String>() || !d["uptime"].is<uint64_t>()) valid=false;
  }) && valid;
  if(!journalOK) storageError="Journal beschaedigt: sichern und pruefen";
}
int64_t recordEpoch(JsonDocument& d) {
  String boot=d["session"].as<String>(); auto it=anchors.find(boot);
  Anchor a; if(it!=anchors.end()) a=it->second;
  return cafe::anchoredEpoch(d["epoch"]|int64_t(0),d["uptime"].as<uint64_t>(),a.epoch,a.uptime);
}
String fitRankingName(const String& name,int maxWidth) {
  String text=lcdText(name);
  lcd.setTextSize(2);
  int16_t x,y; uint16_t w,h;
  lcd.getTextBounds(text,0,0,&x,&y,&w,&h);
  if(w<=maxWidth) return text;
  while(text.length()) {
    text.remove(text.length()-1);
    String shortened=text+".";
    lcd.getTextBounds(shortened,0,0,&x,&y,&w,&h);
    if(w<=maxWidth) return shortened;
  }
  return "";
}
void rankingScreen(cafe::RankingPeriod period) {
  const char* label=period==cafe::RankingPeriod::Today ? "Heute" :
                    period==cafe::RankingPeriod::Month ? "Dieser Monat" : "Insgesamt";
  int64_t now=epochNow();
  std::map<String,uint32_t> counts;
  bool timeMissing=period!=cafe::RankingPeriod::All && !now;
  bool readable=mounted && journalOK && !storageError.length();
  if(readable && !timeMissing) {
    readable=scanJournal([&](JsonDocument& e){
      if(e["kind"]!="coffee" || cancellations.count(e["id"].as<uint32_t>())) return;
      if(cafe::inRankingPeriod(recordEpoch(e),now,period)) ++counts[e["person"].as<String>()];
    });
    if(!readable) { journalOK=false; storageError="Journal nicht lesbar"; }
  }
  std::vector<cafe::RankEntry> entries;
  for(const auto& p:counts) entries.push_back({p.first.c_str(),personName(p.first).c_str(),p.second});
  auto top=cafe::topFive(entries);
  frame(); center(String("Top 5 - ")+label,65,2,GOLD);
  if(!readable) {
    center("Speicher pruefen!",117,2,ST77XX_RED);
  } else if(timeMissing) {
    center("Uhrzeit fehlt",117,2,CREAM);
    center("Bitte im Browser setzen",151,1,GOLD);
  } else if(top.empty()) {
    center("Noch keine Kaffees",117,2,CREAM);
    center("Zeit fuer eine kleine Pause?",151,1,GOLD);
  } else {
    for(size_t i=0;i<top.size();++i) {
      int y=101+int(i)*23;
      String amount=String(top[i].count);
      lcd.setTextSize(2); int16_t x0,y0;uint16_t w,h;
      lcd.getTextBounds(amount,0,0,&x0,&y0,&w,&h);
      int countX=230-int(w);
      lcd.setTextColor(GOLD);lcd.setCursor(10,y);lcd.print(int(i)+1);
      lcd.setTextColor(CREAM);lcd.setCursor(34,y);
      lcd.print(fitRankingName(top[i].name.c_str(),countX-42));
      lcd.setTextColor(GOLD);lcd.setCursor(countX,y);lcd.print(amount);
    }
  }
  center("Kaffees pro Person",226,1,CREAM);
  currentScreen=Screen::Ranking; displayAt=millis();
}
void serviceDisplay() {
  uint32_t now=millis();
  if((currentScreen==Screen::Boot && uint32_t(now-displayAt)>=15000) ||
     (currentScreen==Screen::Result && uint32_t(now-displayAt)>=4000) ||
     (currentScreen==Screen::Ranking && uint32_t(now-displayAt)>=cafe::RANKING_DURATION_MS)) idleScreen();
  if(currentScreen!=Screen::Idle) return;
  if(backlightOn && rankingCycle.due(now)) { rankingScreen(rankingCycle.take(now)); return; }
  if(networkLine()!=lastNetworkLine) { idleScreen(); return; }
  if(uptimeMs()/1000!=lastSteamSecond) {
    if(cupTime()!=lastCupTime) drawCupTime();
    drawSteam();
  }
}

void anchorClock() {
  int64_t now=epochNow(); if(!now || anchors.count(session) || !journalOK || storageError.length()) return;
  Anchor a; a.epoch=now; a.uptime=uptimeMs(); JsonDocument d;
  d["kind"]="clock"; d["epoch"]=now; d["uptime"]=a.uptime; d["session"]=session;
  if(appendRecord(d)) anchors[session]=a;
  else if(!storageError.length()) storageError="Zeitanker nicht gespeichert";
}

void handleCard(const String& uid) {
  if(!mounted || !journalOK || storageError.length()) {
    message("Nicht gebucht","Speicherfehler","Bitte Weboberflaeche","pruefen!!!",false); beep(3,1000,70); return;
  }
  String person=personFor(uid);
  if(!person.length()) {
    bool exists=false; for(const auto& p:pending) if(p.uid==uid) exists=true;
    if(!exists) {
      if(pending.size()>=MAX_PENDING) { message("Nicht gebucht","Chip-Liste voll","Bitte im Browser","aufraeumen!!!",false); return; }
      pending.push_back({uid});
      if(!saveRegistry()) { pending.pop_back(); message("Nicht gespeichert","Speicherfehler","Bitte Browser","pruefen!!!",false); beep(3,1000,70); return; }
    }
    message("Neuer Chip",uid,"Bitte im Browser","zuordnen!!!",false); beep(2,1500,90); return;
  }
  JsonDocument d; d["kind"]="coffee"; d["person"]=person;
  d["epoch"]=epochNow(); d["session"]=session; d["uptime"]=uptimeMs();
  if(!appendRecord(d)) { message("Nicht gebucht","Speicherfehler","Bitte im Browser","pruefen!!!",false); beep(3,1000,70); return; }
  message("+1 Kaffee",personName(person),"Gespeichert.",epochNow()?"Guten Genuss!":"Zeit noch offen",true);
  beep(1,BUZZER_FREQUENCY,BUZZER_MS);
}
void pollRFID() {
  uint32_t now=millis(); if(!readerOK || uint32_t(now-lastPoll)<100) return; lastPoll=now;
  digitalWrite(pins::LCD_CS,HIGH);
  byte atqa[2], len=sizeof(atqa); auto result=rfid.PICC_WakeupA(atqa,&len);
  readerPolled=true;++readerPolls;readerWakeStatus=result;
  bool readFault=result!=MFRC522::STATUS_OK && result!=MFRC522::STATUS_TIMEOUT &&
      result!=MFRC522::STATUS_COLLISION;
  bool present=result==MFRC522::STATUS_OK || result==MFRC522::STATUS_COLLISION;
  if(present) {
    lastPresence=now;
    readerSelectStatus=rfid.PICC_Select(&rfid.uid);readerSelected=true;
    readFault=result==MFRC522::STATUS_OK && readerSelectStatus!=MFRC522::STATUS_OK &&
        readerSelectStatus!=MFRC522::STATUS_COLLISION;
    if(readerSelectStatus==MFRC522::STATUS_OK) {
      String uid;
      for(byte i=0;i<rfid.uid.size;i++) { if(i) uid+=':'; if(rfid.uid.uidByte[i]<16) uid+='0'; uid+=String(rfid.uid.uidByte[i],HEX); }
      uid.toUpperCase();readerReadFault=false;readerHasUid=true;readerUidAt=now;++readerUidReads;readerLastUid=uid;
      rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
      bool fresh=!latched; latched=true;
      // Wake on every new chip contact, even unknown chips or cooldown hits.
      if(fresh) {backlightTimer.chip(now);serviceBacklight();rememberChipContact(uid);}
      if(fresh && (!acceptedOnce || uint32_t(now-lastAccepted)>=3000)) {
        acceptedOnce=true; lastAccepted=now; handleCard(uid);
      }
    }
  } else if(latched && uint32_t(now-lastPresence)>700) latched=false;
  readerFailures.observe(readFault);
  if(readerFailures.repeated()) {
    readerReadFault=true;rfidDiagnostics.healthy=false;rfidDiagnostics.error="repeated_read_errors";
  }
}

void sendJson(JsonDocument& d,int code=200) {
  String s; serializeJson(d,s); web.sendHeader("Cache-Control","no-store"); web.send(code,"application/json; charset=utf-8",s);
}
void error(int code,const String& msg) { JsonDocument d; d["error"]=msg; sendJson(d,code); }
bool auth(bool mutation=false) {
  if(!web.authenticate("admin",cfg.adminPassword.c_str())) { web.requestAuthentication(BASIC_AUTH,"Cafe R107"); return false; }
  if(mutation && web.header("X-Cafe-Request")!="1") { error(403,"Anfrage nicht erlaubt"); return false; }
  return true;
}
bool body(JsonDocument& d) {
  if(web.arg("plain").length()>4096 || deserializeJson(d,web.arg("plain")) || !d.is<JsonObject>()) {error(400,"Ungueltiges JSON");return false;} return true;
}
bool writable() { if(storageError.length() || !mounted || !journalOK) {error(503,"Speicher nicht bereit: "+storageError);return false;}return true; }
String cleanName(String s) { s.trim(); for(size_t i=0;i<s.length();i++) if((uint8_t)s[i]<32) return ""; return s.length()<=48?s:String(""); }

void displayState(JsonDocument& d) {
  serviceBacklight();
  d["on"]=backlightOn;d["mode"]=backlightTimer.modeName();
}
void apiDisplayGet() {
  if(!auth())return;
  JsonDocument d;displayState(d);sendJson(d);
}
void setDisplayMode(const char* mode) {
  if(!backlightTimer.setMode(mode)) {error(400,"mode muss on, off oder auto sein");return;}
  JsonDocument out;out["ok"]=true;displayState(out);sendJson(out);
}
void apiDisplayPost() {
  if(!auth(true))return;
  JsonDocument d;if(!body(d))return;
  setDisplayMode(d["mode"]|"");
}
void sensorDiagnosticJson(JsonObject o,const SensorDiagnostics& diag) {
  o["healthy"]=diag.healthy;o["error"]=diag.error;o["lastError"]=diag.lastError;
  o["checks"]=diag.checks;o["recoveryAttempts"]=diag.attempts;
  o["successfulReinitializations"]=diag.successes;o["lastRecoveryReason"]=diag.lastRecoveryReason;
  if(diag.checks)o["lastCheckAgeMs"]=uint32_t(millis()-diag.checkedAt);else o["lastCheckAgeMs"]=nullptr;
  if(diag.attempts)o["lastRecoveryAgeMs"]=uint32_t(millis()-diag.recoveredAt);else o["lastRecoveryAgeMs"]=nullptr;
}
void rfidDiagnosticJson(JsonObject o) {
  sensorDiagnosticJson(o,rfidDiagnostics);
  o["versionRegister"]=readerVersion;o["antennaEnabled"]=(readerTxControl&3)==3;
  o["antennaCheckAgeMs"]=uint32_t(millis()-readerProbeAt);
  o["antennaControlBits"]=readerTxControl;
  o["verification"]="registers_only";o["cardLatched"]=latched;
  o["polls"]=readerPolls;o["uidReads"]=readerUidReads;
  o["consecutiveReadErrors"]=readerFailures.count();o["readFaultPending"]=readerReadFault;
  if(readerPolled) {
    o["lastWakeStatus"]=uint8_t(readerWakeStatus);
    o["lastWakeStatusText"]=String(rfid.GetStatusCodeName(readerWakeStatus));
    o["lastPollAgeMs"]=uint32_t(millis()-lastPoll);
  } else {o["lastWakeStatus"]=nullptr;o["lastWakeStatusText"]=nullptr;o["lastPollAgeMs"]=nullptr;}
  if(readerSelected) {
    o["lastSelectStatus"]=uint8_t(readerSelectStatus);
    o["lastSelectStatusText"]=String(rfid.GetStatusCodeName(readerSelectStatus));
  } else {o["lastSelectStatus"]=nullptr;o["lastSelectStatusText"]=nullptr;}
  if(readerHasUid) {o["lastUid"]=readerLastUid;o["lastUidAgeMs"]=uint32_t(millis()-readerUidAt);}
  else {o["lastUid"]=nullptr;o["lastUidAgeMs"]=nullptr;}
}
void apiRfidReinit() {
  if(!auth(web.method()==HTTP_POST))return;
  recoverReader("manual_request");
  JsonDocument d;d["ok"]=readerOK;rfidDiagnosticJson(d["rfid"].to<JsonObject>());
  sendJson(d,readerOK?200:503);
}
void tofDiagnosticJson(JsonObject o) {
  sensorDiagnosticJson(o,tofDiagnostics);
  o["originalError"]=tofOriginalError;o["initError"]=tofInitError;
  o["initStage"]=tofInitStage;o["initTimeout"]=tofInitTimeout;
  o["lastI2cStatus"]=tofLastI2cStatus;o["hardwareResets"]=tofHardwareResets;
  o["recoveryPending"]=tofRecovery.pending();o["xshutPin"]=pins::TOF_XSHUT;
  String stalled=cafe::tofHealthError(tofOK,true,tofHasSample && !tofRecovery.pending(),
      uint32_t(millis()-lastTofResult),uint32_t(millis()-tofInitAt));
  if(stalled.length()) {
    o["healthy"]=false;
    if(!tofDiagnostics.error.length() || tofDiagnostics.error=="waiting_for_measurement")o["error"]=stalled;
  }
}
void apiDiagnostics() {
  if(!auth())return;JsonDocument d;
  tofDiagnosticJson(d["tof"].to<JsonObject>());
  rfidDiagnosticJson(d["rfid"].to<JsonObject>());
  d["lastSeenError"]=lastSeenError;d["automaticEspRestart"]=false;
  d["checkIntervalMs"]=cafe::HEALTH_INTERVAL_MS;d["tofStallMs"]=cafe::TOF_STALL_MS;sendJson(d);
}

void apiTof() {
  if(!auth())return;
  uint32_t age=uint32_t(millis()-lastTofResult);
  bool valid=cafe::freshTofSample(tofOK,tofHasSample,tofSampleValid,age);
  JsonDocument d;d["available"]=tofOK;d["valid"]=valid;
  if(valid)d["distanceMm"]=tofDistance;else d["distanceMm"]=nullptr;
  if(tofHasSample)d["ageMs"]=age;else d["ageMs"]=nullptr;
  d["presence"]=presenceGate.present();tofDiagnosticJson(d["diagnostics"].to<JsonObject>());sendJson(d);
}
void apiRestart() {
  if(!auth(web.method()==HTTP_POST))return;
  uint32_t now=millis();restartRequest.schedule(now);
  JsonDocument d;d["ok"]=true;d["restarting"]=true;
  d["delayMs"]=restartRequest.remaining(now);
  web.sendHeader("Connection","close");sendJson(d,202);
}

void apiStatus() {
  if(!auth()) return; JsonDocument d;
  d["backlightOn"]=backlightOn;d["backlightMode"]=backlightTimer.modeName();d["tofOK"]=tofOK;
  d["distanceMm"]=tofDistance;d["presence"]=presenceGate.present();
  d["lastSeenRevision"]=lastSeenSequence;d["lastSeenError"]=lastSeenError;
  tofDiagnosticJson(d["tofDiagnostics"].to<JsonObject>());
  rfidDiagnosticJson(d["rfidDiagnostics"].to<JsonObject>());
  d["version"]=cafe::VERSION; d["apSsid"]=apName(); d["apIP"]=WiFi.softAPIP().toString();
  d["staConnected"]=WiFi.status()==WL_CONNECTED; d["staIP"]=WiFi.localIP().toString(); d["ssid"]=cfg.ssid;
  d["epoch"]=epochNow(); d["readerOK"]=readerOK; d["storageError"]=storageError;
  d["journalOK"]=journalOK; d["fsUsed"]=mounted?LittleFS.usedBytes():0; d["fsTotal"]=mounted?LittleFS.totalBytes():0;
  d["session"]=session; d["uptime"]=uptimeMs(); d["pendingCount"]=pending.size(); d["filemanagerPort"]=FM_PORT;
  sendJson(d);
}
void apiRegistry() {
  if(!auth()) return; JsonDocument d;
  JsonArray ps=d["people"].to<JsonArray>(); for(const auto& p:people) {auto o=ps.add<JsonObject>();o["id"]=p.id;o["name"]=p.name;}
  JsonArray cs=d["cards"].to<JsonArray>(); for(const auto& c:cards) {auto o=cs.add<JsonObject>();o["uid"]=c.uid;o["person"]=c.person;}
  JsonArray us=d["pending"].to<JsonArray>(); for(const auto& p:pending) us.add(p.uid);
  auto seen=d["lastSeen"].to<JsonObject>();
  for(const auto& item:lastSeen)seen[item.first]=lastSeenEpoch(item.second);
  sendJson(d);
}
void apiPerson() {
  if(!auth(true) || !writable()) return; JsonDocument d; if(!body(d)) return;
  String name=cleanName(d["name"]|""), id=d["id"]|"";
  if(!name.length()) {error(400,"Name erforderlich, maximal 48 UTF-8-Bytes");return;}
  if(!id.length()) {
    if(people.size()>=MAX_PEOPLE) {error(409,"Maximal 50 Personen");return;}
    do {id="p"+randomHex().substring(0,8);} while(personExists(id));
    people.push_back({id,name});
    if(!saveRegistry()) {people.pop_back();error(507,"Speichern fehlgeschlagen");return;}
  } else {
    auto it=std::find_if(people.begin(),people.end(),[&](const Person& p){return p.id==id;});
    if(it==people.end()) {error(404,"Person nicht gefunden");return;}
    String old=it->name; it->name=name;
    if(!saveRegistry()) {it->name=old;error(507,"Speichern fehlgeschlagen");return;}
  }
  JsonDocument out;out["id"]=id;out["ok"]=true;sendJson(out);
}
void apiAssign() {
  if(!auth(true) || !writable()) return; JsonDocument d;if(!body(d)) return;
  String uid=d["uid"].as<String>(), id=d["person"].as<String>();
  if(!personExists(id)) {error(400,"Person nicht gefunden");return;}
  bool pendingExists=false;for(const auto& p:pending) if(p.uid==uid) pendingExists=true;
  bool cardExists=personFor(uid).length();
  if(!pendingExists && !cardExists) {error(404,"Chip zuerst am Leser vorhalten");return;}
  if(!cardExists && cards.size()>=MAX_CARDS) {error(409,"Maximal 100 Chips");return;}
  auto oldCards=cards;auto oldPending=pending;
  if(cardExists) {for(auto& c:cards) if(c.uid==uid)c.person=id;}
  else cards.push_back({uid,id});
  pending.erase(std::remove_if(pending.begin(),pending.end(),[&](const Pending& p){return p.uid==uid;}),pending.end());
  if(!saveRegistry()) {cards=oldCards;pending=oldPending;error(507,"Speichern fehlgeschlagen");return;}
  JsonDocument out;out["ok"]=true;sendJson(out);
}
void apiForget() {
  if(!auth(true) || !writable()) return;JsonDocument d;if(!body(d)) return;String uid=d["uid"].as<String>();
  auto oldCards=cards;auto oldPending=pending;
  cards.erase(std::remove_if(cards.begin(),cards.end(),[&](const Card& c){return c.uid==uid;}),cards.end());
  pending.erase(std::remove_if(pending.begin(),pending.end(),[&](const Pending& p){return p.uid==uid;}),pending.end());
  if(!saveRegistry()) {cards=oldCards;pending=oldPending;error(507,"Speichern fehlgeschlagen");return;}
  JsonDocument out;out["ok"]=true;sendJson(out);
}

void apiReport() {
  if(!auth()) return;
  if(!mounted || !journalOK) {error(503,"Journal nicht lesbar");return;}
  int64_t now=epochNow(); String week=web.hasArg("week")?web.arg("week"):String(cafe::weekKey(now).c_str());
  String month=web.hasArg("month")?web.arg("month"):String(cafe::monthKey(now).c_str());
  std::map<String,cafe::Counts> counts; std::set<String> weeks,months;
  for(const auto& p:people) counts[p.id]=cafe::Counts();
  cafe::Counts total;
  bool ok=scanJournal([&](JsonDocument& e) {
    if(e["kind"]!="coffee" || cancellations.count(e["id"].as<uint32_t>())) return;
    int64_t t=recordEpoch(e);String id=e["person"].as<String>();
    counts[id].add(t,now);total.add(t,now);
    if(t) {
      String wk=cafe::weekKey(t).c_str(), mo=cafe::monthKey(t).c_str(); weeks.insert(wk);months.insert(mo);
    }
  });
  if(!ok) {journalOK=false;storageError="Journal nicht lesbar";error(503,storageError);return;}
  // Selected historical periods can differ from the current calendar week/month.
  for(auto& p:counts) {p.second.week=0;p.second.month=0;}total.week=0;total.month=0;
  ok=scanJournal([&](JsonDocument& e) {
    if(e["kind"]!="coffee" || cancellations.count(e["id"].as<uint32_t>())) return;
    int64_t t=recordEpoch(e);if(!t) return; String id=e["person"].as<String>();
    if(week.length() && week==cafe::weekKey(t).c_str()) {++counts[id].week;++total.week;}
    if(month.length() && month==cafe::monthKey(t).c_str()) {++counts[id].month;++total.month;}
  });
  if(!ok) {error(503,"Journal nicht lesbar");return;}
  JsonDocument out;out["week"]=week;out["month"]=month;
  auto addCounts=[](JsonObject o,const cafe::Counts& c){o["total"]=c.total;o["today"]=c.today;o["week"]=c.week;o["month"]=c.month;o["undated"]=c.undated;};
  addCounts(out["totals"].to<JsonObject>(),total);
  JsonArray ps=out["people"].to<JsonArray>();
  for(const auto& p:counts) {auto o=ps.add<JsonObject>();o["id"]=p.first;o["name"]=personName(p.first);addCounts(o,p.second);}
  JsonArray ws=out["weeks"].to<JsonArray>();for(auto it=weeks.rbegin();it!=weeks.rend();++it)ws.add(*it);
  JsonArray ms=out["months"].to<JsonArray>();for(auto it=months.rbegin();it!=months.rend();++it)ms.add(*it);
  sendJson(out);
}
void apiHistory() {
  if(!auth()) return;if(!mounted || !journalOK) {error(503,"Journal nicht lesbar");return;}
  uint32_t before=web.hasArg("before")?strtoul(web.arg("before").c_str(),nullptr,10):UINT32_MAX;
  struct Row {uint32_t id;String person;int64_t epoch;bool cancelled;};std::vector<Row> rows;bool more=false;
  bool ok=scanJournal([&](JsonDocument& e){
    uint32_t id=e["id"]|0U;if(e["kind"]!="coffee" || id>=before)return;
    rows.push_back({id,e["person"].as<String>(),recordEpoch(e),bool(cancellations.count(id))});
    if(rows.size()>50) {rows.erase(rows.begin());more=true;}
  });
  if(!ok) {error(503,"Journal nicht lesbar");return;}
  JsonDocument d;d["more"]=more;auto arr=d["rows"].to<JsonArray>();
  for(auto it=rows.rbegin();it!=rows.rend();++it){auto o=arr.add<JsonObject>();o["id"]=it->id;o["person"]=it->person;o["name"]=personName(it->person);o["epoch"]=it->epoch;o["cancelled"]=it->cancelled;}
  sendJson(d);
}
void apiCancel() {
  if(!auth(true) || !writable())return;JsonDocument d;if(!body(d))return;uint32_t target=d["id"]|0U;
  if(cancellations.count(target)) {JsonDocument out;out["ok"]=true;sendJson(out);return;}
  bool found=false;bool ok=scanJournal([&](JsonDocument& e){if(e["kind"]=="coffee" && e["id"].as<uint32_t>()==target)found=true;});
  if(!ok || !found) {error(404,"Buchung nicht gefunden");return;}
  JsonDocument e;e["kind"]="cancel";e["target"]=target;e["epoch"]=epochNow();e["session"]=session;e["uptime"]=uptimeMs();
  if(!appendRecord(e)) {error(507,"Storno nicht gespeichert");return;}
  cancellations.insert(target);JsonDocument out;out["ok"]=true;sendJson(out);
}
void apiTime() {
  if(!auth(true) || !writable())return;JsonDocument d;if(!body(d))return;int64_t epoch=d["epoch"]|int64_t(0);
  if(!cafe::validTime(epoch)) {error(400,"Uhrzeit ausserhalb 2024-2037");return;}
  // One anchor per boot avoids silently moving previously undated records later.
  struct timeval tv{};tv.tv_sec=epoch;settimeofday(&tv,nullptr);anchorClock();
  JsonDocument out;out["ok"]=storageError.length()==0;sendJson(out);
}
void apiConfig() {
  if(!auth(true) || !writable())return;JsonDocument d;if(!body(d))return;Config old=cfg;
  if(!loadConfig()) {cfg=old;error(503,"Gespeicherte Konfiguration nicht lesbar");return;}
  if(d["ssid"].is<String>())cfg.ssid=d["ssid"].as<String>();
  if(d["wifiPassword"].is<String>())cfg.wifiPassword=d["wifiPassword"].as<String>();
  if(d["apPassword"].is<String>() && d["apPassword"].as<String>().length())cfg.apPassword=d["apPassword"].as<String>();
  if(d["adminPassword"].is<String>() && d["adminPassword"].as<String>().length())cfg.adminPassword=d["adminPassword"].as<String>();
  if(cfg.ssid.length()>32 || cfg.wifiPassword.length()>63 || cfg.apPassword.length()<8 || cfg.apPassword.length()>63 || cfg.adminPassword.length()<8 || cfg.adminPassword.length()>63) {cfg=old;error(400,"SSID max.32, Passwoerter 8-63 Zeichen (Client-WLAN auch leer)");return;}
  if(!saveConfig()){cfg=old;error(507,"Konfiguration nicht gespeichert");return;}
  JsonDocument out;out["ok"]=true;out["restartRequired"]=true;sendJson(out);
  // Apply after restart: don't interrupt the HTTP response/AP or change active auth mid-request.
  cfg=old;
}
void apiExport() {
  if(!auth())return;if(!mounted || !journalOK){error(503,"Journal nicht lesbar");return;}
  // CSV uses stable person IDs and current display names, including explicit cancellations.
  web.sendHeader("Content-Disposition","attachment; filename=Cafe_R107.csv");
  web.setContentLength(CONTENT_LENGTH_UNKNOWN);web.send(200,"text/csv; charset=utf-8","");
  web.sendContent("\xEF\xBB\xBFid;person_id;name;zeit_berlin;epoch;storniert\r\n");
  scanJournal([&](JsonDocument& e){
    if(e["kind"]!="coffee")return;uint32_t id=e["id"]|0U;int64_t t=recordEpoch(e);
    String name=personName(e["person"].as<String>());name.replace("\"","\"\"");
    if(name.startsWith("=")||name.startsWith("+")||name.startsWith("-")||name.startsWith("@"))name="'"+name;
    String line=String(id)+";"+e["person"].as<String>()+";\""+name+"\";"+cafe::dateKey(t,"%Y-%m-%d %H:%M:%S").c_str()+";";
    if(t)line+=String(static_cast<unsigned long>(t));line+=";";line+=cancellations.count(id)?"ja":"nein";line+="\r\n";web.sendContent(line);
  });
  web.sendContent("");
}

uint32_t fmFlags(fs::FS&,String,uint32_t flags) {
  if(flags & ESPFMfGK::flagIsValidAction)return flags & ~ESPFMfGK::flagIsValidAction;
  if(flags & ESPFMfGK::flagIsValidTargetFilename)return flags & ~ESPFMfGK::flagIsValidTargetFilename;
  return ESPFMfGK::flagCanDelete|ESPFMfGK::flagCanRename|ESPFMfGK::flagCanEdit|ESPFMfGK::flagAllowPreview|
    ESPFMfGK::flagCanGZip|ESPFMfGK::flagCanDownload|ESPFMfGK::flagAllowInZip|ESPFMfGK::flagCanUpload|ESPFMfGK::flagCanCreateNew;
}
void setupWeb() {
  const char* headers[]={"X-Cafe-Request"};web.collectHeaders(headers,1);
  web.on("/api/restart",HTTP_GET,apiRestart);
  web.on("/api/restart",HTTP_POST,apiRestart);
  web.on("/api/display",HTTP_GET,apiDisplayGet);
  web.on("/api/display",HTTP_POST,apiDisplayPost);
  web.on("/api/display/on",HTTP_GET,[](){if(auth())setDisplayMode("on");});
  web.on("/api/display/on",HTTP_POST,[](){if(auth(true))setDisplayMode("on");});
  web.on("/api/display/off",HTTP_GET,[](){if(auth())setDisplayMode("off");});
  web.on("/api/display/off",HTTP_POST,[](){if(auth(true))setDisplayMode("off");});
  web.on("/api/display/auto",HTTP_GET,[](){if(auth())setDisplayMode("auto");});
  web.on("/api/display/auto",HTTP_POST,[](){if(auth(true))setDisplayMode("auto");});
  web.on("/api/rfid/reinit",HTTP_GET,apiRfidReinit);
  web.on("/api/rfid/reinit",HTTP_POST,apiRfidReinit);
  web.on("/api/tof",HTTP_GET,apiTof);
  web.on("/api/diagnostics",HTTP_GET,apiDiagnostics);
  web.on("/api/status",HTTP_GET,apiStatus);web.on("/api/registry",HTTP_GET,apiRegistry);
  web.on("/api/report",HTTP_GET,apiReport);web.on("/api/history",HTTP_GET,apiHistory);
  web.on("/api/person",HTTP_POST,apiPerson);web.on("/api/assign",HTTP_POST,apiAssign);
  web.on("/api/forget",HTTP_POST,apiForget);web.on("/api/cancel",HTTP_POST,apiCancel);
  web.on("/api/time",HTTP_POST,apiTime);web.on("/api/config",HTTP_POST,apiConfig);
  web.on("/api/export.csv",HTTP_GET,apiExport);
  web.onNotFound([](){
    if(!auth())return;
    String uri=web.uri();String path,mime;
    if(uri=="/"||uri=="/index.html"){path="/www/index.html";mime="text/html; charset=utf-8";}
    else if(uri=="/app.js"){path="/www/app.js";mime="application/javascript; charset=utf-8";}
    else if(uri=="/domain.js"){path="/www/domain.js";mime="application/javascript; charset=utf-8";}
    else if(uri=="/style.css"){path="/www/style.css";mime="text/css; charset=utf-8";}
    else {error(404,"Nicht gefunden");return;}
    File f=LittleFS.open(path,"r");if(!f){web.send(503,"text/plain; charset=utf-8","LittleFS-Webdateien fehlen. Siehe README zur Erstinstallation.");return;}
    web.sendHeader("Cache-Control","no-cache");web.streamFile(f,mime);f.close();
  });
  web.begin();
  if(mounted) {
    filemgr.AddFS(LittleFS,"LittleFS",true);filemgr.checkFileFlags=fmFlags;
    filemgr.WebPageTitle="Cafe R107 - Dateien";filemgr.textareaCharset="accept-charset=\"utf-8\"";
    filemgr.HttpUsername="admin";filemgr.HttpPassword=cfg.adminPassword;
    filemgr.ExtraHTMLfoot="<p>Vor Aenderungen alle Dateien sichern. Nach manuellen Daten-/Konfigurationsaenderungen ESP neu starten.</p>";
    fmReady=filemgr.begin();
  }
}
void serviceNetwork() {
  if(WiFi.status()==WL_CONNECTED && !ntpStarted) {configTzTime(cafe::TZ_BERLIN,"pool.ntp.org","time.cloudflare.com");ntpStarted=true;}
  if(cfg.ssid.length() && WiFi.status()!=WL_CONNECTED && uint32_t(millis()-lastWifiRetry)>=30000) {lastWifiRetry=millis();WiFi.reconnect();}
}
void setup() {
  Serial.begin(115200);
  pinMode(pins::LCD_BL,OUTPUT);digitalWrite(pins::LCD_BL,HIGH);
  backlightTimer.reset(millis());
  pinMode(pins::BUZZER,OUTPUT);digitalWrite(pins::BUZZER,LOW);
  esp_timer_create_args_t timerArgs{};timerArgs.callback=cutoffTone;timerArgs.name="buzzer-cutoff";
  bool timerOK=esp_timer_create(&timerArgs,&buzzerCutoff)==ESP_OK;
  buzzerReady=timerOK && ledcSetup(BUZZER_CHANNEL,BUZZER_FREQUENCY,10)>0;
  if(buzzerReady){ledcAttachPin(pins::BUZZER,BUZZER_CHANNEL);toneHz(0);}
  pinMode(pins::RFID_CS,OUTPUT);digitalWrite(pins::RFID_CS,HIGH);
  pinMode(pins::LCD_CS,OUTPUT);digitalWrite(pins::LCD_CS,HIGH);
  SPI.begin(18,19,23);lcd.init(240,240,SPI_MODE0);lcd.setSPISpeed(20000000);lcd.setRotation(0);lcd.setTextWrap(false);
  digitalWrite(pins::LCD_CS,HIGH);rfid.PCD_Init();delay(50);
  readerVersion=rfid.PCD_ReadRegister(MFRC522::VersionReg);readerOK=cafe::readerVersionPlausible(readerVersion);
  readerOK=readerOK && probeReader();
  rfidDiagnostics.healthy=readerOK;if(!readerOK)rfidDiagnostics.error="not_initialized";
  setenv("TZ",cafe::TZ_BERLIN,1);tzset();session=randomHex();
  cfg.apPassword=randomHex().substring(0,12);cfg.adminPassword=randomHex().substring(0,12);
  mounted=LittleFS.begin(false);
  if(!mounted)storageError="LittleFS nicht bereit - kein automatisches Formatieren";
  else if(!loadConfig())storageError="config.json ungueltig - Wiederherstellung noetig";
  if(mounted && !storageError.length()) {
    if(!loadRegistry())storageError="registry.json ungueltig";
    else loadJournal();
    if(!loadLastSeen()) {lastSeenWritable=false;lastSeenError="lastseen.json ungueltig: bitte sichern und pruefen";}
  }
  WiFi.persistent(false);WiFi.mode(WIFI_AP_STA);WiFi.setHostname("cafe-r107");
  bool apOK=WiFi.softAP(apName().c_str(),cfg.apPassword.c_str());
  if(cfg.ssid.length()){WiFi.setAutoReconnect(true);WiFi.begin(cfg.ssid.c_str(),cfg.wifiPassword.c_str());}
  setupWeb();setupTof();healthSchedule.reset(millis());
  tofDiagnostics.error=tofOK?"waiting_for_measurement":"not_initialized";
  Serial.printf("\nCafe R107 %s\nAP: %s (%s)\nAP-Passwort: %s\nWeb: http://%s/\nDateien: http://%s:8080/\nLogin: admin / %s\n",cafe::VERSION,apName().c_str(),apOK?"OK":"FEHLER",cfg.apPassword.c_str(),WiFi.softAPIP().toString().c_str(),WiFi.softAPIP().toString().c_str(),cfg.adminPassword.c_str());
  if(storageError.length())Serial.println(storageError);
  frame();center("Start / WLAN",65,2,CREAM);center(apName(),100,2,CREAM);
  center("AP: "+cfg.apPassword,125,2,GOLD);center("Login: admin",150,2,CREAM);
  center(cfg.adminPassword,170,2,GOLD);center(WiFi.softAPIP().toString(),210,2,CREAM);
  displayAt=millis();rankingCycle.reset(displayAt);beep(1,2000,80);

// **************************************************************  Setup OTA      Over the Air Flashing
  ArduinoOTA.setHostname("CAFE-R107");
  ArduinoOTA.setPassword("Kether");
  ArduinoOTA.begin();

}
void loop() {
  ArduinoOTA.handle(); serviceBuzzer();serviceNetwork();anchorClock();serviceTof();serviceBacklight();serviceDeviceHealth();
  pollRFID();web.handleClient();if(fmReady)filemgr.handleClient();serviceBuzzer();
  serviceDisplay();
  // File operations and the HTTP response have completed before restarting.
  if(restartRequest.due(millis())) {toneHz(0);ESP.restart();}
  delay(1);
}

