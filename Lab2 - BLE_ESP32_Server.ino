#include <BLEDevice.h>
#include <BLEServer.h>
//#include <BLEUtils.h>
//#include <BLE2902.h>
#include <M5StickCPlus.h>

//#include <Wire.h>

//Default Temperature in Celsius
#define temperatureCelsius

//change to unique BLE server name
#define bleServerName "CSC2006-BLE#23"

int ledStatus = 0; // define ledStatus variable
float tempC = 25.0;
float tempF;
float vBatt = 5.0;  // initial value

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 100;   // update refresh every 15sec

bool deviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "70f2e3ef-3ae5-47a6-8520-08e38b36ce52"

// Temperature Characteristic and Descriptor
#ifdef temperatureCelsius
  BLECharacteristic imuTemperatureCelsiusCharacteristics("1ad02268-ec9b-49ed-98dd-c70a3bf5eb51", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor imuTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
#else
  BLECharacteristic imuTemperatureFahrenheitCharacteristics("46049787-55ec-4cda-a2a2-b4c019eac6b3", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor imuTemperatureFahrenheitDescriptor(BLEUUID((uint16_t)0x2902));
#endif

// Battery Voltage Characteristic and Descriptor
BLECharacteristic axpVoltageCharacteristics("cd4aef81-f028-401c-b3fd-60149e609d05", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor axpVoltageDescriptor(BLEUUID((uint16_t)0x2903));

// LED Characteristic and Descriptor
BLECharacteristic ledCharacteristic("65b1e0d5-1baf-467e-8cf0-b154a70fb705", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor ledDescriptor(BLEUUID((uint16_t)0x2904));

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("MyServerCallbacks::Connected...");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("MyServerCallbacks::Disconnected...");
  }
};

void setup() {
  // Start serial communication 
  Serial.begin(115200);

  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Server", 0);

  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *bleService = pServer->createService(SERVICE_UUID);

  //home button
  pinMode(M5_BUTTON_HOME, INPUT);

  // initialize digital pin M5_LED as an output.
  pinMode(M5_LED, OUTPUT);
  digitalWrite(M5_LED, HIGH);

  // Create BLE Characteristics and Create a BLE Descriptor
  // Temperature
  #ifdef temperatureCelsius
    bleService->addCharacteristic(&imuTemperatureCelsiusCharacteristics);
    imuTemperatureCelsiusDescriptor.setValue("IMU Temperature(C)");
    imuTemperatureCelsiusCharacteristics.addDescriptor(&imuTemperatureCelsiusDescriptor);
  #else
    bleService->addCharacteristic(&imuTemperatureFahrenheitCharacteristics);
    imuTemperatureFahrenheitDescriptor.setValue("IMU Temperature(F)");
    imuTemperatureFahrenheitCharacteristics.addDescriptor(&imuTemperatureFahrenheitDescriptor);
  #endif  

  // Battery
  bleService->addCharacteristic(&axpVoltageCharacteristics);
  axpVoltageDescriptor.setValue("AXP Battery(V)");
  axpVoltageCharacteristics.addDescriptor(&axpVoltageDescriptor); 

  // led
  bleService->addCharacteristic(&ledCharacteristic);
  ledCharacteristic.addDescriptor(&ledDescriptor);

    
  // Start the service
  bleService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {
      // Read temperature as Celsius (the default)
      // if (random(2)>0)
      //   tempC += random(10)/100.0;
      // else
      //   tempC -= random(10)/100.0;
      tempC = readTemperature();
      // Fahrenheit
      tempF = 1.8*tempC + 32;
      // Battery voltage
      if (vBatt<1.0)
        vBatt = 5.0;
      else
        vBatt -= 0.01;
  
      //Notify temperature reading from IMU
      #ifdef temperatureCelsius
        static char temperatureCTemp[6];
        dtostrf(tempC, 6, 2, temperatureCTemp);
        //Set temperature Characteristic value and notify connected client
        imuTemperatureCelsiusCharacteristics.setValue(temperatureCTemp);
        imuTemperatureCelsiusCharacteristics.notify();
        Serial.print("Temperature = ");
        Serial.print(tempC);
        Serial.print(" C");

        M5.Lcd.setCursor(0, 20, 2);
        M5.Lcd.print("Temperature = ");
        M5.Lcd.print(tempC);
        M5.Lcd.println(" C");
      #else
        static char temperatureFTemp[6];
        dtostrf(tempF, 6, 2, temperatureFTemp);
        //Set temperature Characteristic value and notify connected client
        imuTemperatureFahrenheitCharacteristics.setValue(temperatureFTemp);
        imuTemperatureFahrenheitCharacteristics.notify();
        Serial.print("Temperature = ");
        Serial.print(tempF);
        Serial.print(" F");

        M5.Lcd.setCursor(0, 20, 2);
        M5.Lcd.print("Temperature = ");
        M5.Lcd.print(tempF);
        M5.Lcd.println(" F");
      #endif
      
      //Notify battery status reading from AXP192
      static char voltageBatt[6];
      dtostrf(vBatt, 6, 2, voltageBatt);
      //Set voltage Characteristic value and notify connected client
      axpVoltageCharacteristics.setValue(voltageBatt);
      axpVoltageCharacteristics.notify();   
      Serial.print(" - Battery Voltage = ");
      Serial.print(vBatt);
      Serial.println(" V");

      M5.Lcd.setCursor(0, 40, 2);
      M5.Lcd.print("Battery Votage = ");
      M5.Lcd.print(vBatt);
      M5.Lcd.println(" V");

      if (digitalRead(M5_BUTTON_HOME) == LOW) {
        ledStatus = 1;
        ledCharacteristic.setValue(ledStatus);
        digitalWrite(M5_LED, LOW);
      } else {
        ledStatus = 0;
        ledCharacteristic.setValue(ledStatus);
        digitalWrite(M5_LED, HIGH);
      }
      
      ledCharacteristic.notify();
      
      lastTime = millis();
    }
  }
}

float readTemperature(){
  float t;
  M5.IMU.getTempData(&t);
  t = (t-32.0)*5.0/9.0;
  return t;
}


