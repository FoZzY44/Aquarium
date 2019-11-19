/////////////////////////////////////////////////////////
// Définition des constantes
/////////////////////////////////////////////////////////

// Position de sauvegarde en memoire
#define EEPROM_POSITION_LIGHT_STATUS  0
#define EEPROM_POSITION_RELAIS_STATUS_1 1
#define EEPROM_POSITION_RELAIS_STATUS_2 2
#define EEPROM_POSITION_RELAIS_STATUS_3 3
#define EEPROM_POSITION_RELAIS_STATUS_4 4
#define EEPROM_POSITION_DEBUG_MODE 5
#define EEPROM_POSITION_toDefine_2 6
#define EEPROM_POSITION_toDefine_3 7
#define EEPROM_POSITION_toDefine_4 8
#define EEPROM_POSITION_toDefine_5 9
#define EEPROM_POSITION_LIGHT_TIME  10

// Daylight Saving
#define GMT 1

// Initialisation de la SDCard
#define INITSDCARD false

// Pin du OneWire (Température)
#define ONE_WIRE_PIN 2 

//Pin et constante pH
#define PH_PIN A0
#define PH_OFFSET 0.00
#define PH_ARRAY_LENGTH  40

// Pin et constantes du capteur de dureté de l'eau
#define TDS_PIN A1
#define TDS_VREF 5.0
#define TDS_COUNTER  30

// Pin du capteur de distance
#define DISTANCE_ECHO_PIN 9 
#define DISTANCE_TRIGGER_PIN 8

// Pin de la rampe d'eclairage
#define LIGHT_PIN 3

// Pin Led PH
#define PH_LED_ALARM_HIGH 36
#define PH_LED_WARN_HIGH 37
#define PH_LED_OK 38
#define PH_LED_WARN_LOW 39
#define PH_LED_ALARM_LOW 40

// Pin Led GH
#define GH_LED_LOW 41
#define GH_LED_MEDIUM 42
#define GH_LED_HIGH 43

// Pin Button
#define butPush 35
#define butOnOff 34
#define butOnOffLed 33

// Pin relais
#define RELAIS_PIN_1 47
#define RELAIS_PIN_2 46
#define RELAIS_PIN_3 45
#define RELAIS_PIN_4 44
#define RELAIS_ON  LOW
#define RELAIS_OFF HIGH
#define LED_ON  LOW
#define LED_OFF  HIGH

// LightTime
#define LIGHT_TIME_STRUCT_LENGTH  5
#define ON  1
#define OFF 0

// Distance Min MAX
#define DISTANCE_MAX 200
#define DISTANCE_MIN 0

// Reseau
#define NTP_PACKET_SIZE 48

// Carte SD
#define SDCARD_CHIPSET_SELECT 4

// Définition des levels de débug
#define INFO 0
#define DEBUG 1
#define WARN 2
#define ERROR 3
#define FORCE 4




/////////////////////////////////////////////////////////
// Inclusion des librairies
/////////////////////////////////////////////////////////

// Température
#include <OneWire.h> 
#include <DallasTemperature.h>

// Ethernet
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>


// Timer
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

// Stockage
#include <EEPROM.h>


/////////////////////////////////////////////////////////
// Définition des variables
/////////////////////////////////////////////////////////

//Level du debug
int debugLevel = 2;

// carte SD disponible pour l'écriture
bool cardOK = false;

// Configuration de la couche réseau
byte mac[] = {0x00, 0x3C, 0x05, 0xA7, 0x14, 0x45};
IPAddress ip(192, 168, 1, 11);
IPAddress myDns(192, 168, 1, 254);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
EthernetServer server(80);
const char timeServerAddress[] = "zeus.fozzy.fr";
byte packetBuffer[NTP_PACKET_SIZE];
unsigned int localPort = 8888;

// Configuration de la distance
float Aqua_Distance;

// Configuration de la dureté de l'eau
int analogBuffer[TDS_COUNTER];    // store the analog value in the array, read from ADC
int analogBufferTemp[TDS_COUNTER];
int analogBufferIndex = 0,copyIndex = 0;

// Dureté de l'eau
float Aqua_tdsValue = 0;

