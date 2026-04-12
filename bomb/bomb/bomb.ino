#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// --- System Constants ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

HardwareSerial dfSerial(2); 
DFRobotDFPlayerMini myDFPlayer;

// --- Pinout ---
const int BTN_RESTART = 19; // START / RESET
const int BTN_0       = 4;  // Binary 0
const int BTN_1       = 5;  // Binary 1
const int BTN_HOLD    = 18; // RGB Sync Button

const int LED_RED     = 25; 
const int LED_GREEN   = 26; 
const int RGB_R       = 27; const int RGB_G = 32; const int RGB_B = 33;

// --- Game Logic Variables ---
enum GameState { IDLE, MOD1_HEX, MOD2_COLOR, MOD3_MORSE, DEFUSED, EXPLODED };
GameState currentState = IDLE;

const float GAME_DURATION = 150.0;
unsigned long startTime = 0;
bool timerRunning = false;
int lastDisplayedSec = -1;

// Module Parameters
uint8_t hexTarget = 0;
int bitsEntered = 0;
int colorState = 0; // 0=Red, 1=Yellow, 2=Blue
unsigned long lastColorTime = 0;
unsigned long lastBtnTime = 0; 
bool mod2Locked = true;

// Audio State Management
unsigned long morsePhaseStart = 0;
bool morseAudioTriggered = false;
int morsePlayCount = 0;
bool morseFinished = false;
bool mod2TickingStarted = false;

// Vulnerable Memory (For Terminal Hacks)
struct VulnerableMemory {
  char passwordBuffer[16]; 
  bool isExploitTriggered;          
};
VulnerableMemory bombMem;

// ==========================================
// HARDWARE INTERFACE
// ==========================================

void setRGB(bool r, bool g, bool b) {
  digitalWrite(RGB_R, r); digitalWrite(RGB_G, g); digitalWrite(RGB_B, b);
}

void updateDisplay(int t = 0) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0); display.setTextSize(1);
  
  if (currentState == IDLE) display.println("SYSTEM OFFLINE");
  else if (currentState == DEFUSED) display.println("THREAT NEUTRALIZED");
  else if (currentState == EXPLODED) display.println("DETONATION OCCURRED");
  else display.printf("ARMED: %d s\n", t);

  display.drawLine(0, 10, 128, 10, WHITE);
  display.setCursor(0, 15);
  
  switch (currentState) {
    case IDLE:       display.println("\nPRESS PIN 19\nTO INITIALIZE"); break;
    case MOD1_HEX:   
      display.printf("\nDECRYPT: 0x%02X\n", hexTarget); 
      display.print("BITS: ");
      for(int i=0; i<bitsEntered; i++) display.print("*");
      break;
    case MOD2_COLOR: display.println("\nRGB SYNC\nHOLD PIN 18"); break;
    case MOD3_MORSE: display.println("\nMORSE INTERCEPTED\nAWAITING TERMINAL"); break;
    case DEFUSED:    display.println("\nMISSION SUCCESS"); break;
    case EXPLODED:   display.println("\nBOMB DETONATED"); break;
  }
  display.display();
}

void resetBomb() {
  currentState = IDLE;
  timerRunning = false;
  bitsEntered = 0;
  mod2Locked = true;
  morseAudioTriggered = false;
  morsePlayCount = 0;
  morseFinished = false;
  mod2TickingStarted = false;
  bombMem.isExploitTriggered = false;
  memset(bombMem.passwordBuffer, 0, 16);
  digitalWrite(LED_RED, LOW); digitalWrite(LED_GREEN, LOW); setRGB(0, 0, 0);
  myDFPlayer.stop();
  updateDisplay();
  Serial.println("\n--- SYSTEM REBOOTED TO OFFLINE ---");
}

void triggerExplosion() {
  currentState = EXPLODED;
  timerRunning = false;
  digitalWrite(LED_RED, HIGH); setRGB(0, 0, 0);
  myDFPlayer.playFolder(1, 4); // 004.mp3
  updateDisplay();
}

void triggerDefuse() {
  currentState = DEFUSED;
  timerRunning = false;
  digitalWrite(LED_RED, LOW); digitalWrite(LED_GREEN, HIGH); setRGB(0, 1, 0);
  myDFPlayer.playFolder(1, 2); // 002.mp3
  updateDisplay();
}

// ==========================================
// SETUP
// ==========================================

void setup() {
  Serial.begin(115200);
  pinMode(BTN_RESTART, INPUT_PULLUP);
  pinMode(BTN_0, INPUT_PULLUP);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_HOLD, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT); pinMode(LED_GREEN, OUTPUT);
  pinMode(RGB_R, OUTPUT); pinMode(RGB_G, OUTPUT); pinMode(RGB_B, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  delay(1500);
  if(myDFPlayer.begin(dfSerial, false)) myDFPlayer.volume(30);

  resetBomb();
}

// ==========================================
// MAIN EXECUTION
// ==========================================

