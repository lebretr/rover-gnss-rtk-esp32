//CPU: 240MHZ
//Core Debug Level: Verbose (Just for Test. For prod: None)
//Erase All flash: Enable (Only the 1rst time)
//Flash size: 4Mb
//Partition: NO OTA (2Mb App/2Mb SPIFFS)
//Speed 115200


#include <BleSerial.h>
#include <BluetoothSerial.h>
#include <esp_attr.h>
#include <esp_task_wdt.h>
#include <driver/rtc_io.h>
#include <esp_mac.h>

#define BT_NAME "GNSS_ANTENNA_NTRIP"

const int BUFFER_SIZE = 4096; //8192; 
const int STACK_SIZE =  4096; //8192;

BleSerial SerialBLE;
BluetoothSerial SerialBT;

uint8_t unitMACAddress[6];  // Use MAC address in BT broadcast and display
char deviceName[30];        // The serial string that is broadcast.


// uint8_t serialReadBuffer[BUFFER_SIZE];

// GNSS serial port
HardwareSerial GnssSerial(1);
#define PIN_RX 16
#define PIN_TX 17
// #define GNSS_SERIAL_BAUD_RATE  115200 //115200 Works with F9P/UM980 config on 2Hz.
#define GNSS_SERIAL_BAUD_RATE  460800 //460800 works with UM980 config on 5Hz!


void startBluetooth() {
  // Get unit MAC address
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  
  // Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address
  unitMACAddress[5] += 2;                                                          
  
  //Create device name
  sprintf(deviceName, "GNSS_ANTENNA-%02X%02X", unitMACAddress[4], unitMACAddress[5]); 

  //Init BLE Serial
  SerialBLE.begin(deviceName);
  SerialBLE.setTimeout(10);


  //Start Bluetooth SPP
  if (!SerialBT.begin(deviceName)) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    // SerialBT.register_callback(btCallback);
    Serial.println("Bluetooth initialized with name " + (String)deviceName);
  }
}

//Task for reading GNSS Serial
void ReadGnssSerialTask(void *e) {
  static uint8_t gnssSerialReadBuffer[BUFFER_SIZE];
  while (true) {
    if(GnssSerial.available()){
      auto count = GnssSerial.readBytes(gnssSerialReadBuffer, BUFFER_SIZE);
      SerialBLE.write(gnssSerialReadBuffer, count);
      if (SerialBT.hasClient()){
        SerialBT.write(gnssSerialReadBuffer, count);
      }      
      // Serial.write(gnssSerialReadBuffer, count);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

//Task for reading BLE Serial
void ReadBleTask(void *e) {
  static uint8_t btReadBuffer[BUFFER_SIZE];
  while (true) {
    if (SerialBLE.available()) {
      auto count = SerialBLE.readBytes(btReadBuffer, BUFFER_SIZE);
      GnssSerial.write(btReadBuffer, count);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

//Task for reading BT Serial
void ReadBtTask(void *e) {
  static uint8_t bleReadBuffer[BUFFER_SIZE];
  while (true) {
    if (SerialBT.available()) {
      auto count = SerialBT.readBytes(bleReadBuffer, BUFFER_SIZE);
      GnssSerial.write(bleReadBuffer, count);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  //Start Serial
  Serial.begin(115200);
  delay(100);

  //Start Gnss Serial
  GnssSerial.begin(GNSS_SERIAL_BAUD_RATE, SERIAL_8N1, PIN_RX, PIN_TX); // serial port to send RTCM to F9P, UL980,...
  GnssSerial.setTimeout(10);
  delay(100);

  //Start BLE
  startBluetooth();

  //Disable watchdog timers
  // disableCore0WDT();  //I don't use this on my esp32-WROOM-32D because error "task_wdt: esp_task_wdt_reset" error
  // disableCore1WDT();  //I don't use this on my esp32-WROOM-32D because error "task_wdt: esp_task_wdt_reset" error
  // disableLoopWDT();   //I don't use this on my esp32-WROOM-32D because error "task_wdt: esp_task_wdt_reset" error
  esp_task_wdt_delete(NULL);

  //Start tasks
  xTaskCreate(ReadGnssSerialTask, "ReadGnssSerialTask", STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(ReadBleTask, "ReadBleTask", STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(ReadBtTask, "ReadBtTask", STACK_SIZE, NULL, 1, NULL);
}

void loop() {
  //This task is not used
  vTaskDelete(NULL); //uncomment this line if you have no other code in the loop function.
  
  //Send a "ping" message in the serial monitor
  // static unsigned long lastDiag = 0;
  // if (millis() - lastDiag > 5000) {
  //   Serial.println("ping");
  //   lastDiag = millis();
  // }
}