/*
This ESP32 will act as a Client that will connect to two BLE Servers and read data from them.
The BLE Servers are the ESP32s that are running the ESP32_Nosecone.cpp code and ESP32_motortube.cpp code

The ESP32 will connect to the BLE Servers one at a time, read data from them, then disconnect and connect to the other BLE Server.

server1: ESP32_Motortube.cpp
server2: ESP32_Nosecone.cpp
*/

#include "BLEDevice.h"

// Define UUIDs for the services and characteristics of the BLE servers we want to connect to
static BLEUUID serviceUUID1("0000FE40-cc7a-482a-984a-7f2ed5b3e58f");
static BLEUUID charUUID1("0000FE41-8e22-4541-9d4c-21edae82ed19");
static BLEUUID serviceUUID2("0000FE42-cc7a-482a-984a-7f2ed5b3e58f");
static BLEUUID charUUID2("0000FE43-8e22-4541-9d4c-21edae82ed19");

// Define boolean flags to keep track of the state of the BLE clients
bool connected1 = false;
bool connected2 = false;
bool doScan = false;

// Declare pointers to the remote characteristics and advertised devices of the BLE servers
BLERemoteCharacteristic* pRemoteCharacteristic1 = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic2 = nullptr;
BLEAdvertisedDevice* targetDevice1 = nullptr;
BLEAdvertisedDevice* targetDevice2 = nullptr;

void notifyCallback1(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                     uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);
}

void notifyCallback2(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                     uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);
}

class MyClientCallback1 : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient){}

  void onDisconnect(BLEClient* pclient)
  {
    connected1 = false;
    Serial.println("Disconnected");
    doScan = true;
  }
};

class MyClientCallback2 : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient){}

  void onDisconnect(BLEClient* pclient)
  {
    connected2 = false;
    Serial.println("Disconnected");
    doScan = true;
  }
};

bool connectToServer1() {
  std::string address = targetDevice1->getAddress().toString();
  Serial.print("Forming a connection to ");
  Serial.println(address.c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback1());

  if(!pClient->connect(targetDevice1)) {
    return false;
  }
  
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID1);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID1.toString().c_str());
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic1 = pRemoteService->getCharacteristic(charUUID1);
  if (pRemoteCharacteristic1 == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID1.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if(pRemoteCharacteristic1->canNotify())
  {
    pRemoteCharacteristic1->registerForNotify(notifyCallback1);
  }

  connected1 = true;
  return true;
}

bool connectToServer2() {
  std::string address = targetDevice2->getAddress().toString();
  Serial.print("Forming a connection to ");
  Serial.println(address.c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback2());

  if(!pClient->connect(targetDevice2)) {
    return false;
  }
  
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID2);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID2.toString().c_str());
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic2 = pRemoteService->getCharacteristic(charUUID2);
  if (pRemoteCharacteristic2 == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID2.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if(pRemoteCharacteristic2->canNotify())
  {
    pRemoteCharacteristic2->registerForNotify(notifyCallback2);
  }

  connected2 = true;
  return true;
}

void disconnectFromServer1() {
  // disconnect from server 1 and clean up
  connected1 = false;
}

void disconnectFromServer2() {
  // disconnect from server 2 and clean up
  connected2 = false;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if (advertisedDevice.haveServiceUUID())
    {
      if (advertisedDevice.isAdvertisingService(serviceUUID1)) 
      {
        BLEDevice::getScan()->stop();
        targetDevice1 = new BLEAdvertisedDevice(advertisedDevice);
        doScan = true;
      } 
      else if (advertisedDevice.isAdvertisingService(serviceUUID2)) 
      {
        BLEDevice::getScan()->stop();
        targetDevice2 = new BLEAdvertisedDevice(advertisedDevice);
        doScan = true;
      }
    }
  }
};

void setup()
{
  Serial.begin(115200);  
  Serial.println("Starting Arduino BLE Client application..."); 
  BLEDevice::init("ESP32-BLE-Client"); 

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop()
{
  // connect to server 1, delay, disconnect, connect to server 2, delay, disconnect, repeat
  if(doScan)
  {
    if(targetDevice1 != nullptr)
    {
      connectToServer1();
      delay(1000);
      disconnectFromServer1();
    }
    
    if(targetDevice2 != nullptr)
    {
      connectToServer2();
      delay(1000);
      disconnectFromServer2();
    }
    
    // Add a delay before starting to scan again to avoid excessive power consumption or network congestion
    delay(5000); 

    BLEDevice::getScan()->start(0);  // start scanning again
  }
}
