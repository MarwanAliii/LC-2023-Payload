#include "BLEDevice.h"  

// Define UUIDs for the service and characteristic of the BLE server we want to connect to
const BLEUUID serviceUUID("0000FE40-cc7a-482a-984a-7f2ed5b3e58f");
const BLEUUID charUUID("0000FE41-8e22-4541-9d4c-21edae82ed19");

// Define boolean flags to keep track of the state of the BLE client
bool connected = false;   
bool doScan = false;      

// Declare pointers to the remote characteristic and advertised device of the BLE server
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr; 
BLEAdvertisedDevice* targetDevice = nullptr;


void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                            uint8_t* pData, size_t length, bool isNotify)
{
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);
}


class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient* pclient){}

  void onDisconnect(BLEClient* pclient)
  {
    connected = false;
    Serial.println("Disconnected");
    doScan = true;
  }
};


bool connectToServer()
{
  std::string address = targetDevice->getAddress().toString();
  Serial.print("Forming a connection to ");
  Serial.println(address.c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  if(!pClient->connect(targetDevice)) {
    return false;
  }
  
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    // make a dummy variable that containt=s the Seerivce UUID as a string
    std::string serviceUUIDString = serviceUUID.toString();


    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if(pRemoteCharacteristic->canNotify())
  {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

    connected = true;
    return true;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
    {
      BLEDevice::getScan()->stop();
      targetDevice = new BLEAdvertisedDevice(advertisedDevice);
      doScan = true;
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
    if (!connected && connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
    } 
    else if (connected) 
    {
      uint8_t i = 0;
      while(connected)
      {
        uint8_t value[] = {i};
        pRemoteCharacteristic->writeValue(value, sizeof(value));  
        Serial.println("Sent!");
        i++;
        delay(1000);
      }
    }
    else if(doScan)
    {
      BLEDevice::getScan()->start(0); 
      Serial.println("Scanning");
    }
  
    delay(2000); 
}
