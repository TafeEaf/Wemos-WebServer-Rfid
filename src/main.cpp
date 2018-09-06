#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include "SSD1306.h"    // Oled library
#include "MFRC522.h"    // Rfid library

// Initialize the webserver
#define ssid      "WemosWifi"     // WiFi SSID
#define password  "12345678"      // WiFi password
ESP8266WebServer server ( 80 );   // create Objects

// Initialize the LED output and associated webpage colors
#define GreenLed          D6          // Led Pin
#define RedLed            D7          // Led Pin
#define YellowLed         D5          // Led Pin

// Initialize the DHT11 Temperature sensor
#define DHTPIN            D4         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT11      // DHT 11
float humidity,temperature;          // Storte humidity and temperature
DHT dht(DHTPIN, DHTTYPE);

// Initialize the OLED display using i2c
#define SCL_PIN	          D1         // SCL-PIN
#define SDA_PIN	          D2         // SDA-PIN
#define line1             0          // 0  to 15 vertical pixel
#define line2             16         // 16 to 31 vertical pixel
#define line3             32         // 32 to 47 vertical pixel
#define line4             48         // 48 to 63 vertical pixel
#define pos1              0          // 0  to 31 horizontal pixel
#define pos2              32         // 32 to 63 horizontal pixel
#define pos3              64         // 64 to 95 horizontal pixel
#define pos4              96         // 96 to 127 horizontal pixel
SSD1306  display(0x3c, SDA_PIN, SCL_PIN);

/* wiring the MFRC522 to ESP8266 (ESP-12)
RST     = A0
SDA(SS) = D0    // Slave Select CONNEcT TO SDA ON MFRC522
MOSI    = D7
MISO    = D6
SCK     = D5
GND     = GND
3.3V    = 3.3V
*/
#define RST_PIN	A0  // RST-PIN
#define SS_PIN	D0  // SS-PIN
#define Locked	-1
MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create MFRC522 instance

// List of matching tag IDs
char* allowedTags[] = {
  "61D2F052",           // Tag 0
  "6470621A",           // Tag 1
};
/*
"6ECCBE23",           // Tag 2
"A5F3FFB0",           // Tag 3
*/

// Check the number of tags defined
int numberOfTags = sizeof(allowedTags)/sizeof(allowedTags[0]);
char tagValue[9]; // 8 characters of HEX value + end string character
int door_status;
void dump_byte_array(byte *buffer, byte bufferSize);
void findTag(void);


