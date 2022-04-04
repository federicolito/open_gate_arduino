void executeActions(DynamicJsonDocument doc, bool local) {

  /// pondremos el usuario y clave del mqtt en una posicion de la EEPROM y sera actualizada por la app cuando se configure el Wifi del name.


  String mac = String (WiFi.macAddress()).substring(3, 17);
  //  mensajes    {"t":"devices/mac(sin el primer grupo)","a":"getfc"}
  if ("devices/" + mac == doc["t"]) {
    Serial.println("Mensaje recibido para este dispositivo mac: " + mac);
    bool notFound = true;
    String actions = doc["a"];
    Serial.println("actions: " + actions);
    if (actions == "getfc") { // get finales de carrera
      notFound = false;
      doc["d"]["fc1"] = digitalRead(pinCloseSwitch1)? 0:1;
      doc["d"]["fc2"] = digitalRead(pinCloseSwitch2)? 0:1;
      serializeJson(doc, Serial);
    }
    if (actions == "set1") { // set comandos
      notFound = false;
      timerp1 = millis();
      Serial.println("Pulso1 disparado");
    }
    if (actions == "set2") { // set comandos
      notFound = false;
      timerp2 = millis();
      Serial.println("Pulso2 disparado");
    }
    if (actions == "getv") { // check version
      notFound = false;
      doc["d"]["v"] = ver;
      serializeJson(doc, Serial);
    }
    if (actions == "gettype") { // check version
      notFound = false;
      doc["d"]["t"] = (gateType)? 1: 0 ;
      serializeJson(doc, Serial);
    }
    if (actions == "getmqtt") {
      notFound = false;
      int mqttStatus = 0;
      if (client.connected()) {
        mqttStatus = 1;
      }
      else {
        mqttStatus = 0;
      }

      doc["d"]["m"] = mqttStatus;
      serializeJson(doc, Serial);
    }
    if (actions == "getip") {
      notFound = false;
      String localIp = WiFi.localIP().toString();
      doc["d"]["ip"] = localIp;
      serializeJson(doc, Serial);
    }
    if (actions == "settimers") {
      notFound = false;
      dayTimer = doc["d"]["d"];
      nightTimer = doc["d"]["n"];
      EEPROM.put(204, dayTimer);
      EEPROM.commit();
      EEPROM.put(208, nightTimer);
      EEPROM.commit();
      serializeJson(doc, Serial);
    }
    if (actions == "gettimers") {
      notFound = false;
      EEPROM.get(204, dayTimer);
      EEPROM.get(208, nightTimer);

      doc["d"]["d"] = dayTimer;
      doc["d"]["n"] = nightTimer;
      serializeJson(doc, Serial);
    }
    if (actions == "setutc") {
      notFound = false;
      int utcRe = doc["d"]["u"];
      EEPROM.put(200, utcRe);
      EEPROM.commit();
      timeZone = utcRe;
      serializeJson(doc, Serial);
    }
    if (actions == "getutc") {
      notFound = false;
      EEPROM.get(200, timeZone);

      doc["d"]["u"] = timeZone;
      serializeJson(doc, Serial);
    }
    if (actions == "getcw") { // connect to a wifi
      notFound = false;
      String ssidScaned = "";
      String rssiScaned = "";

      if (WiFi.status() == WL_CONNECTED) {
        ssidScaned = WiFi.SSID();
        rssiScaned = String(WiFi.RSSI());
      }

      doc["d"]["s"] = ssidScaned;
      doc["d"]["r"] = rssiScaned;

      serializeJson(doc, Serial);
    }
    if (actions == "set") {
        notFound = false;
        grabarStr(100, doc["d"]["n"]);
        grabarStr(150, doc["d"]["p"]);
        serializeJson(doc, Serial);
      }
      if (actions == "get") {
        notFound = false;
        doc["d"]["n"] = leerStr(100);
        doc["d"]["p"] = leerStr(150);
        serializeJson(doc, Serial);
      }
    if (local) {                          ////////                                                   only udp
      
      if (actions == "connectwifi") { // connect to a wifi
        notFound = false;
        connectToWiFi(doc["d"]["ssid"], doc["d"]["pass"]);
        if (connectedToWiFi) {
          Serial.println("Connected");
        } else {
          reconnectWiFi();
          doc["d"] = "error";
          doc["status"] = "error";
          Serial.println("Incorrect ssid or password");
        }
      }
      if (actions == "disconnectw") { // connect to a wifi
        notFound = false;
        String ssid = leerStr(0);
        doc["d"]["s"] = ssid;
        grabarStr(0, "");
        grabarStr(50, "");
        serializeJson(doc, Serial);
      }

      if (actions == "getw") { // get wifi list
        notFound = false;
        Serial.println("actions: getlistwifi");
        int n = WiFi.scanNetworks();
        //delay(1000);// ELIMINTAR
        Serial.println("scan done");
        if (n == 0) {
          doc["d"] = "No networks found";
          Serial.println("No networks found");
        } else {
          Serial.print(n);
          Serial.println(" Networks found");
          JsonObject root = doc.to<JsonObject>();

          JsonArray arr = root.createNestedArray("d");
          for (int i = 0; i < n; ++i) {
            String ssidScaned = WiFi.SSID(i);
            String rssiScaned = String(WiFi.RSSI(i));
            char encrypScaned = WiFi.encryptionType(i);
            DynamicJsonDocument wifi(256);
            wifi["s"] = ssidScaned;
            wifi["r"] = rssiScaned;
            wifi["e"] = encrypScaned;
            arr.add(wifi);
          }
          serializeJson(doc, Serial);
        }
      }
      if (actions == "setota") { // activar modo ota
        /// {"a":"setota","t":"devices/mac sin el primer dupla"}
        notFound = false;
        ota = true;
        timerOta = millis();
        char ssid[50];
        char password[50];
        leerStr(0).toCharArray(ssid, 50);
        leerStr(50).toCharArray(password, 50);
        const char* host     = "ESP-OTA"; // Used for MDNS resolution
        init_wifi(ssid, password, host);

        serializeJson(doc, Serial);
      }
      
      
    }
    if (notFound) {
      Serial.println("Error action not found.");
      doc["a"] = "error";

    }
    char json_string[256];
    serializeJson(doc, json_string);
    String result = String(json_string);
    if ( local) {

      Udp1.beginPacket(Udp1.remoteIP(), 8890);
      char conf[result.length() + 1];
      result.toCharArray(conf, result.length() + 1);
      Udp1.write(conf);
      Udp1.endPacket();
    } else {
      sendReply(result);
    }
    if (actions == "reset") {
        ESP.reset();
      }
    if ( local) {
      
      if (actions == "disconnectw") {
        WiFi.disconnect();
      }
      if (actions == "set"){
        if (leerStr(150)!= pass_AP){
          pass_AP = leerStr(150);
          generateSoftAP();
        }
      }
    }

  }
}
