/*
 * IRremoteESP8266: IRServer - demonstrates sending IR codes controlled from a webserver
 * Version 0.3 May, 2019
 * Version 0.2 June, 2017
 * Copyright 2015 Mark Szabo
 * Copyright 2019 David Conran
 *
 * An IR LED circuit *MUST* be connected to the ESP on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */
#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif  // ESP8266
#if defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#endif  // ESP32
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WiFiClient.h>
#include <cstdlib>
#include "IRutils.h"
#include <ir_Gree.h>


const char* kSsid = "avivrad_Y";
const char* kPassword = "RADKOEAZA32";
MDNSResponder mdns;

#if defined(ESP8266)
ESP8266WebServer server(80);
#undef HOSTNAME
#define HOSTNAME "esp8266"
#endif  // ESP8266
#if defined(ESP32)
WebServer server(80);
#undef HOSTNAME
#define HOSTNAME "esp32"
#endif  // ESP32

const uint16_t kIrLed = 4;  // ESP GPIO pin to use. Recommended: 4 (D2).
IRGreeAC ac(kIrLed);  // Set the GPIO to be used for sending messages.
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

void handleRoot() {
  server.send(200, "text/html",
              "<html>" \
                "<head><title>" HOSTNAME " Demo</title></head>" \
                "<body>" \
                  "<h1>Hello from " HOSTNAME ", you can send NEC encoded IR" \
                      "signals from here!</h1>" \
                  "<p><a href=\"ir?code=0x3C09605000200010\">Send HEAT ON</a></p>" \
                  "<p><a href=\"ir?code=0x3409605000200090\">Send HEAT OFF</a></p>" \
                  "<p><a href=\"ir?code=16771222\">Send 0xFFE896</a></p>" \
                "</body>" \
              "</html>");
}

void handleRoot2(uint64_t code) {
  String respone= "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<RESPONE>\n<STATUS>Good</STATUS>\n<CODE>";
  respone+=uint64ToString(code);
  respone+="</CODE>\n</RESPONE>\n";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/xml",respone);
  printState();
}

void handleIr() {
  uint64_t code;
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "code") {
      Serial.println(server.arg(i).c_str());
      code = strtoull(server.arg(i).c_str(), NULL, 0);
      Serial.print("after convertion");
      Serial.println(uint64ToString(code));
//    irsend.sendGree(code);
      ac.send();
    }
  }
  handleRoot2(code);
  //handleRoot();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 232);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1); // optional
//IPAddress secondaryDNS(8, 8, 4, 4); // optional

void printState() {
  // Display the settings.
  Serial.println("GREE A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char* ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kGreeStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

void setup(void) {
  irsend.begin();
  ac.begin();
  Serial.begin(115200);
  
  // Configures static IP address
if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
  Serial.println("STA Failed to configure");
}
  WiFi.begin(kSsid, kPassword);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(kSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());
  ac.on();
//  ac.off();
  ac.setFan(1);
  // kGreeAuto, kGreeDry, kGreeCool, kGreeFan, kGreeHeat
  ac.setMode(kGreeHeat);
  ac.setTemp(25);  // 16-30C
  ac.setSwingVertical(true, kGreeSwingAuto);
  ac.setXFan(false);
  ac.setLight(true);
  ac.setSleep(false);
  ac.setTurbo(false);


#if defined(ESP8266)
  if (mdns.begin(HOSTNAME, WiFi.localIP())) {
#else  // ESP8266
  if (mdns.begin(HOSTNAME)) {
#endif  // ESP8266
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/ir", handleIr);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
}