String getPage(){
  String checkedOn, checkedOff;
  String GreenLedStatus, RedLedStatus, YellowLedStatus;
  //                                           Automatic refresh every 30 seconds
  String page = "<html lang=en-EN><head><meta http-equiv='refresh' content='30'/>";
  page += "<title>ESP8266</title>";
  page += "<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>";
  page += "</head><body><h1>Wemos Wifi Basic Server</h1>";
  page += "<h3>Remote GPIO configuration</h3>";
  // Send temperature and humidity to Web Page
  page += "<p>Temperature ";
  page += temperature; page += " </p>";
  page += "<p>Humidity ";
  page += humidity; page += " </p>";
  page += "<p>Door status ";
  if (door_status == Locked)
    page += "Locked </p>";
  else
    page += "Unlocked </p>";

  // Form to update the Led state
  page += "<form action='/' method='POST'>";

  // handle Green LED
  if (digitalRead(GreenLed)==0){
    GreenLedStatus = "<font color=\"#7fbf7f\"><b>ON</b></font>";
    checkedOn = "checked"; checkedOff = "";
  }
  else{
    GreenLedStatus = "<font color=\"#006600\"><b>OFF</b></font>";
    checkedOn = ""; checkedOff = "checked";
  }
  page += "<ul><li>Green Led Current status: ";
  page += GreenLedStatus;
  page += "<INPUT type='radio' name='LED1' value='0'"; page += checkedOn;  page += ">ON";
  page += "<INPUT type='radio' name='LED1' value='1'"; page += checkedOff;  page += ">OFF</li></ul>";

  // handle Red LED
  if (digitalRead(RedLed)==0){
     RedLedStatus = "<font color=\"#ff7f7f\"><b>ON</b></font>";
     checkedOn = "checked"; checkedOff = "";
  }
  else{
     RedLedStatus = "<font color=\"#cc0000\"><b>OFF</b></font>";
     checkedOn = ""; checkedOff = "checked";
  }
  page += "<ul><li>Red   Led Current status: ";
  page += RedLedStatus;
  page += "<INPUT type='radio' name='LED2' value='0'"; page += checkedOn;  page += ">ON";
  page += "<INPUT type='radio' name='LED2' value='1'"; page += checkedOff;  page += ">OFF</li></ul>";

  // handle Yellow LED
  if (digitalRead(YellowLed)==0){
    YellowLedStatus = "<font color=\"#ffff3b\"><b>ON</b></font>";
    checkedOn = "checked"; checkedOff = "";
  }
  else{
    YellowLedStatus = "<font color=\"#9d9d00\"><b>OFF</b></font>";
    checkedOn = ""; checkedOff = "checked";
  }
  page += "<ul><li>Yellow Led Current status: ";
  page += YellowLedStatus;
  page += "<INPUT type='radio' name='LED3' value='0'"; page += checkedOn;  page += ">ON";
  page += "<INPUT type='radio' name='LED3' value='1'"; page += checkedOff;  page += ">OFF</li></ul>";

  // send form
  page += "<INPUT type='submit' value='Send'>";
  page += "</body></html>";

  return page;
}


void update_output (){
  // Set Green LED
  if ( server.arg("LED1") == "0" )
    digitalWrite(GreenLed, 0);
  else
    digitalWrite(GreenLed, 1);
  // Set Red LED
  if ( server.arg("LED2") == "0" )
    digitalWrite(RedLed, 0);
  else
    digitalWrite(RedLed, 1);
  // Set Yellow LED
  if ( server.arg("LED3") == "0" )
    digitalWrite(YellowLed, 0);
  else
    digitalWrite(YellowLed, 1);
}

void update_serial (){
  // Green LED status
  Serial.print("Set Green Led: ");
  Serial.println(!digitalRead(GreenLed));
  // Red Led LED status
  Serial.print("Set Red Led: ");
  Serial.println(!digitalRead(RedLed));
  // Yellow LED status
  Serial.print("Set Yellow Led: ");
  Serial.println(!digitalRead(YellowLed));
  // Send humidity
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %\t");
  // Send temperature
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C ");
  // Send door status
  if (door_status == Locked)
    Serial.println("Door Status: Locked");
  else
    Serial.println("Door Status: Unlocked");
}

void update_oled (){    // Send temperature and humidity to Oled Screen
  display.clear();      // Clear the Oled display associated memory
  display.display();    // Print an empty screen

  //                 x     y    String
  display.drawString(pos1 , line1, "Temp:");
  display.drawString(pos4 , line1, String(temperature));
  display.drawString(pos1 , line2, "Humidity:");
  display.drawString(pos4 , line2, String(humidity));

  // Green LED status
  display.drawString(pos1 , line3, "G:");
  if (digitalRead(GreenLed)==0)
    display.drawString(pos2 , line3, "On");
  else
    display.drawString(pos2 , line3, "Off");

  // Red LED status
  display.drawString(pos3 , line3, "R:");
  if (digitalRead(RedLed)==0)
    display.drawString(pos4 , line3, "On");
  else
    display.drawString(pos4 , line3, "Off");

  // Yellow LED status
  display.drawString(pos1, line4, "Y:");
  if (digitalRead(YellowLed)==0)
    display.drawString(pos2 , line4, "On");
  else
    display.drawString(pos2 , line4, "Off");

  // Show door status
  if (door_status == Locked)
    display.drawString(pos3, line4,"Locked");
  else
    display.drawString(pos3, line4,"Unlocked");

  display.display();   //  Visualise the new constructed Oled screen
}


