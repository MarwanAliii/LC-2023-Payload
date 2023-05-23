#include "BLEDevice.h"  

// Define UUIDs for the service and characteristic of the BLE server we want to connect to
static BLEUUID serviceUUID("0000FE40-cc7a-482a-984a-7f2ed5b3e58f");
static BLEUUID charUUID("0000FE41-8e22-4541-9d4c-21edae82ed19");

// Define boolean flags to keep track of the state of the BLE client
// static boolean doConnect = false;   
static boolean connected = false;   
static boolean doScan = false;      

// Declare pointers to the remote characteristic and advertised device of the BLE server
static BLERemoteCharacteristic* pRemoteCharacteristic; 
static BLEAdvertisedDevice* targetDevice;

// This function is called when the ESP32 receives a notification or indication from the server
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                            uint8_t* pData, size_t length, bool isNotify)
{
  // Print the characteristic UUID, data length, and data received
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);
}

// This class defines the callbacks for the BLE client
class MyClientCallback : public BLEClientCallbacks
{
  //client successfully connects to the server
  void onConnect(BLEClient* pclient){}

  //client is disconnected from the server and restarts the board
  void onDisconnect(BLEClient* pclient)
  {
    connected = false;
    Serial.println("onDisconnect");
    ESP.restart();
    delay(1000);
  }
};

bool connectToServer()
{
  Serial.print("Forming a connection to ");
  //store target device address in a string
  std::string address = targetDevice->getAddress().toString();
  Serial.println(address.c_str());
  
  // Create a BLE client and set its callbacks
  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE server
  pClient->connect(targetDevice);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    // If the service cannot be found, print an error message and disconnect
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    Serial.println("Disconnecting from server...");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr)
  {
    // If the characteristic cannot be found, print an error message and disconnect
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;

  }
  Serial.println(" - Found our characteristic");

  if(pRemoteCharacteristic->canNotify())
  {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

    connected = true;
    return true;
}


// Scan for BLE servers and find the first one that advertises the service we are looking for
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
    {
      BLEDevice::getScan()->stop();
      targetDevice = new BLEAdvertisedDevice(advertisedDevice);
    //   doConnect = true;
      doScan = true;

    }
  }
};


void setup()
{
  Serial.begin(115200);  
  Serial.println("Starting Arduino BLE Client application..."); 
  BLEDevice::init("ESP32-BLE-Client"); 

  BLEScan* pBLEScan = BLEDevice::getScan(); // create an instance of BLEScan object
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); // set the callback for advertised devices
  pBLEScan->setInterval(1349); // set the scan interval in microseconds
  pBLEScan->setWindow(449); // set the scan window in microseconds
  pBLEScan->setActiveScan(true); // set active scanning
  pBLEScan->start(5, false); // start scanning for BLE devices
}

void loop()
{
//   while(doConnect) // if we need to connect to a BLE server
//   {
    if (connectToServer()) // try to connect to the server
    {
      Serial.println("We are now connected to the BLE Server.");
      doConnect = false; // reset the flag
    } 
    else
    {
      Serial.println("We have failed to connect to the server; trying again"); 
      delay(1000); // wait 1 second before trying again
    }
//   }

  if (connected) // if we are connected to a BLE server
  {
    int i = 0;
    while(connected){
      pRemoteCharacteristic->writeValue(String(i).c_str());  
      Serial.println("Sent!");
      i++;
      delay(100);
    }
    
    // std::string value1 = pRemoteCharacteristic->readValue(); // read the value of the remote characteristic
    // Serial.print("The characteristic value was: "); 
    // Serial.println(value1.c_str()); 
  }
  else if(doScan) // if we need to start scanning for BLE devices
  {
    BLEDevice::getScan()->start(0); 
    Serial.println("HHHHEEERRR"); // start scanning for BLE devices (0 means indefinite scan duration)
  }
  
  delay(2000); 
}
