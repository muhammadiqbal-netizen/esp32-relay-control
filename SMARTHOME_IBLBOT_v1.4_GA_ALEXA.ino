/*
 * SMARTHOME_IBLBOT FINAL v1.4 - Saklar Push Button + Telegram Bot
 * Fitur: Web + Telegram Inline + Multi Admin EEPROM + Status EEPROM + Saklar Toggle
 * Saklar: Push button ke GND, pencet = toggle ON/OFF
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ===== SETTING WAJIB GANTI =====
const char* ssid = "NAMA_WIFI_KAMU";
const char* password = "PASSWORD_WIFI_KAMU";
const char* BOT_TOKEN = "TOKEN_BARU_DARI_BOTFATHER";

Preferences prefs;
String adminList[5];
int adminCount = 0;

// ===== PIN CONFIG =====
const int relayPin[4] = {26, 27, 14, 12};
const int saklarPin[4] = {32, 33, 25, 34};
const char* namaLampu[4] = {"Lampu Sudut", "Lampu Srip", "Lampu Tengah", "Lampu Makan"};
bool statusRelay[4] = {false, false, false, false};

// ===== VARIABEL SAKLAR =====
bool lastSaklarState[4] = {HIGH, HIGH, HIGH, HIGH};
bool saklarState[4] = {HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounce[4] = {0, 0, 0, 0};
const unsigned long debounceDelay = 50;

// ===== GLOBAL =====
WebServer server(80);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
unsigned long lastBotCheck = 0;

void setup() {
  Serial.begin(115200);
  prefs.begin("smarthome", false);

  loadAdmin();
  loadRelayStatus();

  for(int i=0; i<4; i++) {
    pinMode(relayPin[i], OUTPUT);
    digitalWrite(relayPin[i], statusRelay[i]? LOW : HIGH);
    pinMode(saklarPin[i], INPUT_PULLUP);
  }

  WiFi.begin(ssid, password);
  Serial.print("Connect WiFi");
  while(WiFi.status()!= WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(millis() > 20000) { Serial.println(" Timeout"); break; }
  }
  Serial.println("\nIP: " + WiFi.localIP().toString());

  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  
  // ===== SETUP ROUTES =====
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);
  
  server.begin();

  if(WiFi.status() == WL_CONNECTED) {
    broadcastAdmin("🤖 smarthome_iblbot Online v1.4\nIP: " + WiFi.localIP().toString() + "\n✅ Telegram Ready!");
  }
}

void loop() {
  server.handleClient();
  bacaSaklarPushButton();

  if(WiFi.status() == WL_CONNECTED) {
    if(millis() - lastBotCheck > 1000) {
      int numMessages = bot.getUpdates(bot.last_message_received + 1);
      while(numMessages) {
        handleTelegram(numMessages);
        numMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      lastBotCheck = millis();
    }
  }
}

// ===== EEPROM ADMIN =====
void loadAdmin() {
  adminCount = prefs.getInt("adminCount", 0);
  if(adminCount == 0) {
    adminList[0] = "ID_TELEGRAM_KAMU";
    adminCount = 1;
    saveAdmin();
  } else {
    for(int i=0; i<adminCount; i++) {
      adminList[i] = prefs.getString(("admin" + String(i)).c_str(), "");
    }
  }
}

void saveAdmin() {
  prefs.putInt("adminCount", adminCount);
  for(int i=0; i<adminCount; i++) {
    prefs.putString(("admin" + String(i)).c_str(), adminList[i]);
  }
}

// ===== EEPROM RELAY =====
void loadRelayStatus() {
  for(int i=0; i<4; i++) {
    statusRelay[i] = prefs.getBool(("r" + String(i)).c_str(), false);
  }
}

void saveRelayStatus(int idx) {
  prefs.putBool(("r" + String(idx)).c_str(), statusRelay[idx]);
}

bool isAdmin(String chat_id) {
  for(int i=0; i<adminCount; i++) {
    if(adminList[i] == chat_id) return true;
  }
  return false;
}

void broadcastAdmin(String msg) {
  if(WiFi.status()!= WL_CONNECTED) return;
  for(int i=0; i<adminCount; i++) {
    bot.sendMessage(adminList[i], msg, "");
  }
}

// ===== WEB SERVER =====
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>smarthome_iblbot v1.4</title><style>";
  html += "body{font-family:Arial;background:#0f172a;color:#fff;text-align:center;padding:20px}";
  html += "h2{color:#38bdf8}.btn{width:90%;max-width:300px;padding:18px;margin:12px;border:none;border-radius:12px;font-size:18px;font-weight:bold}";
  html += ".on{background:#22c55e}.off{background:#ef4444}.wifi{color:#fbbf24}";
  html += ".badge{display:inline-block;padding:8px 12px;border-radius:8px;margin:5px;font-size:12px;font-weight:bold}";
  html += ".telegram{background:#0088cc;color:white}";
  html += "</style></head><body>";
  html += "<h2>🤖 smarthome_iblbot v1.4</h2>";
  html += "<div style='margin:20px'>";
  html += "<span class='badge telegram'>✓ Telegram</span>";
  html += "</div>";
  
  if(WiFi.status()!= WL_CONNECTED) html += "<p class='wifi'>⚠️ Offline - Kontrol via saklar saja</p>";

  for(int i=0; i<4; i++) {
    String btnClass = statusRelay[i]? "on" : "off";
    String btnText = statusRelay[i]? "ON" : "OFF";
    html += "<button class='btn "+btnClass+"' onclick=\"fetch('/toggle?r="+String(i)+"&s="+(statusRelay[i]?"0":"1")+"').then(()=>location.reload())\">";
    html += namaLampu[i]+" : "+btnText+"</button><br>";
  }
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleToggle() {
  if(server.hasArg("r") && server.hasArg("s")) {
    int r = server.arg("r").toInt();
    int s = server.arg("s").toInt();
    setRelay(r, s==1, "Web");
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  DynamicJsonDocument doc(512);
  for(int i=0; i<4; i++) {
    doc["devices"][i]["name"] = namaLampu[i];
    doc["devices"][i]["status"] = statusRelay[i]? "ON" : "OFF";
  }
  doc["wifi"] = WiFi.status() == WL_CONNECTED? "connected" : "disconnected";
  doc["ip"] = WiFi.localIP().toString();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// ===== BACA SAKLAR PUSH BUTTON - TOGGLE =====
void bacaSaklarPushButton() {
  for(int i=0; i<4; i++) {
    bool reading = digitalRead(saklarPin[i]);

    if(reading!= lastSaklarState[i]) {
      lastDebounce[i] = millis();
    }

    if((millis() - lastDebounce[i]) > debounceDelay) {
      if(reading == LOW && saklarState[i] == HIGH) {
        setRelay(i,!statusRelay[i], "Saklar");
        delay(200);
      }
      saklarState[i] = reading;
    }
    lastSaklarState[i] = reading;
  }
}

// ===== TELEGRAM BOT =====
void handleTelegram(int numMessages) {
  for(int i=0; i<numMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String query_id = bot.messages[i].callback_query_id;
    String callback_data = bot.messages[i].callback_query_data;

    if(query_id!= "") {
      if(!isAdmin(chat_id)) {
        bot.sendAnswerCallbackQuery(query_id, "Akses ditolak", true);
        continue;
      }
      if(callback_data == "refresh") {
        bot.sendAnswerCallbackQuery(query_id, "Refresh", false);
        kirimMenu(chat_id);
        continue;
      }
      int idx = callback_data.substring(3).toInt() - 1;
      bool state = callback_data.startsWith("on");
      setRelay(idx, state, "Telegram");
      bot.sendAnswerCallbackQuery(query_id, "OK", false);
      kirimMenu(chat_id);
      continue;
    }

    if(text == "/start" || text == "/menu") {
      if(!isAdmin(chat_id)) {
        bot.sendMessage(chat_id, "🚫 Akses ditolak.\nChat ID kamu: " + chat_id, "");
        continue;
      }
      kirimMenu(chat_id);
    }
    else if(text == "/status") {
      if(!isAdmin(chat_id)) return;
      String msg = "📊 Status smarthome_iblbot:\n\n";
      for(int j=0; j<4; j++) {
        msg += (statusRelay[j]?"🟢":"🔴") + String(namaLampu[j]) + " : " + (statusRelay[j]?"ON":"OFF") + "\n";
      }
      bot.sendMessage(chat_id, msg, "");
    }
    else if(text == "/info") {
      if(!isAdmin(chat_id)) return;
      String msg = "ℹ️ Informasi smarthome_iblbot v1.4:\n\n";
      msg += "📱 IP Address: " + WiFi.localIP().toString() + "\n";
      msg += "📡 WiFi: " + String(WiFi.RSSI()) + " dBm\n";
      msg += "👥 Admin: " + String(adminCount) + "/5\n";
      bot.sendMessage(chat_id, msg, "");
    }
    else if(text.startsWith("/addadmin ")) {
      if(chat_id!= adminList[0]) {
        bot.sendMessage(chat_id, "🚫 Cuma owner yang bisa tambah admin", "");
        continue;
      }
      String newId = text.substring(10);
      if(adminCount < 5 &&!isAdmin(newId)) {
        adminList[adminCount] = newId;
        adminCount++;
        saveAdmin();
        bot.sendMessage(chat_id, "✅ Admin ditambahkan: " + newId, "");
        bot.sendMessage(newId, "🎉 Kamu sekarang admin smarthome_iblbot. Kirim /menu", "");
      } else {
        bot.sendMessage(chat_id, "Gagal. Sudah ada / admin penuh", "");
      }
    }
    else if(text.startsWith("/deladmin ")) {
      if(chat_id!= adminList[0]) {
        bot.sendMessage(chat_id, "🚫 Cuma owner yang bisa hapus admin", "");
        continue;
      }
      String delId = text.substring(10);
      for(int j=0; j<adminCount; j++) {
        if(adminList[j] == delId && j!=0) {
          for(int k=j; k<adminCount-1; k++) adminList[k] = adminList[k+1];
          adminCount--;
          saveAdmin();
          bot.sendMessage(chat_id, "✅ Admin dihapus: " + delId, "");
          bot.sendMessage(delId, "Kamu sudah dihapus dari admin", "");
          return;
        }
      }
      bot.sendMessage(chat_id, "Admin tidak ditemukan", "");
    }
    else if(text == "/adminlist") {
      if(!isAdmin(chat_id)) return;
      String msg = "👥 Daftar Admin smarthome_iblbot:\n";
      for(int j=0; j<adminCount; j++) {
        msg += String(j+1) + ". " + adminList[j];
        if(j==0) msg += " [OWNER]\n";
        else msg += "\n";
      }
      bot.sendMessage(chat_id, msg, "");
    }
  }
}

void kirimMenu(String chat_id) {
  String keyboardJson = "[";
  for(int i=0; i<4; i++) {
    String btnOn = statusRelay[i]? "✅" : "⚪";
    String btnOff = statusRelay[i]? "⚪" : "✅";
    keyboardJson += "[{\"text\":\"" + btnOn + " " + String(namaLampu[i]) + " ON\",\"callback_data\":\"on" + String(i+1) + "\"},";
    keyboardJson += "{\"text\":\"" + btnOff + " " + String(namaLampu[i]) + " OFF\",\"callback_data\":\"off" + String(i+1) + "\"}]";
    if(i<3) keyboardJson += ",";
  }
  keyboardJson += ",[{\"text\":\"🔄 Refresh\",\"callback_data\":\"refresh\"}]";
  keyboardJson += "]";
  bot.sendMessageWithInlineKeyboard(chat_id, "🎛️ Kontrol Lampu smarthome_iblbot", "", keyboardJson);
}

void setRelay(int idx, bool state, String sumber) {
  if(idx<0 || idx>3) return;
  statusRelay[idx] = state;
  digitalWrite(relayPin[idx], state? LOW : HIGH);
  saveRelayStatus(idx);

  String emoji = state? "🟢" : "🔴";
  String status = state? "DINYALAKAN" : "DIMATIKAN";
  String msg = emoji + " " + String(namaLampu[idx]) + " " + status + " via " + sumber;

  if(WiFi.status() == WL_CONNECTED) {
    broadcastAdmin(msg);
  }
  Serial.println(msg);
}
