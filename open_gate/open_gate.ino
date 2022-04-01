/*
  Control porton automatico mediante app movil y mqtt
*/
String ver = "1.12";
bool debug = false;
bool gateType = false; //cambiar para un porton

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
#include <TimeLib.h>
#include <WebOTA.h>

//**************************************
//*********** WIFICONFIG ***************
//**************************************
String ssid_AP = "Gate_" + String (WiFi.macAddress()).substring(3, 17);
String pass_AP;
bool connectedToWiFi = false;

WiFiClient espClient;
PubSubClient client(espClient);
// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,

WiFiUDP Udp;
WiFiUDP Udp1;

int timerp1 = 0;
int timerp2 = 0;

int timerChange1=0;
int timerChange2=0;
int horas = 0;
int minutos = 0;
int segundos = millis();
int Demora = 25000;

#define Control1 5
#define Control2 4
#define pinCloseSwitch1 12
#define pinCloseSwitch2 14

bool reportWifi;
bool reportMQTT;
int timerRecMQTT = 30000;
int timerRecWiFi = 300000;
bool horainternet = false;
bool Pulso1 = false;

int timer11 = 0;
int timer12 = 0;
bool prendido = false;
bool esperando = false;

bool locked1;
bool locked2;

int timerGate1 = 0;
int timerGate2 = 0;


int status1 = 0;
int status2 = 0;


int repeticion = 0;
int reporte = 1;
int numero = 0;

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

int timeZone = -3;     // Argentina -3  Miami -4
long sincro = 30000;
int dayTimer;
int nightTimer; 

///NTP functions
time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

#define NO_OTA_PORT
bool ota = false;
int timerOta = 0;

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "esp8266";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

/* Soft AP network parameters */
IPAddress apIP(172, 217, 28, 1);
IPAddress netMsk(255, 255, 255, 0);

void setup() {
  EEPROM.begin(2048);
  /*
   * SSID       0
   * Password 50
   * Name     100
    Password AP  150
    UTC      200 
    timerDia  204
    timerNoche 208  
  */
  if (debug) {
    Serial.begin(115200);
  }
  pinMode(Control1, OUTPUT);
  digitalWrite(Control1, LOW);
  pinMode(Control2, OUTPUT);
  digitalWrite(Control2, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pinCloseSwitch1, INPUT_PULLUP);
  pinMode(pinCloseSwitch2, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);
  
  
  Serial.println();
  Serial.println (WiFi.macAddress());
  
  WiFi.mode(WIFI_AP_STA);

  
  if (leerStr(150) == ""){
    grabarStr(150, "OpenGate1234");
  }
  pass_AP = leerStr(150);
  Serial.println("");
  Serial.print("Contraseña obtenida del eprom ");
  Serial.println(leerStr(150));
  Serial.println("");

  generateSoftAP();
  reconnectWiFi();
  
  EEPROM.get(200, timeZone);
  EEPROM.get(204, dayTimer);
  EEPROM.get(208, nightTimer);
  if (dayTimer < 40) {
    EEPROM.put(204, 40);
    EEPROM.commit();
  }
  if (nightTimer < 40) {
    EEPROM.put(208, 30);
    EEPROM.commit();
  }

 

  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  udpSetup();
  MQTTSetup();

  horas = hour();
  minutos = minute();

  bool locked1 =  digitalRead(pinCloseSwitch1);
  bool locked2 =  digitalRead(pinCloseSwitch2);

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  handleHttpSetup();
}

void loop() {
  if (ota and (millis() - timerOta) < 60000) {
    webota.handle();
    int  indicador = 100;

    if (!esperando) {
      espera(indicador);
    }
    if (!prendido) {
      ledon(indicador);
    }

    return;
  } else {
    myLoop();
  }

}

