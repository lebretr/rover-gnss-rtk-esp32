# GNSS_BLE_WITHOUT_NTRIP

This is an example of a GNSS rover without NTRIP That use Bluetooth LE (BLE).

Not all android aps are able to manage BLE.

You can use this example with [SW Maps](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps&hl=fr) on your android phone.

If you need a rover bluetooth but you want use it with another app that not manage BLE devices, please see "GNSS_BT_xxx" examples.

## Important note to build this source in Arduino IDE
The better way is to only install the "esp32 by espressif" board and "ESP32_BleSerial by Avinab Malla" library.

If you encounter a compilation error, specialy with BLE, remove "Arduino ESP32 Boards" and all other unnecessary boards and libraries.

## Note

This source is derived from "Bluetooth LE Serial Bridge Example" by Avinab Malla

## Note 2

I use this code on a ESP32-WROOM-32D with an UM980 module

## How to connect your Gnss module to ESP32

To see how to connect your F9P ou UM980 module to an esp32, visit [this page](https://github.com/jancelin/rover-gnss/blob/master/unit_tests/3b-GNSS_RTK/README.md)


## How to connect an ESP32-WROOM-32D and a UM980 module

![image](/assets/images/ESP32-32D_UM980.png)

## How to configure Aduino IDE for an ESP32-WROOM-32D

![image](/assets/images/ArduinoIde_config.png)
