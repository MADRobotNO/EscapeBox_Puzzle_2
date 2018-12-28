#define DEBUG

#include <SPI.h>
#include <MFRC522.h>


const byte sdaPin = 9;
const byte rstPin = 8;

const byte towerPin = 6;

const byte LED = 4;

const String tag = "0407a90a3e4d81";

int towerStatus;

MFRC522 rfid(sdaPin, rstPin);


void setup() {
  #ifdef DEBUG
  Serial.begin(9600); // Initialize serial communications with the PC
  Serial.println("Setup starts...");
  #endif

  SPI.begin();        // Init SPI bus

  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial();
  Serial.println("RFID Connected");

  pinMode(towerPin, INPUT_PULLUP);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  
  Serial.println("Setup done");
  
}


void loop() {

  towerStatus = digitalRead(towerPin);

  Serial.print("Tower status: ");
  Serial.println(towerStatus);
  
  String readRFID = "";
  
  if (towerStatus == 0){
    
    MFRC522 rfid(sdaPin, rstPin);
    rfid.PCD_Init();
    
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      readRFID = dump_byte_array(rfid.uid.uidByte, rfid.uid.size);
      Serial.print("Tag ID: ");
      Serial.println(readRFID);
    }

    // Halt PICC
    rfid.PICC_HaltA();
    // Stop encryption on PCD
    rfid.PCD_StopCrypto1();
  }

  if (readRFID.equals(tag)){
    Serial.println("Tag correct. LED ON.");
    digitalWrite(LED, LOW);
  }

  else{
    digitalWrite(LED, HIGH);
  }


  delay(100);
  
}

String dump_byte_array(byte *buffer, byte bufferSize) {
  String a = "";
  for (byte i = 0; i < bufferSize; i++) {
    a += String(buffer[i] < 0x10 ? "0" : "");
    a += String(buffer[i], HEX);
  }
  return a;
}