// Initialisation du Bus One Wire (température)
OneWire oneWire(ONE_WIRE_PIN); 
DallasTemperature sensors(&oneWire);

// Variables de la température
float Aqua_tempHaut = 0;
float Aqua_tempBas = 0;
float Aqua_tempInt = 0;

// Variable date heure
int Year = 0;
int Month = 0;
int Day = 0;
int DayOfWeekNumber = 0;
int Hour = 0;
int Minute = 0;
int Second = 0;
tmElements_t tm;
String Aqua_dateTime="";

// Variable du pH
static float Aqua_pHValue;
int pHBuffer[10];

// Variable eclairage
int Aqua_lightStatus = 0;

// Variable relais
int RELAIS_PIN_1_status = RELAIS_OFF;
int RELAIS_PIN_2_status = RELAIS_OFF;
int RELAIS_PIN_3_status = RELAIS_OFF;
int RELAIS_PIN_4_status = RELAIS_OFF;



/////////////////////////////////////////////////////////
// Déclaration des fonctions
/////////////////////////////////////////////////////////

void printDebug(String message, int level=1);
void printDebugln(String message, int level=1);
void setTimerNTP ();
int readValROM(int address);
void replyJson(EthernetClient client, String message);
String print2digits(int number);
int getMedianNum(int bArray[], int iFilterLen);
String clearROM(int startAddress, int stopAddress);

/////////////////////////////////////////////////////////
// Configuration du programme
/////////////////////////////////////////////////////////
void setup() 
{ 

  // Ouverture du port COM
  Serial.begin(115200); 


  // Initialisation des pins
  initPins();


  // Initialisation de la couche reseau 
  Ethernet.begin(mac, ip, myDns, gateway, subnet);


  // Intialisation du bus OneWire
  printDebug("Initialisation du OneWire",DEBUG); 
  sensors.begin();
  printDebugln(".....OK",DEBUG); 


  // Configuration de la couche réseau
  printDebug("Configuration de la couche réseau",DEBUG);
  // Test si le shield réseau est présent
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    printDebugln(".....KO :(",DEBUG);
    while (true) {
      delay(1); 
    }
  } else {
    printDebugln(".....OK",DEBUG);
  }


  // Test de la connexion du cable réseaux
  printDebug("Test de la connexion réseau",DEBUG);
  if (Ethernet.linkStatus() == LinkOFF) {
    printDebugln(".....KO :(",DEBUG);
  } else {
    printDebugln(".....OK",DEBUG);
  }


  // Récupération de l'heure système depuis le serveur NTP
  printDebug("Initialisation de l'heure",DEBUG);
  setTimerNTP ();
  printDebugln(".....OK",DEBUG);

  
  // Démarrage de la couche réseau et récupération de l'IP pour affichage
  server.begin();
  printDebug("IP du serveur : ",DEBUG);
  printDebugln(String(Ethernet.localIP()));


  // Initialiation de la carte SD
  if(INITSDCARD) {
    initSDCard();
  }


  // Récupération de l'etat de la lumière
  Aqua_lightStatus = readValROM(EEPROM_POSITION_LIGHT_STATUS);

  // recupération de l'etat des relais
  RELAIS_PIN_1_status = readValROM(EEPROM_POSITION_RELAIS_STATUS_1);
  RELAIS_PIN_2_status = readValROM(EEPROM_POSITION_RELAIS_STATUS_2);
  RELAIS_PIN_3_status = readValROM(EEPROM_POSITION_RELAIS_STATUS_3);
  RELAIS_PIN_4_status = readValROM(EEPROM_POSITION_RELAIS_STATUS_4);


  // Execution de la configuration des pins
  setPins();

  
}
// Fin setup()



/////////////////////////////////////////////////////////
// Boucle 
/////////////////////////////////////////////////////////
void loop() 
{ 

  // Récupération des température
  getTemperature();

  // Récupération de la dureté de l'eau
  getGH();

  //Récupération de la distance
  getDistance();

  // Récupération du pH
  getPH();

  // Récupération de 'heure
  getTime();

  // ecoute du client web
  listenClient();
  
  // Test de la consigne lumineuse
  getLightConsigne();

  // Action sur les pins
  setPins();

  
  
} 
// Fin loop()



