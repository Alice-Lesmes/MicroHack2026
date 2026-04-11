// --- HARDWARE CONFIGURATION ---
// Comment out the line below to disable the MPU6050 and use placeholder data instead.
//#define ENABLE_MPU 

#ifdef ENABLE_MPU
  #include <Adafruit_MPU6050.h>
  #include <Adafruit_Sensor.h>
  #include <Wire.h>
#endif

#include <WiFi.h>
#include <HTTPClient.h>

#ifdef ENABLE_MPU
  Adafruit_MPU6050 mpu;
#endif

#define DEBUG_MODE 0



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

WiFiClient client;

const char* ssid = "GL-SFT1200-428";
const char* pass = "goodlife";
const char* host = "192.168.8.204";
const int16_t port = 8080;

// Hardware Pins
const int TRIGGER_PIN = 15;
const int OUT_TEST = 25;
int out_toggle = 0;

#define STM32_RX_PIN 17
#define STM32_TX_PIN 18

// Moving consts.
const int STOPPED = 0;
const int MOVING = 1;

// INTENSITY
const int WALK_SPEED= 0;
const int RUN_SPEED = 1;

// Direction consts.
const int MOV_CENTER = 0;
const int MOV_FORWARD = 1;
const int MOV_BACKWARDS = 2;
// Tilt.
const int CENTER = 0;
const int LEFT = 1;
const int RIGHT = 2;

// Helper variables.
float accelXOldValue = 0.0;
float accelXCurrValue = 0.0;
float accelXMoveThreshold = 2.0;

float gyroXCurrValue = 0.0;
float gyroXOldValue = 0.0;
float gyroXMoveThreshold = 0.2;

// Result to socket.
int currentMovement = STOPPED;
int currentSpeed = WALK_SPEED;
int currentDirection = MOV_FORWARD;
int currentTilt = CENTER;
int currentTrigger = 0; 

int read_delay = 10;

HardwareSerial STM32Serial(1);

void setup(void) {
  Serial.begin(115200);
  STM32Serial.begin(115200, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);

  // Configure the trigger pin
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(OUT_TEST, OUTPUT);

#ifdef ENABLE_MPU
  // Try to initialize MPU!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6050 Initialized!");
#else
  Serial.println("MPU6050 Disabled. Using placeholder data.");
#endif

  // Connecting to wifi...
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("Connecting to socket!");
  connectSocket();
  Serial.println("Connected!");
  Serial.println("Starting main loop!\n");
  delay(100);
}

void loop() {
  if (client.connected()) {
    mpu_read(); // This will use real or placeholder data based on the #define
    
    // Read the button state (Works regardless of MPU state)
    if (digitalRead(TRIGGER_PIN) == LOW) {
        currentTrigger = 1;
    } else {
        currentTrigger = 0;
    }

    // if (out_toggle) {
    //     digitalWrite(OUT_TEST, HIGH);
    // } else {
    //     digitalWrite(OUT_TEST, LOW);
    // }

    // out_toggle = 1 - out_toggle;
    // delay(100);

    // Convert all values to strings
    String currMov = String(currentMovement);
    String currSpeed = String(currentSpeed);
    String currDir = String(currentDirection);
    String currTilt = String(currentTilt);
    String currTrig = String(currentTrigger);
    
    // Append the trigger state to the end of the payload
    String result = String(currMov + currSpeed + currDir + currTilt + currTrig);
    
    DEBUG_PRINTLN(result);
    STM32Serial.print("F\n");
    client.print(result);
  } else {
    client.stop();
    connectSocket();
  }
  DEBUG_PRINTLN("----");
  delay(read_delay);
}

void connectSocket() {
  while (!client.connect(host, port)) {
    Serial.println("Connection to host failed");
    delay(500);
  }
}

void mpu_read() {
#ifdef ENABLE_MPU
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  accelXCurrValue = abs(a.acceleration.x);
  float accelXDiff = accelXCurrValue - accelXOldValue;
  float accelXDiffAbs = abs(accelXDiff);

  gyroXCurrValue = abs(g.gyro.x);
  float gyroXDiff = gyroXCurrValue - gyroXOldValue;
  float gyroXDiffAbs = abs(gyroXDiff);
  
  if (gyroXDiffAbs > gyroXMoveThreshold && accelXDiffAbs > accelXMoveThreshold) {
    currentMovement = MOVING;
  } else {     
    currentMovement = STOPPED;
  }

  gyroXOldValue = gyroXCurrValue;
  accelXOldValue = accelXCurrValue;

#else
  // --- PLACEHOLDER DATA ---
  // When the MPU is disabled, this code runs instead.
  // Change these values if you want to test different states via the socket.
  currentMovement = STOPPED; 
  currentSpeed = WALK_SPEED;
  currentDirection = MOV_FORWARD;
  currentTilt = CENTER;
#endif
}