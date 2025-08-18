#include <BleSerial.h>
#include <esp_attr.h>
#include <esp_task_wdt.h>
#include <driver/rtc_io.h>
#include <esp_mac.h>

const int BUFFER_SIZE = 8192;
const int STACK_SIZE = 8192;

BleSerial SerialBT;

uint8_t unitMACAddress[6];  // Use MAC address in BT broadcast and display
char deviceName[30];        // The serial string that is broadcast.

uint8_t bleReadBuffer[BUFFER_SIZE];
uint8_t serialReadBuffer[BUFFER_SIZE];
uint8_t gnssSerialReadBuffer[BUFFER_SIZE];

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
  SerialBT.begin(deviceName);
  SerialBT.setTimeout(10);
}

//Task for reading GNSS Serial
void ReadGnssSerialTask(void *e) {
  while (true) {
    if(GnssSerial.available()){
      auto count = GnssSerial.readBytes(gnssSerialReadBuffer, BUFFER_SIZE);
      SerialBT.write(gnssSerialReadBuffer, count);
      // Serial.write(gnssSerialReadBuffer, count);
    }
    delay(20);
  }
}

//Task for reading BLE Serial
void ReadBtTask(void *e) {
  while (true) {
    if (SerialBT.available()) {
      auto count = SerialBT.readBytes(bleReadBuffer, BUFFER_SIZE);
      GnssSerial.write(bleReadBuffer, count);
    }
    delay(20);
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
  xTaskCreate(ReadBtTask, "ReadBtTask", STACK_SIZE, NULL, 1, NULL);
}

void loop() {
  //This task is not used
  // vTaskDelete(NULL); //uncomment this line if you remove all other code in the loop function and the loop function is unucessary
  
  //Send a "ping" message in the serial monitor
  static unsigned long lastDiag = 0;
  if (millis() - lastDiag > 5000) {
    Serial.println("ping");
    lastDiag = millis();
  }
}