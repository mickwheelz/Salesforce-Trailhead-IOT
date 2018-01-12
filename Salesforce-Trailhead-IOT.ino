/* ESP8266 to Salesforce Sketch for Trailhead IoT Project
 * 
 * Uses an ESP866 to connect to salesforce, login via OAuth user/password flow and create platform event records
 * This is an example based on the trailhead project "Build an IoT Integration with Electric Imp" (https://trailhead.salesforce.com/projects/workshop-electric-imp)
 * The ESP8266 replaces the Electric Imp with minimal change to the instructions for the badge
 * 
 * You will need an ESP8266 (I am using a NodeMCU 1.0), a DHT11 (or DHT22) temp/humidity sensor, and a switch/5k resistor (or a light sensitive switch/resistor)
 * 
 * Changes required;
 * 
 * Step 2 - Set Up the Electric Imp Hardware
 * Ignore this step, connect your DHT11 (or DHT22) sensor to +3.3v, GND and D0 (data) of your ESP8266
 * Connect a switch between D1 and +3.3v and a 5k resistory from D1 to GND
 * 
 * Step 4 - Create a Salesforce Connected Appplication
 * Rather than the URL https://agent.electricimp.com/???? you can use http://localhost/_auth in Step 5, section 2
 * 
 * Step 10 - Build and Run the Electric Imp Application
 * Ignore this, simply flash this code to your ESP8266
 * 
 * Step 11 - Place Your impExplorer Kit in a Refrigerator 
 * Place the ESP8266 in your refrigerator, you will need a way to trigger the switch, alternatively you could use a light sensor (as per the electric imp)
 * This would need additional code to support it. If you are using an NodeMCU you can easily power it from a USB mobile phone battery
 * 
 * You can simply use the code in loop() to generate random data to test with if you don't wish to do this
 * 
 * Setup required
 *
 * 1. You will need to set the SSID (Network name) and Password variables to match your WiFi network
 *
 * 2. You will need to generate certificate fingerprints for both your login url (e.g login.salesforce.com) and your instance url (e.g eu1.salesforce.com) and insert them below
 * you can do this by running this command on macOS/*nix 
 * "echo | openssl s_client -connect <your instance url>:443 | openssl x509 -fingerprint -noout"
 *
 * 3. You will need to change the username, password, token, client secret and client key to match your org's configuration
 *
 */
 
//Required Libraries
#include <ESP8266HTTPClient.h> //Inbuilt ESP8266 Library
#include <ESP8266WiFi.h> //Inbuilt ESP8266 Library
#include <WiFiUdp.h> //Inbuilt ESP8266 Library

#include <TimeLib.h> //From https://github.com/PaulStoffregen/Time
#include <ArduinoJson.h> //From https://arduinojson.org/
#include <NTPClient.h> //From https://github.com/arduino-libraries/NTPClient
#include <DHT.h> //From https://github.com/adafruit/DHT-sensor-library
#include <Adafruit_Sensor.h> //From https://github.com/adafruit/DHT-sensor-library

// change to true to enable debugging, false to disable it
#define DEBUG true

//Wifi Settings
const char* ssid = "name"
const char* password = "password";

//Salesforce User Settings
const char* sfUsername = "username@domain.com";
const char* sfPassword = "password";
const char* sfToken = "token";

//Client ID and Secret from your connected app
const char* sfClientId = "clientID";
const char* sfClientKey = "clientSecret"; //Client Secret

/* SSL Certificate fingerprints, you need one for login.salesforce.com and one for your instance url (e.g eu1.salesforce.com or domain.my.salesforce.com)
   you can get this by executing: "echo | openssl s_client -connect <your instance url>:443 | openssl x509 -fingerprint -noout" on liux or macos, if you use windows you're on your own
   The ESP lacks the horsepower to do this on the fly */
const char*  sfLoginFingerprint = "EC:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx";
const char*  sfInstanceFingerprint = "EA:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx";

//these are used for NTP time setup
WiFiUDP udp;
NTPClient timeClient(udp);

//this is used for the DHT11 temp sensor
DHT dht(D0, DHT11);

//this is used for door switch
int doorPin = D1;

//vars to store SF auth token and SF instance url
String sfAuthToken = "";
String sfInstanceURL = "";

// Login method, will return a JSON object with auth token and instance URL you can use to perform requests
void doLogin(String username, String password, String token, String clientId, String clientKey, String fingerprint) {
        
    HTTPClient http;
    //you can change this to test.salesforce.com if you need to
    http.begin("https://login.salesforce.com/services/oauth2/token", fingerprint);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String postBody = "?&grant_type=password";
    postBody += "&client_id=" + clientId;
    postBody += "&client_secret=" + clientKey;
    postBody += "&username=" + username;
    postBody += "&password=" + password + token;
    
    int httpCode = http.POST(postBody);
    if(DEBUG) {
      Serial.print("http result:");
      Serial.println(httpCode);
    }
    
    String payload = http.getString();
    
    http.end();

    StaticJsonBuffer<1024> jsonBuffer;
    
    JsonObject& auth = jsonBuffer.parseObject(payload);
    
    if(DEBUG) {
      Serial.println("Response: ");
      Serial.println(payload);
    }
    
    if(httpCode == 200) {    
      Serial.println("Successfully logged in!");
      String token = auth["access_token"];
      String url = auth["instance_url"];
      sfAuthToken = token;
      sfInstanceURL = url;
    }
    
    else {
      Serial.println("An error occured, not logged in!");
    }

}

