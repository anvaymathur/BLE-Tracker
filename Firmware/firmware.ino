
    #include <BLEDevice.h>
    #include <BLEScan.h>
    #include <BLEAdvertisedDevice.h>
    #include <WiFi.h>
    #include <WiFiClientSecure.h>
    #include <HTTPClient.h>
    #include <vector>

    // --- Configuration ---
    const char* wifiSSID = "Anvay OnePlus Nord CE4 Lite 5G";
    const char* wifiPASS = "anvayhotspot";

    const String firebaseProjectID = "ble-tracker-62831";
    const String gatewayID = "A01";

    const int scanTime = 5;

    BLEScan* pBLEScan = nullptr;

    // --- Data Structures ---
    struct TelemetryItem {
        String macAddress;
        int rssi;
        unsigned long last_seen;
    };

    std::vector<TelemetryItem> itemBuffer;

    // Registered tag MAC addresses, stored in uppercase.
    const String targetTags[] = {
        "DF:5F:CA:DF:95:3F",
        // "FE:ED:BB:11:22:33"
    };

    const int numTargetTags = sizeof(targetTags) / sizeof(targetTags[0]);

    // --- Helper Functions ---

    bool isRegisteredTag(String macAddress) {
        macAddress.toUpperCase();

        for (int i = 0; i < numTargetTags; i++) {
            if (macAddress == targetTags[i]) {return true;}
        }

        return false;
    }

    // Keep only the latest reading for each MAC address.
    void updateBuffer(String macAddress, int rssi,
                    unsigned long lastSeen) {
        for (auto& item : itemBuffer) {
            if (item.macAddress == macAddress) {
                item.rssi = rssi;
                item.last_seen = lastSeen;
                return;
            }
        }

        itemBuffer.push_back({macAddress, rssi, lastSeen});
    }

    void makePatchRequest(const String& url,
                        const String& payload) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Wi-Fi disconnected. Skipping request.");
            return;
        }

        WiFiClientSecure client;

        // Testing only: disables TLS certificate verification.
        client.setInsecure();

        HTTPClient http;
        http.setTimeout(10000);

        if (!http.begin(client, url)) {
            Serial.println("Failed to initialize HTTP connection.");
            return;
        }

        http.addHeader("Content-Type", "application/json");

        int statusCode = http.sendRequest("PATCH", payload);

        Serial.print("HTTP Response Code: ");
        Serial.println(statusCode);

        if (statusCode > 0) {
            Serial.print("Firebase response: ");
            Serial.println(http.getString());
        } else {
            Serial.print("HTTP error: ");
            Serial.println(http.errorToString(statusCode));
        }

        http.end();
    }

    void processAndFlushBuffer() {
        for (const auto& item : itemBuffer) {
            // Database path: /A01/<MAC address>
            String sanitizedMac = item.macAddress;
            sanitizedMac.replace(":", "");
            String url =
                "https://ble-tracker-62831-default-rtdb.firebaseio.com/" +
                gatewayID + "/" + sanitizedMac + ".json";

            Serial.print("Writing tag ");
            Serial.println(sanitizedMac);
            Serial.print("Target URL: ");
            Serial.println(url);

            // Match your existing database structure.
            String payload =
                "{\"rssi\":" + String(item.rssi) +
                ",\"last_seen\":" + String(item.last_seen) + "}";

            makePatchRequest(url, payload);
        }

        itemBuffer.clear();
    }

    // --- BLE Callbacks ---
    class MyAdvertisedDeviceCallbacks
        : public BLEAdvertisedDeviceCallbacks {

        void onResult(BLEAdvertisedDevice advertisedDevice) override {
            String deviceAddress =
                advertisedDevice.getAddress().toString().c_str();

            deviceAddress.toUpperCase();

            int deviceRSSI = advertisedDevice.getRSSI();

            // Serial.println("Device found with address: " + deviceAddress);

            if (isRegisteredTag(deviceAddress) &&
                deviceRSSI > -85) {

                Serial.print("Target tag found: ");
                Serial.print(deviceAddress);
                Serial.print(" | RSSI: ");
                Serial.println(deviceRSSI);

                // Time since ESP32 startup; not Unix time.
                unsigned long now = millis();

                updateBuffer(deviceAddress, deviceRSSI, now);
            }
        }
    };

    // --- Main Program ---
    void setup() {
        Serial.begin(115200);

        Serial.println("Initializing BLE tracker...");

        // Connect to Wi-Fi.
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiSSID, wifiPASS);

        Serial.print("Connecting to Wi-Fi");

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }

        Serial.println("\nWi-Fi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Initialize BLE scanner.
        BLEDevice::init("");

        pBLEScan = BLEDevice::getScan();

        pBLEScan->setAdvertisedDeviceCallbacks(
            new MyAdvertisedDeviceCallbacks()
        );

        pBLEScan->setActiveScan(true);
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99);
    }

    void loop() {
        Serial.println("\n--- Starting new scan window ---");

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Reconnecting to Wi-Fi...");
            WiFi.reconnect();

            unsigned long startAttempt = millis();

            while (WiFi.status() != WL_CONNECTED &&
                millis() - startAttempt < 10000) {
                delay(250);
            }
        }

        // Do not scan or send requests if Wi-Fi is still unavailable.
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Wi-Fi unavailable. Retrying shortly.");
            delay(2000);
            return;
        }

        pBLEScan->start(scanTime, false);

        Serial.print("Unique target tags found: ");
        Serial.println(itemBuffer.size());

        processAndFlushBuffer();

        pBLEScan->clearResults();

        delay(2000);
    }