void loop() {
  // 1. RESTART / INITIALIZE LOGIC
  if (digitalRead(BTN_RESTART) == LOW) {
    if (millis() - lastBtnTime > 1000) {
      resetBomb();
      hexTarget = random(0x10, 0xFF);
      startTime = millis();
      timerRunning = true;
      currentState = MOD1_HEX;
      myDFPlayer.playFolder(1, 3); // Start Ticking (003)
      
      Serial.printf("BOMB ARMED! Target Hex: 0x%02X\n", hexTarget);
      Serial.print("Required Binary: ");
      for(int i=7; i>=0; i--) Serial.print((hexTarget >> i) & 0x01);
      Serial.println();
      lastBtnTime = millis();
    }
  }

  // 2. REAL-TIME TIMER
  float timeLeft = 0;
  if (timerRunning) {
    timeLeft = GAME_DURATION - (millis() - startTime) / 1000.0;
    if (timeLeft <= 0) { triggerExplosion(); return; }
    
    int currentSec = (int)ceil(timeLeft);
    if (currentSec != lastDisplayedSec) {
      updateDisplay(currentSec);
      lastDisplayedSec = currentSec;
    }
    digitalWrite(LED_RED, (millis() / 500) % 2); // Pulse status
  }

  // 3. TERMINAL COMMANDS (Backdoor & Logic)
  if (Serial.available() > 0 && currentState != IDLE) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Stack Smash Exploit (> 16 chars)
    memcpy(bombMem.passwordBuffer, input.c_str(), input.length());
    if (bombMem.isExploitTriggered) { 
      Serial.println("MEMORY EXPLOIT DETECTED. FORCING OFFLINE.");
      resetBomb(); 
      return; 
    }

    // Master Key Bypass
    if (input == "paopao") { 
      triggerDefuse(); 
      return; 
    }

    // Module 3 Password
    if (currentState == MOD3_MORSE) {
      if (input == "UQ26") triggerDefuse();
      else triggerExplosion();
    }
    memset(bombMem.passwordBuffer, 0, 16);
  }

  // 4. MODULE STATE MACHINE
  switch (currentState) {
    
    case MOD1_HEX:
      if (millis() - lastBtnTime > 300) {
        int inputBit = -1;
        if (digitalRead(BTN_0) == LOW) inputBit = 0;
        else if (digitalRead(BTN_1) == LOW) inputBit = 1;

        if (inputBit != -1) {
          int correctBit = (hexTarget >> (7 - bitsEntered)) & 0x01;
          if (inputBit == correctBit) {
            bitsEntered++;
            lastBtnTime = millis();
            updateDisplay((int)timeLeft);
            if (bitsEntered >= 8) {
              myDFPlayer.playFolder(1, 5); // Success Sound 005
              currentState = MOD2_COLOR;
              mod2Locked = true;
              lastColorTime = millis(); // Track start of MOD2
              mod2TickingStarted = false;
            }
          } else triggerExplosion();
        }
      }
      break;

    case MOD2_COLOR:
      {
        // Resume Ticking sound after success jingle (1s)
        if (!mod2TickingStarted && (millis() - lastColorTime > 1000)) {
          myDFPlayer.playFolder(1, 3); 
          mod2TickingStarted = true;
        }

        bool isHolding = (digitalRead(BTN_HOLD) == LOW);
        if (isHolding) {
          mod2Locked = false; 
          if (millis() - lastColorTime > 2000) { // Cycle: 2 seconds
            colorState = (colorState + 1) % 3;
            if (colorState == 0) setRGB(1, 0, 0); 
            else if (colorState == 1) setRGB(1, 1, 0); 
            else if (colorState == 2) setRGB(0, 0, 1); 
            lastColorTime = millis();
          }
        } 
        else if (!mod2Locked && !isHolding) { 
          setRGB(0, 0, 0);
          float rem = fmod(timeLeft, 10.0);
          bool win = false;
          // Timing check with +/- 0.5s tolerance
          if (colorState == 1 && rem >= 4.5 && rem <= 5.5) win = true;      // Yellow (5)
          else if (colorState == 0 && rem >= 2.5 && rem <= 3.5) win = true; // Red (3)
          else if (colorState == 2 && (rem <= 0.5 || rem >= 9.5)) win = true; // Blue (0)
          
          if (win) {
            myDFPlayer.playFolder(1, 7); // Success Sound 007
            currentState = MOD3_MORSE;
            morsePhaseStart = millis(); 
            morseAudioTriggered = false;
            morsePlayCount = 0;
            morseFinished = false;
          } else triggerExplosion();
          mod2Locked = true;
        }
      }
      break;
      
    case MOD3_MORSE:
      {
        unsigned long elapsedInMorse = millis() - morsePhaseStart;

        if (!morseFinished) {
          // Cycle: 2s Gap -> 13s Audio -> 2s Gap = 17s total loop
          if (elapsedInMorse < 2000) {
            // Gap phase: Ensure silence
            if (morseAudioTriggered) { myDFPlayer.stop(); morseAudioTriggered = false; }
          } 
          else if (elapsedInMorse >= 2000 && elapsedInMorse < 15000) {
            // Play phase
            if (!morseAudioTriggered) {
              myDFPlayer.playFolder(1, 6); 
              morseAudioTriggered = true;
              morsePlayCount++;
              Serial.print("Morse Loop: "); Serial.println(morsePlayCount);
            }
          } 
          else if (elapsedInMorse >= 17000) {
            // Check if we need another loop
            if (morsePlayCount >= 2) {
              morseFinished = true;
              myDFPlayer.playFolder(1, 3); // Resume Ticking indefinitely
              Serial.println("Morse Complete. Resuming Ticking.");
            } else {
              morsePhaseStart = millis(); // Reset for Loop 2
              morseAudioTriggered = false;
            }
          }
        }
      }
      break;

    default: break;
  }
}