// Method to insert a SObject (or Platform Event!) pass it both the JSON auth object and an JSON object repersenting the record you wish to insert
bool insertSObject(String objectName, JsonObject& object) {
  
  bool insertSuccess = false;
  String reqURL = (String)sfInstanceURL + "/services/data/v40.0/sobjects/" + (String)objectName;

  if(DEBUG) {
    Serial.println("Instance URL: " + (String)sfInstanceURL);
    Serial.println("Auth Token: " + (String)sfAuthToken);
    Serial.println("Request URL: " + reqURL);
    Serial.println("JSON Sent: ");
    object.printTo(Serial);
    Serial.println();
  }

  HTTPClient http;
  http.begin(reqURL, sfInstanceFingerprint);
  http.addHeader("Authorization", "Bearer " + (String)sfAuthToken);
  http.addHeader("Content-Type", "application/JSON");
  
  String objectToInsert;
  object.printTo(objectToInsert);
    
  int httpCode = http.POST(objectToInsert);
  String payload = http.getString();
  
  http.end();

  if(httpCode == 201) {
    insertSuccess = true;
  }

  if (DEBUG) {
    Serial.println("HTTP Code:");
    Serial.println(httpCode);
    Serial.println("HTTP Response:");
    Serial.println(payload);
  }

  return insertSuccess;
}

//returns the current date/time in a format salesforce will accept
String getSFFormattedTime() {
  timeClient.update();
  setTime(timeClient.getEpochTime());
  String sfDateTime = (String)year() + "-" + padNumber((int)month(), 2) + "-" +padNumber((int)day(), 2);
  sfDateTime += "T" + padNumber((int)hour(), 2) + ":" + padNumber((int)minute(), 2) + ":" + padNumber((int)second(), 2) + ".000Z";
  return sfDateTime;
}

//pads numbers with leading 0's
String padNumber(int input, int maxDigits) {
  //max digits is either 2 or 3
  String output;
  if (maxDigits == 2) {
    if (input < 10) {
      output = "0" + (String)input;
    }
    else {
      output = (String)input;
    }
  }
  if (maxDigits == 3) {
    if (input < 10) {
      output = "00" + (String)input;
    }
    else if (input >= 10 && input < 100) {
      output = "0" + (String)input;
    }
    else {
      output = (String)input;
    }
  }
  return output;
}

// returns a JSON object of a platform event record
JsonObject& buildPlatformEvent(float temp, float humid, bool doorStatus) {

  StaticJsonBuffer<1024> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  
  root["deviceId__c"] = ESP.getChipId();
  root["door__c"] = doorStatus ? "Open" : "Closed";
  root["humidity__c"] = humid;
  root["temperature__c"] = temp;
  root["ts__c"] = getSFFormattedTime();
  
  return root;

}

//standard arduino setup method
void setup() {
  
  Serial.begin(115200);
  WiFi.begin(ssid,password);
  pinMode(doorPin, INPUT);
  
  Serial.println("Waiting for connection");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  timeClient.begin();
  
  Serial.println("WiFi Connected!");
  
  //once online, login to salesforce
  doLogin(sfUsername, sfPassword, sfToken, sfClientId, sfClientKey, sfLoginFingerprint);

}

//standard arduino loop method
void loop() {

  /*Uncomment if using actual DHT sensor & Door Switch
  if (isnan(dht.readTemperature())) { 
    //chill, sensor isn't ready yet
  }
  else {
      JsonObject& event = buildPlatformEvent(dht.readTemperature(), dht.readHumidity(), digitalRead(doorPin);
      bool insertSuccess = insertSObject("Smart_Fridge_Reading__e", event);
      if(insertSuccess) {
        Serial.println("Record Inserted Successfully!");
      }
      else {
          Serial.println("Record Insert Failed!");
      }
  } */

  /* Uncomment to test with random data (no sensors required!) */
  float temp = random(0,10);
  float humidity = random(0,100);
  bool doorStatus = (int)random(0,1);

  JsonObject& event = buildPlatformEvent(temp, humidity, doorStatus);

  bool insertSuccess = insertSObject("Smart_Fridge_Reading__e", event);
  if(insertSuccess) {
    Serial.println("Record Inserted Successfully!");
  }
  else {
      Serial.println("Record Insert Failed!");
  }
  
  //change this to increase/decrease time between creating readings (60000 = 1min)
  delay(30000);

}
