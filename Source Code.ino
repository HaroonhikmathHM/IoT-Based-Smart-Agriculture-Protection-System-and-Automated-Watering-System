 #define BLYNK_TEMPLATE_ID "TMPL6igQeSu6P"
#define BLYNK_TEMPLATE_NAME "Smart agriculture protection system and watering"
#define BLYNK_AUTH_TOKEN "qO1Gk_kHk7FaQPIL_7-E3lroJW37KOH8"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define DHTPIN D1
#define DHTTYPE DHT22
#define BUZZER D7
#define RELAY D8
#define SOIL_MOISTURE_PIN A0

// PIR sensor pins
#define PIR_PIN D6
#define PIR_PIN_2 D0
#define PIR_PIN_3 D2 // New PIR sensor

// I2C pins for LCD
#define SDA_PIN D4
#define SCL_PIN D5

// DHT22 power control pin
#define DHT_POWER_PIN D3

char auth[] = "qO1Gk_kHk7FaQPIL_7-E3lroJW37KOH8";  // Enter your Blynk Auth token
char ssid[] = "Haroon Hikmath Router";  // Enter your WIFI SSID
char pass[] = "Hikmath100";  // Enter your WIFI Password

LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to the correct address
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;

// Variables to store sensor data
float temperature = 0;
float humidity = 0;
int soilMoisture = 0;
bool alarmActive = false;
bool pirSwitchState = false; // Store the state of PIR switch
bool pumpState = false;
bool animalDetected = false;
bool animalDetectedByPIR1 = false;
bool animalDetectedByPIR2 = false;
bool animalDetectedByPIR3 = false; // New PIR sensor

unsigned long displayTimeout = 0;
unsigned long welcomeTimeout = 20000; // Display welcome message for 20 seconds
unsigned long projectNameTimeout = 20000; // Display project name for 20 seconds
unsigned long soilMoistureAlertTimeout = 20000; // 20 seconds for soil moisture alert
unsigned long alarmTimeout = 0; // To track when to stop the alarm
bool showWelcome = true;
bool showProjectName = false;
bool soilMoistureAlert = false;
unsigned long lastChecked = 0; // To track last time rain possibilities were checked
bool rainPossible = false;

void soilMoistureSensor() {
  int rawValue = analogRead(SOIL_MOISTURE_PIN); // Read raw analog value
  Serial.print("Raw Soil Moisture Value: ");
  Serial.println(rawValue); // Print raw value for debugging

  // Convert the raw value to percentage
  soilMoisture = map(rawValue, 0, 1024, 0, 100);
  soilMoisture = (soilMoisture - 100) * -1;
  Serial.print("Mapped Soil Moisture Value: ");
  Serial.println(soilMoisture); // Print mapped value for debugging

  if (soilMoisture < 30) { // Adjust the threshold value as needed
    soilMoistureAlert = true;
  } else {
    soilMoistureAlert = false;
  }
}

void readSensors() {
  // Only read DHT22 sensor if the pump is off
  if (!pumpState) {
    digitalWrite(DHT_POWER_PIN, HIGH); // Turn on DHT22
    delay(2000); // Wait for DHT22 to stabilize
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    digitalWrite(DHT_POWER_PIN, LOW); // Turn off DHT22
  }
  soilMoistureSensor(); // Call the soil moisture sensor function
  
  Blynk.virtualWrite(V3, soilMoisture);

  if (millis() > displayTimeout && !animalDetected && !showWelcome && !showProjectName && !soilMoistureAlert) {
    updateLCD();
  }
  
}

void checkPIR() {
  if (pirSwitchState) {
    bool pir1 = digitalRead(PIR_PIN);
    bool pir2 = digitalRead(PIR_PIN_2);
    bool pir3 = digitalRead(PIR_PIN_3);

    Serial.print("PIR1: ");
    Serial.println(pir1);
    Serial.print("PIR2: ");
    Serial.println(pir2);
    Serial.print("PIR3: ");
    Serial.println(pir3);

    if (pir1) {
      animalDetectedByPIR1 = true;
      triggerAlarm();
    }
    if (pir2) {
      animalDetectedByPIR2 = true;
      triggerAlarm();
    }
    if (pir3) {  // Check new PIR sensor
      animalDetectedByPIR3 = true;
      triggerAlarm();
    }
  }

  // Check if 10 seconds have passed since the alarm was triggered
  if (alarmActive && millis() - alarmTimeout >= 10000) {
    stopAlarm();
  }
}


void triggerAlarm() {
  digitalWrite(BUZZER, HIGH);
  alarmActive = true;
  animalDetected = true;
  displayTimeout = millis() + 5000; // Display "Animal Detected" for 5 seconds
  alarmTimeout = millis(); // Reset the alarm timeout
  Blynk.logEvent("animaldetect", "WARNING! Animals have entered your agricultural area. Take the necessary steps to protect your crops. Thank you! Good luck! This project was made by Haroon Hikmath & Sarjoon."); // Log the event to Blynk
  WidgetLED LED(V5);
  LED.on();
  updateLCD();
}

