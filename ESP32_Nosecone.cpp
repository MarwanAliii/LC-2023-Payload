// server1 code
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs for the service and characteristic
BLEUUID serviceUUID("0000FE40-cc7a-482a-984a-7f2ed5b3e58f");
BLEUUID charUUID("0000FE41-8e22-4541-9d4c-21edae82ed19");

bool deviceConnected = false;
BLECharacteristic *pCharacteristic;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init("ESP32-BLE-Server1");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(serviceUUID);

  pCharacteristic = pService->createCharacteristic(
                     charUUID,
                     BLECharacteristic::PROPERTY_READ   |
                     BLECharacteristic::PROPERTY_WRITE  |
                     BLECharacteristic::PROPERTY_NOTIFY |
                     BLECharacteristic::PROPERTY_INDICATE
                   );

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  // start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting for clients to connect...");
}

void loop() {
  if (deviceConnected) {
    // TODO: Replace the hardcoded string with actual sensor readings
    pCharacteristic->setValue("Hello World");
    pCharacteristic->notify(); 
    delay(1000); // send every second
  }
}
