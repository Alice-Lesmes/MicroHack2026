
#include <WiFi.h>

// Replace with your Wi-Fi credentials
const char* ssid = "GL-SFT1200-428";
const char* password = "goodlife";

// Create a TCP server listening on port 8080
WiFiServer server(8080);

void setup() {
  Serial.begin(115200);

  // 1. Connect to Wi-Fi
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  
  // You will need this IP address for your Python script!
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP()); 

  // 2. Start the TCP server
  server.begin();
  Serial.println("Server started. Listening for messages...");
}

void loop() {
  // 3. Check if a Python client has connected
  WiFiClient client = server.available();

  if (client) {
    Serial.println("\n[New Client Connected]");
    
    // 4. Read data while the client remains connected
    while (client.connected()) {
      if (client.available()) {
        // Read the incoming string until a newline character is received
        String request = client.readStringUntil('\n');
        
        Serial.print("Received: ");
        Serial.println(request);
      }
    }
    
    // 5. Close the connection when done
    client.stop();
    Serial.println("[Client Disconnected]");
  }
}