/////////////////////////////////////////////////////////
// Initialisation de la carte SD
/////////////////////////////////////////////////////////
void initSDCard() {
  // init de la carte SD
   printDebug("Initialisation de la carte SD",DEBUG);

  if (!SD.begin(SDCARD_CHIPSET_SELECT)) {
    cardOK = false;
    printDebugln(".....KO :(",DEBUG);
  } else {
    cardOK = true;
    printDebugln(".....OK",DEBUG);
  }
}
// Fin initSDCard()



/////////////////////////////////////////////////////////
// Ecoute du client web
/////////////////////////////////////////////////////////
void listenClient() {
    // Ecoute d'une connexion réseau
    EthernetClient client = server.available();
    if (client) {
      printDebugln("Nouvelle connexion web",DEBUG);
  
      boolean currentLineIsBlank = true;
      boolean firstLine = true;
  
      // Parse URL
      char getAnswer[2048]; 
      int getAnswerCount = 0; 
      char answerValue[10]; 
      int answerValueCount = 0; 
      String url = "";
      String jsonAnswer = "";
      
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          if (c == '\n') {
            jsonAnswer = readURL(url.substring(4,url.length()-9));
            
            if(jsonAnswer != "") {
              replyJson(client, jsonAnswer);
            }
            
            break;
          } else {
            url += c;
          }
        }
      }
      // give the web browser time to receive the data
      delay(1);
      // close the connection:
      client.stop();
      printDebugln("Client deconnecté",DEBUG);
    }
  }
// Fin listenClient()



/////////////////////////////////////////////////////////
// Récupération de l'heure depuis le DS1307RTC
/////////////////////////////////////////////////////////
void getTime() {
    // Lecture de l'heure et action 
    if (RTC.read(tm)) {
  
      Aqua_dateTime =  print2digits(tm.Hour) + ":" + print2digits(tm.Minute) + ":" + print2digits(tm.Second) + " " + print2digits(tm.Day) + "/" + print2digits(tm.Month) + "/" + print2digits(tmYearToCalendar(tm.Year));
     
  
      Hour = tm.Hour;
      Minute = tm.Minute;
      
      
      if(tm.Month >= 3) {
        int z=tmYearToCalendar(tm.Year) - 1;
        DayOfWeekNumber = (((23*tm.Month)/9) + tm.Day + 4 + tmYearToCalendar(tm.Year) + (z/4) - (z/100) + (z/400) - 2 ) % 7;
      } else {
        int z=tmYearToCalendar(tm.Year);
        DayOfWeekNumber = (((23*tm.Month)/9) + tm.Day + 4 + tmYearToCalendar(tm.Year) + (z/4) - (z/100) + (z/400) ) % 7;
      }
      
    } else {
      if (RTC.chipPresent()) {
        printDebugln("The DS1307 is stopped.  Please run the SetTime",ERROR);
        printDebugln("example to initialize the time and begin running.",ERROR);
        printDebugln("",ERROR);
      } else {
        printDebugln("DS1307 read error!  Please check the circuitry.",ERROR);
        printDebugln("",ERROR);
      }
    }
   }
// Fin getTime()



/////////////////////////////////////////////////////////
// Récupération des températures
/////////////////////////////////////////////////////////
void getTemperature() {
    // Lecture des températures
    printDebug(" Récupération des températures...",DEBUG); 
    sensors.requestTemperatures(); // request command to get temperature readings 
    printDebugln("OK"); 
  
    Aqua_tempHaut = sensors.getTempCByIndex(1);
    Aqua_tempBas  = sensors.getTempCByIndex(2);
    Aqua_tempInt  = sensors.getTempCByIndex(0);
  
    printDebug("Températures => ",DEBUG); 
    printDebug("Haut : ",DEBUG); 
    printDebug(String(Aqua_tempHaut),DEBUG);
    printDebug("  "),DEBUG; 
    printDebug("Bas : ",DEBUG); 
    printDebug(String(Aqua_tempBas),DEBUG);
    printDebug("  ",DEBUG); 
    printDebug("Int : ",DEBUG); 
    printDebugln(String(Aqua_tempInt),DEBUG);
  }
// Fin getTemperature()



