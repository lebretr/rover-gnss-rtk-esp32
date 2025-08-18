//CPU: 240MHZ
//Core Debug Level: Verbose (Just for Test. For prod: None)
//Erase All flash: Enable (Only the 1rst time)
//Flash size: 4Mb
//Partition: NO OTA (2Mb App/2Mb SPIFFS)
//Speed 115200


/* ###################
* #    LIBRARIES    #
* ###################
*/
#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <BluetoothSerial.h>
#include "Secret.h"
#include <queue>

String buffer_nmea="";

struct nmea_t {
  String nmea;
  unsigned long datetime;
} nmea_R;
std::queue<nmea_t> myqueue;

// esp_spp_cfg_t spp_cfg = {
//     .mode = ESP_SPP_MODE_CB,
//     .enable_l2cap_ertm = true,
//     .tx_buffer_size = 1024*10
// };

// GNSS serial port
HardwareSerial MySerial(1);
#define PIN_RX 16
#define PIN_TX 17
#define MYSERIAL_BAUD_RATE  115200// 460800 //115200 Works with F9P config on 2Hz ( not in 5Hz ! )

// BLUETOOTH Name
#define BT_NAME "GNSS_ANTENNA_NTRIP"

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

char* host = NTRIP_SERVER_HOST;
int httpPort = 2101; // port 2101 is default port of NTRIP caster
char* mntpnt = NTRIP_CASTER_MOUNTPOINT;
char* user   = NTRIP_USER;
char* passwd = NTRIP_PASSWORD;
bool sendGGA = true;
NTRIPClient ntrip_c;

long intervalBT = 25; // timer
const long intervalPause = 25;     // Duration between 2 NMEA Sending to BT
const long interval = 10000;     // Duration between 2 GGA Sending to ntrip

BluetoothSerial SerialBT;

bool is_BT_congested=false;

void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  switch (event) {
    case ESP_SPP_INIT_EVT:
    Serial.println("SPP initialisé");
    break;
    
    case ESP_SPP_START_EVT:
    Serial.println("SPP démarré");
    break;
    
    case ESP_SPP_CL_INIT_EVT:
    Serial.println("Client SPP connecté");
    break;
    
    // case ESP_SPP_WRITE_EVT:
    //     if (param->write.status == ESP_SPP_SUCCESS) {
    //         // Serial.println("Écriture réussie");
    //         // if(intervalBT>25){
    //         //     intervalBT=intervalBT-25;
    //         //     Serial.print("intervalBT - : ");
    //         //     Serial.println(intervalBT);
    //         // }
    //         // if(intervalBT<25){
    //         //     intervalBT=25;
    //         //     Serial.print("intervalBT - : ");
    //         //     Serial.println(intervalBT);
    //         // }
    //     } else {
    //         Serial.println("Échec d'écriture SPP");
    //     }
    break;
    
    case ESP_SPP_CONG_EVT:
    if (param->cong.cong) {
      Serial.println("SPP congestionné");
      // Arrêter temporairement l'envoi
      intervalBT=intervalBT+100;
      Serial.print("intervalBT + : ");
      Serial.println(intervalBT);
      // delay(100);
    }
    break;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  // We start by connecting to a WiFi network
  delay(500);
  MySerial.begin(MYSERIAL_BAUD_RATE, SERIAL_8N1, PIN_RX, PIN_TX); // serial port to send RTCM to F9P, UL980,...
  // MySerial.setTimeout(1000);
  
  // We start by connecting to a WiFi network
  delay(100);
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
  
  if (!SerialBT.begin(BT_NAME)) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    // SerialBT.register_callback(btCallback);
    Serial.println("Bluetooth initialized with name " + (String)BT_NAME);
  }
  
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
  static unsigned long previousMillisPause = 0; // timer
  static unsigned long currentMillisPause = 0;  // timer
  
  static unsigned long previousMillis = 0; // timer
  static unsigned long currentMillis = 0;  // timer
  
  currentMillisPause = millis();
  if(!myqueue.empty()){
    unsigned long qd=myqueue.front().datetime;
    long diff=currentMillisPause - qd;
    if(diff>450){
      while(!myqueue.empty()){
        unsigned long qdt=myqueue.front().datetime;
        long qdt_diff=currentMillisPause - qd;
        Serial.print(myqueue.size()); //get the first data
        Serial.print(" * "); //get the first data
        Serial.print(myqueue.front().nmea); //get the first data        
        Serial.print(" * "); //get the first data
        Serial.print(currentMillisPause); //get the first data        
        Serial.print(" * "); //get the first data
        Serial.print(myqueue.front().datetime); //get the first data        
        Serial.print(" * "); //get the first data
        Serial.println(qdt_diff); //get the first data
        
        myqueue.pop(); //delete the first data
      }
    }
  }
  
  currentMillisPause = millis();
  if(MySerial.available()){
    String s=findValidNmeaString(MySerial.readStringUntil('\n'));
    if(s!=""){
      nmea_t n={s,currentMillisPause};
      // n.nmea=s;
      // n.datetime=millis();
      myqueue.push(n);
    }
  }
  
  currentMillisPause = millis();
  if(currentMillisPause - previousMillisPause >= intervalPause) {
    if(!myqueue.empty()){
      String s=myqueue.front().nmea; //get the first data
      if (/*currentMillisPause - previousMillisPause >=  intervalBT &&*/ SerialBT.hasClient()){
        SerialBT.println(s); 
      }
      
      currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        // Serial.println("10s ecoulé");
        // Serial.println(s); 
        if (s.startsWith("$GNGGA") || s.startsWith("$GPGGA")) {
          // Validation du format GGA
          int numFields = 0;
          int numFieldsEmptyConsec=0;
          for (char ch : s) {
            if (ch == ',') {
              numFields++;
              numFieldsEmptyConsec++;
            }else{
              numFieldsEmptyConsec=0;
            }
            if(numFieldsEmptyConsec==3) { // Log // Si il y a au moins 3 virgules consecutives, alors l'antenne n'a pas encore fixé de satelittes error
              // log_d("NO GPS COORDINATE: NO NTRIP CALL ");  // Log error
              break; // Sortir de la boucle intérieure
            }
          }
          
          if (numFields == 14) { // 14 virgules attendues dans un message GGA complet 
            if(numFieldsEmptyConsec==3) { // si pas de coordonnée
              previousMillis = currentMillis;   // Reinit compteur de temps et on attend 10s pour laisser le temps de trouver des coordonées
            }else{ 
              ntrip_c.setLastGGA(s);                  // Stocker le dernier message GGA reçu
              Serial.println("Extracted GGA: " + s);  // Log du message GGA extrait
              
              // Send the last GGA message stored
              String lastGGA = ntrip_c.getLastGGA();
              if (lastGGA != "") {
                ntrip_c.sendGGA(lastGGA.c_str(), host, httpPort, user, passwd, mntpnt);
                Serial.println("Sent GGA: " + lastGGA);  // Log sent GGA message
                previousMillis = currentMillis;   // Reinit compteur de temps après succes envoi CGA
              } else {
                Serial.println("No GGA message to send.");
              }
              Serial.println();
              Serial.println();
            }
          }
        }
      }
      previousMillisPause=currentMillisPause;
      myqueue.pop(); //delete the first data
    }
  }
  
  // if(ntrip_c.available()){
  // long start = millis();
  
  
  // static unsigned long previousMillisPauseNtrip = 0; // timer
  // unsigned long currentMillisPauseNtrip = millis();  // timer
  // // currentMillisPause = millis();
  // if (currentMillisPauseNtrip - previousMillisPauseNtrip >=  intervalBT ){
  int ntrip_c_countdown=128;
  while (ntrip_c_countdown>0 && ntrip_c.available()) {
    char ch = ntrip_c.read();
    MySerial.write(ch);
    ntrip_c_countdown--;
  }
  //     previousMillisPauseNtrip=currentMillisPauseNtrip;
  
  //         if(intervalBT>25){
  //             intervalBT=intervalBT-25;
  //             Serial.print("intervalBT - : ");
  //             Serial.println(intervalBT);
  //         }
  // }
  
  
  // static unsigned long lastDiag = 0;
  // if (millis() - lastDiag > 5000) {
  //     diagnosticBT();
  //     lastDiag = millis();
  // }
  
  
  // long stop = millis();
  // long delta=stop - start;
  // Serial.print("While time: ");
  // Serial.print(delta);
  // Serial.print(" ntrip_c_countdown: ");
  // Serial.println(ntrip_c_countdown);
  // }
}

