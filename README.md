# ESP32 MQTT Smart Home Node 🏠🔌

An intelligent, lightweight IoT node built for the ESP32. This project integrates a DHT11 temperature/humidity sensor, a relay for power control, and capacitive touch sensing, all communicating over secure MQTT.



## 📱 Companion Android App
To get the most out of this project, you can use my official companion app to send commands and monitor sensor data effortlessly.

**Download the app from the Google Play Store:**
👉 **[[Aur: Home IoT] on Google Play](https://play.google.com/store/apps/details?id=com.pankajkumarrout.homeiot)**

The app comes pre-configured to send the exact MQTT commands required to control the relay, toggle touch modes, and receive real-time temperature and humidity updates from the ESP32.

---

## 🛠️ Hardware Requirements
* **ESP32 Development Board**
* **DHT11 Sensor** (Temperature & Humidity)
* **5V Relay Module** (Active Low)
* **Jumper Wires** & Breadboard

### Pin Configuration
| Component | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **DHT11 Data** | `GPIO 4` | Requires a 10k pull-up resistor if not on a breakout board. |
| **Relay IN** | `GPIO 23` | Controls external devices. |
| **Touch Sensor** | `GPIO 15 (T3)` | Connect a jumper wire to act as a capacitive touch pad. |

---

## 💻 Software & Libraries
This project is built using the Arduino IDE. You will need to install the following libraries via the Arduino Library Manager:
1.  **PubSubClient** by Nick O'Leary (For MQTT communication)
2.  **DHT sensor library** by Adafruit (For reading the DHT11)
3.  **Adafruit Unified Sensor** by Adafruit (Required by the DHT library)

---

## 🚀 Installation & Setup

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/pnrt/esp32-home-iot.git](https://github.com/pnrt/esp32-home-iot.git)
    ```
2.  **Open the project** in the Arduino IDE.
3.  **Update Credentials:** Open the `.ino` file and update the `USER CONFIGURATION` section at the top:
    ```cpp
    const char* WIFI_SSID       = "YOUR_WIFI_SSID";
    const char* WIFI_PASSWORD   = "YOUR_WIFI_PASSWORD";
    const char* MQTT_BROKER     = "YOUR_MQTT_BROKER_URL";
    const char* MQTT_USERNAME   = "YOUR_MQTT_USERNAME"; 
    const char* MQTT_PASSWORD   = "YOUR_MQTT_PASSWORD";
    ```
4.  **Select your ESP32 board** from the `Tools > Board` menu.
5.  **Compile and Upload** the code to your ESP32.

---

## 📡 MQTT Topics & Commands

The device communicates using the following MQTT topics. 

### Subscribed Topic (Listening for Commands)
* `pnrt/command`

**Available Commands:**
*Note: Send these exact strings as payloads to the command topic.*
* `ACTIVATE_POWER` - Turns the relay ON.
* `ACTIVATE_TOUCH` - Enables capacitive touch mode.
* `EXIT_POWER` - Turns the relay OFF.
* `EXIT_TOUCH` - Disables capacitive touch mode.
* `GET_DATA` - Forces an immediate publish of temperature and humidity.
* `REBOOT_SYSTEM` - Safely restarts the ESP32.

### Published Topics (Sending Data)
| Topic | Description | Example Payload |
| :--- | :--- | :--- |
| `pnrt/sensor/temp` | Temperature in Celsius (Auto-publishes every 1 hr) | `24.50` |
| `pnrt/sensor/humidity` | Humidity percentage (Auto-publishes every 1 hr) | `60.00` |
| `pnrt/touch/status` | Current state of touch mode | `ON` or `OFF` |
| `pnrt/touch_values` | Raw capacitive touch values (Streams when touched) | `12` |
| `pnrt/power/status` | Current state of the relay | `ON` or `OFF` |
| `pnrt/feedback` | System messages and acknowledgments | `Device Online` |
| `pnrt/error` | System errors | `DHT Read Failed` |

---

## 🤝 Contributing
Feel free to fork this project, submit pull requests, or open an issue if you find a bug or have a feature request.

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
