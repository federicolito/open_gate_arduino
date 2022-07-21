//**************************************
//*********** MQTT CONFIG **************
//**************************************
String clientId = "arduino_did-";
const char* mqtt_server = "psironi.dynamic-dns.net";
const int mqtt_port = 1883;
const char* mqtt_user = "homeassistant";
const char* mqtt_pass = "doh0aeShae5urooghahxaijeiv7zesh9viech4sie4opheaj9iebouhaixah4eez";
const char* root_topic_subscribe = "control/devices/#";



void MQTTSetup(){
    client.setServer(mqtt_server, mqtt_port);
    client.setBufferSize(512);
    client.setCallback(callback);
}

void callback(char* topic, byte * payload, unsigned int length) {
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);
  Serial.println("");
  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  Serial.println(WiFi.macAddress());
  DynamicJsonDocument doc(500);
  char json[incoming.length() + 1];
  incoming.toCharArray(json, incoming.length() + 1);
  deserializeJson(doc, json);
  executeActions(doc, false, false);
}


void sendReply(String mensaje) {
  if(client.connected()){
    char rt[50];
    char conf[mensaje.length() + 1];
    mensaje.toCharArray(conf, mensaje.length() + 1);
    ("controlgates/devices/"  + String (WiFi.macAddress()).substring(3, 17)).toCharArray(rt, 50);
    client.publish(rt, conf);
    client.loop();
  }
}

void reconnectMQTT() {
  if (connectedToWiFi){
    clientId += String(random(0xffff), HEX);


  
    //if (connectedToWiFi) {
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    // Intentamos conectar
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Conectado!");
      // Nos suscribimos
      if (client.subscribe(root_topic_subscribe)) {
        Serial.println("Suscripcion ok");
      } else {
        Serial.println("fallo Suscripciión");
        timerRecMQTT = millis();
      }
    } else {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 15 segundos");
      
    }
  }

}