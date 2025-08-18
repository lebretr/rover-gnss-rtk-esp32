# GNSS_BT_WITHOUT_NTRIP

This is an example of a GNSS rover without NTRIP That use classical Bluetooth (SPP).

You can use this example with [SW Maps](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps&hl=fr) on your android phone, or with Lefebure Ntrip Client.


## Important note to build this source
All esp32 can't use serial Bluetooth. For example, you can use this code on an esp32-wroom-32D (Bluetooth 4) but not on esp32-S3 (bluetooth LE only).

Install the "esp32 by espressif" board in Arduino IDE.

## Note

This source is derived from [jancelin / rover-gnss / 8-BT](https://github.com/jancelin/rover-gnss/tree/master/unit_tests/8-BT). 

## Note 2

I used this code on a ESP32-WROOM-32D with an UM980 module

## How to connect your Gnss module to ESP32

To see how to connect your F9P ou UM980 module to an esp32, visit [this page](https://github.com/jancelin/rover-gnss/blob/master/unit_tests/3b-GNSS_RTK/README.md)


## How to connect an ESP32-WROOM-32D and a UM980 module

![image](/assets/images/ESP32-32D_UM980.png)

## How to configure Aduino IDE for an ESP32-WROOM-32D

![image](/assets/images/ArduinoIde_config.png)
