#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <ArduinoOTA.h>
#include <Hash.h>

#include <ArduinoJson.h>

#include <color.h>

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
uint32_t messageId = 1;
ColorQueue Queue[500];





void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());

    String id = String(client->id());
    String json;

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["id"] = id;
    root["action"] = "connected";
    root.printTo(json);
    client->text(json);

  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      //Serial.printf("[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }

      //Serial.printf("%s\n",msg.c_str());

      if(msg == "heap"){
        String heap = String(ESP.getFreeHeap());
        client->text(heap);
      }

    DynamicJsonBuffer jsonReadBuffer;
    JsonObject& rootRead = jsonReadBuffer.parseObject(msg);
    String action = rootRead["action"];


    if(action == "message"){
      String user = rootRead["user"];
      String msg = rootRead["msg"];
      Serial.printf("User: %S Message:%S \n",user.c_str(),msg.c_str());

      String json;
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();

      root["name"] = user;
      root["action"] = "message";
      root["id"] = client->id();
      root["msg"] = msg;
      root.printTo(json);

      ws.textAll(json);
    }
    if(action == "color"){
      uint8_t r = rootRead["data"]["color"]["r"];
      uint8_t g = rootRead["data"]["color"]["g"];
      uint8_t b = rootRead["data"]["color"]["b"];
      uint8_t d = rootRead["data"]["duration"];
      Serial.printf("R: %d G: %d B: %d Duration: %d \n",r,g,b,d);

      rootRead["id"] = client->id();
      rootRead["messageid"] = messageId;

      String json;
      rootRead.printTo(json);
      ws.textAll(json);

      Queue[messageId] = ColorQueue(r,g,b,d,messageId);
      //Serial.println(ESP.getFreeHeap());
      //Serial.println(Queue[messageId]._messageId);

      messageId++;
    }

  }
}
}


const char* ssid = "SSID";
const char* password = "PASSWORD";
const char * hostName = "esp32";
const char* http_username = "admin";
const char* http_password = "admin";


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}



void setup(){
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  //WiFi.hostname(hostName);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(hostName);
  WiFi.begin(ssid, password);
  Serial.println("COnnecting Wifi");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }

  Serial.println(WiFi.localIP());
  if (!MDNS.begin(hostName)) {
    Serial.println("Error setting up MDNS responder!");
        delay(1000);
  }

  //Send OTA events to the browser
  ArduinoOTA.onStart([]() { });
  ArduinoOTA.onEnd([]() {  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));

  });
  ArduinoOTA.onError([](ota_error_t error) {
  });
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();

  MDNS.addService("http","tcp",80);

  Serial.println(SPIFFS.begin());


  listDir(SPIFFS, "/", 0);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);


  server.addHandler(new SPIFFSEditor(SPIFFS,http_username,http_password));

  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  server.serveStatic("/www/", SPIFFS, "/www/").setDefaultFile("index.html");
  server.serveStatic("/assets/", SPIFFS, "/assets/").setCacheControl("max-age=6000");
  //server.serveStatic("/", SPIFFS, "/").setCacheControl("max-age=6000");
  //server.serveStatic("/bootstrap.min.js",SPIFFS,"/bootstrap.min.js").setCacheControl("max-age=600");

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }



    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char*)data);
    if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });

  server.begin();
}

uint16_t processedId = 1;
unsigned long lastCheck = 0;

void queueHandler(){
  uint8_t duration = Queue[processedId]._duration;
  unsigned long now = millis();

  if(processedId < messageId){

    if(lastCheck == 0){
      lastCheck = millis();
    }

    if((now - lastCheck) > (duration * 1000)){
      Serial.printf("Processed Queue ID: %d \n",processedId);
      lastCheck = millis();


      String json;
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();

      root["messageid"] = processedId;
      root["action"] = "processed";
      root.printTo(json);
      ws.textAll(json);

      processedId++;
    }
  }
}

void loop(){
  ArduinoOTA.handle();
  if(processedId < messageId){
  queueHandler();
}else{lastCheck=millis();}


}
