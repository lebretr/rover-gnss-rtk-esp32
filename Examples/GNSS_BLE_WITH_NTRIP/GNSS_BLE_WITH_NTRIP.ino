//CPU: 240MHZ
//Core Debug Level: Verbose (Just for Test. For prod: None)
//Erase All flash: Enable (Only the 1rst time)
//Flash size: 4Mb
//Partition: NO OTA (2Mb App/2Mb SPIFFS)
//Speed 115200


#include <BleSerial.h>
#include <esp_attr.h>
#include <esp_task_wdt.h>
#include <driver/rtc_io.h>
#include <esp_mac.h>

#include <queue>

#include <WiFi.h>
#include "NTRIPClient.h"
#include "Secret.h"

const int BUFFER_SIZE = 4096 ; 
const int STACK_SIZE = 4096 ; 

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

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

char* host = NTRIP_SERVER_HOST;
int httpPort = 2101; // port 2101 is default port of NTRIP caster
char* mntpnt = NTRIP_CASTER_MOUNTPOINT;
char* user   = NTRIP_USER;
char* passwd = NTRIP_PASSWORD;
bool sendGGA = true;
NTRIPClient ntrip_c;

const long interval = 10000;     // Duration between 2 GGA Sending to ntrip

String nmea_buffer="";

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
      // auto count = GnssSerial.readBytes(gnssSerialReadBuffer, BUFFER_SIZE);
      // SerialBT.write(gnssSerialReadBuffer, count);

      String s=findValidNmeaString(GnssSerial.readStringUntil('\n'));
      if(s!=""){
        if (s.startsWith("$GNGGA") || s.startsWith("$GPGGA")) {
          nmea_buffer=s;
          // Serial.println(s);
        }
        SerialBT.println(s);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

//Task for reading BLE Serial
void ReadBtTask(void *e) {
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
  delay(500);

  //Start Gnss Serial
  GnssSerial.begin(GNSS_SERIAL_BAUD_RATE, SERIAL_8N1, PIN_RX, PIN_TX); // serial port to send RTCM to F9P, UL980,...
  GnssSerial.setTimeout(10);
  delay(100);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(100);

  //Start BLE
  startBluetooth();

  //Disable watchdog timers
  // disableCore0WDT();  //I don't use this on my esp32-WROOM-32D because error "task_wdt: esp_task_wdt_reset" error
  // disableCore1WDT();  //I don't use this on my esp32-WROOM-32D because error "task_wdt: esp_task_wdt_reset" error
  disableLoopWDT();   //I don't use this on my esp32-WROOM-32D because error "task_wdt: esp_task_wdt_reset" error
  esp_task_wdt_delete(NULL);

  //Start tasks
  xTaskCreate(ReadGnssSerialTask, "ReadGnssSerialTask", STACK_SIZE, NULL, 2, NULL);
  xTaskCreate(ReadBtTask, "ReadBtTask", STACK_SIZE, NULL, 1, NULL);
  
  Serial.println("Requesting MountPoint's Raw data");
  if (!ntrip_c.reqRaw(host, httpPort, mntpnt, user, passwd)) {
    delay(5000);
    if (!ntrip_c.reqRaw(host, httpPort, mntpnt, user, passwd)) {
      delay(5000);
      ESP.restart();
    }
  }
  
  Serial.println("Requesting MountPoint is OK");
}

void loop() {
  //This task is not used
  // vTaskDelete(NULL); //uncomment this line if you have no other code in the loop function.
  
  static unsigned long previousMillis = 0; // timer
  unsigned long currentMillis = 0;  // timer
  
  
  //if countdown is done, we send a GNGGA (or GPGGA) data to the Ntrip server
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    if (nmea_buffer.startsWith("$GNGGA") || nmea_buffer.startsWith("$GPGGA")) {
      // Check GGA format
      int numFields = 0;
      int numFieldsEmptyConsec=0;
      for (char ch : nmea_buffer) {
        if (ch == ',') {
          numFields++;
          numFieldsEmptyConsec++;
        }else{
          numFieldsEmptyConsec=0;
        }
        if(numFieldsEmptyConsec==3) { //if 3 commas without data between => Gnss module didn't find coordinates yes
          // log_d("NO GPS COORDINATE => NO NTRIP CALL "); 
          break;
        }
      }
      
      if (numFields == 14) { // 14 commas for a good GGA message 
        if(numFieldsEmptyConsec==3) { 
          previousMillis = currentMillis; // Reinit countdown and wait 10s
        }else{ 
          ntrip_c.setLastGGA(nmea_buffer); // store last GGA message
          Serial.println("Extracted GGA: " + nmea_buffer);
          
          // Send the last GGA message stored
          String lastGGA = ntrip_c.getLastGGA();
          if (lastGGA != "") {
            ntrip_c.sendGGA(lastGGA.c_str(), host, httpPort, user, passwd, mntpnt);
            Serial.println("Sent GGA: " + lastGGA);
            previousMillis = currentMillis;  // Reinit countdown and wait 10s after sent  CGA with success
          } else {
            Serial.println("No GGA message to send.");
          }
          Serial.println();
          Serial.println();
        }
      }
    }
  }

  //Sent RTCM from NTRIP Server to Gnss module
  int ntrip_c_countdown=512;
  while (ntrip_c_countdown>0 && ntrip_c.available()) {
    char ch = ntrip_c.read();
    GnssSerial.write(ch);
    ntrip_c_countdown--;
  }

  delay(1);
}

String findValidNmeaString(String s){
  static String n_buffer="";
  static int star_countdown=120;

  String nmea="";
  for (char c : s){
    if(int(c)>=32 && int(c)<127){
      if(c=='*'){//Si flag checksumm
        star_countdown=2;// on va prendre les 2 characteres suivants.
      }
      if(c=='$' || star_countdown<0){
        if(n_buffer!="" && n_buffer[0]=='$'){
          nmea=n_buffer;
          // Serial.println(nmea);
        }
        // else{
        //     Serial.println(n_buffer);
        // }
        
        //RESET FLAGS
        star_countdown=150;
        n_buffer="";
      }
      n_buffer+=c;
      star_countdown--;
    }
  }
  return nmea;
}