void ledStatus(){
  if (millis() - timer12 > 10000) {
    if (reporte == 1) {
      if (repeticion < 15) {
        if (!esperando) {
          ledon(50);
        }
        if (!prendido) {
          espera(50);
        }
        if (!prendido & !esperando) {
          repeticion++;
        }
      }
    }
    if (reporte == 2) {
      if (!prendido) {
        espera(2000);
      }
      if (!prendido & !esperando) {
        reporte++;
      }
    }
    // REPORTE ESTADO CONEXION WIFI
    if (reporte == 3) {
      if (!esperando) {
        ledon(200);
      }
      if (!prendido) {
        espera(1000);
      }
      if (!prendido & !esperando) {
        reporte++;
      }
    }
    if (reporte == 4) {
      if (WiFi.status() == WL_CONNECTED) {
        numero = 2;
      }
      else {
        numero = 1;
      }
      if (repeticion < numero) {
        if (!esperando) {
          ledon(200);
        }
        if (!prendido) {
          espera(200);
        }
        if (!prendido & !esperando) {
          repeticion++;
        }
      }
    }
    if (reporte == 5) {
      if (!prendido) {
        espera(2000);
      }
      if (!prendido & !esperando) {
        reporte++;
      }
    }
    // REPORTE ESTADO CONEXION MQTT
    if (reporte == 6) {
      if (repeticion < 2) {
        if (!esperando) {
          ledon(200);
        }
        if (!prendido) {
          espera(200);
        }
        if (!prendido & !esperando) {
          repeticion++;
        }
      }
    }
    if (reporte == 7) {
      if (!prendido) {
        espera(1000);
      }
      if (!prendido & !esperando) {
        reporte++;
      }
    }
    if (reporte == 8) {
      if (client.connected() ) {
        numero = 2;
      }
      else {
        numero = 1;
      }
      if (repeticion < numero) {
        if (!esperando) {
          ledon(200);
        }
        if (!prendido) {
          espera(200);
        }
        if (!prendido & !esperando) {
          repeticion++;
        }
      }
    }
    if (reporte == 9) {
      if (!prendido) {
        espera(2000);
      }
      if (!prendido & !esperando) {
        reporte++;
      }
    }
    // REPORTE ESTADO RELE
    if (reporte == 1 & repeticion == 15) {
      repeticion = 0;
      reporte++;
    }
    if (reporte == 4 & repeticion == numero) {
      repeticion = 0;
      reporte++;
    }
    if (reporte == 6 & repeticion == 2) {
      repeticion = 0;
      reporte++;
    }
    if (reporte == 8 & repeticion == numero) {
      repeticion = 0;
      reporte++;
    }
    if (reporte > 9  ) {
      timer12 = millis();
      repeticion = 0;
      reporte = 1;
    }
  }
}

void myLoop() {
  if (digitalRead(pinCloseSwitch1) == !locked1){
    locked1 = digitalRead(pinCloseSwitch1);
    if (millis()-timerChange1 > 500) {
      reportStateOfGate();
    }
    timerChange1=millis();
  }
  if (digitalRead(pinCloseSwitch2) == !locked2){
    locked2 = digitalRead(pinCloseSwitch2);
    if (millis()-timerChange2 > 500) {
      reportStateOfGate();
    }
    timerChange2=millis();
  }
  
  ledStatus();
  
  if (horainternet) {
    minutos = minute();
    segundos = second();
    horas = hour();
  }
  else {
    if  (millis() - segundos  > 60000) {  ///RELOJ
      minutos++;
      segundos = millis();
      if (minutos > 59) {
        horas++;
        minutos = 0;
        if (horas > 23) {
          horas = 0;
        }
      }
      Serial.println(String (horas) + ":" + String(minutos));
    }
  }
  int packetSize = Udp1.parsePacket();
  udpListen(packetSize);
  
  if (WiFi.status() == WL_CONNECTED) {   ///
    connectedToWiFi = true;
    
  } else {
    connectedToWiFi = false;
  }

  dnsServer.processNextRequest();
  server.handleClient();
  MDNS.update();
  if (reportWifi){
    DynamicJsonDocument doc(256);
    String mac = String (WiFi.macAddress()).substring(3, 17);
    doc["t"] = "devices/" + mac;
    doc["a"] = "getcw";
    executeActions(doc, true) ;
    reportWifi = false;
  }
  if (reportMQTT){
    DynamicJsonDocument doc(256);
    String mac = String (WiFi.macAddress()).substring(3, 17);
    doc["t"] = "devices/" + mac;
    doc["a"] = "getmqtt";
    executeActions(doc, true) ;
    reportMQTT = false;
  }
  if (!connectedToWiFi) {  ///verificarlo????d
    if (!reportWifi){
      reportWifi = true;
    }
    if ((millis() - timerRecWiFi) > 300000 ) {
      reconnectWiFi();
      timerRecWiFi = millis();
    }
  }else{
    
    if (!client.connected() ) {
      if (!reportMQTT){
        reportMQTT = true;
      }
      //Serial.println("rec:" + String(millis() - timerRecMQTT));
      if (millis() - timerRecMQTT > 30000) {
        reconnectMQTT();
        timerRecMQTT =  millis();
      }
    }else{
      client.loop();
      
    }
  }

  if ((millis() - timerp1 < 1000) or (millis() - timerp2 < 1000) ) {
    //Serial.println("Pulso presente");
    digitalWrite(LED_BUILTIN, LOW);
  }
  else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if (millis() - timerp1 < 500) {
    digitalWrite(Control1, HIGH);
  }
  else {
    digitalWrite(Control1, LOW);
  }
  if (millis() - timerp2 < 500) {
    digitalWrite(Control2, HIGH);
  }
  else {
    digitalWrite(Control2, LOW);
  }
  if (hour() < 23 and hour() > 7 ) {
    Demora = dayTimer * 1000;
  }
  else {
    Demora = nightTimer * 1000;
  }
  // CIERRE AUTOMATICO DE LOS PORTONES
  if (digitalRead(pinCloseSwitch1)) {
    if (status1 == 0) {
      timerGate1 = millis();
      status1 = 1;
    }
    if (status1 == 1 and (millis() - timerGate1 > Demora)) {
      status1 = 2;
      timerp1 = millis();
      timerGate1 = millis();
    }
    if (status1 == 2 and (millis() - timerGate1 > Demora)) {
      status1 = 1;
    }
  }
  else {
    status1 = 0;
  }
  if (digitalRead(pinCloseSwitch2)) {
    if (status2 == 0) {
      timerGate2 = millis();
      status2 = 1;
    }
    if (status2 == 1 and (millis() - timerGate2 > Demora)) {
      status2 = 2;
      timerp2 = millis();
      timerGate2 = millis();
    }
    if (status2 == 2 and (millis() - timerGate2 > Demora)) {
      status2 = 1;
    }
  }
  else {
    status2 = 0;
  }
    /*
   * SSID       0
   * Password 50
   * Name     100
    Password AP  150
    UTC      200 
    timerDia  204
    timerNoche 208  
  */
  EEPROM.get(200, timeZone);
  EEPROM.get(204, dayTimer);
  EEPROM.get(208, nightTimer);
}