/////////////////////////////////////////////////////////
// Récupération la dureté de l'eau
/////////////////////////////////////////////////////////
void getGH() {
    
  float averageVoltage = 0;

    // Lecture de la dureté de l'eau
    static unsigned long analogSampleTimepoint = millis();
    if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
    {
      analogSampleTimepoint = millis();
      analogBuffer[analogBufferIndex] = analogRead(TDS_PIN);    //read the analog value and store into the buffer
      analogBufferIndex++;
      if(analogBufferIndex == TDS_COUNTER) 
        analogBufferIndex = 0;
    }   
    
    static unsigned long printTimepoint = millis();
    if(millis()-printTimepoint > 400U)
    {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<TDS_COUNTER;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,TDS_COUNTER) * (float)TDS_VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(Aqua_tempHaut-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      Aqua_tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      printDebug("voltage:",DEBUG);
      printDebug(String(averageVoltage),DEBUG);
      printDebug("V   ",DEBUG);
      printDebug("TDS Value:",DEBUG);
      printDebug(String(Aqua_tdsValue),DEBUG);
      printDebugln("ppm",DEBUG);
    }
  }
// Fin getGH()



/////////////////////////////////////////////////////////
// Récupération du pH
/////////////////////////////////////////////////////////
void getPH(){

  unsigned long int avgValue; 
  int temp;
  
  // calcul du pH
   for(int i=0;i<10;i++) 
   { 
    pHBuffer[i]=analogRead(PH_PIN);
    delay(10);
   }
   for(int i=0;i<9;i++)
   {
    for(int j=i+1;j<10;j++)
    {
     if(pHBuffer[i]>pHBuffer[j])
     {
      temp=pHBuffer[i];
      pHBuffer[i]=pHBuffer[j];
      pHBuffer[j]=temp;
     }
    }
   }
   avgValue=0;
   for(int i=2;i<8;i++)
   avgValue+=pHBuffer[i];
   float pHVol=(float)avgValue*5.0/1024/6;
   Aqua_pHValue = -5.00 * pHVol + 21.09;
}
// Fin getPH()



/////////////////////////////////////////////////////////
// Récupération de la distance
/////////////////////////////////////////////////////////
void getDistance() {
  
  float duree;
  
  // Calcul de la distance
  digitalWrite (DISTANCE_TRIGGER_PIN, LOW); 
  delayMicroseconds (2); 

  digitalWrite (DISTANCE_TRIGGER_PIN, HIGH);
  delayMicroseconds (10); 
 
  digitalWrite (DISTANCE_TRIGGER_PIN, LOW);
  duree = pulseIn (DISTANCE_ECHO_PIN, HIGH);
 
  // Calcule la distance (en cm) par rapport à la vitesse du son.
  Aqua_Distance = duree / 58,2;
  printDebug ("Distance : ",DEBUG);
  if (Aqua_Distance>= DISTANCE_MAX || Aqua_Distance <= DISTANCE_MIN) 
  {
    printDebugln ( "hors de portee",WARN); 
  } 
  else
  { // Envoyer la distance à l'ordinateur via le moniteur série 
     
    printDebugln (String(Aqua_Distance),DEBUG); 
  } 
}
// Fin getDistance()



/////////////////////////////////////////////////////////////////////////
// Vérifie les consignes dans l'EEPROM
/////////////////////////////////////////////////////////////////////////
void getLightConsigne() {

  //test Off
  int posMemOff = EEPROM_POSITION_LIGHT_TIME + (2*DayOfWeekNumber + OFF)  * LIGHT_TIME_STRUCT_LENGTH;
  int posMemOn  = EEPROM_POSITION_LIGHT_TIME + (2*DayOfWeekNumber + ON)  * LIGHT_TIME_STRUCT_LENGTH;


  
  if(readValROM(posMemOff+2) == Hour and readValROM(posMemOff+3) == Minute) {
    printDebugln("setOff",DEBUG);
    Aqua_lightStatus = 0;
  }

  if(readValROM(posMemOn+2) == Hour and readValROM(posMemOn+3) == Minute) {
    Aqua_lightStatus = 255;
    printDebugln("setOn",DEBUG);
  }
}
// Fin getLightConsigne()



