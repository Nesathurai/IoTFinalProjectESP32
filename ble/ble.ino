/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   Changed to a beacon scanner to report iBeacon, EddystoneURL and EddystoneTLM beacons by beegee-tokyo
*/

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

int scanTime = 5; // In seconds
BLEScan *pBLEScan;
char bleData[10000];

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        // Serial.print("RSSI: ");
        // Serial.println(advertisedDevice.getRSSI());
        // Serial.println(advertisedDevice.getAddress().toString().c_str());
        // Serial.println("");

        strcat(bleData, advertisedDevice.getAddress().toString().c_str());
        strcat(bleData, ";");

        char rssiChar[8];
        String rssiString = String(advertisedDevice.getRSSI());
        rssiString.toCharArray(rssiChar, rssiString.length() + 1);
        strcat(bleData, rssiChar);
        strcat(bleData, "!");
    }
};

void setup()
{
    Serial.begin(115200);
    Serial.println("Scanning...");

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); // create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99); // less or equal setInterval value

    strcpy(bleData, "");
}

void loop()
{
    // put your main code here, to run repeatedly:
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
    delay(2000);
    Serial.println(bleData);
    strcpy(bleData, "");
}