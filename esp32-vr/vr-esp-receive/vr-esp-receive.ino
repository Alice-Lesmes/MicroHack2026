#include <WiFi.h>

// --- DEBUG CONFIGURATION ---
// Set to 1 to enable Serial output, set to 0 to completely remove it
#define DEBUG_MODE 1

#if DEBUG_MODE
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_BEGIN(x)    Serial.begin(x)
#else
  // If DEBUG_MODE is 0, these macros do absolutely nothing
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_BEGIN(x)
#endif
// ---------------------------

const char* ssid = "GL-SFT1200-428";
const char* password = "goodlife";

WiFiServer server(8080);

void setup() {
  // This will only run if DEBUG_MODE is 1
  DEBUG_BEGIN(115200);
  delay(1000); // Small delay to allow USB CDC to connect

  DEBUG_PRINT("\nConnecting to ");
  DEBUG_PRINTLN(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN("\nWiFi connected.");
  DEBUG_PRINT("ESP32 IP Address: ");
  DEBUG_PRINTLN(WiFi.localIP()); 

  server.begin();
  DEBUG_PRINTLN("Server started. Listening for messages...");
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    DEBUG_PRINTLN("\n[New Client Connected]");
    
    while (client.connected()) {
      if (client.available()) {
        String request = client.readStringUntil('\n');
        
        DEBUG_PRINT("Received: ");
        DEBUG_PRINTLN(request);
        Serial.print("G");
      }
    }
    
    client.stop();
    DEBUG_PRINTLN("[Client Disconnected]");
  }
}