/////////////////////////////////////////////////////////////////////////
// Initialisation des pins
/////////////////////////////////////////////////////////////////////////
void initPins() {
  // Initialisation des sorties Leds
  pinMode(PH_LED_ALARM_HIGH, OUTPUT);
  pinMode(PH_LED_WARN_HIGH, OUTPUT);
  pinMode(PH_LED_OK, OUTPUT);
  pinMode(PH_LED_WARN_LOW, OUTPUT);
  pinMode(PH_LED_ALARM_LOW, OUTPUT);
  pinMode(GH_LED_LOW, OUTPUT);
  pinMode(GH_LED_MEDIUM, OUTPUT);
  pinMode(GH_LED_HIGH, OUTPUT);

  // Initialisation des valeurs Leds
  digitalWrite(PH_LED_ALARM_HIGH, LED_OFF);
  digitalWrite(PH_LED_WARN_HIGH, LED_OFF);
  digitalWrite(PH_LED_OK, LED_OFF);
  digitalWrite(PH_LED_WARN_LOW, LED_OFF);
  digitalWrite(PH_LED_ALARM_LOW, LED_OFF);
  digitalWrite(GH_LED_LOW, LED_OFF);
  digitalWrite(GH_LED_MEDIUM, LED_OFF);
  digitalWrite(GH_LED_HIGH, LED_OFF);
  
  // Initialisation des entrées boutons
  pinMode(butPush, OUTPUT);
  pinMode(butOnOff, OUTPUT);
  pinMode(butOnOffLed, OUTPUT);
  
  // Initialisation du capteur de dureté de l'eau
  printDebug("Initialisation du Gravity",DEBUG); 
  pinMode(TDS_PIN,INPUT);
  printDebugln(".....OK",DEBUG); 

  // Initialisation du capteur de distance de l'eau
  pinMode (DISTANCE_TRIGGER_PIN, OUTPUT);
  pinMode (DISTANCE_ECHO_PIN, INPUT);
  // Initialisation des relais
  pinMode(RELAIS_PIN_1, OUTPUT);
  pinMode(RELAIS_PIN_2, OUTPUT);
  pinMode(RELAIS_PIN_3, OUTPUT);
  pinMode(RELAIS_PIN_4, OUTPUT);
}
// Fin initPins()



/////////////////////////////////////////////////////////////////////////
// Ecrit l'action sur les pins
/////////////////////////////////////////////////////////////////////////
void setPins() {

  
  analogWrite(LIGHT_PIN, Aqua_lightStatus);
  
  
  
  if(RELAIS_PIN_1_status == ON) {
    digitalWrite(RELAIS_PIN_1, RELAIS_ON);
  } else {
    digitalWrite(RELAIS_PIN_1, RELAIS_OFF);
  }

  if(RELAIS_PIN_2_status == ON) {
    digitalWrite(RELAIS_PIN_2, RELAIS_ON);
  } else {
    digitalWrite(RELAIS_PIN_2, RELAIS_OFF);
  }

  if(RELAIS_PIN_3_status == ON) {
    digitalWrite(RELAIS_PIN_3, RELAIS_ON);
  } else {
    digitalWrite(RELAIS_PIN_3, RELAIS_OFF);
  }

  if(RELAIS_PIN_4_status == ON) {
    digitalWrite(RELAIS_PIN_4, RELAIS_ON);
  } else {
    digitalWrite(RELAIS_PIN_4, RELAIS_OFF);
  }
}
// Fin setPins()



