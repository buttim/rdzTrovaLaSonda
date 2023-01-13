#include "MySondyGOProto.h"

struct intProp_t { 
	int ord;
	const char *name;
	int defVal;
	void (*set)(Proto *p,int val);
	bool requiresRestart;
};

const static intProp_t intProps[]={
	{ 0,"lcd", 0, [](Proto *p,int val) {p->lcd=val;}, true },
	{ 1,"lcdOn", 1, [](Proto *p,int val) {p->lcdOn=val;}, true },
	{ 2,"blu", 1, [](Proto *p,int val) {p->blu=val;}, true },
	{ 3,"baud", 1, [](Proto *p,int val) {p->baud=val;}, true },
	{ 4,"com", 0, [](Proto *p,int val) {p->com=val;}, true },
	{ 5,"oled_sda", 21, [](Proto *p,int val) {p->oledSDA=val;}, true },
	{ 6,"oled_scl", 22, [](Proto *p,int val) {p->oledSCL=val;}, true },
	{ 7,"oled_rst", 16, [](Proto *p,int val) {p->oledRST=val;}, true },
	{ 8,"led_pout", 25, [](Proto *p,int val) {p->ledPin=val;}, true },
	{ 9,"buz_pin", 0, [](Proto *p,int val) {p->buzzPin=val;}, true },
	{ 10,"rs41.rxbw", 1, [](Proto *p,int val) {p->RS41Band=val;}, false },
	{ 11,"m20.rxbw", 7, [](Proto *p,int val) {p->M20Band=val;}, false },
	{ 12,"m10.rxbw", 7, [](Proto *p,int val) {p->M10Band=val;}, false },
	{ 13,"pilot.rxbw", 7, [](Proto *p,int val) {p->PilotBand=val;}, false },
	{ 14,"dfm.rxbw", 6, [](Proto *p,int val) {p->DFMBand=val;}, false },
	{ 15,"aprsName", 0, [](Proto *p,int val) {p->nameType=val;}, false },
	{ 16,"freqofs", 0, [](Proto *p,int val) {p->freqOffs=val;}, false },
	{ 17,"battery", 35, [](Proto *p,int val) {p->battPin=val;}, true },
	{ 18,"vBatMin", 2950, [](Proto *p,int val) {p->battMin=val;}, false },
	{ 19,"vBatMax", 4200, [](Proto *p,int val) {p->battMax=val;}, false },
	{ 20,"vBatType", 1, [](Proto *p,int val) {p->battType=val;}, false }
};

const char Proto::ver[]="0.90", 
  Proto::appName[]="rdzTrovaLaSonda"; //max length 15

static const char *sondeTypes[]={ "RS41","M20", "M10", "PILOT", "DFM" };

const float bands[]={ //kHz
	2.6,3.1,3.9,5.2,6.3,7.8,10.4,12.5, 15.6,20.8,25.0,
	31.3,41.7,50.0,62.5,83.3,100.0,125.0,166.7,200.0
};

static float bandFromId(unsigned id) {
  id=constrain(id,0,sizeof bands/sizeof(*bands)-1);
	return 1000*bands[id];
}

/*static int idFromBand(float band) {
	for (unsigned i=0;i<sizeof bands/sizeof(*bands);i++)
		if (band==bands[i]) return i;
	return 0;
}*/

int sondeTypeFromName(String s) {
	for (unsigned i=0;i<sizeof sondeTypes/sizeof(*sondeTypes);i++)
		if (s==sondeTypes[i])
			return i+1;
	return 1;
}

const char *nameFromSondeType(unsigned n) {
	if (n==0 || n>sizeof sondeTypes/sizeof(*sondeTypes)) n=1;
	return sondeTypes[n-1];
}

void Proto::setDefaults() {
  preferences.begin(appName,false);
	for (unsigned i=0;i<sizeof intProps/sizeof(*intProps);i++) {
		intProps[i].set(this,intProps[i].defVal);
    preferences.putInt(intProps[i].name,intProps[i].defVal);
	}
	strcpy(callsign,"MYCALLL");
  preferences.putString("callsign",callsign);
	freq=403.0;
	sondeType=1;
  preferences.putFloat("freq",freq);
  preferences.putInt("sondeType",sondeType);
  preferences.end();
	protoUser->freq(freq);
	protoUser->type(sondeType);
}

void Proto::loadProps() {
  preferences.begin(appName,true);
	for (unsigned i=0;i<sizeof intProps/sizeof(*intProps);i++) {
    debugPrintf("%s=",intProps[i].name);
		int val=preferences.getInt(intProps[i].name,intProps[i].defVal);
		intProps[i].set(this,val);
    debugPrintf("%d\n",val);
	}
  debugPrintln("===================");
  preferences.getString("callsign",callsign,sizeof callsign);
  freq=preferences.getFloat("freq",403.0);
  sondeType=preferences.getInt("sondeType",0);
  preferences.end();
	if (freq!=freq || freq==0) //not a number
		setDefaults();
	else {
		protoUser->freq(freq);
		protoUser->type(sondeType);
	}
}

void Proto::sendSettings() {
	vTaskDelay(500);
	serial->printf("3/%s/%.3f/%d/%d/%d/%d/%d/%d/%d/%d/%d/%s/%d/%d/%d/%d/%d/%d/%d/%d/%s/o\r\n",
		nameFromSondeType(sondeType),freq,oledSDA,oledSCL,oledRST,ledPin,
		RS41Band,M20Band,M10Band,PilotBand,DFMBand,callsign,freqOffs,
		battPin,battMin,battMax,battType,lcd,nameType,buzzPin,ver);
	vTaskDelay(100);
}

