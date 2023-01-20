#include "BLEDevice.h"
#include <M5StickCPlus.h>

//Default Temperature in Celsius
#define temperatureCelsius

// change the BLE Server name to connect to
#define bleServerName "CSC2006-BLE#23"

/* UUID's of the service, characteristic that we want to read*/
// BLE Service
static BLEUUID bleServiceUUID("70f2e3ef-3ae5-47a6-8520-08e38b36ce52");

// BLE Characteristics
#ifdef temperatureCelsius
  // Temperature Celsius Characteristic
  static BLEUUID temperatureCharacteristicUUID("1ad02268-ec9b-49ed-98dd-c70a3bf5eb51");
#else
  // Temperature Fahrenheit Characteristic
  static BLEUUID temperatureCharacteristicUUID("46049787-55ec-4cda-a2a2-b4c019eac6b3");
#endif

// Battery Voltage Characteristic
static BLEUUID voltageCharacteristicUUID("cd4aef81-f028-401c-b3fd-60149e609d05");

// LED Characteristic
static BLEUUID ledCharacteristicUUID("65b1e0d5-1baf-467e-8cf0-b154a70fb705");

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;
 
//Characteristicd that we want to read
static BLERemoteCharacteristic* temperatureCharacteristic;
static BLERemoteCharacteristic* voltageCharacteristic;
static BLERemoteCharacteristic* ledCharacteristic;

//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

//Variables to store temperature and voltage
char* temperatureStr;
char* voltageStr; 
int ledStatus = 0; // define ledStatus variable

//Flags to check whether new temperature and voltage readings are available
boolean newTemperature = false;
boolean newVoltage = false;
boolean newLedStatus = false;

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(bleServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bleServiceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  temperatureCharacteristic = pRemoteService->getCharacteristic(temperatureCharacteristicUUID);
  voltageCharacteristic = pRemoteService->getCharacteristic(voltageCharacteristicUUID);
  ledCharacteristic = pRemoteService->getCharacteristic(ledCharacteristicUUID);

  if (temperatureCharacteristic == nullptr || voltageCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Assign callback functions for the Characteristics
  temperatureCharacteristic->registerForNotify(temperatureNotifyCallback);
  voltageCharacteristic->registerForNotify(voltageNotifyCallback);
  ledCharacteristic->registerForNotify(ledNotifyCallback);

  return true;
}

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
    else
      Serial.print(".");
  }
};
 
//When the BLE Server sends a new temperature reading with the notify property
static void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  //store temperature value
  temperatureStr = (char*)pData;
  newTemperature = true;
//  Serial.println("temperatureNotifyCallback");
}

//When the BLE Server sends a new voltage reading with the notify property
static void voltageNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store voltage value
  voltageStr = (char*)pData;
  newVoltage = true;
//  Serial.println("voltageNotifyCallback");
}

//callback function to handle notifications when the LED characteristic changes
void ledNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                          uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("LED status changed: ");
  ledStatus = pData[0];
  newLedStatus = true;
  //Serial.println(ledStatus);
}


//function that prints the latest sensor readings in the OLED display
void printReadings(){
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.print("Temperature = ");
  M5.Lcd.print(temperatureStr);

  Serial.print("Temperature = ");
  Serial.print(temperatureStr);

  #ifdef temperatureCelsius
    //Temperature Celsius
    M5.Lcd.println(" C");
    Serial.print(" C");
  #else
    //Temperature Fahrenheit
    M5.Lcd.print(" F");
    Serial.print(" F");
  #endif

  //display voltage
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.print("Battery Voltage = ");
  M5.Lcd.print(voltageStr);
  M5.Lcd.println(" V");

  Serial.print(" - Battage Voltage = ");
  Serial.print(voltageStr); 
  Serial.println(" V");

  M5.Lcd.setCursor(0, 60, 2);
  M5.Lcd.print(" - LED Status = ");
  M5.Lcd.print(ledStatus); 
}

void setup() {
  //Start serial communication
  Serial.begin(115200);
  Serial.println("Starting BLE Client application...");

  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Client", 0);

  //Init BLE device
  BLEDevice::init("");
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server that we want to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("Connected to the BLE Server.");
      
      //Activate the Notify property of each Characteristic
      temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      voltageCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2903))->writeValue((uint8_t*)notificationOn, 2, true);
      
      connected = true;
    } else {
      Serial.println("Failed to connect to the server; Restart device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  //if new temperature readings are available, print in the OLED
  if (newTemperature && newVoltage){
    newTemperature = false;
    newVoltage = false;
    printReadings();
  }

  //TODO: Refactor later
  //check if a new LED status is available and update the LED accordingly
  if(newLedStatus) {
    digitalWrite(M5_LED, ledStatus);
    newLedStatus = false;
  }

  delay(1000); // Delay one second between loops.
}
