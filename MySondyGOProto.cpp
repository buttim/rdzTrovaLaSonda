#include "MySondyGOProto.h"

struct intProp_t { 
	uint8_t ord;
	const char *name;
	short defVal;
  short Proto::* field;
	bool requiresRestart;
};

const static intProp_t intProps[]={
	{ 0,"lcd",         0, &Proto::lcd,       true  },
	{ 1,"lcdOn",       1, &Proto::lcdOn,     true  },
	{ 2,"blu",         1, &Proto::blu,       true  },
	{ 3,"baud",        1, &Proto::baud,      true  },
	{ 4,"com",         0, &Proto::com,       true  },
	{ 5,"oled_sda",   21, &Proto::oledSDA,   true  },
	{ 6,"oled_scl",   22, &Proto::oledSCL,   true  },
	{ 7,"oled_rst",   16, &Proto::oledRST,   true  },
	{ 8,"led_pout",   25, &Proto::ledPin,    true  },
	{ 9,"buz_pin",     0, &Proto::buzzPin,   true  },
	{ 10,"rs41.rxbw",  1, &Proto::RS41Band,  false },
	{ 11,"m20.rxbw",   7, &Proto::M20Band,   false },
	{ 12,"m10.rxbw",   7, &Proto::M10Band,   false },
	{ 13,"pilot.rxbw", 7, &Proto::PilotBand, false },
	{ 14,"dfm.rxbw",   6, &Proto::DFMBand,   false },
	{ 15,"aprsName",   0, &Proto::nameType,  false },
	{ 16,"freqofs",    0, &Proto::freqOffs,  false },
	{ 17,"battery",   35, &Proto::battPin,   true  },
	{ 18,"vBatMin", 2950, &Proto::battMin,   false },
	{ 19,"vBatMax", 4200, &Proto::battMax,   false },
	{ 20,"vBatType",   1, &Proto::battType,  false }
};

const char Proto::appName[]="rdzTrovaLaSonda"; //max length 15

static const char *sondeTypes[]={ "RS41","M20", "M10", "PILOT", "DFM" };
static const int bands[]={ //Hz
	2600,3100,3900,5200,6300,7800,10400,12500, 15600,20800,25000,
	31300,41700,50000,62500,83300,100000,125000,166700,200000
};

int bandFromId(unsigned id) {
  id=constrain(id,0,sizeof bands/sizeof(*bands)-1);
	return 1000*bands[id];
}

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
    this->*intProps[i].field=intProps[i].defVal;
    preferences.putShort(intProps[i].name,intProps[i].defVal);
	}
	strcpy(callsign,"MYCALLL");
  preferences.putString("callsign",callsign);
	freq=403.0;
	sondeType=1;
  preferences.putFloat("freq",freq);
  preferences.putShort("sondeType",sondeType);
  preferences.end();
}

void Proto::loadProps() {
  preferences.begin(appName,true);
	for (unsigned i=0;i<sizeof intProps/sizeof(*intProps);i++) {
		int val=preferences.getShort(intProps[i].name,intProps[i].defVal);
    this->*intProps[i].field=val;
	}
  preferences.getString("callsign",callsign,sizeof callsign);
  freq=preferences.getFloat("freq",403.0);
  sondeType=preferences.getShort("sondeType",0);
  preferences.end();
	if (freq!=freq || freq==0) //not a number
		setDefaults();
}

