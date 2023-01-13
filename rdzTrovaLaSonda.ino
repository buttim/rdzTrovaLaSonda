#include <SSD1306.h>
#include <SH1106.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <BluetoothSerial.h>
#include <math.h>
#include "Sonde.h"
#include "DecoderBase.h"
#include "rdzTrovaLaSonda.h"
#include "SX1278FSK.h"
#include "RS41.h"
#include "M10M20.h"
#include "DFM.h"
#include "MySondyGOProto.h"

const char *version_name = "rdzTTGOsonde";
const char *version_id = "devel20221217";

OLEDDisplay *display;
SemaphoreHandle_t sem;
SemaphoreHandle_t semSX1278;
TimerHandle_t tBuzzOff;
DecoderBase *decoder=&rs41;
Proto proto;
MyProtoUser myProtoUser;
BluetoothSerial ESP_BT;

const int bt_width=13,bt_height=25;
static uint8_t bt_bits[] = {
  0x00, 0x00, 0x40, 0x00, 0xC0, 0x00, 0xC0, 0x01, 0xC0, 0x03, 0xC0, 0x07, 
  0xC2, 0x0E, 0xC7, 0x1C, 0xCE, 0x0E, 0xDC, 0x07, 0xF8, 0x03, 0xF0, 0x01, 
  0xE0, 0x00, 0xF0, 0x01, 0xF8, 0x03, 0xDC, 0x07, 0xCE, 0x0E, 0xC7, 0x1C, 
  0xC2, 0x0E, 0xC0, 0x07, 0xC0, 0x03, 0xC0, 0x01, 0xC0, 0x00, 0x40, 0x00, 
  0x00, 0x00
};

DecoderBase *getDecoder(unsigned sondeType) {
  DecoderBase *decoders[]={&rs41,&m10m20,&m10m20,&rs41,&dfm};
  sondeType=constrain(sondeType,1,sizeof decoders/sizeof(*decoders));
  return decoders[sondeType-1];
}

void MyProtoUser::mute(bool on) {
  if (!on) bip(50,8000);
}

void MyProtoUser::type(int sondeType) {

}

void MyProtoUser::freq(float f) {

}

bool initBluetooth() {
  if (!btStart()) {
    Serial.println("Failed to initialize controller");
    return false;
  }
 
  if (esp_bluedroid_init() != ESP_OK) {
    Serial.println("Failed to initialize bluedroid");
    return false;
  }
 
  if (esp_bluedroid_enable() != ESP_OK) {
    Serial.println("Failed to enable bluedroid");
    return false;
  }
  return true;
}

void bip(int duration,int freq) {
  analogWriteFrequency(freq);
  digitalWrite(proto.ledPin,HIGH);
  analogWrite(proto.buzzPin,128);
  xTimerChangePeriod(tBuzzOff,duration,0);
  xTimerStart(tBuzzOff,0);
}

void initDisplay() {
  if (proto.lcd==0)
    display=new SSD1306(0x3c,proto.oledSDA,proto.oledSCL);
  else
    display=new SH1106(0x3c,proto.oledSDA,proto.oledSCL);
  display->init();
  display->setFont(ArialMT_Plain_16);
  display->drawString(0,34,"rdzTrovaLaSonda");
  display->flipScreenVertically();
  display->display();
}

