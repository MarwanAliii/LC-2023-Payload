#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"
#include "BLE2902.h"

#define SERVICE_UUID1       "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SERVICE_UUID2       "abcdef01-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
BLEServer *pServer;
uint32_t connectionTime = 0;
bool isAdvertisingService1 = true;
uint16_t connectionID = 0;

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) {
        deviceConnected = true;
        connectionTime = millis();
        connectionID = param->connect.conn_id;
        Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
    }
};

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.println("Received data:");
            for (int i = 0; i < value.length(); i++)
                Serial.print(value[i]);
            Serial.println();
            
            // TODO: Log data to SD card here
            // Log data into different files based on the transmitter?
        }
    }
};

void setup() {
    Serial.begin(115200);
    BLEDevice::init("ESP32-BLE-Server");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create first service
    BLEService *pService1 = pServer->createService(SERVICE_UUID1);
    pCharacteristic = pService1->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    pCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pService1->start();

    // Create second service
    BLEService *pService2 = pServer->createService(SERVICE_UUID2);
    pCharacteristic = pService2->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    pCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pService2->start();

    startAdvertisingService1();
}

void startAdvertisingService1() {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID1);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->start();
    isAdvertisingService1 = true;
    Serial.println("Advertising service 1");
}

void startAdvertisingService2() {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID2);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->start();
    isAdvertisingService1 = false;
    Serial.println("Advertising service 2");
}

void loop() {
    // Notify connected client
    if (deviceConnected) {
        pCharacteristic->setValue("Hello, World!");
        pCharacteristic->notify();
        delay(10);
        if (millis() - connectionTime >= 3000) {
            if (deviceConnected) {
                pServer->disconnect(connectionID);
                delay(500);
            }
            if (isAdvertisingService1) {
                startAdvertisingService2();
            } else {
                startAdvertisingService1();
            }
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        if (isAdvertisingService1) {
            startAdvertisingService2();
        } else {
            startAdvertisingService1();
        }
        Serial.println("Start advertising...");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}