void Proto::sendSettings() {
	vTaskDelay(500);
	serial->printf("3/%s/%.3f/%d/%d/%d/%d/%d/%d/%d/%d/%d/%s/%d/%d/%d/%d/%d/%d/%d/%d/%s/o\r\n",
		nameFromSondeType(sondeType),freq,oledSDA,oledSCL,oledRST,ledPin,
		RS41Band,M20Band,M10Band,PilotBand,DFMBand,callsign,freqOffs,
		battPin,battMin,battMax,battType,lcd,nameType,buzzPin,protoUser->version());
  //TODO: seriale
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
  else if (cmd=="ota") {
    otaRunning=true;
    debugPrintf("INIZIO OTA %s\n",val.c_str());
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(running);
    otaLength=val.toInt();
    esp_err_t err=esp_ota_begin(next,otaLength,&handleOta);
    if (err!=ESP_OK) {
      debugPrintf("Errore esp_ota_begin %d\n",err);
      otaErr=err;
      return;
    }
    return;
  }
  if (cmd=="f") {
    freq=val.toFloat();
    preferences.begin(appName,false);
    preferences.putFloat("freq",freq);
    preferences.end();
    //HACKHACK: problema in app Android con MySOndyGO
    int n=val.indexOf("tipo=");
    if (n>=0) {
      cmd="tipo";
      val=val.substring(n+5);
      sondeType=val.toInt();
      preferences.begin(appName,false);
      preferences.putShort("sondeType",sondeType);
      preferences.end();
      protoUser->type(sondeType);
    }
    protoUser->freq(freq);
    return;
  }
  if (cmd=="tipo") {
    sondeType=val.toInt();
    preferences.begin(appName,false);
    preferences.putShort("sondeType",sondeType);
    preferences.end();
    protoUser->type(sondeType);
    return;
  }

  for (unsigned  i=0;i<sizeof intProps/sizeof(*intProps);i++)
    if (cmd==intProps[i].name) {
      int v=val.toInt();
      this->*intProps[i].field=v;
      preferences.begin(appName,false);
      preferences.putShort(intProps[i].name,v);
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
  if (otaRunning) {
    esp_err_t err=esp_ota_write(handleOta,buffer,size);
    if (err!=ESP_OK) {
      debugPrintf("Errore esp_ota_write %d\n");
      otaErr=err;
      return;
    }
    otaProgress+=size;
    if (otaProgress==otaLength) {
      esp_ota_end(handleOta);
      const esp_partition_t *running = esp_ota_get_running_partition(),
        *next = esp_ota_get_next_update_partition(running);

      esp_ota_set_boot_partition(next);
      protoUser->restart();
    }
    return;
  }
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

void Proto::sondePos(float vBatt,String id,float lat,float lon,float alt,float vel,
    bool bk,int bkTime,int rssi,int afc) {
  int battPercent=constrain(map(vBatt,battMin,battMax,0,100),0,100);
  //TODO:bk bkTime
	debugPrintf("1/%s/%.3f/%s/%.6f/%.6f/%.1f/%.1f/%.1f/%d/%d/%d/%d/%d/%d/0/0/0/%s/o\r\n",
		nameFromSondeType(sondeType),freq,
		id.c_str(),lat,lon,alt,vel,-rssi/2.0,
		battPercent,afc,bk?1:0,bkTime,
    (int)vBatt,mute?1:0,protoUser->version());
	serial->printf("1/%s/%.3f/%s/%.6f/%.6f/%.1f/%.1f/%.1f/%d/%d/%d/%d/%d/%d/0/0/0/%s/o\r\n",
		nameFromSondeType(sondeType),freq,
		id.c_str(),lat,lon,alt,vel,-rssi/2.0,
		battPercent,afc,bk?1:0,bkTime,
    (int)vBatt,mute?1:0,protoUser->version());
	tLastBTMessage=millis();
}

void Proto::sondeNoPos(float vBatt,String id,int rssi) {
  int battPercent=constrain(map(vBatt,battMin,battMax,0,100),0,100);
	debugPrintf("2/%s/%.3f/%s/%.1f/%d/0/%d/%d/%s/o\r\n",
		nameFromSondeType(sondeType),freq,
		id.c_str(),-rssi/2.0,
		battPercent,(int)vBatt,
		mute?1:0,protoUser->version());
	serial->printf("2/%s/%.3f/%s/%.1f/%d/0/%d/%d/%s/o\r\n",
		nameFromSondeType(sondeType),freq,
		id.c_str(),-rssi/2.0,
		battPercent,(int)vBatt,
		mute?1:0,protoUser->version());
}

void Proto::init(ProtoUser *protoUser,BluetoothSerial *serial) {
	this->protoUser=protoUser;
	this->serial=serial;
	loadProps();
}

void Proto::loop(float vBatt,bool sondePresent,bool posOk,const char *id,
    float lat,float lon,float alt,float vel,bool bk,int bkTime,int rssi,int afc) {
  if (otaRunning)
    return;
  if (!sondePresent /*|| id[0]==0*/) {
    int battPercent=constrain(map(vBatt,battMin,battMax,0,100),0,100);
    debugPrintf("0/%s/%.3f/%.2f/%d/%d/%d/%s/o\r\n",
      nameFromSondeType(sondeType),freq,rssi/2.0,battPercent,(int)vBatt,mute?1:0,protoUser->version());
    serial->printf("0/%s/%.3f/%.2f/%d/%d/%d/%s/o\r\n",
      nameFromSondeType(sondeType),freq,rssi/2.0,battPercent,(int)vBatt,mute?1:0,protoUser->version());
    tLastBTMessage=millis();
  }
  else if (posOk)
    sondePos(vBatt,id,lat,lon,alt,vel,bk,bkTime,rssi,afc);
  else
    sondeNoPos(vBatt,id,rssi);
}