void handleRoot(){
  // read the arguments sent through the web page
  //               Green Led                  Red Led                  Blue Led
  if ( server.hasArg("LED1") || server.hasArg("LED2") || server.hasArg("LED3")) {
    update_output();
  }
  // read the temperature and humidity from the sensor
  float humidity_previous_reading = humidity;
  float temperature_previous_reading = temperature;
  humidity = dht.readHumidity();                  // Read humidity as percentual
  temperature = dht.readTemperature();            // Read temperature as Celsius (the default)
  if (isnan(humidity) || isnan(temperature)) {    // If there is an error reading from the sensor
    humidity=humidity_previous_reading;           // Use previous reading
    temperature=temperature_previous_reading;     // Use previous reading
  }
  // send information to serial monitor and Oled screen and recompile web page
  update_serial();                                // Send info to Serial Monitor
  update_oled();                                  // Send info to Oled Display
  server.send ( 200, "text/html", getPage() );    // Send the recompiled Web Page
  door_status=Locked;
}

void setup() {
  pinMode(GreenLed, OUTPUT);            // sets the digital pin D6 as output
  digitalWrite(GreenLed, 1);            // sets the digital pin D6 high

  pinMode(RedLed, OUTPUT);              // sets the digital pin D7 as output
  digitalWrite(RedLed, 1);              // sets the digital pin D7 high

  pinMode(YellowLed, OUTPUT);           // sets the digital pin D5 as output
  digitalWrite(YellowLed, 1);           // sets the digital pin D5 high

  Serial.begin ( 115200 );              // init serial communication

  WiFi.begin ( ssid, password );
  while ( WiFi.status() != WL_CONNECTED ) {     // Wait for connection
    delay ( 500 ); Serial.print ( "." );
  }

  // WiFi connection is OK
  Serial.println ( "" );
  Serial.print ( "Connected to " ); Serial.println ( ssid );
  Serial.print ( "IP address: " ); Serial.println ( WiFi.localIP() );

  // Link to the function that manage launch page
  server.on ( "/", handleRoot );
  server.begin();
  Serial.println ( "HTTP server started" );

  // Initialising the Oled screen
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.clear();      // Clear the Oled display associated memory
  display.display();    // Print an empty screen
  display.drawString(pos1 , line1, ssid);
  display.drawString(pos1 , line2, (char*) WiFi.localIP().toString().c_str());
  display.display();    // Print an empty screen
  Serial.println("Oled Screen initialized");

  // Initialising the temperatuse sensor
  dht.begin();
  humidity=0;
  temperature=0;
  Serial.println("Temperature sensor initialized");

  // Initialising the rfid reader
  SPI.begin();	         // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522
  door_status=-1;
  // Print to serial the ready status
  Serial.println(F("Ready!"));
  Serial.println(F("======================================================"));
  Serial.println(F("Scan for Card and print UID:"));
}

void loop() {
  server.handleClient();
  delay(500);
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  /**
   *  This section is only executed when a RFID card is readed
   */

  // Show some details of current read RFID tag
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  // Find tag in the access list
  findTag();
  // Update web page
  handleRoot();
}

/**
 * Helper routine to dump a byte array as hex values to Serial
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
  String content= "";
  Serial.print(F("Card UID:"));
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
    content.concat(String(buffer[i] < 0x10 ? "0" : ""));
    content.concat(String(buffer[i], HEX));
  }
  Serial.println(F(""));
  content.toUpperCase();
  content.toCharArray(tagValue, 9);
}

/**
 * Search for a specific tag in the database
 */
void findTag(void) {
  for (int thisCard = 0; thisCard < numberOfTags; thisCard++) {
    // Check if the tag value matches this row in the tag database
    if(strcmp(tagValue, allowedTags[thisCard]) == 0){
      door_status=thisCard;
    }
  }
  // If we don't find the tag return a tag ID of -1 to show there was no match
  door_status = Locked;
}
