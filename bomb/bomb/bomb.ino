#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// --- Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Use Hardware Serial 2 (TX2=17, RX2=16)
HardwareSerial dfSerial(2); 
DFRobotDFPlayerMini myDFPlayer;

// Pin Definitions
const int BTN_BEEP = 4;
const int BTN_WIN  = 5;
const int BTN_STOP = 18;

// --- Memory Exploit Structure (Stack Smashing) ---
// Using a struct to ensure the buffer and flag are adjacent in memory.
struct VulnerableMemory {
  char passwordBuffer[16]; 
  bool isDefused;          
};
VulnerableMemory bombMem;

unsigned long lastHeartbeat = 0;

// UI update helper
void updateDisplay(const char* title, const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(title);
  
  display.setCursor(0, 22);
  display.setTextSize(1);
  display.println(line1);
  
  display.setCursor(0, 42);
  display.setTextSize(1);
  display.println(line2);
  display.display();
}

void setup() {
  // 1. Initialize Terminal Serial
  Serial.begin(115200);
  Serial.setTimeout(500); 

  // Reset internal state
  bombMem.isDefused = false;
  memset(bombMem.passwordBuffer, 0, sizeof(bombMem.passwordBuffer));

  // 2. Initialize OLED (I2C)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("OLED Init Failed");
  } else {
    updateDisplay("BOMB SYSTEM", "BOOTING UP...", "PLEASE WAIT");
  }

  // 3. Initialize DFPlayer Mini (UART)
  // Default baud rate is 9600
  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  
  // Critical: Wait for SD card to mount
  delay(3000); 

  // Start DFPlayer without waiting for ACK to prevent software hangs
  if (!myDFPlayer.begin(dfSerial, false)) { 
    updateDisplay("SYS ERROR", "AUDIO TIMEOUT", "CHECK SD CARD");
    Serial.println("DFPlayer Error: Check wiring and SD card (FAT32).");
  } else {
    myDFPlayer.volume(28); // Volume range 0-30
    updateDisplay("BOMB SYSTEM", "AUDIO READY", "STATUS: ARMED");
  }

  // 4. Initialize Buttons
  pinMode(BTN_BEEP, INPUT_PULLUP);
  pinMode(BTN_WIN,  INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);

  Serial.println(">>> BOMB TERMINAL ONLINE <<<");
}

void loop() {
  // --- 0. System Heartbeat ---
  if (millis() - lastHeartbeat > 2000) {
    Serial.println("Heartbeat: Active");
    lastHeartbeat = millis();
  }

  // --- 1. Physical Trigger Logic ---
  
  // Button 4: Play Beeping (001.mp3)
  if (digitalRead(BTN_BEEP) == LOW) {
    updateDisplay("ACTION", "PLAYING BEEP", "01/001.MP3");
    myDFPlayer.playFolder(1, 1);
    delay(400); 
  }
  
  // Button 5: Play Victory (002.mp3)
  if (digitalRead(BTN_WIN) == LOW) {
    updateDisplay("ACTION", "DEFUSED!", "01/002.MP3");
    myDFPlayer.playFolder(1, 2);
    delay(400); 
  }

  // Button 18: Stop Audio
  if (digitalRead(BTN_STOP) == LOW) {
    updateDisplay("ACTION", "STOPPED", "SYSTEM IDLE");
    delay(50); // Small delay to let I2C finish
    myDFPlayer.pause(); 
    delay(400); 
  }

  // --- 2. Serial Console (Stack Smashing Attack) ---
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim(); 

    if (input.length() > 0) {
      updateDisplay("TERMINAL", "INPUT DATA:", input.c_str());

      // VULNERABLE FUNCTION: Copying input into 16-byte buffer without bounds check.
      // If input > 16 chars, it overwrites 'isDefused' in memory.
      memcpy(bombMem.passwordBuffer, input.c_str(), input.length());
      
      // Standard password logic
      if (strcmp(bombMem.passwordBuffer, "SECRET_CODE") == 0) {
        bombMem.isDefused = true;
      }

      // Check if memory was tampered via overflow
      if (bombMem.isDefused) {
        updateDisplay("HACKED!!", "STATUS: DEFUSED", "CT WIN");
        myDFPlayer.playFolder(1, 2); 
        Serial.println("ALERT: Buffer overflow detected. Bomb hacked.");
        bombMem.isDefused = false; // Reset for demonstration
      } else {
        Serial.println("ERROR: Access Denied.");
      }
      
      // Clear buffer for next attempt
      memset(bombMem.passwordBuffer, 0, sizeof(bombMem.passwordBuffer));
    }
  }
}