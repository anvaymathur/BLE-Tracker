#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <vector>

// --- Configuration ---
const String wifiSSID = "YOUR_SSID";
const String wifiPASS = "YOUR_PASSWORD";
const String firebaseProjectID = "YOUR-PROJECT-ID"; // Replace with your Firebase Project ID
const String gatewayID = "Aisle_1";                 // Identifier for this specific ESP32

int scanTime = 5; // Scan duration in seconds
BLEScan* pBLEScan;
int numDevices = 0;

// --- Data Structures ---
struct TelemetryItem {
    String macAddress;
    int rssi;
    unsigned long last_seen;
};

// Global buffer to collect tags during a scan window
std::vector<TelemetryItem> itemBuffer;

// Register target tag MAC addresses (Lowercase)
const String targetTags[] = {
  "ac:23:3f:89:12:01",
  "fe:ed:bb:11:22:33"
};
const int numTargetTags = sizeof(targetTags) / sizeof(targetTags[0]);

// --- Helper Functions ---
bool isRegisteredTag(String macAddress) {
    macAddress.toLowerCase();
    for (int i = 0; i < numTargetTags; i++) {
        if (macAddress == targetTags[i]) return true;
    }
    return false;
}

void makePatchRequest(String url, String payload) {
    
    WiFiClientSecure client;
    client.setInsecure(); // Bypass strict SSL certificate validation for testing

    

    HTTPClient http;

    if (http.begin(client, url)) {
        http.addHeader("Content-Type", "application/json");
        int statusCode = http.sendRequest("PATCH", payload);
        
        Serial.print("HTTP Response Code: ");
        Serial.println(statusCode); // 200 indicates success OK
        
        http.end(); // Release network resources
    }
}

void processAndFlushBuffer() {
    for (const auto& item : itemBuffer) {
        // Construct database endpoint targeting the specific tag ID node
        String url = "https://" + firebaseProjectID + "-default-rtdb.firebaseio.com/active_tags/" + item.macAddress + ".json";
        
        // Build the JSON string payload
        String payload = "{\"gateway_id\":\"" + gatewayID + "\",\"rssi\":" + String(item.rssi) + ",\"last_seen\":" + String(item.last_seen) + "}";

        makePatchRequest(url, payload);
    }
    // Clear stored tags so the next scan starts fresh
    itemBuffer.clear();
}

// --- BLE Callbacks ---
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // Retrieve the device's MAC Address
        String deviceAddress = advertisedDevice.getAddress().toString().c_str();
        int deviceRSSI = advertisedDevice.getRSSI();

        // Filter out unwanted BLE signals
        if (isRegisteredTag(deviceAddress) && deviceRSSI > -85) {
            String deviceName = advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "[No Name]";

            Serial.print("Target Tag Found! -> Name: ");
            Serial.print(deviceName);
            Serial.print(" | MAC: ");
            Serial.print(deviceAddress);
            Serial.print(" | RSSI: ");
            Serial.println(deviceRSSI);

            // Store in buffer to send via Wi-Fi outside the callback
            unsigned long now = millis();
            itemBuffer.push_back({deviceAddress, deviceRSSI, now});
        }
    }
};

// --- Main Program ---
void setup() {
    Serial.begin(115200);
    Serial.println("Initializing ESP32 General BLE Scanner...");

    // Connect to Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println("\nWi-Fi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize BLE Scanner
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); 
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);  // Requests extra scan response data from tags
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void loop() {
    Serial.println("--- Starting New Scan Window ---");

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
    
    // Start scanning
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    
    Serial.print("Total target devices found in this scan: ");
    Serial.println(itemBuffer.size());

    // Send HTTP POST/PATCH requests for all cached data points
    processAndFlushBuffer();
    
    // Delete results from buffer to free up ESP32 memory
    pBLEScan->clearResults();
    
    // Wait 2 seconds before looping the scan
    delay(2000);
}