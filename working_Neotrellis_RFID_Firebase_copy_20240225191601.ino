#include <Firebase_ESP_Client.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "Adafruit_NeoTrellis.h"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// RFID setup
#define RST_PIN 0
#define SS_PIN  2
MFRC522 mfrc522(SS_PIN, RST_PIN);
bool fireJsonExecuted = false;
String displayName = "/prarthanadeepak002";
String trial = "";
String selectedElementsPath ="";
bool getUsernameFlag = true; // Flag to control the getUsername function execution

// Neotrellis setup
unsigned long sendDataPrevMillis = 0;
int count = 0;
const int MAX_VALUES = 100;
int storedValues[MAX_VALUES];
bool storeValues = false;

#define Y_DIM 8
#define X_DIM 8
Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {
  {Adafruit_NeoTrellis(0x31), Adafruit_NeoTrellis(0x2F)},
  {Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x35)}
};
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM / 4, X_DIM / 4);
uint32_t TealColor = seesaw_NeoPixel::Color(0, 128, 128);

// Callback for Neotrellis
TrellisCallback blink(keyEvent evt) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    trellis.setPixelColor(evt.bit.NUM, TealColor);
    Serial.print(evt.bit.NUM + 1);

    if (storeValues && count < MAX_VALUES) {
      storedValues[count] = evt.bit.NUM + 1;
      count++;
    } else if (count >= MAX_VALUES) {
      Serial.println("Array is full!");
    }
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    // Don't turn off the LED on falling edge, keep it teal
  }

  trellis.show();
  return 0;
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  // Connect to Wi-Fi
  WiFi.begin("Setup", "Steve123");
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Firebase initialization
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = "AIzaSyCNuwZJU-EUuc6o0LGHNUCuxpZqHBLtVhE";
  auth.user.email = "prarthanadeepak002@gmail.com";
  auth.user.password = "Flowers48*";
  config.database_url = "https://staff-357a4-default-rtdb.firebaseio.com/";
  config.token_status_callback = tokenStatusCallback;
  fbdo.setBSSLBufferSize(2048, 1024);
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);

  // Neotrellis initialization
  if (!trellis.begin()) {
    Serial.println("Failed to begin trellis");
    while (1)
      delay(1);
  }

  // Neotrellis LED setup
  for (int i = 0; i < Y_DIM * X_DIM; i++) {
    trellis.setPixelColor(i, 0);
    trellis.show();
    delay(50);
  }

  // Neotrellis key setup
  for (int y = 0; y < Y_DIM; y++) {
    for (int x = 0; x < X_DIM; x++) {
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, blink);
      trellis.show();
      delay(50);
    }
  }
}

void loop() {

  // RFID Loop
  while (getUsernameFlag) {
    getUsername(trial,selectedElementsPath);
    delay(1000);
  }
  FirebaseJson json;
  json.add("gameConnected","Minesotta Dexterity Test");
  trial = displayName+trial;
  fire_json(trial,json);
  //Serial.print(selectedElementsPath);
  trellis.read();
  // Neotrellis Loop
  // Serial Input Loop
  if (Serial.available() > 0) {
    char inputChar = Serial.read();
    if (inputChar == 's') {
      storeValues = true;
      Serial.println("Start storing values...");
    } else if (inputChar == 'S') {
      storeValues = false;
      Serial.println("\nStop storing values. Printing stored values:");
      FirebaseJsonArray arr;
      for (int i = 0; i < count; i++) {
        arr.add(storedValues[i]);
        Serial.print(storedValues[i]);
        Serial.print(" ");
      }
      Serial.print(selectedElementsPath);
      fire(selectedElementsPath, arr);
      delay(1000);
      ESP.restart();
      // Send data to Firebase
    }
  
  }
}

String getUsername(String &passed, String &passed2) {
  Serial.print("Card not detected");
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  // Check for a new RFID card
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return "this sucks";
  }

  Serial.println(F("**Card Detected:**"));

  byte block = 4;
  byte len = 24; // Assuming you want to read 16 bytes

  // Authenticate and read RFID data
  if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
    Serial.println(F("Authentication failed"));
    return "this sucks authentication error";
  }

  byte buffer1[26];
  if (mfrc522.MIFARE_Read(block, buffer1, &len) != MFRC522::STATUS_OK) {
    Serial.println(F("Reading failed"));
    return "this sucks reading error";
  }

  // Extract username from RFID data
  String check ="";
  for (uint8_t i = 0; i < 16 && buffer1[i] != 32; i++) {
    check += (char)buffer1[i];
  }
  
  check.trim();
  String result = "/"+check;
  passed = result;
  passed2 = "prarthanadeepak002"+result+"/SelectedElements";
  delay(1000);

  // Halt RFID and stop crypto1
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // Set the flag to false to stop further execution of getUsername
  getUsernameFlag = false;

  return result;
}

void fire(String x, FirebaseJsonArray array1) {

    Serial.println(x);
    Serial.println();
    Serial.printf("Set array... %s\n", Firebase.RTDB.setArray(&fbdo, x.c_str(), &array1) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Get array... %s\n", Firebase.RTDB.getArray(&fbdo, x.c_str()) ? fbdo.to<FirebaseJsonArray>().raw() : fbdo.errorReason().c_str());
}

void fire_json(String p, FirebaseJson j) {
  if (!fireJsonExecuted) {

    fireJsonExecuted = true;

    Serial.println(p);
    Serial.println();
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, p.c_str(), &j) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Get json... %s\n", Firebase.RTDB.getJSON(&fbdo, p.c_str()) ? fbdo.to<FirebaseJson>().raw() : fbdo.errorReason().c_str());
  }
}