void Proto::execCommand(String s) {
  if (s=="?") {
    sendSettings();
    return;
  }
  else if (s=="re") {
    protoUser->restart();
    return;
  }
  else if (s=="Re") {
    setDefaults();
    protoUser->restart();
    return;
  }

  int n=s.indexOf('=');
  if (n<0) {
    debugPrintln("Comando sconosciuto!");
    return;
  }
  String cmd=s.substring(0,n),val=s.substring(n+1);
  debugPrint("cmd (");
  debugPrint(cmd.c_str());
  debugPrint(",");
  debugPrint(val.c_str());
  debugPrintln(")");
  if (cmd=="mute") {
    mute=val=="1";
    protoUser->mute(mute);
    return;
  }
  else if (cmd=="myCall") {
    strncpy(callsign,val.c_str(),min(sizeof callsign,val.length()));
    preferences.begin(appName,false);
    preferences.putString("callsign",callsign);
    preferences.end();
    return;
  }
  if (cmd=="f") {
    freq=val.toFloat();
    preferences.begin(appName,false);
    preferences.putFloat("freq",freq);
    preferences.end();
    protoUser->freq(freq);
    //HACKHACK: problema in app Android
    int n=val.indexOf("tipo=");
    if (n<0)
      return;
    cmd="tipo";
    val=val.substring(n+5);
  }
  if (cmd=="tipo") {
    sondeType=val.toInt();
    debugPrint("SONDETYPE: ");
    debugPrintf("%d",sondeType);
    preferences.begin(appName,false);
    preferences.putInt("sondeType",sondeType);
    preferences.end();
    protoUser->type(sondeType);
    return;
  }

  for (unsigned  i=0;i<sizeof intProps/sizeof(*intProps);i++)
    if (cmd==intProps[i].name) {
      int v=val.toInt();
      intProps[i].set(this,v);
      preferences.begin(appName,false);
      preferences.putInt(intProps[i].name,v);
      preferences.end();
      if (intProps[i].requiresRestart) protoUser->restart();
      return;
    }
  debugPrintln("Comando sconosciuto!!");
}

void Proto::execCommands(String s) {
  debugPrint("CMDS: ");
  debugPrintln(s.c_str());
  int start=0,pos;
  while ((pos=s.indexOf('/',start))>=0) {
    execCommand(s.substring(start,pos));
    start=pos+1;
  }
  execCommand(s.substring(start,s.length()));
}

void Proto::onData(const uint8_t *buffer, size_t size) {
	while (size--) {
		char c=*buffer++;

		switch (status) {
			case IDLE:
			if (c=='o') status=O;
			break;
		case O:
			if (c=='{') status=OPEN;
			else status=IDLE;
			break;
		case OPEN:
			if (c=='}') status=CLOSE;
			else sBuf+=c;
			break;
		case CLOSE:
			status=IDLE;
			if (c=='o') { execCommands(sBuf); sBuf=""; }
			break;
		}
	}
}

void Proto::sondePos(float vBatt,String id,float lat,float lon,float alt,float vel,int rssi) {
	debugPrintf("1/%s/%.3f/%s/%.6f/%.6f/%.1f/%.1f/%.1f/%d/0/0/0/%d/%d/0/0/0/%s/o\r\n",
		nameFromSondeType(sondeType),freq,
		id.c_str(),lat,lon,alt,vel,-rssi/2.0,
		(int)((vBatt-battMin)*100/(battMax-battMin)),(int)vBatt,
		mute?1:0,ver);
	serial->printf("1/%s/%.3f/%s/%.6f/%.6f/%.1f/%.1f/%.1f/%d/0/0/0/%d/%d/0/0/0/%s/o\r\n",
		nameFromSondeType(sondeType),freq,
		id.c_str(),lat,lon,alt,vel,-rssi/2.0,
		(int)((vBatt-battMin)*100/(battMax-battMin)),(int)vBatt,
		mute?1:0,ver);
	tLastBTMessage=millis();
}

void Proto::sondeNoPos(float vBatt,String id,int rssi) {
	debugPrintf("2/%s/%.3f/%s/%.1f/%d/0/%d/%d/%s/o\r\n",
		nameFromSondeType(sondeType),freq,
		id.c_str(),-rssi/2.0,
		(int)((vBatt-battMin)*100/(battMax-battMin)),(int)vBatt,
		mute?1:0,ver);
	serial->printf("2/%s/%.3f/%s/%.1f/%d/0/%d/%d/%s/o\r\n",
		nameFromSondeType(sondeType),freq,
		id.c_str(),-rssi/2.0,
		(int)((vBatt-battMin)*100/(battMax-battMin)),(int)vBatt,
		mute?1:0,ver);
}

void Proto::init(ProtoUser *protoUser,BluetoothSerial *serial) {
	this->protoUser=protoUser;
	this->serial=serial;
	loadProps();
}

void Proto::loop(float vBatt,bool sondePresent,bool posOk,const char *id,float lat,float lon,float alt,float vel,int rssi) {
  if (!sondePresent || id[0]==0) {
    debugPrintf("0/%s/%.3f/%.2f/%d/%d/%d/%s/o\r\n",
      nameFromSondeType(sondeType),freq,rssi/2.0,map(vBatt,battMin,battMax,0,100),(int)vBatt,mute?1:0,ver);
    serial->printf("0/%s/%.3f/%.2f/%d/%d/%d/%s/o\r\n",
      nameFromSondeType(sondeType),freq,rssi/2.0,map(vBatt,battMin,battMax,0,100),(int)vBatt,mute?1:0,ver);
    tLastBTMessage=millis();
  }
  else if (posOk)
    sondePos(vBatt,id,lat,lon,alt,vel,rssi);
  else
    sondeNoPos(vBatt,id,rssi);
}
