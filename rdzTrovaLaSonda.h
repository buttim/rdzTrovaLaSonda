#ifndef __TOVALASONDA_H__
#define __TOVALASONDA_H__
#include <Preferences.h>
#include "MySondyGOProto.h"

/*#define LED         25
#define BUZZER      4
#define SDA         21
#define SCL         22*/

#define RADIO_SCLK_PIN               5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN              26
#define RADIO_DIO1_PIN              33
#define RADIO_DIO2_PIN              32
#define RADIO_RST_PIN               23
#define RADIO_BUSY_PIN              32

extern void bip(int duration=100,int freq=880);

extern DecoderBase *decoder;

class MyProtoUser : public ProtoUser {
	void mute(bool on) {
    if (!on) bip(50,8000);
  }
	void restart() {
		ESP.restart();
	}
  const char *version() {
    return "0.91";
  }
};

#endif