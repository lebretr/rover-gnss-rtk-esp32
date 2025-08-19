# GNSS_BT_WITH_NTRIP_SLOW_QUEUE

This is an example of a GNSS rover with NTRIP embedded in the esp32 that use classical Bluetooth (SPP).

You can use this example with [SW Maps](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps&hl=fr) on your android phone, or with Lefebure Ntrip Client.

Key features include:
- Integrated calls to the NTRIP server via Wi-Fi. This is useful for achieving centimeter-level accuracy in Android applications like Locus GIS or QField without using Lefebure's Ntrip client or changing the configuration in Android's developers settings.
- Use of a queue to limit Bluetooth SPP buffer congestion.

## Important note to build this source
All esp32 can't use serial Bluetooth. For example, you can use this code on an esp32-wroom-32D (Bluetooth 4) but not on esp32-S3 (bluetooth LE only).

Install the "esp32 by espressif" board in Arduino IDE.

For good Bluetooth performance, the better way is to only have the "esp32 by espressif" board. 
I recommend to remove "Arduino ESP32 Boards" and all other unnecessary boards and libraries if you want to avoid a lot of "SPP Write Congested!" errors in your esp32.

## Note

This source is derived from [jancelin / rover-gnss / 3b-GNSS_RTK](https://github.com/jancelin/rover-gnss/tree/master/unit_tests/3b-GNSS_RTK). 

## Note 2

I used this code on a ESP32-WROOM-32D with an UM980 module

## Note 3

If you use your Gnss module with a "5Ghz conf" and you have too many "SPP Write Congested!" errors in your esp32 serial monitor, so change the config of your Gnss module to "2Ghz".


2Ghz conf for UM980:
```
CONFIG COM2 115200
GPGGA COM2 0.5
GPGSA COM2 0.5
GPRMC COM2 0.5
GPGSV COM2 0.5
GPGST COM2 0.5
```


And don't forget to change this variables in your code:
```
#define MYSERIAL_BAUD_RATE  115200
...
const long intervalPause = 25;
```

## How to connect your Gnss module to ESP32

To see how to connect your F9P ou UM980 module to an esp32, visit [this page](https://github.com/jancelin/rover-gnss/blob/master/unit_tests/3b-GNSS_RTK/README.md)


## How to connect an ESP32-WROOM-32D and a UM980 module

![image](/assets/images/ESP32-32D_UM980.png)

## How to configure Aduino IDE for an ESP32-WROOM-32D

![image](/assets/images/ArduinoIde_config.png)
