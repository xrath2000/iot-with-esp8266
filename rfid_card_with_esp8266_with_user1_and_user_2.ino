#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <SPI.h>
#include <MFRC522.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#define WIFI_SSID "Galaxy"
#define WIFI_PASSWORD "12345678"

#define API_KEY "AIzaSyCAG2TGv-Bo9ErepRXqvIPmFmsmgaDH3kM"
#define DATABASE_URL "https://humtemp-411aa-default-rtdb.firebaseio.com/"

#define USER_EMAIL "sarathkumarsbmc19ec009@gmail.com"
#define USER_PASSWORD "sarath@2024"

#define SS_PIN D4
#define RST_PIN D3

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;
byte nuidPICC[4];

// Define UIDs for specific users (in HEX format)
String user1UID = "0649F01B"; // Example UID for user1
String user2UID = "A367871A"; // Example UID for user2

// Convert byte array to hex string
String byteArrayToHexString(byte *buffer, byte bufferSize) {
  String hexString = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) hexString += "0";
    hexString += String(buffer[i], HEX);
  }
  hexString.toUpperCase();  // Modify the string in place
  return hexString;
}

void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  Serial.println(byteArrayToHexString(key.keyByte, MFRC522::MF_KEY_SIZE));

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  String uidHexString = byteArrayToHexString(rfid.uid.uidByte, rfid.uid.size);

  if (uidHexString != user1UID && uidHexString != user2UID) {
    Serial.println(F("User not found."));
    return;
  }

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 500 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (uidHexString == user1UID) {
      Serial.println(F("User1 detected."));
      Firebase.setString(fbdo, F("/users/user1/id/"), uidHexString);
      Firebase.setString(fbdo, F("/users/user1/name/"), "User 1 login");
    } 
    else if (uidHexString == user2UID) {
      Serial.println(F("un identified user"));
      Firebase.setString(fbdo, F("/users/user2/id/"), uidHexString);
      Firebase.setString(fbdo, F("/users/user2/name/"), "un identified user");
    }
    /*else if (uidHexString == user3UID) {
      Serial.println(F("User2 detected."));
      Firebase.setString(fbdo, F("/users/user2/id/"), uidHexString);
      Firebase.setString(fbdo, F("/users/user2/name/"), "user not found");
    }*/

    Serial.printf("Set string (hex)... %s\n", Firebase.setString(fbdo, F("/last_login/id/"), uidHexString) ? "ok" : fbdo.errorReason().c_str());
  }

  count++;
}
