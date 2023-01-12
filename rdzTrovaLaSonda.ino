// #include <SSD1306.h>
// #include <SH1106.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <BluetoothSerial.h>
#include <math.h>
#include "Sonde.h"
#include "DecoderBase.h"
#include "rdzTrovaLaSonda.h"
#include "SX1278FSK.h"
#include "Display.h"
#include "RS41.h"
#include "M10M20.h"
#include "DFM.h"
#include "MySondyGOProto.h"

const char *version_name = "rdzTTGOsonde";
const char *version_id = "devel20221217";

//OLEDDisplay *display;
SemaphoreHandle_t sem;
SemaphoreHandle_t semSX1278;
TimerHandle_t tBuzzOff;
DecoderBase *decoder=&rs41;
Proto proto;
MyProtoUser myProtoUser;
BluetoothSerial ESP_BT;

DecoderBase *getDecoder(unsigned sondeType) {
  DecoderBase *decoders[]={&rs41,&m10m20,&m10m20,&rs41,&dfm};
  sondeType=constrain(sondeType,1,sizeof decoders/sizeof(*decoders));
  return decoders[sondeType-1];
}

void MyProtoUser::mute(bool on) {

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
  digitalWrite(LED,HIGH);
  analogWrite(BUZZER,128);
  xTimerChangePeriod(tBuzzOff,duration,0);
  xTimerStart(tBuzzOff,0);
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

  pinMode(LED,OUTPUT);
  pinMode(BUZZER,OUTPUT);

  sem=xSemaphoreCreateBinary();
  semSX1278=xSemaphoreCreateBinary();
  xSemaphoreGive(semSX1278);
  tBuzzOff=xTimerCreate("buzzer off",pdMS_TO_TICKS(100),pdFALSE,NULL,[](TimerHandle_t){
    analogWrite(BUZZER,0);
    digitalWrite(LED,LOW);
  });

  bip(250,2000);

  sonde.defaultConfig();

  sx1278.setup(semSX1278);
  decoder=getDecoder(proto.sondeType);
  decoder->setup(proto.freq*1E6);

  //disp.init();
  //Serial.printf("disp.rdis %08X\n",disp.rdis);
  //disp.rdis->drawString(20,20,"ZZZ");
}

void loop() {
  SondeInfo *si=sonde.si();
  int res=decoder->receive();
  //Serial.printf("RX:%d,lat:%f,lon:%f,ser:%s\n",res,si->d.lat,si->d.lon,si->d.ser);
  if (res==0) bip(50,8000);
  proto.loop(3500,res==0,!isnan(si->d.lat),si->d.ser,si->d.lat,si->d.lon,si->d.alt,si->d.hs,si->rssi);
}