/////////////////////////////////////////////////////////////////////////
// Traduction de l'URL appelé en action
/////////////////////////////////////////////////////////////////////////
String readURL(String getAnswer) {

  String urlParameters = "";
  int countChunk = 0;
  String key="";
  String value="";
  String chunk="";
  int chunkPosition = 0;


  String cmd ="";
  int setLightTime_day = -1;
  int setStatus = -1;
  String setLightTime_hour = "";
  String systemParam = "";
  String jsonAnswer = "";
  int relaisPosition = 0;
  int relaisValue = 0;
  
  if(getAnswer.substring(0,2) == "/?") {
    urlParameters = getAnswer.substring(2,getAnswer.length()-1);
    printDebugln(getAnswer);
    for(int i=0; i<urlParameters.length();i++) {
      if(urlParameters.charAt(i) == '&') {
        countChunk++;
      }
    }


    for(int j=0; j<=countChunk; j++) {
      chunkPosition = urlParameters.indexOf('&');
      chunk = urlParameters.substring(0,chunkPosition);

      key = chunk.substring(0,chunk.indexOf('='));
      value = chunk.substring(chunk.indexOf('=')+1,chunk.length());


      if(key == "cmd") {
        cmd=value;
      }

      if(key == "day") {
        setLightTime_day=value.toInt();
      }

      if(key == "status") {
        setStatus=value.toInt();
      }

      if(key == "hour") {
        setLightTime_hour=value;
      }

      if(key == "param") {
        systemParam=value;
      }

      if(key == "relaisPosition") {
        relaisPosition=value.toInt();
      }

      if(cmd == "clearLigthTime") {
        printDebugln("Reset Heure allumage de " + String(EEPROM_POSITION_LIGHT_TIME) + " à " + String(7*ON*LIGHT_TIME_STRUCT_LENGTH*2),DEBUG);
        jsonAnswer = clearROM(EEPROM_POSITION_LIGHT_TIME, 7*ON*LIGHT_TIME_STRUCT_LENGTH*2);
      }


      
      urlParameters = urlParameters.substring(chunkPosition+1,urlParameters.length());
    }



    if(cmd=="debugLevel") {
      debugLevel = systemParam.toInt();
      jsonAnswer = "{\"debugLevel\"=\"" + systemParam +"\"}";
      printDebugln("debugLevel : " + systemParam ,FORCE);
    }

    if(cmd == "setRelais") {
      printDebugln("set Relais : " + String(relaisPosition) + " à " + String(setStatus),DEBUG);

      switch (relaisPosition) {
        case 1: 
          printDebugln("relais 1",DEBUG);
          RELAIS_PIN_1_status = setStatus;
          writeValROM(EEPROM_POSITION_RELAIS_STATUS_1, setStatus);
          break;
        case 2: 
          printDebugln("relais 2",DEBUG);
          RELAIS_PIN_2_status = setStatus;
          writeValROM(EEPROM_POSITION_RELAIS_STATUS_2, setStatus);
          break;
        case 3: 
          printDebugln("relais 3",DEBUG);
          RELAIS_PIN_3_status = setStatus;
          writeValROM(EEPROM_POSITION_RELAIS_STATUS_3, setStatus);
          break;
        case 4: 
          printDebugln("relais 4",DEBUG);
          RELAIS_PIN_4_status = setStatus;
          writeValROM(EEPROM_POSITION_RELAIS_STATUS_4, setStatus);
          break;  
      
      }
      
      jsonAnswer = "{\"Relais\":\"" + String(setStatus) + "\"}";
      
    
    } else if(cmd == "setLightStatus") {
      Aqua_lightStatus = setStatus;
      jsonAnswer = "{\"lightStatus\":\"" + String(Aqua_lightStatus) + "\"}";
      printDebugln("setLightStatus" + String(Aqua_lightStatus),DEBUG);
      writeValROM(EEPROM_POSITION_LIGHT_STATUS, Aqua_lightStatus);
    
    } else if(cmd == "getLightStatus") {
      jsonAnswer = "{\"lightStatus\":\"" + String(Aqua_lightStatus) + "\"}";
      
    } else if(cmd == "setLightTime") {
      jsonAnswer = setLightTime(setLightTime_day, setStatus, setLightTime_hour);
    
    } else if(cmd == "getLightTime") {
      printDebugln("Récupération de l'heure demandé : " + String(setLightTime_day) + " " + String(setStatus),DEBUG);
      jsonAnswer = getLightTime(setLightTime_day,setStatus);
    
    } else if(cmd == "getInfo") {
      
      if(systemParam == "dateTime") {
        jsonAnswer = "{\"dateTime\":\"" + Aqua_dateTime + "\"}";
   
      } else if(systemParam == "tempHaut") {
        jsonAnswer = "{\"tempHaut\":\"" + String(Aqua_tempHaut) + "\"}";
      
      } else if(systemParam == "tempBas") {
        jsonAnswer = "{\"tempBas\":\"" + String(Aqua_tempBas) + "\"}";
      
      } else if(systemParam == "tempInt") {
        jsonAnswer = "{\"tempInt\":\"" + String(Aqua_tempInt) + "\"}";
      
      } else if(systemParam == "distance") {
        jsonAnswer = "{\"distance\":\"" + String(Aqua_Distance) + "\"}";
      
      } else if(systemParam == "tdsValue") {
        jsonAnswer = "{\"tdsValue\":\"" + String(Aqua_tdsValue) + "\"}";
      
      }else if(systemParam == "pHValue") {
        jsonAnswer = "{\"pHValue\":\"" + String(Aqua_pHValue) + "\"}";
      
      } else {
        jsonAnswer = "{\"lightStatus\":\"" + String(Aqua_lightStatus) + "\",\"dateTime\":\"" + Aqua_dateTime + "\",\"tempHaut\":\"" + String(Aqua_tempHaut) + "\",\"tempBas\":\"" + String(Aqua_tempBas) + "\",\"Aqua_tempInt\":\"" + String(Aqua_tempInt) + "\",\"distance\":\"" + String(Aqua_Distance) + "\",\"tdsValue\":\"" + String(Aqua_tdsValue) + "\",\"pHValue\":\"" + String(Aqua_pHValue) + "\", \"relaisStatus_1\":\"" + RELAIS_PIN_1_status + "\", \"relaisStatus_2\":\"" + RELAIS_PIN_2_status + "\", \"relaisStatus_3\":\"" + RELAIS_PIN_3_status + "\", \"relaisStatus_4\":\"" + RELAIS_PIN_4_status + "\"}";
      
      }
    }
  }

  return jsonAnswer;
}
// Fin readURL()



