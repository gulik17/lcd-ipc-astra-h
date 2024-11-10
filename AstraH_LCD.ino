#include <SPI.h>
#include <Arduino_ST7789_Fast.h> // Hardware-specific library for ST7789
#include <mcp2515.h>
//#include "bitmaps.h"
#include<SPIMemory.h>

//#define TFT_SCLK  13   // for hardware SPI sclk pin (all of available pins)
//#define TFT_MOSI  11   // for hardware SPI data pin (all of available pins)
#define TFT_DC    9
#define TFT_CS    8    // only for displays with CS pin
#define TFT_BL    7
#define CAN_CS    6
#define TFT_RST   5
#define FLASH_CS  4

#define BG_COLOR  BLACK

Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST, TFT_CS); //for display without CS pin
MCP2515 can = MCP2515(CAN_CS); // CAN_CS 6
SPIFlash flash = SPIFlash(FLASH_CS);

struct can_frame canMsg;
String screen = "RPM";
uint8_t speed = 0;
int rpm = 0;
int coolant = 0;
float voltage = 0;
uint32_t mileage = 0;
uint32_t odometr = 0;
String gear = "N";
uint32_t myTimer1;
bool screen_on = true;

void setup(void) {
  Serial.begin(115200);

  flash.begin();
  tft.init(240, 320);   // initialize a ST7789 chip, 240x240 pixels

  tft.fillScreen(BG_COLOR);
  digitalWrite(TFT_BL, HIGH);
  tft.drawRect(10, 40, 220, 180, WHITE);

  setCoolant(String(coolant));
  setVoltage(String(voltage));
  //setRPM(String(rpm));
  setGear(gear);
  setMileage(String(mileage));
  setOdometr(String(odometr));

  //tft.drawImageF(20, 70, 80, 80, icon_01);
  Serial.println("Init tft");
  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
  can.reset();
  // HS CAN Astra H
  can.setBitrate(CAN_33KBPS, MCP_8MHZ);
  can.setNormalMode();

  setDoorOpen("close");
  delay(5000);
  setDoorOpen("b");
  delay(5000);
  setDoorOpen("bl");
  delay(5000);
  setDoorOpen("blfr");
  delay(5000);
  setDoorOpen("blr");
  delay(5000);
  setDoorOpen("br");
  delay(5000);
  setDoorOpen("f");
  delay(5000);
  setDoorOpen("fl");
  delay(5000);
  setDoorOpen("flbr");
  delay(5000);
  setDoorOpen("flr");
  delay(5000);
  setDoorOpen("fr");
  delay(5000);
  setDoorOpen("l");
  delay(5000);
  setDoorOpen("lfr");
  delay(5000);
  setDoorOpen("lr");
  delay(5000);
  setDoorOpen("r");
  delay(5000);
  setDoorOpen("close");

  //tft.drawImageF(15, 45, 60, 116, astra);
  //tft.drawImageF(34, 45, 24, 24, passenger2);
  //tft.drawImageF(20, 45, 16, 16, passenger);
  //tft.drawImageF(48, 45, 24, 24, sport);
  //tft.drawImageF(76, 45, 24, 24, babyseat);
}

