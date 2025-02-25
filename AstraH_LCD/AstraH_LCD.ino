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
struct astraDoors { // создаём ярлык myStruct
  uint32_t bcfc; // bcfc задние закрыты перед закрыт
  uint32_t bofc; // bofc задние открыты перед закрыт
  uint32_t blfc; // blfc задняя левая перед закрыт
  uint32_t blfr; // blfr задняя левая передняя правая
  uint32_t bofr; // bofr задние открыты передняя правая
  uint32_t brfc; // brfc задняя правая перед закрыт
  uint32_t bcfo; // bcfo задние закрыты передние открыты
  uint32_t bcfl; // bcfl передняя левая
  uint32_t brfl; // brfl передняя левая задняя правая
  uint32_t brfo; // brfo задняя правая передние открыты
  uint32_t bcfr; // bcfr задние закрыты передняя правая
  uint32_t blfl; // blfl задняя левая передняя левая
  uint32_t blfo; // blfo задняя левая перед открыт
  uint32_t bofo; // bofo зад открыт перед открыт
  uint32_t brfr; // brfr задняя правая передняя правая
};

astraDoors ad;
String screen = "RPM";
int speed = 0;
int rpm = 0;
int coolant = 0;
float voltage = 0;
uint16_t distance = 0;
uint32_t odometr = 0;
String gear = "N";
uint32_t timer1;
uint32_t timer2;
bool screen_on = true;
byte door_rear = 0x00;
byte door_front = 0x00;
byte handbrake = 0x00;
byte washwater = 0x00;
float fuel = 0.0;
float wheel_l = 0.0;
float wheel_r = 0.0;

void setup(void) {
  ad.bcfc = 16;     // bcfc задние закрыты перед закрыт
  ad.bofc = 20200;  // bofc задние открыты перед закрыт
  ad.blfc = 40400;  // blfc задняя левая перед закрыт
  ad.blfr = 60600;  // blfr задняя левая передняя правая
  ad.bofr = 80800;  // bofr задние открыты передняя правая
  ad.brfc = 101000; // brfc задняя правая перед закрыт
  ad.bcfo = 121200; // bcfo задние закрыты передние открыты
  ad.bcfl = 141400; // bcfl передняя левая
  ad.brfl = 161600; // brfl передняя левая задняя правая
  ad.brfo = 181800; // brfo задняя правая передние открыты
  ad.bcfr = 202000; // bcfr задние закрыты передняя правая
  ad.blfl = 222200; // blfl задняя левая передняя левая
  ad.blfo = 242400; // blfo задняя левая перед открыт
  ad.bofo = 262600; // bofo зад открыт перед открыт
  ad.brfr = 282800; // brfr задняя правая передняя правая
  Serial.begin(115200);

  flash.begin();
  tft.init(240, 320);   // initialize a ST7789 chip, 240x240 pixels

  tft.fillScreen(BG_COLOR);
  digitalWrite(TFT_BL, HIGH);
  tft.drawRect(10, 40, 220, 180, WHITE);

  setCoolant(String(coolant));
  setVoltage(String(voltage, 1));
  setRPM(String(rpm));
  setGear(gear);
  setDistance(String(distance, 1));
  setOdometr(String(odometr));

  Serial.println("Init tft");
  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
  can.reset();
  // HS CAN Astra H
  can.setBitrate(CAN_33KBPS, MCP_8MHZ);
  can.setNormalMode();
}

