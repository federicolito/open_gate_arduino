

void udpSetup(){
  Serial.println("Starting UDP");
  Udp.begin(8889);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());

  Udp1.begin(8888);
}

//Funcion udp listen
void udpListen(int packetSize) {
  if (packetSize) {
    int n = Udp1.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
    Serial.println("Contents:");
    Serial.println(packetBuffer);

    ////  LEER COMANDOS DE UDP
    int posini = 0;
    int nrocomando = 0;
    String recibido = "";
    for (int i = 0 ; i < packetSize ; i++) {
      recibido = recibido + packetBuffer[i];
    }

    Serial.println(WiFi.macAddress());
    DynamicJsonDocument doc(500);
    char json[recibido.length() + 1];
    recibido.toCharArray(json, recibido.length() + 1);
    deserializeJson(doc, json);
    executeActions(doc, true,false);
  }
}