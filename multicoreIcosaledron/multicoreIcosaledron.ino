#include <WiFi.h>
#include <ArduinoOTA.h>
// fft\\ #include "arduinoFFT.h"

// fft\\ arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
#define CHANNEL 2 //A0

TaskHandle_t Task1;
TaskHandle_t Task2;


//led defines
#include <FastLED.h>
#define DATA_PIN    4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    300
CRGB leds[NUM_LEDS];
#define BRIGHTNESS          80
#define FRAMES_PER_SECOND  120
const int Knob1 = 35; // left
const int Knob2 = 34; //right


//led vars
int corNum = 12;
int corners[12];
int cornerSides[12][5] = {{0,2,3,5,6}, {0,1,18,11,10}, {2,1,17,23,22}, {3,22,21,27,4}, {4,5,28,8,7}, {7,9,12,10,6}, {13,15,16,11,12}, {16,19,24,17,18}, {20,26,21,23,24}, {29,28,27,26,25}, {14,13,9,8,29}, {20,19,15,14,25}};
int cornerSidesDir[12][5] ={{1,0,1,0,1},{0,1,1,1,0},{1,0,0,1,0},{0,1,0,1,1},{0,0,0,1,0},{1,0,1,1,0},{1,0,1,0,0},{0,1,0,1,0},{0,0,1,0,1},{0,1,0,1,0},{1,0,1,0,1},{1,0,1,0,1}};
int sideNum = 30;
int ledPerSide = 10;
int side[30][10];
int cornerSideLeds[12][5][10];
int h2 = 0;
int soundsides[30];
int knob1Val = 0;
int knob2Val = 0;


// FFT stuff

// fft\\ const uint16_t samples = 32; //This value MUST ALWAYS be a power of 2
// fft\\ const double samplingFrequency = 90000; //Hz, must be less than 10000 due to ADC

// fft\\ unsigned int sampling_period_us;
// fft\\ unsigned long microseconds;

/*
These are the input and output vectors
Input vectors receive computed results from FFT
*/
// fft\\ double vReal[samples];
// fft\\ double vImag[samples];

// fft\\ int highest = 1; // highest volume
// fft\\ int lowest = 1; // lowest volume
// fft\\ int runcount = 0;
// fft\\ #define SCL_INDEX 0x00
// fft\\ #define SCL_TIME 0x01
// fft\\ #define SCL_FREQUENCY 0x02
// fft\\ #define SCL_PLOT 0x03

//wifi stuff
WiFiServer server(80);


String responseHTML = ""
  "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
  "<h1>Hello World!</h1><p>This is a captive portal example. All requests will "
  "be redirected here.</p></body></html>";



void setup() {
  delay(3000); // 3 second delay for recovery
  // fft\\ sampling_period_us = round(1000000*(1.0/samplingFrequency));
  Serial.begin(115200); 
  Serial.println("Booting");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("IcosaLEDron");
  Serial.print("local IP address: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
  // Port defaults to 3232
  ArduinoOTA.setPort(3232);
  // led setup
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  //cornerSetup();
  // -- Create the FastLED show task
  // xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, &FastLEDshowTaskHandle, FASTLED_SHOW_CORE);

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}

//Task1code: blinks an LED every 1000 ms
int otaMode = 0;
int runcount = 0;
void Task1code( void * pvParameters ){
  // core0 loop goes here network interaction
  for(;;){
    knob1Val = analogRead(Knob1);
    knob2Val = analogRead(Knob2);
  delay(500);
  Serial.println("running core 0");
    if ((knob1Val == 4095 && knob2Val == 4095) || otaMode == 1) {
       ArduinoOTA.handle();
    } else {
      
      // run the server
      //dnsServer.processNextRequest();
      WiFiClient client = server.available();   // listen for incoming clients

      if (client) {
        String currentLine = "";
        while (client.connected()) {
          if (client.available()) {
            char c = client.read();
            if (c == '\n') {
              if (currentLine.length() == 0) {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/html");
                client.println();
                client.print(responseHTML);
                break;
              } else {
                currentLine = "";
              }
            } else if (c != '\r') {
              currentLine += c;
            }
          }
        }
        client.stop();
      }
    }
  } 
}

//Task2code: blinks an LED every 700 ms
void Task2code( void * pvParameters ){
  // core1 loop goes here flashy lights bits
  for(;;){
    runningLight();
  }
}

void loop() {
  // pewpew
}




void runningLight() {
  int h = 0;
  int s = 255; 
  int v = 200;
    for(int dot = 0; dot < NUM_LEDS; dot++) { 

      if ( h < 240) {
        h = h + 10;
      } else {
        h = 0;
      }
        leds[dot].setHSV( h, s, v);
        FastLED.show();
        // clear this led for the next time around the loop
        leds[dot] = CRGB::Black;
        delay(30);
    }
    s = 255;
    v = 250;
}
