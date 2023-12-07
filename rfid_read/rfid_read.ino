#include <SPI.h>
#include <MFRC522.h>

#define SDA_PIN     21     
#define SCK_PIN     5      
#define MOSI_PIN    34   
#define MISO_PIN    19
#define RST_PIN     2  

// Create MFRC522 instance
MFRC522 mfrc522(SDA_PIN, RST_PIN);  
// Set default key
MFRC522::MIFARE_Key key = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

void setup() {
  Serial.begin(9600);

  // initalize SPI and RFID object
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);
  mfrc522.PCD_Init();
}

void loop() {
  // If a new card is detected in th range 
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {

    // Get RFID card UID and PICC type
    Serial.print("Card UID: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.print(" PICC type: ");
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Set sector number and block offset, must correspond to set numbers of written data
    byte sectorNumber = 1;  
    byte blockOffset = 1;   

    // Calculate the block number within the sector
    byte blockNumber = (sectorNumber * 4) + blockOffset;

    // Authentication with the default key (0xFF)
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid)) == MFRC522::STATUS_OK) {
      Serial.println("Authentication successful");

      byte buffer[18]; 
      byte bufferSize = sizeof(buffer);

      if (mfrc522.MIFARE_Read(blockNumber, buffer, &bufferSize) == MFRC522::STATUS_OK) {
        Serial.println("Read successful");
        Serial.print("Data read: ");
        String dataString = "";
        for (byte i = 0; i < bufferSize - 2; i++) {
          dataString += (char)buffer[i];
        }
        Serial.println(dataString);
      } else {
        Serial.println("Read failed");
      }
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    } else {
      Serial.println("Authentication failed");
    }
  }

  delay(1000);  
}