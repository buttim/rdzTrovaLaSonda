#ifndef __MYSONDYGOPROTO_H__
#define __MYSONDYGOPROTO_H__
#include <stdint.h>
#include <string.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

#define DEBUG_MYSONDYGOPROTO 1
#define CALLSIGN_LENGTH 8

#ifdef DEBUG_MYSONDYGOPROTO
#define debugPrintf(...) fprintf (stderr, __VA_ARGS__)
#define debugPrint(s) fputs(s,stderr)
#define debugPrintln(s) { fputs(s,stderr); fputc('\n',stderr); }
#else
#define debugPrintf(...)
#define debugPrint(s)
#define debugPrintln(s)
#endif

int bandFromId(unsigned id);
const char *nameFromSondeType(unsigned n);

class ProtoUser {
public:
  virtual void freq(float f)=0;
  virtual void type(unsigned short t)=0;
	virtual void mute(bool on)=0;
	virtual void restart()=0;
	virtual const char *version()=0;
};

class Proto {
	static const char appName[];
	ProtoUser *protoUser;
	BluetoothSerial *serial;
	enum status_t {IDLE,O,OPEN,CLOSE};
	status_t status;
	String sBuf="";
	unsigned long tLastBTMessage=0;
  Preferences preferences;
  esp_ota_handle_t handleOta;
	
	void loadProps();
	void sendSettings();
	void execCommand(String s);
	void execCommands(String s);
	void setDefaults();
	void sondePos(float vBatt,String id,float lat,float lon,float alt,float vel,bool bk,int bkTime,int rssi,int afc);
	void sondeNoPos(float vBatt,String id,int rssi);
public:
	bool mute=false, otaRunning=false;
  int otaErr=0, otaProgress=0, otaLength=0;
	char callsign[CALLSIGN_LENGTH+1]="MYCALL";
	float freq;
	short sondeType,lcd,lcdOn,blu,baud,com,oledSDA,oledSCL,oledRST,
		ledPin,RS41Band,M20Band,M10Band,PilotBand,DFMBand,freqOffs,
		battPin,battMin,battMax,battType,nameType,buzzPin;

	void init(ProtoUser *protoUser,BluetoothSerial *serial);
	void onData(const uint8_t *buffer, size_t size);
	void loop(float vBatt,bool sondePresent,bool posOk,const char *id,float lat,float lon,float alt,float vel,
    bool bk,int bkTime,int rssi,int afc);
};
#endif
