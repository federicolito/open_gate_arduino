
void handleHttpSetup(){
  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/",HTTP_GET,  handleRoot);
  server.on(F("/settings"), HTTP_GET, getSettings);
  server.onNotFound(handleNotFound);
  server.begin();  // Web server start
  Serial.println("HTTP server started");
}

/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) {  // If caprive portal redirect instead of displaying the page.
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page += "<!DOCTYPE html><html lang='en'><head>"
            "<meta name='viewport' content='width=device-width'>"
            "<title>Go Back to the app Open Gate</title></head><body>"
            "<h1></h1>"
            "<p>To continue go back to the app Open Gate to finish the configuration</p>"
            "<a href='https://federicosironi.page.link/devices'>Open Gate</a>"
            "</body></html>";

  server.send(200, "text/html", Page);
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname) + ".local")) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop();              // Stop is needed because we sent no content length
    return true;
  }
  return false;

}
void getSettings() {

    String message = server.arg("message");
    DynamicJsonDocument doc(500);
    char json[message.length() + 1];
    message.toCharArray(json, message.length() + 1);
    deserializeJson(doc, json);
    String response = executeActions(doc, true, true);
    server.send(200, "text/json", response);
}

//server.arg("n")

/** Wifi config page handler */



void handleNotFound() {
  if (captivePortal()) {  // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) { message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n"); }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}