void loop() {
  if (millis() - timer1 >= 5000) {
    timer1 = millis();               // сброс таймера

    // Создаем структуру can_frame для отправки
    struct can_frame frame;
    frame.can_id = 0x255;
    frame.can_dlc = 8;
    byte data[8] = {0x03, 0xAA, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00};
    memcpy(frame.data, data, sizeof(data));
    can.sendMessage(&frame);
  }
  if (millis() - timer2 >= 500) {
    timer2 = millis();               // сброс таймера
    setCoolant(String(coolant));
    setVoltage(String(voltage, 1));
    setGear(gear);
    setDistance(String(distance/10.0, 1));
    setOdometr(String(odometr));

    if (screen == "RPM") {
      setSpeed(String(speed));
      setRPM(String(rpm));
    }

    if (screen_on) {
      tft.sleepDisplay(false);
      digitalWrite(TFT_BL, HIGH);
    } else {
      tft.sleepDisplay(true);
      digitalWrite(TFT_BL, LOW);
    }
  }

  if (can.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id == 0x11A) {
      Serial.print(canMsg.can_id, HEX); // print ID
      Serial.print(" "); 
      Serial.print(canMsg.can_dlc, HEX); // print DLC
      Serial.print(" ");

      for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
        Serial.print(canMsg.data[i], HEX);
        if (i<canMsg.can_dlc-1) {
          Serial.print(":");
        }
      }
      Serial.println();
    }

    if (canMsg.can_id == 0x100) {
      Serial.println("Start CAN");
    }
    if (canMsg.can_id == 0x108) {
      speed = ((canMsg.data[4]<<8) | canMsg.data[5])/139;
      rpm = ((canMsg.data[1]<<8) | canMsg.data[2])/4; // 108 8 10:C:A4:0:0:0:0:0
    }
    if (canMsg.can_id == 0x110) {
      wheel_l = ((canMsg.data[1]<<8) | (canMsg.data[2])) * 1.5748;
      wheel_r = ((canMsg.data[3]<<8) | (canMsg.data[4])) * 1.5748;
      // Example: 00:B4:92:B4:14
      // Left: 0xB492 = 46226 * 1.5748 = 72796.7048 cm = 727.96 m
      // Right: 0xB414 = 46100 * 1.5748 = 72598.2800 cm = 725.98 m
    }
    if (canMsg.can_id == 0x145) {
      coolant = canMsg.data[3] - 40;
    }
    if (canMsg.can_id == 0x170) { // IPC
      screen_on = (canMsg.data[0] != 0x00);
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
      // 190 7 0:0:F9:78:C0:0:26
      // 255459
      odometr = (((uint32_t)canMsg.data[2]<<16) | (canMsg.data[3]<<8) | canMsg.data[4]) / 64;
    }
    if (canMsg.can_id == 0x230) { // Doors
      if ((door_rear != canMsg.data[1]) || (door_front != canMsg.data[2])) {
        door_rear = canMsg.data[1];
        door_front = canMsg.data[2];
        setDoors(door_rear, door_front);
      }
    }
    if (canMsg.can_id == 0x370) { // HandBrake
      handbrake = canMsg.data[1];
      washwater = canMsg.data[2];
    }
    if (canMsg.can_id == 0x375) { // Fuel Level
      fuel = 94 - (canMsg.data[1] / 2.0);
    }
    if (canMsg.can_id == 0x500) {
      voltage = (canMsg.data[1]+28)/10.0;
    }
    if (canMsg.can_id == 0x555) { // Distance traveled 350.4
      //255459
      //555 8 9:D:B0:3:E5:E3:0:0
      distance = ((canMsg.data[1] << 8) | canMsg.data[2]);
    }
  }
}

void setDoors(byte rear, byte front) {
  uint16_t _door = (rear<<8) | front;
  // b[l,r,o] - back (rear) // l - left open, r - right open, o - all back open
  // f[l,r,o] - front       // l - left open, r - right open, o - all front open
  if (_door == 0x0000) {
    if (screen == "DOORS") {
      screen = "RPM";
      tft.fillRect(11, 41, 218, 178, BG_COLOR);
    }
  } else if (screen == "RPM") {
    screen = "DOORS";
    tft.fillRect(11, 41, 218, 178, BG_COLOR);
  }
  if (_door == 0x1000) {
    setDoorOpen(ad.brfc);
  }
  if (_door == 0x4000) {
    setDoorOpen(ad.blfc);
  }
  if (_door == 0x5000) {
    setDoorOpen(ad.bofc);
  }
  if (_door == 0x0010) {
    setDoorOpen(ad.bcfr);
  }
  if (_door == 0x0040) {
    setDoorOpen(ad.bcfl);
  }
  if (_door == 0x0050) {
    setDoorOpen(ad.bcfo);
  }
  if (_door == 0x4010) {
    setDoorOpen(ad.blfr);
  }
  if (_door == 0x5010) {
    setDoorOpen(ad.bofr);
  }
  if (_door == 0x1040) {
    setDoorOpen(ad.brfl);
  }
  if (_door == 0x1050) {
    setDoorOpen(ad.brfo);
  }
  if (_door == 0x4040) {
    setDoorOpen(ad.blfl);
  }
  if (_door == 0x4050) {
    setDoorOpen(ad.blfo);
  }
  if (_door == 0x1010) {
    setDoorOpen(ad.brfr);
  }
  if (_door == 0x5050) {
    setDoorOpen(ad.bofo);
  }
}

void setDoorOpen(uint32_t addr) {
  drawPicture(addr);
}

void drawPicture(uint32_t address) {
  uint16_t x = 80;
  uint16_t y = 70;
  uint16_t itr = 0;
  for (uint16_t i=0; i<10080; i++) {
    if (i%80 == 0) {
      y++;
      x = 80;
    }
    tft.drawPixel(x, y, flash.readWord(address+i*2));
    x++;
  }
}

void setCoolant(String text) {
  tft.setCursor(10, 12);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  tft.println(addSpaceAfter(text + " " + char(0xF7) + "C", 5));
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

void setDistance(String text) {
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
  tft.setTextSize(4);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  drawCentreString(text, 95, 70);
  tft.setCursor(97, 102);
  tft.setTextSize(2);
  tft.println("km/h");
}

void setRPM(String text) {
  tft.setTextSize(4);
  tft.setTextColor(WHITE, BG_COLOR);
  tft.setTextWrap(false);
  drawCentreString(text, 95, 140);
  tft.setCursor(101, 176);
  tft.setTextSize(2);
  tft.println("RPM");
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