void drawBattery(int level) {
#define BATT_W 20
#define BATT_H 39
#define BATT_X 106
#define BATT_Y 24
#define BATT_CORNER_RADIUS 5
#define BATT_PLUS_W 10
#define BATT_PLUS_H 4

  display->fillRect(BATT_X+BATT_W/2-BATT_PLUS_W/2,BATT_Y,BATT_PLUS_W,BATT_PLUS_H-1);
  display->drawLine(BATT_X+BATT_CORNER_RADIUS,BATT_Y+BATT_PLUS_H,                   BATT_X+BATT_W-BATT_CORNER_RADIUS,BATT_Y+BATT_PLUS_H);
  display->drawLine(BATT_X+BATT_CORNER_RADIUS,BATT_Y+BATT_H,                        BATT_X+BATT_W-BATT_CORNER_RADIUS,BATT_Y+BATT_H);
  display->drawLine(BATT_X,                   BATT_Y+BATT_PLUS_H+BATT_CORNER_RADIUS,BATT_X,                          BATT_Y+BATT_H-BATT_CORNER_RADIUS);
  display->drawLine(BATT_X+BATT_W,            BATT_Y+BATT_PLUS_H+BATT_CORNER_RADIUS,BATT_X+BATT_W,                   BATT_Y+BATT_H-BATT_CORNER_RADIUS);
  
  display->drawCircleQuads(BATT_X+BATT_CORNER_RADIUS, BATT_Y+BATT_PLUS_H+BATT_CORNER_RADIUS, BATT_CORNER_RADIUS, 0b10);
  display->drawCircleQuads(BATT_X+BATT_W-BATT_CORNER_RADIUS, BATT_Y+BATT_PLUS_H+BATT_CORNER_RADIUS, BATT_CORNER_RADIUS, 0b1);
  display->drawCircleQuads(BATT_X+BATT_CORNER_RADIUS, BATT_Y+BATT_H-BATT_CORNER_RADIUS, BATT_CORNER_RADIUS, 0b100);
  display->drawCircleQuads(BATT_X+BATT_W-BATT_CORNER_RADIUS, BATT_Y+BATT_H-BATT_CORNER_RADIUS, BATT_CORNER_RADIUS, 0b1000);
  if (level==0) {
    display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display->setFont(ArialMT_Plain_24);
    display->drawString(BATT_X+BATT_W/2,BATT_Y+BATT_PLUS_H+(BATT_H-BATT_PLUS_H)/2,"?");
  }
  else
  {
    int h=BATT_H-BATT_PLUS_H-4;
    if (level>100) level=100;
    display->fillRect(BATT_X+2,BATT_Y+BATT_PLUS_H+2+h*(100-level)/100,BATT_W-3,h*level/100);
  }
}

void updateDisplay(float vBatt) {
  char s[1+3+1+5+1];  //sign,integer,dot,decimals,null
  float f;

  display->clear();
  if (ESP_BT.connected()) {
#define BT_X 90
#define BT_Y 34

    display->drawXbm(BT_X,BT_Y,bt_width,bt_height,bt_bits);
  }
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,0,nameFromSondeType(proto.sondeType));
  display->drawString(1,0,nameFromSondeType(proto.sondeType));
  snprintf(s,sizeof s,"%.3f",proto.freq);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(127,0,s);
  display->drawString(126,0,s);
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,18,sonde.si()->d.ser);
  display->drawString(1,18,sonde.si()->d.ser);
  f=sonde.si()->d.lat;
  if (isnan(f)) f=0;
  snprintf(s,sizeof s,"%+010.5f",f);
  display->drawString(0,33,s);
  f=sonde.si()->d.lon;
  if (isnan(f)) f=0;
  snprintf(s,sizeof s,"%+010.5f",f);
  display->drawString(0,49,s);
  drawBattery(map(vBatt,proto.battMin,proto.battMax,0,100));
  display->display();
}

void setup() {
  Serial.begin(115200);
  initBluetooth();
  const uint8_t* add = esp_bt_dev_get_address();
  char s[25];
  snprintf(s,sizeof s,"TrovaLaSonda%02X%02X%02X%02X%02X%02X",add[0],add[1],add[2],add[3],add[4],add[5]);
  ESP_BT.begin(s);
  ESP_BT.setTimeout(0);
  ESP_BT.onData([](const uint8_t *buf,int size) {
    proto.onData(buf,size);
  });
	proto.init(&myProtoUser,&ESP_BT);

  vTaskDelay(1000);

  pinMode(proto.battPin,INPUT);
  pinMode(proto.ledPin,OUTPUT);
  pinMode(proto.buzzPin,OUTPUT);

  sem=xSemaphoreCreateBinary();
  semSX1278=xSemaphoreCreateBinary();
  xSemaphoreGive(semSX1278);
  tBuzzOff=xTimerCreate("buzzer off",pdMS_TO_TICKS(100),pdFALSE,NULL,[](TimerHandle_t){
    analogWrite(proto.buzzPin,0);
    digitalWrite(proto.ledPin,LOW);
  });

  bip(250,2000);

  initDisplay();
  
  sonde.defaultConfig();

  sx1278.setup(semSX1278);
  decoder=getDecoder(proto.sondeType);
  decoder->setup(proto.freq*1E6);
}

void loop() {
  SondeInfo *si=sonde.si();
  int res=decoder->receive();
  //Serial.printf("RX:%d,lat:%f,lon:%f,ser:%s\n",res,si->d.lat,si->d.lon,si->d.ser);
  if (res==0 && !proto.mute) bip(50,8000);
  float vBatt=analogRead(proto.battPin)/4096.0*2*3.3*1100;
  updateDisplay(vBatt);
  proto.loop(vBatt,res==0,!isnan(si->d.lat) && !si->d.lat==0,si->d.ser,si->d.lat,si->d.lon,si->d.alt,si->d.hs,si->rssi);
}
