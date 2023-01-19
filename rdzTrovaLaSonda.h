#ifndef __TOVALASONDA_H__
#define __TOVALASONDA_H__
#include <Preferences.h>
#include "MySondyGOProto.h"

const int RADIO_SCLK_PIN=5,
  RADIO_MISO_PIN=19,
  RADIO_MOSI_PIN=27,
  RADIO_CS_PIN  =18,
  RADIO_DIO0_PIN=26,
  RADIO_DIO1_PIN=33,
  RADIO_DIO2_PIN=32,
  RADIO_RST_PIN =23,
  RADIO_BUSY_PIN=32;

extern void bip(int duration=100,int freq=880);
extern const char *version;
extern DecoderBase *decoder;

class MyProtoUser : public ProtoUser {
public:
  void freq(float f);
  void type(unsigned short t);
	void mute(bool on) {
    if (!on) bip(50,8000);
  }
	void restart() {
		ESP.restart();
	}
  const char *version() {
    return ::version;
  }
};

#endif