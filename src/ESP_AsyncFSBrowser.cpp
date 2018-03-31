//WIFI
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>

//WEB SERVER
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
//#include <ArduinoOTA.h>
#include <Hash.h>

//JSON SUPPORT
#include <ArduinoJson.h>

//SMART LED
#include <Adafruit_NeoPixel.h>
#define PIXEL_PIN    15
#define PIXEL_COUNT  8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

//CUSTOM HEADERS
#include <color.h>
#include <ssid.h>

// HTTP SERVER AND WEBSOCKET SERVER
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//COLORQUEUE
uint32_t messageId = 1;
ColorQueue Queue[500];

//WEBSOCKET CALLBACK
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  //IF CLINET IS FIRST CONNECTIONG
  //SEND THEM THEIR CLIENT ID
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    String json;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["id"] = String(client->id());
    root["action"] = "connected";
    root.printTo(json);
    client->text(json);

  //FUNCTION THAT GETS CALLES IF CLIENT DISCONNECTS
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //IF THE MESSAGE IS OF TYPE TEXT
      //LOOP THROUGH THE DATA AND ADD
      //TO THE MESSAGE STRING
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

    //PARSE THE JSON MESSAGE THAT
    //CAME FROM THE CLIENT
    DynamicJsonBuffer jsonReadBuffer;
    JsonObject& rootRead = jsonReadBuffer.parseObject(msg);
    String action = rootRead["action"];

    //USE THE ACTION PROPERTY
    //FROM THE MESSAGE TO DETERMINE WHAT
    //TO DO WITH THE MESSAGE
    if(action == "message"){
      //THE MESSAGE ACTION IS MESSAGE
      //SEND ALL CLIENTS THE MESSAGE BACK
      //FOR THE CHAT
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
      //IF THE ACTION IS COLOR
      //PARSE THE DATA INTO AN OBJECT AND
      //ADD IT TO THE QUEUE
      uint8_t r = rootRead["data"]["color"]["r"];
      uint8_t g = rootRead["data"]["color"]["g"];
      uint8_t b = rootRead["data"]["color"]["b"];
      uint32_t d = rootRead["data"]["duration"];
      if(d <= 10){
        Queue[messageId] = ColorQueue(r,g,b,d,messageId);

        Serial.printf("R: %d G: %d B: %d Duration: %d \n",r,g,b,d);

        //SEND MESSAGE TO EVERYONE CONTAINING THE COLOR INFORMATION
        rootRead["id"] = client->id();
        rootRead["messageid"] = messageId;

        String json;
        rootRead.printTo(json);
        ws.textAll(json);

        //INCREMENT THE MESSAGEID
        messageId++;
      }

    }
  }
}
}

//SSID INFORMATION NORMALLY GOES HERE
//THIS IS JUST AN EXAMPLE
const char* example_ssid = "ssid";
const char* example_password = "password";
const char* hostName = "esp32";
const char* http_username = "admin";
const char* http_password = "admin";


//THIS FUNCTION READS EVERY FILE ON THE FLASH
//MEMORY AND LISTS THEIR NAMES, PATH, AND SIZE
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

//BEGIN THE MAIN PROGRAM
void setup(){
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  //START THE WIFI
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(hostName);
  WiFi.begin(ssid, password);

  Serial.println("Connecting Wifi");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }

  Serial.print("IP Address: ");Serial.println(WiFi.localIP());

  if (!MDNS.begin(hostName)) {
    Serial.println("Error setting up MDNS responder!");
        delay(1000);
  }


  MDNS.addService("http","tcp",80);

  //INITIALIZE THE FLASH MEMORY TO SERVE WEBPAGES FROM
  Serial.println(SPIFFS.begin());
  listDir(SPIFFS, "/", 0);

  //ADD WEBSOCKET CALLBACK FUNCTION FOR DATA
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  //SERVES THE MOBILE IDE WITH A USERNAME AND PASSWORD
  server.addHandler(new SPIFFSEditor(SPIFFS,http_username,http_password));

  //THIS FUNCTION ROUTES ALL BASE DIRECTORY
  //WEB REQUESTS TO THE WWW FOLDER
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  server.serveStatic("/www/", SPIFFS, "/www/").setDefaultFile("index.html");

  //THIS FUNCTION SERVERS THE ASSETS FILES SO THEY WILL BE CACHED
  //THE FILE IS QUITE LARGE SO WE DON'T WANT THE CLIENTS TO KEEP REQUESTING
  server.serveStatic("/assets/", SPIFFS, "/assets/").setCacheControl("max-age=6000");

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

  //START THE HTTP SERVER
  Serial.println("HTTP Server Starting");
  server.begin();
  Serial.println("HTTP Server Started");

  //START THE LEDS
  Serial.println("Setting up LEDS");
  strip.setBrightness(50);
  strip.begin();
  strip.show();
  Serial.println("LEDS Setup Complete");

}

//QUEUE INFORMATION
uint16_t processedId = 1;
unsigned long lastCheck = 0;

//FUNCTION CHANGES THE LED COLORS FOR THE WHOLE STRIP
void changeLedColor(uint8_t r,uint8_t g, uint8_t b){
  for (int i = 0;i<PIXEL_COUNT;i++){
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
}
//FUNCTION TO RUN TO CHECK IF THE QUEUE NEEDS TO BE PROCESSED
void queueHandler(){
  uint8_t duration = Queue[processedId]._duration;
  unsigned long now = millis();

  if(processedId < messageId){
    ColorQueue currentMessage = Queue[processedId];
    changeLedColor(currentMessage._r, currentMessage._g, currentMessage._b);

    if(lastCheck == 0){
      lastCheck = millis();
    }

    //CHECKS IF THE LAST TIME WE CHECKED WAS GREATER THAN THE
    //DURATION THAT THE COLOR WAS SUPPOSED TO BE SET FOR
    if((now - lastCheck) > (duration * 1000)){
      //THIS CODE GETS HIT ONCE THE TOTAL DURATION
      //FOR THE COLOR MESSAGE HAS BEEN REACHED
      //ALERTS ALL CONNECTED CLIENTS THAT THE MESSAGE
      //PROCESSED
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
  //IF NO QUEUE MESSAGES SET STRIP TO HAVE NO COLOR
  changeLedColor(0, 0, 0);
}

//THIS CODE RUNS EVERY AVAILABLE CHANCE
void loop(){

  //SAVE CPU TIME TO ONLY RUN CALLBACK WHEN WE NEED
  //TO NOT BLOCK THE CPU
  if(processedId < messageId){
    queueHandler();
  }
  else{
    lastCheck=millis();
  }


}
