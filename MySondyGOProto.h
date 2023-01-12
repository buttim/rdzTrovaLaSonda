#ifndef __MYSONDYGOPROTO_H__
#define __MYSONDYGOPROTO_H__
#include <stdint.h>
#include <string.h>
#include <BluetoothSerial.h>
#include <Preferences.h>

#define CALLSIGN_LENGTH 8
#define debugPrintf(...) fprintf (stderr, __VA_ARGS__)
#define debugPrint(s) fputs(s,stderr)
#define debugPrintln(s) { fputs(s,stderr); fputc('\n',stderr); }

class ProtoUser {
public:
	virtual void mute(bool on)=0;
	virtual void type(int sondeType)=0;
	virtual void freq(float f)=0;
	virtual void restart()=0;
};

class Proto {
	static const char ver[],appName[];
	ProtoUser *protoUser;
	BluetoothSerial *serial;
	enum status_t {IDLE,O,OPEN,CLOSE};
	status_t status;
	String sBuf="";
	unsigned long tLastBTMessage=0;
	int incoming;
  Preferences preferences;
	
	void loadProps();
	void sendSettings();
	void execCommand(String s);
	void execCommands(String s);
	void setDefaults();
	void sondePos(float vBatt,String id,float lat,float lon,float alt,float vel,int rssi);
	void sondeNoPos(float vBatt,String id,int rssi);
public:
	bool mute=false;
	char callsign[CALLSIGN_LENGTH+1]="MYCALL";
	float freq;
	int sondeType,lcd,lcdOn,blu,baud,com,oledSDA,oledSCL,oledRST,
		ledPin,RS41Band,M20Band,M10Band,PilotBand,DFMBand,freqOffs,
		battPin,battMin,battMax,battType,nameType,buzzPin;

	void init(ProtoUser *protoUser,BluetoothSerial *serial);
	void onData(const uint8_t *buffer, size_t size);
	void loop(float vBatt,bool sondePresent,bool posOk,const char *id,float lat,float lon,float alt,float vel,int rssi);
};

#endif
