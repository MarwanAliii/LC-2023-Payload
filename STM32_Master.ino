/*
This is the workflow of the P2P server application:

1. Start Command:
  • Purpose: Initiates communication between the master and slave devices with unique characteristic value.
  • Flags: Start Flag, Payload Length (in bytes), End Flag (indicating the end of a packet).

2. Data Command:
	• Purpose: Transmits sensor data from the slave to the master.
	• Flags: Start Flag, Payload Length, End Flag.
	• Payload: Sensor data.
3. Acknowledgment Command:
	• Purpose: Confirms the successful reception of a packet.
	• Flags: Start Flag, Command Flag (specifying the acknowledgment command), Payload Length (optional), Checksum, End Flag.
	• Payload: Optional acknowledgment data.
4. Error Command:
	• Purpose: Indicates an error or failure in the communication.
	• Flags: Start Flag, Command Flag (specifying the error command), Payload Length (optional), Checksum, End Flag.
  • Payload: Optional error information or error code.
*/

/*
TODOs:

- Set up errorCharacteristic event
- Set up UUID juggling
- 
*/


/* Includes ------------------------------------------------------------------*/
#include <STM32duinoBLE.h>
#include <chrono>

/* Private typedef -----------------------------------------------------------*/
typedef struct __attribute__((packed)) {
    byte boardSelection;
    byte dataToLog; 
} P2P_STM32WB_data_t;
P2P_STM32WB_data_t dataInfo;

typedef struct __attribute__((packed)) {
    byte errBool;
} error_flag_t;
error_flag_t errInfo;

typedef struct __attribute__((packed)) {
    byte tmrBool;
} tmr_flag_t;
tmr_flag_t tmrInfo;

typedef struct __attribute__((packed)) {
    byte slpBool;
} espSlp_flag_t;
espSlp_flag_t slpInfo;

/* Private variables ---------------------------------------------------------*/
/* UUID for P2P Service-------------------------------------------------------*/
BLEService P2PService("0000FE40-cc7a-482a-984a-7f2ed5b3e58f"); 

//STM Characteristics
//Read/Write data characteristic. Data characteristic is the data we received
BLECharacteristic dataCharacteristic("0000FE41-8e22-4541-9d4c-21edae82ed19", BLERead | BLEWriteWithoutResponse,sizeof(dataInfo),true);
//Read only timer characteristic. Timer characteristic is sent to show we will switch clients.
BLECharacteristic timerCharacteristic("0000FE42-8e22-4541-9d4c-21edae82ed19", BLERead,sizeof(ackInfo),true);
//Read only error characteristic. Error characteristic means we should restart the device.
BLECharacteristic errorCharacteristic("0000FE43-8e22-4541-9d4c-21edae82ed19", BLERead,sizeof(tmrInfo),true);

//ESP Characteristics
// Read only sleep characteristic. Sleep characteristic is a flag to tell ESP to sleep.
/*
ESP#1 -> P2P UUID 0000FE40-cc7a-482a-984a-7f2ed5b3e58f
ESP#2 -> P2P UUID 0000FE40-cc7a-482a-984a-7f2ed5b3e58e

ESP#1 -> Sleep when slpBool=0x01
ESP#2 -> Sleep when slpBool=0x00
*/
BLECharacteristic espSleepCharacteristic("0000FE44-8e22-4541-9d4c-21edae82ed19", BLERead,sizeof(slpInfo),true);


HCISharedMemTransportClass HCISharedMemTransport;
BLELocalDevice BLEObj(&HCISharedMemTransport);
BLELocalDevice& BLE = BLEObj;
//This is the time the ESP last wrote data to the STM
std::chrono::steady_clock::time_point lastDataWriteTime;
std::chrono::steady_clock::time_point ESPTimer;


void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("==> Connection established with central: ");
  Serial.print(central.address());
    if (BLE.connected()) {
      Serial.print(" -- RSSI = ");
      Serial.println(BLE.rssi());

      // Timer characteristic is written to show we are ready to log data.
      tmrInfo.tmrBool=0x01;
      tmrCharacteristic.writeValue((byte*)&ackInfo, sizeof(ackInfo));
      //Start Timer
      ESPTimer = std::chrono::steady_clock::now();
    }
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("==> Disconnection event with central: ");
  Serial.println(central.address());
  // NVIC_SystemReset();
  // delay(1000);
}

void dataCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  byte data[sizeof(dataInfo)];
  // central wrote new value to characteristic
  if (dataCharacteristic.written()) {
    // Update the last data write time
    lastDataWriteTime = std::chrono::steady_clock::now();

    Serial.println("==>Data Characteritics written:");
    Serial.print("Data Received: ");
    dataCharacteristic.readValue(data, sizeof(dataInfo));
    Serial.print(dataCharacteristic.valueLength());
    Serial.print(" ");
    Serial.print("bytes -- ");
    for (int i = 0; i < sizeof(dataInfo); i++) {
      Serial.print("0x");
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    dataInfo.boardSelection=data[0];
    dataInfo.dataToLog=data[1];
    Serial.print("Board Selected:");
    Serial.println(dataInfo.boardSelection);
    Serial.print("Value:");
    Serial.println(dataInfo.dataToLog);

    //Flash the dtaa to an SD Card 
    if(dataInfo.boardSelection==0x01){
      Serial.println("==>Writing to SD Card");
      File myFile = SD.open("test.txt", FILE_WRITE);

      if (myFile) {
        //Print the real time clock and data
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        myFile.print(std::ctime(&in_time_t));
        myFile.print("\t");
        myFile.println(dataInfo.dataToLog);
        myFile.close();
      } 
    }
  }
}  

void checkTimeout() {
  // Calculate the elapsed time since the last data write
  auto currentTime = std::chrono::steady_clock::now();
  auto elapsedWriteTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastDataWriteTime);
  auto elapsedESPTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - ESPTimer);

  // Check if the elapsed time is more than 2 seconds
  if (elapsedWriteTime.count() >= 2) {
    // Write True to the errorCharacteristic
    errorInfo.errorBool=0x01;
    errorCharacteristic.writeValue((byte*)&errorInfo, sizeof(errorInfo));
  }

  // Check if the elapsed ESP time is more than 2 seconds
  if (elapsedESPTime.count() >= 2) {
    // Write False to the timerCharacteristic, indicating we are not ready to log data.
    tmrInfo.tmrBool=0x00;
    tmrCharacteristic.writeValue((byte*)&tmrInfo, sizeof(tmrInfo));
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial);

  //Project P2P server
  Serial.println("======================================");
  Serial.println("== STM32WB - P2P Server Application ==");
  Serial.println("======================================");
  
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("==>starting BLE failed!");

    while (1);
  }
  
  //Set the device name in the built in device name characteristic.
  BLE.setDeviceName("STM32WB-Arduino");
  //set the local name peripheral advertises
  Serial.println("==> Advertising Name - P2P_Server");
  BLE.setLocalName("P2P_Server");
  //Set the advertising interval in units of 0.625 ms. 
  //Defaults to 100ms (160 * 0.625 ms) if not provided. 
  BLE.setAdvertisingInterval(160); // 160 * 0.625 ms
  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(P2PService);
  // add the characteristics to the service
  P2PService.addCharacteristic(dataCharacteristic);
  P2PService.addCharacteristic(timerCharacteristic);
  P2PService.addCharacteristic(errorCharacteristic);
  // assign event handlers for characteristic
  dataCharacteristic.setEventHandler(BLEWritten, dataCharacteristicWritten);
  // add the service
  BLE.addService(P2PService);
  // start advertising
  BLE.advertise();

  String address = BLE.address();
  Serial.print("==>Local address is: ");
  Serial.println(address);

  Serial.println("==>Advertising Started, waiting for connections...");

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
}

void loop() {
  //Poll for BLE events
  BLE.poll();

  //Check if the data write timeout has occurred
  checkTimeout();

  //If tmrBool is False, switch the P2PService UUID
  if(tmrInfo.tmrBool==0x00){
    // Check if the P2PService UUID is the default UUID
    if(P2PService.uuid() == "0000FE40-cc7a-482a-984a-7f2ed5b3e58f"){
      // Put ESP#1 to sleep by writing 0x01 to the sleep characteristic
      slpInfo.slpBool=0x01;
      espSleepCharacteristic.writeValue((byte*)&slpInfo, sizeof(slpInfo));
      // Set the P2PService UUID to the ESP UUID
      P2PService.setUUID("0000FE40-cc7a-482a-984a-7f2ed5b3e58e");
    }
    else{
      // Put ESP#2 to sleep by writing 0x00 to the sleep characteristic
      slpInfo.slpBool=0x00;
      espSleepCharacteristic.writeValue((byte*)&slpInfo, sizeof(slpInfo));
      // Set the P2PService UUID to the STM UUID
      P2PService.setUUID("0000FE40-cc7a-482a-984a-7f2ed5b3e58f");
    }
    BLE.advertise();
  }
}