// void diagnosticBT() {
//     Serial.print("Client connecté: ");
//     Serial.println(SerialBT.hasClient() ? "Oui" : "Non");

//     Serial.print("Mémoire libre: ");
//     Serial.println(ESP.getFreeHeap());

//     // Vérifier l'état des stacks FreeRTOS
//     Serial.print("Stack BT libre: ");
//     Serial.println(uxTaskGetStackHighWaterMark(NULL));
// }

int star_countdown=120;
String findValidNmeaChar(char c){
  String nmea="";
  if(int(c)>=32&&int(c)<127){
    if(c=='*'){//Si flag checksumm
      star_countdown=2;// on va prendre les 2 characteres suivants.
    }
    if(c=='$' || star_countdown<0){
      if(buffer_nmea!="" && buffer_nmea[0]=='$'){
        nmea=buffer_nmea;
        // Serial.println(nmea);
      }
      // else{
      //     Serial.print("buffer_nmea: ");
      //     Serial.println(buffer_nmea);
      // }
      
      //RESET FLAGS
      star_countdown=150;
      buffer_nmea="";
    }
    buffer_nmea+=c;
    star_countdown--;
  }
  // else{
  //     Serial.print("c:");
  //     Serial.print(c);
  //     Serial.print(" - ");
  //     Serial.print(buffer_nmea);
  //     Serial.println(".");
  // }
  return nmea;
}

String findValidNmeaString(String s){
  String nmea="";
  for (char c : s){
    if(int(c)>=32&&int(c)<127){
      if(c=='*'){//Si flag checksumm
        star_countdown=2;// on va prendre les 2 characteres suivants.
      }
      if(c=='$' || star_countdown<0){
        if(buffer_nmea!="" && buffer_nmea[0]=='$'){
          nmea=buffer_nmea;
          // Serial.println(nmea);
        }
        // else{
        //     Serial.println(buffer_nmea);
        // }
        
        //RESET FLAGS
        star_countdown=150;
        buffer_nmea="";
      }
      buffer_nmea+=c;
      star_countdown--;
    }
    // else{
    //     Serial.print("c:");
    //     Serial.print(c);
    //     Serial.print(" - ");
    //     Serial.print(buffer_nmea);
    //     Serial.println(".");
    // }
  }
  return nmea;
}