void stopAlarm() {
  digitalWrite(BUZZER, LOW);
  alarmActive = false;
  animalDetected = false;
  animalDetectedByPIR1 = false;
  animalDetectedByPIR2 = false;
  animalDetectedByPIR3 = false; // Reset the new PIR sensor variable
  WidgetLED LED(V5);
  LED.off();
  updateLCD();
}

BLYNK_WRITE(V12) {
  pumpState = param.asInt();
  Serial.print("Pump state received: ");
  Serial.println(pumpState);
  digitalWrite(RELAY, pumpState);
  if (pumpState == 1) {
    Serial.println("Pump turned ON");
    digitalWrite(DHT_POWER_PIN, LOW); // Turn off DHT sensor
  } else {
    Serial.println("Pump turned OFF");
    digitalWrite(DHT_POWER_PIN, HIGH); // Turn on DHT sensor
    delay(2000); // Wait for DHT22 to stabilize
  }
  updateLCD();
}

BLYNK_WRITE(V6) {
  int pirSwitch = param.asInt();
  if (pirSwitch == 0) { // If PIR switch is turned off
    pirSwitchState = false; // Update PIR switch state
    stopAlarm(); // Stop the alarm
    Serial.println("PIR switch turned OFF");
  } else {
    pirSwitchState = true; // Update PIR switch state
    Serial.println("PIR switch turned ON");
  }
  updateLCD();
}

void updateLCD() {
  lcd.clear();
  if (showWelcome) {
    lcd.setCursor(0, 0);
    lcd.print("Welcome!");
  } else if (showProjectName) {
    lcd.setCursor(0, 0);
    lcd.print("Smart Agri Protection");
    lcd.setCursor(0, 1);
    lcd.print("Smart Watering Sys");
  } else if (animalDetected) {
    lcd.setCursor(0, 0);
    lcd.print("Animal Detected");
    lcd.setCursor(0, 1);
    lcd.print("PIR1: ");
    lcd.print(animalDetectedByPIR1 ? "Detected" : "Not Detected");
    lcd.setCursor(0, 2);
    lcd.print("PIR2: ");
    lcd.print(animalDetectedByPIR2 ? "Detected" : "Not Detected");
    lcd.setCursor(0, 3);
    lcd.print("PIR3: ");  // Add the new line for PIR3
    lcd.print(animalDetectedByPIR3 ? "Detected" : "Not Detected");
  } else if (soilMoistureAlert) {
    lcd.setCursor(0, 0);
    lcd.print("Soil moisture low");
    lcd.setCursor(0, 1);
    lcd.print("Turn on water pump");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature);
    lcd.print("C H:");
    lcd.print(humidity);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("S:");
    lcd.print(soilMoisture);
    lcd.print("% ");

    if (pumpState) {
      lcd.print("Pump:ON");
    } else {
      lcd.print("Pump:OFF");
    }

    if (alarmActive) {
      lcd.setCursor(0, 2);
      lcd.print("Alarm:ON");
    } else {
      lcd.setCursor(0, 2);
      lcd.print("Alarm:OFF");
    }
  }
}

void setup() {
  Serial.begin(115200);

  Blynk.begin(auth, ssid, pass);

  Wire.begin(SDA_PIN, SCL_PIN); // Initialize I2C with specific pins
  lcd.init();  // Initialize LCD
  lcd.backlight();

  pinMode(DHT_POWER_PIN, OUTPUT);
  digitalWrite(DHT_POWER_PIN, HIGH); // Ensure DHT sensor is powered on

  dht.begin();

  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(PIR_PIN_2, INPUT);
  pinMode(PIR_PIN_3, INPUT); // Initialize new PIR sensor

  digitalWrite(BUZZER, LOW);
  digitalWrite(RELAY, LOW);

  // Timer to read sensors and send data to Blynk
  timer.setInterval(2000L, readSensors);

  // Timer to check PIR sensors
  timer.setInterval(100L, checkPIR);

  // Timer to show soil moisture alert
  timer.setInterval(soilMoistureAlertTimeout, []() {
    if (soilMoistureAlert) {
      updateLCD();
      displayTimeout = millis() + 5000; // Display alert for 5 seconds
    }
  });

  // Initialize display with welcome message
  updateLCD();
}

void loop() {
  Blynk.run();
  timer.run();

  if (millis() > welcomeTimeout && showWelcome) {
    showWelcome = false;
    showProjectName = true;
    updateLCD();
  } else if (millis() > projectNameTimeout && showProjectName) {
    showProjectName = false;
    updateLCD();
  }

  if (millis() > displayTimeout && (animalDetected || soilMoistureAlert)) {
    animalDetected = false;
    soilMoistureAlert = false;
    updateLCD();
  }
}


