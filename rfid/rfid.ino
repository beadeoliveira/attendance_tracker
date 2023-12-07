#include <WiFi.h>
#include <heltec.h>
#include <MFRC522.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SDA_PIN     21     
#define SCK_PIN     5      
#define MOSI_PIN    34   
#define MISO_PIN    19
#define RST_PIN     2     

const char* ssid     = "DukeVisitor";
const char* password = "";

// Create instance
MFRC522 mfrc522(SDA_PIN, RST_PIN);  
// Set default key
MFRC522::MIFARE_Key key = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

void setup() {
    Serial.begin(9600);
    
    // initalize SPI, RFID object, and wifi 
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);  
    mfrc522.PCD_Init();  
    WiFi.begin(ssid, password);
}

void loop() {
    // If a new card is detected in th range 
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        Serial.println("RFID Card Detected!");

        // Get RFID card UID
        String cardUID = "";
        for (byte i = 0; i < mfrc522.uid.size; ++i) {
            cardUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
            cardUID += String(mfrc522.uid.uidByte[i], HEX);
        }
        String name = "";
        // Print UID 
        Serial.println("Card UID: " + cardUID);

        // Start to read the memory storage blocks 
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
            name = dataString;
            Serial.println(dataString);
          } else {
            Serial.println("Read failed");
          }

          mfrc522.PICC_HaltA();
          mfrc522.PCD_StopCrypto1();
        } else {
          Serial.println("Authentication failed");
        }

        
        // Transfer name and UID using HTTP request  
        HTTPClient http;
        
        // Create JSON doc
        DynamicJsonDocument doc(2048);
        doc["id"] = cardUID;
        doc["name"] = name;
        String json;
        serializeJson(doc, json);

        // Start Request 
        if (http.begin("http://3.21.105.25/update")) {
          Serial.print("linked");
          http.addHeader("Content-Type", "application/json");
          int httpResponseCode = http.POST(json);
          Serial.println("HTTP Response Code: " + String(httpResponseCode));
          String response = http.getString();
          Serial.println("Response: " + response);

          // Print out Request Response Code 
          if (httpResponseCode > 0) {
            Serial.println("HTTP Response Code: " + String(httpResponseCode));
            String response = http.getString();
            Serial.println("Response: " + response);
          }
        }
        http.end();
  
    }
    Serial.println("nothing");

    // Check if a new card has entered range every 0.5 seconds 
    delay(500);
}