#include <M5StickC.h>
#include "display.h"

using namespace display;

void Display::showStart(String deviceID){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.print("ES PWV");

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(90, 10);
  M5.Lcd.print("32");

  //lock body
  M5.Lcd.drawRect(65, 35, 85, 40, RED );
  //lock shackle
  M5.Lcd.drawLine(75, 35, 75, 10, RED);
  M5.Lcd.drawLine(80, 35, 80, 10, RED);
  M5.Lcd.drawLine(140, 35, 140, 10, RED);
  M5.Lcd.drawLine(135, 35, 135, 10, RED);

  M5.Lcd.drawLine(75, 10, 85, 0, RED);
  M5.Lcd.drawLine(80, 10, 90, 0, RED);
  M5.Lcd.drawLine(140, 10, 130, 0, RED);
  M5.Lcd.drawLine(135, 10, 125, 0, RED);

  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.print(deviceID);
}

void Display::showPin(uint32_t pin){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.print("PIN:");
  M5.Lcd.setCursor(0, 30);
  M5.Lcd.setTextSize(3);
  M5.Lcd.print(pin);
}

void Display::updateBatteryPercentage(uint8_t percentage){
  uint8_t posX = 127;
  uint8_t posY = 1;
  
  //lightning logo
  M5.Lcd.drawLine(posX+5, posY+0, posX+3, posY+3, BLUE);
  M5.Lcd.drawLine(posX+3, posY+3, posX+5, posY+4, BLUE);
  M5.Lcd.drawLine(posX+5, posY+4, posX+2, posY+7, BLUE);
  M5.Lcd.drawLine(posX+2, posY+7, posX+2, posY+5, BLUE);
  M5.Lcd.drawLine(posX+2, posY+7, posX+4, posY+7, BLUE);

  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setCursor(posX+7, posY);
  M5.Lcd.setTextSize(1);
  M5.Lcd.print(100);
  M5.Lcd.print("%");
}
