#include <WiFi.h>
#include <WiFiUdp.h>



// --- DEBUG CONFIGURATION ---
// Set to 1 to enable Serial output, set to 0 to completely remove it
#define DEBUG_MODE 1
#define USE_UDP 1
#define BAUD_RATE 9600


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
// Define your custom pins to keep the code clean
// Assuming you want RX on 17 and TX on 18
#define STM32_RX_PIN 17
#define STM32_TX_PIN 18
#define STM32_NEIGHBOUR 16
#define STM32_NEIGHBOUR_2 8


const char* ssid = "GL-SFT1200-428";
const char* password = "goodlife";

const int port = 8080;
WiFiServer server(port);
// --- USB Host Config ---
HardwareSerial STM32Serial(1);

WiFiUDP udp;
char incomingPacket[255]; // Buffer for incoming data


void setup() {
  // This will only run if DEBUG_MODE is 1
  Serial.begin(BAUD_RATE);
  STM32Serial.begin(BAUD_RATE, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
  delay(100); // Small delay to allow USB CDC to connect
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
  pinMode(STM32_NEIGHBOUR, OUTPUT);
  pinMode(STM32_NEIGHBOUR_2, OUTPUT);
  digitalWrite(STM32_NEIGHBOUR, LOW);
  digitalWrite(STM32_NEIGHBOUR_2, LOW);

  #if USE_UDP
// 2. Start listening on the UDP port
  if (udp.begin(port)) {
    DEBUG_PRINTLN("Listening for UDP packets on port");
  }
#else 
  server.begin();
#endif
  DEBUG_PRINTLN("Server started. Listening for messages...");
}

void loop() {
    #if USE_UDP
// 3. Check if a new UDP packet has arrived
  int packetSize = udp.parsePacket();
  // 4. Read the packet into the buffer
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0; // Null-terminate the string so it prints correctly
      DEBUG_PRINTLN("Sent F to STM32 via uart (UDP)");
      STM32Serial.print("F\n");
    }
#else 
  WiFiClient client = server.available();

  if (client) {
    DEBUG_PRINTLN("\n[New Client Connected]");
    
    while (client.connected()) {
      if (client.available()) {
        String request = client.readStringUntil('\n');
        if (request == "F") {
          // Send "F" to the STM32 over USB CDC
          STM32Serial.print("F\n");
          DEBUG_PRINTLN("Sent F to STM32 via uart");
        }
        DEBUG_PRINT("Received: ");
        DEBUG_PRINTLN(request);
 

      }
    }
    
    client.stop();
    DEBUG_PRINTLN("[Client Disconnected]");
  }
  #endif
}