void loop() {
  if (millis() - myTimer1 >= 1000) {   // таймер на 500 мс (2 раза в сек)
    myTimer1 = millis();              // сброс таймера
    struct can_frame frame;
    frame.can_id = 0x170;
    frame.can_dlc = 4;
    frame.data[0] = 0x54;
    frame.data[1] = 0xDA;
    frame.data[2] = 0x82;
    frame.data[3] = 0x00;
    can.sendMessage(&frame);

    setCoolant(String(coolant));
    setVoltage(String(voltage));
    //setRPM(String(rpm));
    setGear(gear);
    setMileage(String(mileage));
    setOdometr(String(odometr));

    if (screen_on) {
      tft.sleepDisplay(false);
      digitalWrite(TFT_BL, HIGH);
    } else {
      tft.sleepDisplay(true);
      digitalWrite(TFT_BL, LOW);
    }
  }

  if (can.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    //Serial.print(canMsg.can_id, HEX); // print ID
    //Serial.print(" "); 
    //Serial.print(canMsg.can_dlc, HEX); // print DLC
    //Serial.print(" ");

    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i], HEX);
      if (i<canMsg.can_dlc-1) {
        Serial.print(":");
      }
    }
    //Serial.println();

    if (canMsg.can_id == 0x100) {
      Serial.println("Start CAN");
    }
    if (canMsg.can_id == 0x108) {
      if ((screen != "SPEED") && (screen != "RPM")) {
        return;
      }
      if (speed < 5) {
        rpm = (canMsg.data[1]<<6) + (canMsg.data[2]>>2);
      } else {
        speed = (canMsg.data[4]<<1) + (canMsg.data[5]>>7);
      }
    }
    if (canMsg.can_id == 0x110) {
      
    }
    if (canMsg.can_id == 0x145) {
      coolant = canMsg.data[3] - 40;
    }
    if (canMsg.can_id == 0x170) { // IPC
      screen_on = (canMsg.data[0] == 0x54);
    }
    if (canMsg.can_id == 0x11A) { // Передача
      if (canMsg.data[2] == 0x88) {
        gear = "R";
      }
      if (canMsg.data[2] == 0x89) {
        gear = "N";
      }
      if (canMsg.data[2] == 0x8B) {
        gear = "A1";
      }
      if (canMsg.data[2] == 0x8F) {
        gear = "M1";
      }
    }
    if (canMsg.can_id == 0x190) { // Пробег
      //uint32_t res1 = ((canMsg.data[6]<<8) | canMsg.data[7]) / 64;
      String s1 = String(canMsg.data[6], HEX) + String(canMsg.data[7], HEX);
      mileage = (uint32_t) strtol(&s1[0], NULL, 16) / 64;

      //uint32_t res2 = ((canMsg.data[2]<<24) | (canMsg.data[3]<<16) | (canMsg.data[4]<<8) | canMsg.data[5]) / 64;
      String s2 = String(canMsg.data[2], HEX) + String(canMsg.data[3], HEX) + String(canMsg.data[4], HEX) + String(canMsg.data[5], HEX);
      odometr = (uint32_t) strtol(&s2[0], NULL, 16) / 64;
    }
    /*
    if (canMsg.can_id == 0x230) { // Doors
      if ((screen == "SPEED") || (screen == "RPM")) {
        if (canMsg.data[1] != 0x00) { // задние
          rearDoor(canMsg.data[1]);
        }
        if (canMsg.data[2] != 0x00) { // передние
          frontDoor(canMsg.data[1]);
        }
      }
      if ((screen == "WARNING_1") || (screen == "WARNING_2") || (screen == "WARNING_3") || (screen == "WARNING_4") || (screen == "WARNING_5") || (screen == "WARNING_6")) {
        if ((canMsg.data[1] == 0x00) && (canMsg.data[2] == 0x00)) { // двери закрыты
          if (speed < 5) {
            screen = "RPM";
          } else {
            screen = "SPEED";
          }
        }
      }
    }*/
    /*
    if (canMsg.can_id == 0x370) { // HandBrake
      if ((canMsg.data[1]) & 0x01) {
        tft.drawImageF(104, 45, 24, 24, handbrake);
      } else {
        tft.fillRect(104, 45, 24, 24, BG_COLOR);
      }
    }*/
    if (canMsg.can_id == 0x500) {
      voltage = (canMsg.data[1]+28)/10;
    }
    //Serial.println();
  }
}

void rearDoor(byte data) {
  if (data == 0x10) {
    Serial.println("Открыта правая дверь");
    setWarning(5);
  }
  if (data == 0x40) {
    Serial.println("Открыта левая дверь");
    setWarning(4);
  }
  if (data == 0x50) {
    Serial.println("Открыты обе двери");
    setWarning(6);
  }
}

void frontDoor(byte data) {
  if (data == 0x10) {
    Serial.println("Открыта правая дверь");
    setWarning(2);
  }
  if (data == 0x40) {
    Serial.println("Открыта левая дверь");
    setWarning(1);
  }
  if (data == 0x50) {
    Serial.println("Открыты обе двери");
    setWarning(3);
  }
}

void setDoorOpen(String text) {
  uint16_t x = 80;
  uint16_t y = 80;
  uint16_t itr = 0;
  uint16_t _buffer[80] = {};
  if (text == "close") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        x = 80;
        itr = 0;
        tft.drawImage(x, y, 80, 1, _buffer);
        y = y + 1;
      }
      //tft.drawPixel(x, y, flash.readWord(16+i*2));
      _buffer[itr] = flash.readWord(16+i*2);
      x = x + 1;
      itr = itr + 1;
    }
  }
  if (text == "b") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(20200+i*2));
      x = x + 1;
    }
  }
  if (text == "bl") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(40400+i*2));
      x = x + 1;
    }
  }
  if (text == "blfr") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(60600+i*2));
      x = x + 1;
    }
  }
  if (text == "blr") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(80800+i*2));
      x = x + 1;
    }
  }
  if (text == "br") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(101000+i*2));
      x = x + 1;
    }
  }
  if (text == "f") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(121200+i*2));
      x = x + 1;
    }
  }
  if (text == "fl") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(141400+i*2));
      x = x + 1;
    }
  }
  if (text == "flbr") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(161600+i*2));
      x = x + 1;
    }
  }
  if (text == "flr") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(181800+i*2));
      x = x + 1;
    }
  }
  if (text == "fr") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(202000+i*2));
      x = x + 1;
    }
  }
  if (text == "l") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(222200+i*2));
      x = x + 1;
    }
  }
  if (text == "lfr") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(242400+i*2));
      x = x + 1;
    }
  }
  if (text == "lr") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(262600+i*2));
      x = x + 1;
    }
  }
  if (text == "r") {
    for (uint16_t i=0; i<10080; i++) {
      if (i%80 == 0) {
        y = y + 1;
        x = 80;
      }
      tft.drawPixel(x, y, flash.readWord(282800+i*2));
      x = x + 1;
    }
  }

}