/////////////////////////////////////////////////////////////////////////
// Affiche dans la console la consigne lumière
/////////////////////////////////////////////////////////////////////////
String getLightTime (int getLightTime_day, int getLightTime_status) {
    
  int posMem = EEPROM_POSITION_LIGHT_TIME + (2*getLightTime_day + getLightTime_status)  * LIGHT_TIME_STRUCT_LENGTH;
    
  printDebugln(String(readValROM(posMem), DEC),DEBUG);
  printDebugln(String(readValROM(posMem+1), DEC),DEBUG);
  printDebugln(String(readValROM(posMem+2), DEC),DEBUG);
  printDebugln(String(readValROM(posMem+3), DEC),DEBUG);
  printDebugln(String(readValROM(posMem+4), DEC),DEBUG);

  return "{response:OK}";  //ToDo renvoyer la reponse attendue
}
// Fin getLightTime()



/////////////////////////////////////////////////////////////////////////
// Enregistre dans l'EEPROM un timer lumière
/////////////////////////////////////////////////////////////////////////
String setLightTime (int setLightTime_day, int setStatus, String setLightTime_hour) {
  int posMem = EEPROM_POSITION_LIGHT_TIME + (2*setLightTime_day + setStatus)  * LIGHT_TIME_STRUCT_LENGTH;

  writeValROM(posMem, setLightTime_day);
  writeValROM(posMem+1, setStatus);
  writeValROM(posMem+2, setLightTime_hour.substring(0,2).toInt());
  writeValROM(posMem+3, setLightTime_hour.substring(3,5).toInt());
  writeValROM(posMem+4, setLightTime_hour.substring(6,8).toInt());

  
  return "{response: complet: " + setLightTime_hour + " - H:" + setLightTime_hour.substring(0,2) + " - M:" + setLightTime_hour.substring(3,5) + "}";
}
// Fin setLightTime()



/////////////////////////////////////////////////////////////////////////
// Ecrit debug String sur la sortie Serial SANS retour à la ligne
/////////////////////////////////////////////////////////////////////////
void printDebug(String message, int level = 1) {
  if(level >= debugLevel) {
    Serial.print(message);
  }
}



/////////////////////////////////////////////////////////////////////////
// Ecrit debug String sur la sortie Serial AVEC retour à la ligne
/////////////////////////////////////////////////////////////////////////
void printDebugln(String message, int level = 1) {
  
  
  if(level >= debugLevel) {
    Serial.println(message);
    
  }
}
// Fin printDebugln()