//----------------Función para grabar en la EEPROM-------------------
void grabarStr(int addr, String a) {
  int tamano = a.length();
  char inchar[50];
  a.toCharArray(inchar, tamano + 1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr + i, inchar[i]);
  }
  for (int i = tamano; i < 50; i++) {
    EEPROM.write(addr + i, 255);
  }
  EEPROM.commit();
}

//-----------------Función para leer la EEPROM------------------------
String leerStr(int addr) {
  byte lectura;
  String strlectura;
  for (int i = addr; i < addr + 50; i++) {
    lectura = EEPROM.read(i);
    if (lectura != 255) {
      strlectura += String((char)lectura);
    }
  }
  return strlectura;
}

void reportStateOfGate(){
  DynamicJsonDocument doc(500);
  String mac = String (WiFi.macAddress()).substring(3, 17);
  doc["t"] = "devices/" + mac;
  doc["a"] = "getfc";
  executeActions(doc, true) ;
  executeActions(doc, false) ;
}

//----------------Función para conectarse a un nuevo wifi-------------------
void connectToWiFi(String ssidTest, String passTest) {
  ssidTest.trim();
  passTest.trim();
  
  WiFi.begin(ssidTest, passTest);
  Serial.println("");
  Serial.println(ssidTest);
  for (int i = 0; i < 50 & (WiFi.status() != WL_CONNECTED); i++) {
    digitalWrite(LED_BUILTIN, HIGH); ///LOW PARA OTROS
    Serial.print(".");
    delay(150);
  }

  if (WiFi.status() == WL_CONNECTED) {   ///
    connectedToWiFi = true;
    guardarWiFi(ssidTest, passTest);
  } else {
    connectedToWiFi = false;
  }
}

//----------------Función para grabar wifi-------------------
void guardarWiFi(String ssidTest, String passTest) {
  int inicioGrabacionssid;
  int inicioGrabacionpass;
  ssidTest.trim();
  passTest.trim();
  Serial.println("guardando wifi");

  if (ssidTest != leerStr(0) or passTest != leerStr(50)) {
    grabarStr(0, ssidTest);
    grabarStr(50, passTest);
  }

}

void reconnectWiFi() {
  
  
  Serial.println("Reconectando WiFi");
  Serial.println(leerStr(0));
  
  connectToWiFi(leerStr(0), leerStr(50));


  if (connectedToWiFi) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(leerStr(0));
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    if (!MDNS.begin(myHostname)) {
      Serial.println("Error setting up MDNS responder!");
    } else {
      Serial.println("mDNS responder started");
      // Add service to MDNS-SD
      MDNS.addService("http", "tcp", 80);
    }
  } else {
    Serial.println("");
    Serial.println("incorect SSID, Password, or out of range");
  }
  //generateSoftAP();
  
}
void generateSoftAP(){
  WiFi.softAPConfig(apIP, apIP, netMsk);
  delay(500);
  Serial.println("");
  Serial.print("Setting soft-AP ... ");
  Serial.println( WiFi.softAP(ssid_AP, pass_AP) ? "Ready" : "Failed!");
  Serial.println("");
}
//FUNCIONES DE NTP
void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');

  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer1[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer1, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer1[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer1[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer1[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer1[43];
      horainternet = true;
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      horas = hour();
      minutos = minute();

    }
  }
  Serial.println("No NTP Response :-(");
  horainternet = false;
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address

void sendNTPpacket(IPAddress & address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}



void ledon(int tiempo) {
  if (!prendido) {
    timer11 = millis();
    prendido = true;
  }
  if (prendido) {
    digitalWrite(LED_BUILTIN, LOW);
    if ((millis() - timer11)  > tiempo) {
      prendido = false;
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
}

void espera(int tiempo) {
  if (!esperando) {
    timer11 = millis();
    esperando = true;
  }
  if (esperando) {
    if ((millis() - timer11)  > tiempo) {
      esperando = false;
    }
  }
}