void setCoolant(String text) {
  tft.setCursor(10, 12);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  tft.println(addSpaceAfter(text + " C", 5));
}

void setVoltage(String text) {
  tft.setCursor(148, 12);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  tft.println(addSpaceBefore(text + " V", 7));
}

void setGear(String text) {
  tft.setCursor(10, 245);
  tft.setTextSize(8);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  tft.println(addSpaceAfter(text, 2));
}

void setMileage(String text) {
  tft.setCursor(107, 245);
  tft.setTextSize(3);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  tft.println(addSpaceBefore(text, 7));
}

void setOdometr(String text) {
  tft.setCursor(107, 280);
  tft.setTextSize(3);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  tft.println(addSpaceBefore(text, 7));
}

void setSpeed(String text) {
  if (screen != "SPEED") {
    tft.fillRect(11, 41, 218, 178, BG_COLOR);
    screen = "SPEED";
  }
  tft.setTextSize(5);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  drawCentreString(text, 90, 90);
  tft.setCursor(92, 132);
  tft.setTextSize(2);
  tft.cp437(true);
  tft.println(utf8rus("км/ч"));
}

void setRPM(String text) {
  if (screen != "RPM") {
    tft.fillRect(11, 41, 218, 178, BG_COLOR);
    screen = "RPM";
  }
  tft.setTextSize(5);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  drawCentreString(text, 90, 90);
  tft.setCursor(98, 132);
  tft.setTextSize(2);
  tft.println("RPM");
}

void setWarning(int warn) {
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  tft.cp437(true);
  // case
  switch (warn) {
    case 1:
      if (screen != "WARNING_1") {
        tft.fillRect(11, 41, 218, 178, BG_COLOR);
        screen = "WARNING_1";
      }
      tft.setCursor(25, 110);
      tft.println(utf8rus("Открыта передняя"));
      tft.setCursor(56, 130);
      tft.println(utf8rus("левая дверь"));
      break;
    case 2:
      if (screen != "WARNING_2") {
        tft.fillRect(11, 41, 218, 178, BG_COLOR);
        screen = "WARNING_2";
      }
      tft.setCursor(25, 110);
      tft.println(utf8rus("Открыта передняя"));
      tft.setCursor(52, 130);
      tft.println(utf8rus("правая дверь"));
      break;
    case 3:
      if (screen != "WARNING_3") {
        tft.fillRect(11, 41, 218, 178, BG_COLOR);
        screen = "WARNING_3";
      }
      tft.setCursor(25, 110);
      tft.println(utf8rus("Открыты передние"));
      tft.setCursor(90, 130);
      tft.println(utf8rus("двери"));
      break;
    case 4:
      if (screen != "WARNING_4") {
        tft.fillRect(11, 41, 218, 178, BG_COLOR);
        screen = "WARNING_4";
      }
      tft.setCursor(34, 110);
      tft.println(utf8rus("Открыта задняя"));
      tft.setCursor(54, 130);
      tft.println(utf8rus("левая дверь"));
      break;
    case 5:
      if (screen != "WARNING_5") {
        tft.fillRect(11, 41, 218, 178, BG_COLOR);
        screen = "WARNING_5";
      }
      tft.setCursor(34, 110);
      tft.println(utf8rus("Открыта задняя"));
      tft.setCursor(50, 130);
      tft.println(utf8rus("правая дверь"));
      break;
    case 6:
      if (screen != "WARNING_6") {
        tft.fillRect(11, 41, 218, 178, BG_COLOR);
        screen = "WARNING_6";
      }
      tft.setCursor(34, 110);
      tft.println(utf8rus("Открыты задние"));
      tft.setCursor(90, 130);
      tft.println(utf8rus("двери"));
      break;
  }
}

void drawCentreString(String buf, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
    tft.setCursor(x - w / 2, y);
    tft.println(" "+String(buf)+" ");
}

String addSpaceBefore(String text, int count) {
  int space = count - text.length();
  String str = "";
  for (int i = 0; i < space; i++) {
    str += " ";
  }
  return str + text;
}

String addSpaceAfter(String text, int count) {
  int space = count - text.length();
  String str = "";
  for (int i = 0; i < space; i++) {
    str += " ";
  }
  return text + str;
}

String utf8rus(String source)
{
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
	return target;
}