/////////////////////////////////////////////////////////////////////////
// Ecriture des logs sur la carte SD
/////////////////////////////////////////////////////////////////////////
void logData(String file, String message) {

  // Creation de la chaine a inscrire
  String fullMessage = "";

  fullMessage = Aqua_dateTime + ":" + message;
  if(cardOK) {
    File dataFile = SD.open(file, FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(message);
      dataFile.close();
    }
    // if the file isn't open, pop up an error:
    else {
      printDebugln("error opening " + file,ERROR);
    }
  }
}
// Fin logData()



void replyJson(EthernetClient client, String message) {
  client.print(message);
}
// Fin replyJson()






//-----------------------------------------------------
//-----------------------------------------------------
// Heure par NTP
//-----------------------------------------------------
//-----------------------------------------------------

///////////////////////////////////////////////////////
// Configuration de l'heure système depuis un NTP
///////////////////////////////////////////////////////
void setTimerNTP () {
  
  EthernetUDP Udp;
  int counter=0;
  bool timeSet = false;
  // Récupération de l'heure
  Udp.begin(localPort);
  do {
    sendNTPpacket(timeServerAddress, Udp); // send an NTP packet to a time server
    delay(1000);
    Udp.read(packetBuffer, NTP_PACKET_SIZE); 
    if (Udp.parsePacket()) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
     
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears + GMT * SECS_PER_HOUR;

      Year = year(epoch);
      Month = month(epoch);
      Day = day(epoch);
      Hour = hour(epoch);
      Minute = minute(epoch);
      Second = second(epoch);
      
      tm.Year = CalendarYrToTm(Year);
      tm.Month = Month;
      tm.Day = Day;
      tm.Hour = Hour;
      tm.Minute = Minute;
      tm.Second = Second;

      Udp.stop();
      timeSet = true;
      RTC.write(tm);

    }
    if(!timeSet) {
      printDebug("...",DEBUG); 
    }
    
  } while (!timeSet);
}
// Fin setTimerNTP()



///////////////////////////////////////////////////////
// Envoie d'une requete NTP au server
///////////////////////////////////////////////////////
void sendNTPpacket(const char * address, EthernetUDP Udp) {
  int counter=0;
  
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
  
}
// Fin sendNTPpacket()









///////////////////////////////////////////////////////
// Affiche un nombre sur 2 digits prefixé de 0 si besoin
///////////////////////////////////////////////////////
String print2digits(int number) {
  String tempString = String(number,DEC);
  if (number >= 0 && number < 10) {
    tempString = '0' + tempString;
  }
  return tempString;
}
// Fin print2digits()



///////////////////////////////////////////////////////
// Renvoie la moyenne d'un tableau
///////////////////////////////////////////////////////
double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    printDebugln("Erreur de position pour le tableau des moyenne",ERROR);
    return 0;
  }
  if(number<5){   
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}
// Fin avergearray()



///////////////////////////////////////////////////////
// Récupère la valeur médiane d'un tableau
///////////////////////////////////////////////////////
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) 
  {
    for (i = 0; i < iFilterLen - j - 1; i++) 
    {
      if (bTab[i] > bTab[i + 1]) 
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
// Fin getMedianNum()







//-----------------------------------------------------
//-----------------------------------------------------
// Stockage local des données
//-----------------------------------------------------
//-----------------------------------------------------

///////////////////////////////////////////////////////
// Effacer l'EEPROM
///////////////////////////////////////////////////////
String clearROM(int startAddress, int stopAddress) {
  printDebug("Cleaning de l'EEPROM ", DEBUG);
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  printDebugln("...OK", DEBUG);
  return "{response:OK}";
}
// Fin clearROM()



///////////////////////////////////////////////////////
// Ecrire une valeur dans l'EEPROM
///////////////////////////////////////////////////////
void writeValROM(int address, int value ) {
  EEPROM.write(address, value);
  printDebugln("Ecriture de la valeur " + String(value) + " à l'adresse ", DEBUG);
}
// Fin writeValROM()



///////////////////////////////////////////////////////
// Lire une valeur dans l'EEPROM
///////////////////////////////////////////////////////
int readValROM(int address) {
  int value;
  value = EEPROM.read(address);
  printDebugln("Lecture de la valeur à l'adresse 0x" + String(address) + " : " + String(value), DEBUG);
  return value;
}
// Fin readValROM()
