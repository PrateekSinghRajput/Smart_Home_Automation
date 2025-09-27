#define ESP_DRD_USE_SPIFFS true

#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WebServer.h>

#define JSON_CONFIG_FILE "/relay_config.json"

#define RELAY_1_PIN 4
#define RELAY_2_PIN 16
#define RELAY_3_PIN 17
#define RELAY_4_PIN 5

bool shouldSaveConfig = false;
bool relayState[4] = { false, false, false, false };

WiFiManager wm;
WebServer server(80);

void saveRelayState() {
  StaticJsonDocument<256> json;
  json["relay1"] = relayState[0];
  json["relay2"] = relayState[1];
  json["relay3"] = relayState[2];
  json["relay4"] = relayState[3];

  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("failed to open relay config file for writing");
    return;
  }
  serializeJson(json, configFile);
  configFile.close();
}

void loadRelayState() {
  if (SPIFFS.exists(JSON_CONFIG_FILE)) {
    File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
    if (configFile) {
      StaticJsonDocument<256> json;
      DeserializationError error = deserializeJson(json, configFile);
      if (!error) {
        relayState[0] = json["relay1"];
        relayState[1] = json["relay2"];
        relayState[2] = json["relay3"];
        relayState[3] = json["relay4"];
      }
      configFile.close();
    }
  }
}

void updateRelays() {
  digitalWrite(RELAY_1_PIN, relayState[0] ? HIGH : LOW);
  digitalWrite(RELAY_2_PIN, relayState[1] ? HIGH : LOW);
  digitalWrite(RELAY_3_PIN, relayState[2] ? HIGH : LOW);
  digitalWrite(RELAY_4_PIN, relayState[3] ? HIGH : LOW);
}

String relayHtml() {
  String icons[2] = { "&#128308;", "&#128994;" };
  String page = "<!DOCTYPE html>";
  page += "<html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title> Home Automation </title>";
  page += "<style>"
          "body {background: #232946; color: #fff; font-family: Nunito,sans-serif; margin:0; padding:0;}"
          ".container {max-width:470px; margin:40px auto; background:#16161a; border-radius:25px; box-shadow:0 4px 20px #1114; padding:35px 15px 15px;}"
          "h2 {text-align:center; margin-bottom:24px; letter-spacing:.5px;}"
          ".relay-card {display:flex; align-items:center; border-radius:18px; margin:22px 16px 18px; background:#232946; min-height:70px; box-shadow:0 2px 10px #0002;}"
          ".relay-info {flex:1; font-size:1.12em;}"
          ".relay-icon {font-size:2.2em; margin-left:18px; margin-right:14px;}"
          ".switch {position:relative; display:inline-block; width:55px; height:32px; margin:0 12px;}"
          ".switch input {opacity:0; width:0; height:0;}"
          ".slider {position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; background:#b0b3c6; transition:.4s; border-radius:30px;}"
          ".slider:before {position:absolute; content:''; height:26px; width:26px; left:4px; bottom:3px; background:#fff; box-shadow:0 2px 8px #8882; transition:.4s; border-radius:50%;}"
          "input:checked + .slider {background: #22d36b;}"
          "input:checked + .slider:before {transform: translateX(21px); background:#22d36b;}"
          ".status-on  {color:#22d36b; font-weight:700; letter-spacing:.5px;}"
          ".status-off {color:#ff5555; font-weight:500; letter-spacing:.2px;}"
          ".ip {margin:20px 0 12px; text-align:center; color:#eebbc3;}"
          "@media (max-width:600px){.container{margin:0px 0; border-radius:0; padding:19px 2vw 8px;}}"
          "</style></head><body>";
  page += "<div class='container'>";
  page += "<h2>&#9889; <b> Home Automation </b> &#9889;</h2>";
  for (int i = 0; i < 4; i++) {
    String icon = icons[relayState[i] ? 1 : 0];
    String statusClass = relayState[i] ? "status-on" : "status-off";
    String statusText = relayState[i] ? "ON" : "OFF";
    page += "<form action='/relay" + String(i + 1) + "' method='POST' class='relay-card'>";
    page += "<span class='relay-icon'>" + icon + "</span><span class='relay-info'>Relay " + String(i + 1) + "</span>";
    page += "<label class='switch'><input type='checkbox' name='state' value='1' onChange='this.form.submit();' " + String(relayState[i] ? "checked" : "") + ">";
    page += "<span class='slider'></span></label>";
    page += "<span class='" + statusClass + "'>" + statusText + "</span></form>";
  }
  page += "<div class='ip'>Device IP: <b>" + WiFi.localIP().toString() + "</b></div>";
  page += "<div style='text-align:center; margin-top: 20px;'>";
  page += "<a href='/resetwifi' style='color:#ff5555; text-decoration:none;'>Reset WiFi Credentials</a>";
  page += "</div>";
  page += "</div></body></html>";
  return page;
}

void handleRelay(int relayNumber) {
  if (server.method() == HTTP_POST) {
    String newStateStr = server.arg("state");
    relayState[relayNumber] = (newStateStr == "1");
    updateRelays();
    saveRelayState();
  }
  server.send(200, "text/html", relayHtml());
}

void handleRoot() {
  server.send(200, "text/html", relayHtml());
}

void handleResetWiFi() {
  wm.resetSettings();
  // Clear saved WiFi credentials
  server.send(200, "text/html", "<html><body><h2>WiFi credentials reset. Restarting...</h2></body></html>");
  delay(2000);
  ESP.restart();
  // Restart to apply reset
}

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered Configuration Mode");
  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(RELAY_3_PIN, OUTPUT);
  pinMode(RELAY_4_PIN, OUTPUT);

  Serial.begin(115200);

  if (!SPIFFS.begin(false) && !SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
  }

  loadRelayState();
  updateRelays();

  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);

  if (!wm.autoConnect("Home_Automation", "12345678")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/relay1", HTTP_POST, []() {
    handleRelay(0);
  });
  server.on("/relay2", HTTP_POST, []() {
    handleRelay(1);
  });
  server.on("/relay3", HTTP_POST, []() {
    handleRelay(2);
  });
  server.on("/relay4", HTTP_POST, []() {
    handleRelay(3);
  });

  // Add reset WiFi route
  server.on("/resetwifi", HTTP_GET, handleResetWiFi);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
