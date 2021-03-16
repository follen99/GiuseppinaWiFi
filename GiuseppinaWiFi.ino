#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const int relayPin=D0;

const int moisturePin=A0;
const int airValue=636;
const int waterValue=288;

//tempo
const unsigned long SECOND = 1000;
const unsigned long MINUTE = 60000;
const unsigned long HOUR = 3600*SECOND;

int soilMoisture=0;
int soilMoisturePercent=0;

int wetPlantInterval=30*MINUTE;    //periodo ogni quanto viene innaffiata la pianta, OGNI MEZZ' ORA
int totalWaterInterval=12*HOUR; //12 ore
unsigned long previousTime=0;

int totalWaterCount=0;               //numero totale di volte che ho innaffiato
int lastTwelveHours=0;



//######################## SERVER STUFF ##########################
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 28800;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
unsigned long epochTime = timeClient.getEpochTime();
struct tm *ptm = gmtime ((time_t *)&epochTime);

const char* ssid = "WIFI SSID";  
const char* password = "WIFI PASSWORD"; 

ESP8266WebServer server(80);

String SendHTML(float moistureWeb,float totalWaterWeb, String TimeWeb, String DateWeb);
void handle_OnConnect();
void handle_NotFound();

float moisture;
float totalWater;
String formattedTime;
String Date;
int Day;
int Month;
int Year;

void setup() {
  Serial.begin(9600);        

  pinMode(relayPin,OUTPUT);
  digitalWrite(relayPin,HIGH);
  
  Serial.println("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  } 
  Serial.println("");
  Serial.println("Connected to WiFi");
  Serial.print("IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  timeClient.begin();
  timeClient.setTimeOffset(3600);
}
void loop() {
  readMoisturePercent();
  delay(200);
  server.handleClient();
  
  
  if(soilMoisturePercent < 20){
    if(canIWet()){
      annaffia();
      handleWet();
      //Serial.print("Annaffio!");
    }
  }
  
  //delay(2000);
}

void handle_OnConnect() {
  timeClient.update();
 
  unsigned long epochTime = timeClient.getEpochTime(); 
  String formattedTime = timeClient.getFormattedTime();
  
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
 
  formattedTime = timeClient.getFormattedTime(); 
  //Date = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  
  Date = String(monthDay) + "-" +String(currentMonth)+ "-" + String(currentYear);
  moisture = soilMoisturePercent;
  totalWater = totalWaterCount;
  server.send(200, "text/html", SendHTML(moisture,totalWater,formattedTime,Date)); 


}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float moistureWeb,float totalWaterWeb, String TimeWeb,String DateWeb){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Giuseppina</title>\n";

  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>Pannello di controllo Giuseppina</h1>\n";

  ptr +="<p>Data: ";
  ptr +=(String)DateWeb;
  ptr +="</p>";
  ptr +="<p>Orario: ";
  ptr +=(String)TimeWeb;
  ptr +="</p>";
  ptr +="<p>Umidita' Terreno: ";
  ptr +=(int)moistureWeb;
  ptr +=" %</p>";
  ptr +="<p>Ho innaffiato la pianta per un totale di ";
  ptr +=(int)totalWaterWeb;
  ptr +=" Volte.</p>";
  
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
/*
 * Calcola il valore in percentuale
 * dell'umidità del terreno
**/
void readMoisturePercent(){
  soilMoisture=analogRead(moisturePin);
  soilMoisturePercent=map(soilMoisture,airValue,waterValue,0,100);

  if(soilMoisturePercent>100){
    soilMoisturePercent=100;
  }else if(soilMoisturePercent<0){
    soilMoisturePercent=0;
  }
}
/**
 * funzione che automatizza il processo
 * di "innaffiamento"
*/
void annaffia(){
  digitalWrite(relayPin,LOW);
  delay(1000);
  digitalWrite(relayPin,HIGH);
  delay(1000);
}

bool canIWet(){
  unsigned long currentTime=millis();
  if(currentTime-previousTime >= wetPlantInterval){
    previousTime=currentTime;
    return true;
  }
  return false;
}
void handleWet(){
  //incremento le volte che innaffio
  totalWaterCount+=1;
  lastTwelveHours+=1;
}
