/**
 * ESP32 MQTT Smart Home Node
 * Features: DHT11 Sensor, Capacitive Touch, Relay Control, and MQTT Command Handling.
 * * Instructions:
 * 1. Update the WIFI_SSID and WIFI_PASSWORD.
 * 2. Update the MQTT Broker credentials.
 * 3. Ensure your command list remains ALPHABETICAL if you add new commands.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "DHT.h"

// ==========================================
// 1. USER CONFIGURATION (Credentials)
// ==========================================
const char* WIFI_SSID       = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD   = "YOUR_WIFI_PASSWORD";

const char* MQTT_BROKER     = "YOUR_MQTT_BROKER_URL";
const int   MQTT_PORT       = 8883; 
const char* MQTT_USERNAME   = "YOUR_MQTT_USERNAME"; 
const char* MQTT_PASSWORD   = "YOUR_MQTT_PASSWORD";

// ==========================================
// 2. HARDWARE PINS & SETTINGS
// ==========================================
const int TOUCH_SENSOR_PIN  = 15; // T3 Touch Pin
const int DHT_SENSOR_PIN    = 4;  // DHT11 Data Pin
const int RELAY_CONTROL_PIN = 23; // Relay Pin

#define DHT_SENSOR_TYPE DHT11
DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// ==========================================
// 3. MQTT TOPICS
// ==========================================
const char* TOPIC_COMMAND        = "pnrt/command";
const char* TOPIC_FEEDBACK       = "pnrt/feedback";
const char* TOPIC_ERROR          = "pnrt/error";
const char* TOPIC_TEMP           = "pnrt/sensor/temp";
const char* TOPIC_HUMIDITY       = "pnrt/sensor/humidity";
const char* TOPIC_TOUCH_STATUS   = "pnrt/touch/status";
const char* TOPIC_TOUCH_VALUES   = "pnrt/touch_values";
const char* TOPIC_POWER_STATUS   = "pnrt/power/status";

// ==========================================
// 4. TIMERS & STATE VARIABLES
// ==========================================
unsigned long lastTouchPublishTime  = 0;
unsigned long lastSensorPublishTime = 0;
const long SENSOR_PUBLISH_INTERVAL  = 3600000; // 1 Hour (in milliseconds)
const long TOUCH_PUBLISH_INTERVAL   = 500;     // 0.5 Seconds (in milliseconds)

const int TOUCH_THRESHOLD   = 500; 
bool isCurrentlyTouched     = false; 
bool isTouchModeActive      = false; 

// --- NETWORK OBJECTS ---
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// ==========================================
// 5. COMMAND HANDLING & ACTIONS
// ==========================================
using ActionHandler = void (*)();

struct CommandMapping {
    const char* commandString; 
    ActionHandler handler;    
};

void publishSensorData() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature(); // Celsius

    // Check if read failed
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println(">> Error: Failed to read from DHT sensor!");
        mqttClient.publish(TOPIC_ERROR, "DHT Read Failed");
        return;
    }

    // Publish Temperature and Humidity
    mqttClient.publish(TOPIC_TEMP, String(temperature).c_str());
    mqttClient.publish(TOPIC_HUMIDITY, String(humidity).c_str());

    Serial.printf(">> Data Sent: Temp=%.2f°C, Hum=%.2f%%\n", temperature, humidity);
}

void enableTouch() { 
    isTouchModeActive = true; 
    mqttClient.publish(TOPIC_TOUCH_STATUS, "ON");
}

void disableTouch() { 
    isTouchModeActive = false; 
    isCurrentlyTouched = false; 
    mqttClient.publish(TOPIC_TOUCH_STATUS, "OFF");
}

void powerOn() {
    digitalWrite(RELAY_CONTROL_PIN, LOW); // Active LOW relay
    mqttClient.publish(TOPIC_POWER_STATUS, "ON");
}

void powerOff() { 
    digitalWrite(RELAY_CONTROL_PIN, HIGH); 
    mqttClient.publish(TOPIC_POWER_STATUS, "OFF");
}

void rebootDevice() { 
    mqttClient.publish(TOPIC_FEEDBACK, "Rebooting...");
    delay(500); 
    ESP.restart(); 
}

// --- COMMAND LIST ---
// IMPORTANT: This list MUST remain in ALPHABETICAL order (A-Z) for the binary search to work!
const CommandMapping commands[] = {
    { "ACTIVATE_POWER", powerOn           }, 
    { "ACTIVATE_TOUCH", enableTouch       }, 
    { "EXIT_POWER",     powerOff          }, 
    { "EXIT_TOUCH",     disableTouch      }, 
    { "GET_DATA",       publishSensorData }, 
    { "REBOOT_SYSTEM",  rebootDevice      }
};

const int numCommands = sizeof(commands) / sizeof(commands[0]);

// --- BINARY SEARCH FOR COMMANDS ---
int findCommandIndex(const char* key) {
    int left = 0;
    int right = numCommands - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int result = strcmp(key, commands[mid].commandString);
        if (result == 0) return mid;      
        if (result > 0) left = mid + 1;   
        else right = mid - 1;             
    }
    return -1; 
}

// ==========================================
// 6. MQTT CALLBACK
// ==========================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char cmdBuffer[length + 1];
    memcpy(cmdBuffer, payload, length);
    cmdBuffer[length] = '\0'; 
  
    Serial.printf("Received MQTT Command: %s\n", cmdBuffer);

    int index = findCommandIndex(cmdBuffer);
    if (index != -1) {
        commands[index].handler();
    } else {
        mqttClient.publish(TOPIC_FEEDBACK, "Unknown Command");
    }
}

// ==========================================
// 7. SETUP
// ==========================================
void setup() {
    Serial.begin(115200);

    // Initialize Pins
    pinMode(RELAY_CONTROL_PIN, OUTPUT);
    digitalWrite(RELAY_CONTROL_PIN, HIGH); // Default off (Active LOW)
  
    // Initialize Sensor
    dht.begin();

    // Initialize WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    WiFi.setSleep(true);
    Serial.println("\nWiFi Connected & Power Save Mode ON");
  
    // Initialize MQTT
    espClient.setInsecure(); // Required for port 8883 without specific certs
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

// ==========================================
// 8. RECONNECT LOGIC
// ==========================================
void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32-Node-" + String(random(0xffff), HEX);
        
        if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("connected");
            mqttClient.subscribe(TOPIC_COMMAND); 
            mqttClient.publish(TOPIC_FEEDBACK, "Device Online");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// ==========================================
// 9. MAIN LOOP
// ==========================================
void loop() {
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();

    unsigned long currentMillis = millis();

    // --- TASK 1: AUTO-PUBLISH SENSOR DATA ---
    if (currentMillis - lastSensorPublishTime >= SENSOR_PUBLISH_INTERVAL) {
        lastSensorPublishTime = currentMillis;
        publishSensorData(); 
    }

    // --- TASK 2: TOUCH SENSOR LOGIC ---
    if (isTouchModeActive) {
        int touchValue = touchRead(TOUCH_SENSOR_PIN);
    
        // Touch Detected
        if (touchValue < TOUCH_THRESHOLD && !isCurrentlyTouched) {
            isCurrentlyTouched = true; 
            mqttClient.publish(TOPIC_FEEDBACK, "TOUCH_START");
        }
        // Touch Released
        else if (touchValue > TOUCH_THRESHOLD && isCurrentlyTouched) {
            isCurrentlyTouched = false;
            mqttClient.publish(TOPIC_FEEDBACK, "TOUCH_END");
        }

        // Publish constant touch values while held
        if (isCurrentlyTouched) {
            if (currentMillis - lastTouchPublishTime > TOUCH_PUBLISH_INTERVAL) {
                lastTouchPublishTime = currentMillis;
                mqttClient.publish(TOPIC_TOUCH_VALUES, String(touchValue).c_str());
            }
        }
    }
  
    delay(50); // Small delay for stability 
}
