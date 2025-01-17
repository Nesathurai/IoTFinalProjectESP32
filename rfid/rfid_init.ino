/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-rfid-nfc
 */

#include <SPI.h>
#include <MFRC522.h> // https://github.com/miguelbalboa/rfid

#define SS_PIN  5  // ESP32 pin GIOP5 
#define RST_PIN 27 // ESP32 pin GIOP27 

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  delay (1000);
  Serial.println("serial begin");
  SPI.begin(); // init SPI bus
  Serial.println("spi begin");
  rfid.PCD_Init(); // init MFRC522
  Serial.println("rfid begin");

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
}

void loop() {
  //Serial.println("loop begin");
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.print("RFID/NFC Tag Type: ");
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // print UID in Serial Monitor in the hex format
      Serial.print("UID:");
      String uuidString = "";
      for (int i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        uuidString += rfid.uid.uidByte[i] < 0x10 ? " 0" : " ";
        Serial.print(rfid.uid.uidByte[i], HEX);
        uuidString += String(rfid.uid.uidByte[i],HEX);
      }
      uuidString.toUpperCase();
      Serial.println();
      Serial.print(uuidString);
      Serial.println();

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}
