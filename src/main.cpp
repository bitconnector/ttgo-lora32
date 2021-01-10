#include <Arduino.h>

#define LED 2

#include <SPI.h>
#include <LoRa.h>

// LoRa pins
#define LORA_MISO 19
#define LORA_CS 18
#define LORA_MOSI 27
#define LORA_SCK 5
#define LORA_RST 14
#define LORA_IRQ 26

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define OLED PIN
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);

void setup()
{

  pinMode(LED, OUTPUT);
  //initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("starting up...");

  //setup display
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  Serial.println("Display initialized");
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Welcome");

  display.setTextSize(1);
  display.println("ttgo-lora32-v1");
  display.display();

  //setup LoRa transceiver module
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  //replace the LoRa.begin(---E-) argument with your location's frequency
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(866E6))
  {
    Serial.println(".");
    delay(500);
  }
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void blinkLED(unsigned long duration);
void updateLED();

void displayTimestamp();
void parsePacket();
bool Button = 1;
unsigned long ButtonTime;

unsigned long Time1, Timed;
bool role = 0;
int param = 0;

int counter;

String RecivedData;
unsigned long RecivedDataTime;

void loop()
{

  if (Button != digitalRead(0))
  {
    //Serial.print("Button ");
    //Serial.println(Button);
    Button = digitalRead(0);
    if (Button)
    {
      if ((ButtonTime + 1000) < millis())
      { //pressed for 1s
        role = !role;
        param = 0;
      }
      else
      {
        param++;
        if (param > 9)
          param = 0;
      }
    }
    ButtonTime = millis();
  }

  if (millis() - Time1 > 100)
  {
    Timed = millis();
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Ro: ");
    if (role)
    {
      display.print("send ");
      display.print(param);
      display.print("   ");
      displayTimestamp();
      display.println();

      display.print("send: ");
      display.print(counter);
      display.println();

      display.print("last reply: ");
      float lastSeen = (millis() - RecivedDataTime);
      display.print(lastSeen / 1000);
      display.println("s");
      display.print("Data: ");
      display.print(LoRa.packetRssi());
      display.print(" ->");
      display.println(pow(10,(65-LoRa.packetRssi())/(10*3))/1000,0);
      display.print(RecivedData);
      display.drawRect(0, 60, int(125+LoRa.packetRssi()),4,SSD1306_WHITE);
    }

    else
    {
      display.print("empf ");
      display.print(param);
      display.print("   ");
      displayTimestamp();
      display.println();
      display.print("last: ");
      display.print(LoRa.packetRssi());
      display.print(" @");
      float lastSeen = (millis() - RecivedDataTime);
      display.print(lastSeen / 1000);
      display.println("s");
      display.println("Data:");
      display.print(RecivedData);
    }

    display.display();
  }

  if (role)
  {
    if (millis() - Time1 > 200 * (param + 1))
    {
      blinkLED(100);
      Time1 = millis();
      LoRa.beginPacket();
      LoRa.print(counter);
      LoRa.endPacket();
      counter++;
    }
    // try to parse packet
    parsePacket();
  }
  else
  {
    int packetSize = LoRa.parsePacket();
    if (packetSize != 0)
    {
      blinkLED(200);
      RecivedDataTime = millis();
      Serial.print("Received packet '");
      while (LoRa.available())
      {
        RecivedData = LoRa.readString();
        Serial.print(RecivedData);
      }
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
      if (param == 1)
      {
        LoRa.beginPacket();
        LoRa.print(counter);
        LoRa.endPacket();
        Serial.print("Replied: ");
        Serial.print(counter);
        Serial.println();
        counter++;
      }
    }
  }
  updateLED();
}

void parsePacket()
{
  int packetSize = LoRa.parsePacket();
  if (packetSize == 0)
    return;
  RecivedDataTime = millis();
  Serial.print("Received packet '");
  while (LoRa.available())
  {
    RecivedData = LoRa.readString();
    Serial.print(RecivedData);
  }
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
}

void displayTimestamp()
{
  uint32_t seconds = millis() / 1000;
  uint32_t minutes = seconds / 60;
  seconds %= 60;
  uint32_t hours = minutes / 60;
  minutes %= 60;
  if (hours < 10)
    display.print("0");
  display.print(hours);
  display.print(":");
  if (minutes < 10)
    display.print("0");
  display.print(minutes);
  display.print(":");
  if (seconds < 10)
    display.print("0");
  display.print(seconds);
}

unsigned long LEDIntervall;

void blinkLED(unsigned long duration)
{
  digitalWrite(LED, HIGH);
  LEDIntervall = millis() + duration;
}

void updateLED()
{
  if (LEDIntervall < millis())
    digitalWrite(LED, LOW);
}