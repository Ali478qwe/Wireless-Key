#include <WebServer.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <EEPROM.h>

#define EEPROM_BYTE 97

unsigned long last_bt_ac = 0;
const unsigned long delay_ac = 500;


String name;
String pass;
const char* ssid ;
const char* password ;
const char* file_name = "/aindex.html";


bool block;
const uint8_t TOUCH_PIN = 13;
const uint8_t LED = 14;

bool last_read = 1;
bool LED_State = false;
volatile bool flag = false;






void initSPIFFS();
void set_header();
void Handle_Poll();
void Handle_Send();
void ICACHE_RAM_ATTR TOUCH_READ() {flag = true;}
void set_str();
String get_str();

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

String message = "";
String state = "";







void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_PIN, INPUT_PULLUP);

  pinMode(LED, OUTPUT);

  EEPROM.begin(EEPROM_BYTE);  //1 byte
  uint8_t save = EEPROM.read(0);

  if (save != 0 && save != 1) {
    EEPROM.write(0, false);
    EEPROM.commit();
  }
  //defualt
  if (get_str(1).length() == 0) {set_str(1, "KEY");}
  if (get_str(32).length() == 0) {set_str(32, "Ali478qwe");}
  
  name = get_str(1);
  pass = get_str(32);
  ssid = name.c_str();
  password = pass.c_str();  
  initSPIFFS();
  
   //Serial.println(name);
   //Serial.println(pass);
  //WiFi.softAPConfig(local_IP, gateway, subnet);
  
  WiFi.softAP( ssid , password);//  get_str(1),get_str(32)


  attachInterrupt(TOUCH_PIN,TOUCH_READ,HIGH);


  server.serveStatic("/", SPIFFS, file_name);
  server.on("/poll", Handle_Poll);
  server.on("/send", Handle_Send);

  server.begin();
}

void loop() {
  server.handleClient();
  
  LED_State = EEPROM.read(0);
  message = LED_State ? "on" : "off";  
  digitalWrite(LED, LED_State ? HIGH : LOW);
 

  if (flag) {
    Serial.println("flag active");
    flag = false;
    Serial.println("after flag");
    Serial.println(LED_State ? "flag HIGH": "flag LOW");  
    block = true; 
    last_bt_ac =  millis(); 
    Serial.println(LED_State);       
    LED_State =  EEPROM.read(0);
    LED_State = !LED_State;  
    Serial.println(LED_State); 
    EEPROM.write(0, LED_State);
    EEPROM.commit();
     Serial.println(LED_State ? "flag HIGH": "flag LOW");
    message = LED_State ? "on" : "off";
  }

  if(block && millis() - last_bt_ac > delay_ac){
    block = false;
  }
   
  //Serial.println(WiFi.softAPIP());

    if (state != "" && (state ==  "on" || state == "off") && !block  && state != message) {
      EEPROM.write(0, state == "on" ? true : false);
      EEPROM.commit();
    }

  

  


  // delay(10);
}








void initSPIFFS() {
  if (!SPIFFS.begin(false)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  if (SPIFFS.exists(file_name)) {
    Serial.println("file exists");
  } else {
    Serial.println("file not found");
  }
}


void Handle_Poll() {
  set_header();
  server.send(200, "text/plain", message);
  Serial.println("server res: " + message);
}

void Handle_Send() {
  
  //set_header();
  if (server.hasArg("msg")) {
    
    state = server.arg("msg");
     IPAddress Client_IP = server.client().remoteIP();   
    server.send(200,"text/plain","NETWORK NAME:" + name + "@Your IP:" + Client_IP.toString());
    
    //Serial.println(Client_IP.toString());
    
    
    if (state == message) {

      server.send(200, "text/plain", "none");

    } else {

      Serial.println("client message: " + state);
      server.send(200, "text/plain", "Server Recived Request: OK");
      
    }
    if (state == "reset") {
      set_str(1, "KEY");
       ESP.restart();      
    }
  } else {
    server.send(400, "text/plain", "parmeter msg not found");
  }
  if (server.hasArg("name") && server.hasArg("password")) { 
    String new_name = server.arg("name");
    String new_pass = server.arg("password");
    //new_name.trim();
    new_pass.trim();
    set_str(1,new_name);
    set_str(32,new_pass);
     ESP.restart();    
  }
}

void set_header() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
}

void set_str(int address, const String &def) {
  EEPROM.begin(EEPROM_BYTE);
  EEPROM.write(address,0xAA);
  EEPROM.put(address + 1, def);
  EEPROM.commit();
}

String get_str(int address){
    EEPROM.begin(EEPROM_BYTE);
    if(EEPROM.read(address) != 0xAA){
        return "";      
    }
    String data; 
    EEPROM.get(address + 1,data);  
    return data;
}
