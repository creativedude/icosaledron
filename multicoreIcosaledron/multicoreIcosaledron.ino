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

String header;
String otaMode = "off";
String lightMode = "off";

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

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
int runcount = 0;
int lightModeInt = 0;
void Task1code( void * pvParameters ){
  // core0 loop goes here network interaction
  for(;;){
    knob1Val = analogRead(Knob1);
    knob2Val = analogRead(Knob2);
  delay(500);
  Serial.println("running core 0");
    if ((knob1Val == 4095 && knob2Val == 4095) || otaMode == "on") {
       ArduinoOTA.handle();
    } else {
      
      WiFiClient client = server.available();   // Listen for incoming clients

      if (client) {                             // If a new client connects,
        Serial.println("New Client.");          // print a message out in the serial port
        String currentLine = "";                // make a String to hold incoming data from the client
        currentTime = millis();
        previousTime = currentTime;
        while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
          currentTime = millis();         
          if (client.available()) {             // if there's bytes to read from the client,
            char c = client.read();             // read a byte, then
            Serial.write(c);                    // print it out the serial monitor
            header += c;
            if (c == '\n') {                    // if the byte is a newline character
              // if the current line is blank, you got two newline characters in a row.
              // that's the end of the client HTTP request, so send a response:
              if (currentLine.length() == 0) {
                // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                // and a content-type so the client knows what's coming, then a blank line:
                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/html");
                client.println("Connection: close");
                client.println();
                
                // turns the GPIOs on and off
                if (header.indexOf("GET /1/on") >= 0) {
                  Serial.println("GPIO 5 on");
                  otaMode = "on";
                } else if (header.indexOf("GET /1/off") >= 0) {
                  Serial.println("GPIO 5 off");
                  otaMode = "off";
                } else if (header.indexOf("GET /2/on") >= 0) {
                  Serial.println("GPIO 4 on");
                  lightMode = "on";
                } else if (header.indexOf("GET /2/off") >= 0) {
                  Serial.println("GPIO 4 off");
                  lightMode = "off";
                } else if (header.indexOf("GET /lightmode0") >= 0) {
                  lightModeInt = 0;
                } else if (header.indexOf("GET /lightmode1") >= 0) {
                  lightModeInt = 1;
                } else if (header.indexOf("GET /lightmode2") >= 0) {
                  lightModeInt = 2;
                }
                
                // Display the HTML web page
                client.println("<!DOCTYPE html><html>");
                client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                client.println("<link rel=\"icon\" href=\"data:,\">");
                // CSS to style the on/off buttons 
                // Feel free to change the background-color and font-size attributes to fit your preferences
                client.println("        <style> html { font-family: Helvetica; color:#fff; display: inline-block; margin: 0px auto; text-align: center; background-color:#234;}.button {background-color: #195B6A; border: none; color: white; padding: 16px 40px;text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}.button2 {background-color: #77878A;}.cornerbtn {display:block;position:absolute;width:35px;height:35px;line-height:25px;text-align:center;background-color:#bada55;font-weight:bold;font-size:18px;color:#333;border:0px solid transparent;border-radius:100%;padding:0;}.cornerbtn:hover{background-color:#55bada}.icosaledron-container {position: relative;width:320px;height:371px;margin:30px auto;}.icosaledron-container svg {position: absolute;left:0;right:0;}#cbtn0 {left:142px;top:340px;}#cbtn1 {left:-5px;top:255px;}#cbtn2 {left:50px;top:230px;}#cbtn3 {left:235px;top:230px;}#cbtn4 {left:290px;top:255px;}#cbtn5 {left:142px;top:275px;}#cbtn6 {left:50px;top:110px;}#cbtn7 {left:-5px;top:85px;}#cbtn8 {left:142px;top:65px;}#cbtn9 {left:290px;top:85px;}#cbtn10 {left:235px;top:110px;}#cbtn11 {left:142px;top:-5px;}.selectbox {position:relative;display: block;max-width:320px;margin:0 auto;}.select {border: 2px solid #bada55;display:block;height:30px;}.select select {width:100%;height:30px;line-height:30px;border:0;background-color:transparent;color:#fff;stroke:none;}.go {text-decoration:none; line-height:34px;position:absolute;right:0;top:0;height:32px;width:50px;background-color:#bada55; stroke:none; border:0 solid transparent;}.otabutton{position:absolute;right:0;top:0;height:30px;background-color:#bada55;color:#fff; line-height:30px; text-decoration:none; padding:0 10px;}.red {background-color:#f00;color:#fff}</style></head>");
                
                // Web Page Heading
                client.println("<body><h1>IcosaLEDron</h1>");
                // icosaledron container
                client.println("<div class='icosaledron-container'><svg version='1.1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' x='0px' y='0px' width='320px' height='371.64px' viewBox='0 0 320 371.64' style='enable-background:new 0 0 320 371.64;' xml:space='preserve'><style type='text/css'>.st0{fill:#EB008B;}.st1{fill:#00ADEE;}.st2{fill:#FFFFFF;}</style><defs></defs><path class='st0' d='M253.54,253.78H66.82c-2.76,0-5.32-1.47-6.71-3.86c-1.39-2.39-1.4-5.34-0.03-7.74l92.65-162.41 c1.38-2.41,3.94-3.91,6.71-3.91c0.01,0,0.02,0,0.03,0c2.77,0,5.33,1.47,6.71,3.87l94.07,162.41c1.39,2.4,1.39,5.36,0.01,7.76 C258.87,252.3,256.31,253.78,253.54,253.78z M80.18,238.26h159.9L159.52,99.18L80.18,238.26z'/><path class='st0' d='M253.53,253.78c-2.75,0-5.32-1.46-6.71-3.87L152.76,87.5c-1.46-2.52-1.38-5.65,0.2-8.1     c1.58-2.45,4.4-3.8,7.3-3.51l152.42,15.56c2.42,0.25,4.58,1.61,5.84,3.69c1.26,2.08,1.48,4.63,0.58,6.89l-58.35,146.85 c-1.1,2.76-3.68,4.65-6.64,4.87C253.91,253.77,253.72,253.78,253.53,253.78z M173.8,92.88l78.43,135.41l48.65-122.44L173.8,92.88z' /><path class='st0' d='M160.18,371.45c-2.19,0-4.35-0.93-5.87-2.68c-2.47-2.85-2.53-7.06-0.15-9.98l95.79-117.67 c2.26-2.78,6.13-3.65,9.36-2.1l55.92,26.74c2.58,1.23,4.27,3.78,4.4,6.64c0.13,2.85-1.31,5.55-3.76,7.02l-151.71,90.93 C162.92,371.09,161.55,371.45,160.18,371.45z M258.13,255.66l-64.04,78.66l101.42-60.79L258.13,255.66z'/><path class='st0' d='M311.89,280.52c-1.1,0-2.2-0.23-3.23-0.71l-58.35-26.74c-3.75-1.72-5.5-6.08-3.98-9.92l58.35-146.85 c1.37-3.46,5.01-5.45,8.67-4.76c3.66,0.7,6.3,3.9,6.3,7.62v173.59c0,2.64-1.34,5.1-3.57,6.53 C314.81,280.11,313.35,280.52,311.89,280.52z M263.47,242.04l40.66,18.64V139.71L263.47,242.04z'/><path class='st0' d='M160.18,371.45c-2.37,0-4.61-1.08-6.08-2.94L60.74,250.84c-1.85-2.33-2.2-5.52-0.91-8.2 c1.3-2.68,4.01-4.38,6.99-4.38h186.72c2.98,0,5.69,1.7,6.99,4.38c1.29,2.68,0.94,5.87-0.91,8.2l-93.36,117.67 C164.78,370.37,162.55,371.45,160.18,371.45z M82.88,253.78l77.3,97.43l77.3-97.43H82.88z'/><path class='st0' d='M311.89,106.93c-0.26,0-0.53-0.01-0.79-0.04L158.68,91.33c-3.96-0.4-6.97-3.74-6.97-7.72V7.76 c0-2.8,1.5-5.37,3.94-6.75c2.43-1.38,5.42-1.34,7.81,0.1l152.42,91.41c3.12,1.87,4.52,5.65,3.37,9.1 C318.19,104.82,315.2,106.93,311.89,106.93z M167.23,76.61l110.81,11.31L167.23,21.46V76.61z'/><path class='st0' d='M66.11,253.78c-0.19,0-0.38-0.01-0.57-0.02c-2.96-0.22-5.54-2.11-6.64-4.87L0.55,102.04 c-0.9-2.26-0.68-4.81,0.58-6.89c1.26-2.08,3.43-3.45,5.84-3.69L159.39,75.9c2.9-0.29,5.72,1.06,7.3,3.51 c1.58,2.45,1.66,5.58,0.2,8.1L72.82,249.91C71.43,252.32,68.86,253.78,66.11,253.78z M18.76,105.85l48.65,122.44l78.43-135.41 L18.76,105.85z'/><path class='st0' d='M159.47,371.45c-1.37,0-2.74-0.36-3.98-1.1L3.77,279.42c-2.45-1.47-3.9-4.17-3.76-7.02 c0.13-2.85,1.83-5.4,4.4-6.64l55.92-26.74c3.23-1.55,7.1-0.68,9.36,2.1l95.79,117.67c2.38,2.92,2.31,7.13-0.15,9.98 C163.81,370.53,161.66,371.45,159.47,371.45z M24.13,273.53l101.42,60.79l-64.04-78.66L24.13,273.53z'/><path class='st0' d='M7.76,280.52c-1.46,0-2.92-0.41-4.19-1.23C1.35,277.87,0,275.41,0,272.76V99.17c0-3.72,2.64-6.92,6.3-7.62 c3.65-0.7,7.29,1.3,8.67,4.76l58.35,146.85c1.52,3.84-0.22,8.2-3.98,9.92l-58.35,26.74C9.96,280.29,8.86,280.52,7.76,280.52z  M15.52,139.71v120.96l40.66-18.64L15.52,139.71z'/><path class='st0' d='M7.76,106.93c-3.31,0-6.3-2.12-7.36-5.31c-1.15-3.45,0.25-7.23,3.37-9.1L156.19,1.11 c2.4-1.44,5.38-1.48,7.81-0.1c2.43,1.38,3.94,3.96,3.94,6.75v75.85c0,3.98-3.01,7.31-6.97,7.72L8.55,106.89 C8.28,106.92,8.02,106.93,7.76,106.93z M152.42,21.46L41.61,87.92l110.81-11.31V21.46z'/><path class='st1' d='M66.11,134.89c-1.13,0-2.27-0.25-3.35-0.76L4.41,106.17c-2.58-1.23-4.26-3.78-4.4-6.63s1.31-5.55,3.76-7.02 L156.19,1.11c3.21-1.93,7.34-1.28,9.81,1.53c2.47,2.81,2.59,6.99,0.27,9.93L72.2,131.93C70.69,133.85,68.42,134.89,66.11,134.89z  M24.12,98.41l39.74,19.04l64.07-81.3L24.12,98.41z'/><path class='st1' d='M253.54,134.89H66.11c-2.97,0-5.68-1.7-6.98-4.37c-1.3-2.67-0.95-5.85,0.89-8.19L154.08,2.96 C155.55,1.09,157.8,0,160.18,0c0,0,0.01,0,0.01,0c2.38,0,4.63,1.1,6.1,2.98l93.36,119.37c1.83,2.34,2.16,5.51,0.86,8.18 C259.21,133.19,256.5,134.89,253.54,134.89z M82.1,119.37h155.52l-77.46-99.05L82.1,119.37z'/><path class='st1' d='M159.47,299.31c-0.32,0-0.64-0.02-0.95-0.06L6.81,280.46c-2.39-0.3-4.5-1.68-5.73-3.76s-1.42-4.6-0.52-6.83 l58.35-145.64c1.11-2.77,3.7-4.65,6.68-4.86c2.97-0.2,5.8,1.32,7.27,3.91l93.36,164.42c1.45,2.55,1.33,5.7-0.29,8.14 C164.47,298.03,162.04,299.31,159.47,299.31z M18.71,266.3l126.39,15.65L67.32,144.97L18.71,266.3z'/><path class='st1' d='M159.47,371.45c-1.38,0-2.76-0.37-3.99-1.1L3.77,279.42c-3.14-1.89-4.54-5.71-3.35-9.18 c1.19-3.47,4.66-5.63,8.29-5.17l151.71,18.78c3.89,0.48,6.81,3.78,6.81,7.7v72.15c0,2.79-1.5,5.37-3.93,6.75 C162.11,371.12,160.79,371.45,159.47,371.45z M43.22,284.97L151.71,350V298.4L43.22,284.97z'/><path class='st1' d='M7.76,280.52c-0.49,0-0.98-0.05-1.47-0.14c-3.65-0.71-6.29-3.9-6.29-7.62V99.17c0-2.67,1.37-5.15,3.62-6.57 c2.26-1.42,5.08-1.58,7.49-0.43l58.35,27.95c3.69,1.77,5.37,6.09,3.85,9.88L14.96,275.65C13.77,278.64,10.88,280.52,7.76,280.52z  M15.52,111.49v121.04L56.2,130.99L15.52,111.49z'/><path class='st1' d='M253.89,134.89c-2.32,0-4.59-1.04-6.1-2.96L153.73,12.56c-2.32-2.94-2.2-7.12,0.27-9.93 c2.47-2.81,6.6-3.46,9.81-1.53l152.42,91.41c2.45,1.47,3.89,4.16,3.76,7.02c-0.13,2.85-1.82,5.4-4.4,6.63l-58.35,27.95 C256.17,134.64,255.03,134.89,253.89,134.89z M192.07,36.15l64.07,81.3l39.74-19.04L192.07,36.15z'/><path class='st1' d='M160.53,371.45c-1.32,0-2.64-0.34-3.82-1.01c-2.43-1.38-3.93-3.96-3.93-6.75v-72.15c0-3.92,2.92-7.22,6.81-7.7 l151.71-18.78c3.64-0.46,7.1,1.71,8.29,5.17s-0.2,7.3-3.35,9.18l-151.71,90.93C163.29,371.08,161.91,371.45,160.53,371.45z  M168.29,298.4V350l108.49-65.02L168.29,298.4z'/><path class='st1' d='M312.24,280.52c-3.12,0-6-1.89-7.2-4.87l-58.35-145.64c-1.52-3.79,0.16-8.12,3.85-9.88l58.35-27.95 c2.4-1.15,5.23-0.99,7.49,0.43c2.26,1.42,3.62,3.9,3.62,6.57v173.59c0,3.72-2.64,6.91-6.29,7.62 C313.22,280.48,312.72,280.52,312.24,280.52z M263.8,130.99l40.69,101.55V111.49L263.8,130.99z'/><path class='st1' d='M159.47,299.31c0,0-0.01,0-0.01,0c-2.79,0-5.36-1.5-6.73-3.93L59.36,130.96c-1.36-2.4-1.35-5.35,0.04-7.73 c1.39-2.39,3.94-3.86,6.71-3.86h187.43c2.77,0,5.32,1.47,6.71,3.87c1.39,2.39,1.4,5.34,0.02,7.75L166.2,295.4 C164.82,297.82,162.25,299.31,159.47,299.31z M79.44,134.89l80.06,140.99l80.66-140.99H79.44z'/><g><path class='st2' d='M162.03,364.77v3.37c0,0.77-0.04,1.32-0.12,1.67c-0.08,0.35-0.26,0.68-0.52,0.98 c-0.26,0.3-0.58,0.52-0.95,0.66c-0.37,0.13-0.78,0.2-1.24,0.2c-0.6,0-1.1-0.07-1.5-0.21c-0.4-0.14-0.72-0.36-0.95-0.65 c-0.24-0.3-0.41-0.61-0.51-0.93c-0.1-0.33-0.15-0.85-0.15-1.56v-3.53c0-0.93,0.08-1.62,0.24-2.08c0.16-0.46,0.48-0.83,0.95-1.11 c0.48-0.28,1.05-0.42,1.73-0.42c0.55,0,1.05,0.1,1.49,0.29c0.44,0.19,0.76,0.43,0.98,0.71c0.22,0.28,0.36,0.59,0.44,0.94 S162.03,364,162.03,364.77z M159.51,363.92c0-0.54-0.03-0.87-0.08-1.01c-0.05-0.13-0.17-0.2-0.37-0.2c-0.19,0-0.31,0.07-0.37,0.21 c-0.06,0.14-0.09,0.48-0.09,1v4.93c0,0.59,0.03,0.94,0.08,1.06c0.06,0.12,0.18,0.18,0.37,0.18s0.31-0.07,0.37-0.21 c0.06-0.14,0.09-0.46,0.09-0.95V363.92z'/></g><g><path class='st2' d='M12.1,262.66v10.1H9.58v-5.42c0-0.78-0.02-1.25-0.06-1.41s-0.14-0.28-0.31-0.36 c-0.17-0.08-0.54-0.12-1.13-0.12H7.83v-1.18c1.22-0.26,2.14-0.8,2.78-1.62H12.1z'/></g><g><path class='st2' d='M68.13,123.89h-2.52c0-0.47-0.01-0.77-0.02-0.89c-0.01-0.12-0.06-0.23-0.15-0.31 c-0.09-0.08-0.21-0.12-0.37-0.12c-0.13,0-0.23,0.04-0.32,0.12c-0.08,0.08-0.13,0.18-0.14,0.31c-0.01,0.12-0.02,0.38-0.02,0.76v1.61 c0.13-0.27,0.32-0.47,0.57-0.61c0.25-0.14,0.56-0.2,0.92-0.2c0.46,0,0.87,0.12,1.22,0.37c0.35,0.25,0.58,0.55,0.67,0.9 c0.1,0.35,0.14,0.82,0.14,1.42v0.8c0,0.71-0.03,1.23-0.08,1.58c-0.05,0.34-0.2,0.66-0.43,0.96c-0.23,0.29-0.56,0.52-0.97,0.68 s-0.89,0.24-1.43,0.24c-0.68,0-1.23-0.09-1.67-0.28c-0.43-0.19-0.77-0.46-1-0.81c-0.23-0.35-0.37-0.72-0.41-1.11 c-0.04-0.39-0.06-1.14-0.06-2.25v-1.4c0-1.2,0.02-2.01,0.05-2.41c0.04-0.4,0.18-0.79,0.43-1.15c0.25-0.36,0.6-0.63,1.04-0.81 c0.44-0.18,0.95-0.27,1.52-0.27c0.71,0,1.29,0.11,1.74,0.34s0.78,0.55,0.97,0.97C68.04,122.74,68.13,123.26,68.13,123.89z  M65.61,127.05c0-0.36-0.04-0.6-0.12-0.74c-0.08-0.13-0.21-0.2-0.39-0.2c-0.17,0-0.3,0.06-0.38,0.19 c-0.08,0.13-0.12,0.38-0.12,0.75v1.83c0,0.44,0.04,0.74,0.12,0.87s0.21,0.21,0.38,0.21c0.11,0,0.22-0.05,0.34-0.16 c0.12-0.11,0.17-0.39,0.17-0.86V127.05z'/></g><g><path class='st2' d='M12.65,97.03v2.2l-1.61,7.9H8.53l1.79-8.29H7.83v-1.82H12.65z'/></g><g><path class='st2' d='M160.9,86.97c0.37,0.15,0.66,0.37,0.85,0.67c0.2,0.3,0.3,1.01,0.3,2.13c0,0.82-0.08,1.42-0.24,1.8 c-0.16,0.39-0.47,0.7-0.92,0.95c-0.46,0.25-1.07,0.37-1.84,0.37c-0.75,0-1.34-0.12-1.78-0.36c-0.44-0.24-0.75-0.55-0.94-0.94 c-0.19-0.39-0.28-1.04-0.28-1.95c0-0.61,0.05-1.14,0.14-1.59c0.09-0.45,0.39-0.81,0.88-1.08c-0.31-0.15-0.55-0.39-0.7-0.71 c-0.16-0.33-0.23-0.74-0.23-1.24c0-0.87,0.24-1.52,0.72-1.95c0.48-0.43,1.2-0.65,2.17-0.65c1.11,0,1.88,0.23,2.3,0.69 c0.42,0.46,0.64,1.12,0.64,1.98c0,0.54-0.07,0.94-0.21,1.18C161.62,86.5,161.33,86.74,160.9,86.97z M159.53,89 c0-0.39-0.04-0.64-0.12-0.77c-0.08-0.13-0.2-0.19-0.37-0.19c-0.17,0-0.29,0.06-0.36,0.18c-0.07,0.12-0.11,0.38-0.11,0.78v1.31 c0,0.44,0.04,0.72,0.12,0.84c0.08,0.12,0.21,0.19,0.38,0.19c0.18,0,0.3-0.06,0.36-0.19c0.06-0.13,0.09-0.41,0.09-0.86V89z  M159.48,84.8c0-0.34-0.03-0.57-0.09-0.68c-0.06-0.11-0.18-0.16-0.35-0.16c-0.17,0-0.28,0.06-0.34,0.18 c-0.06,0.12-0.09,0.34-0.09,0.66v0.77c0,0.3,0.03,0.51,0.1,0.63c0.07,0.12,0.18,0.17,0.34,0.17c0.17,0,0.28-0.06,0.34-0.17 c0.06-0.11,0.09-0.35,0.09-0.71V84.8z'/></g><g><path class='st2' d='M301.35,104.45h2.52c0,0.47,0.01,0.77,0.02,0.89c0.01,0.12,0.06,0.23,0.15,0.31c0.09,0.08,0.21,0.12,0.37,0.12 c0.13,0,0.23-0.04,0.32-0.12c0.08-0.08,0.13-0.18,0.14-0.31s0.02-0.38,0.02-0.76v-1.61c-0.12,0.27-0.31,0.47-0.56,0.61 c-0.25,0.13-0.56,0.2-0.93,0.2c-0.46,0-0.87-0.12-1.22-0.37c-0.35-0.25-0.58-0.55-0.67-0.9s-0.14-0.82-0.14-1.42v-0.8 c0-0.71,0.03-1.23,0.08-1.58c0.06-0.34,0.2-0.66,0.43-0.96c0.23-0.29,0.56-0.52,0.97-0.68c0.41-0.16,0.89-0.24,1.44-0.24 c0.67,0,1.23,0.09,1.66,0.28c0.43,0.19,0.77,0.46,1,0.81c0.24,0.35,0.37,0.72,0.41,1.11c0.04,0.39,0.06,1.14,0.06,2.25v1.4 c0,1.2-0.02,2.01-0.05,2.41c-0.03,0.41-0.18,0.79-0.43,1.15c-0.26,0.36-0.6,0.63-1.04,0.81c-0.44,0.18-0.94,0.27-1.52,0.27 c-0.71,0-1.29-0.11-1.74-0.34c-0.45-0.23-0.78-0.55-0.97-0.97S301.35,105.08,301.35,104.45z M303.88,101.3 c0,0.42,0.06,0.69,0.17,0.79c0.11,0.1,0.22,0.15,0.34,0.15c0.17,0,0.3-0.06,0.38-0.19c0.08-0.13,0.12-0.38,0.12-0.75v-1.83 c0-0.44-0.04-0.74-0.12-0.87s-0.21-0.21-0.38-0.21c-0.11,0-0.22,0.05-0.34,0.16c-0.12,0.11-0.17,0.39-0.17,0.86V101.3z'/></g><g><path class='st2' d='M246.05,126.42v10.1h-2.52v-5.42c0-0.78-0.02-1.25-0.06-1.41c-0.04-0.16-0.14-0.28-0.31-0.36 c-0.17-0.08-0.54-0.12-1.13-0.12h-0.25v-1.18c1.22-0.26,2.14-0.8,2.78-1.62H246.05z'/><path class='st2' d='M252.98,129.84v3.37c0,0.76-0.04,1.32-0.12,1.67s-0.26,0.68-0.52,0.98c-0.26,0.3-0.58,0.52-0.95,0.66 c-0.37,0.13-0.78,0.2-1.24,0.2c-0.6,0-1.1-0.07-1.5-0.21c-0.4-0.14-0.72-0.36-0.95-0.65c-0.24-0.3-0.41-0.61-0.51-0.93 c-0.1-0.33-0.15-0.85-0.15-1.56v-3.52c0-0.93,0.08-1.62,0.24-2.08c0.16-0.46,0.48-0.83,0.95-1.11c0.48-0.28,1.05-0.42,1.73-0.42 c0.55,0,1.05,0.1,1.49,0.29c0.44,0.19,0.76,0.43,0.98,0.71c0.22,0.28,0.36,0.59,0.44,0.94 C252.94,128.52,252.98,129.07,252.98,129.84z M250.46,128.99c0-0.54-0.03-0.87-0.08-1.01c-0.05-0.13-0.17-0.2-0.37-0.2 c-0.19,0-0.31,0.07-0.37,0.21c-0.06,0.14-0.09,0.48-0.09,1v4.93c0,0.59,0.03,0.94,0.08,1.06c0.06,0.12,0.18,0.18,0.37,0.18 s0.31-0.07,0.37-0.21c0.06-0.14,0.09-0.46,0.09-0.95V128.99z'/></g><g><path class='st2' d='M159.97,8.06v10.1h-2.52v-5.42c0-0.78-0.02-1.25-0.06-1.41c-0.04-0.16-0.14-0.28-0.31-0.36 c-0.17-0.08-0.54-0.12-1.13-0.12h-0.25V9.68c1.22-0.26,2.14-0.8,2.78-1.62H159.97z'/><path class='st2' d='M164.84,8.06v10.1h-2.52v-5.42c0-0.78-0.02-1.25-0.06-1.41c-0.04-0.16-0.14-0.28-0.31-0.36 c-0.17-0.08-0.54-0.12-1.13-0.12h-0.25V9.68c1.22-0.26,2.14-0.8,2.78-1.62H164.84z'/></g><g><path class='st2' d='M71.99,249.97v1.72h-5.55l0-1.44c1.64-2.69,2.62-4.35,2.93-4.99c0.31-0.64,0.47-1.14,0.47-1.49 c0-0.27-0.05-0.48-0.14-0.62c-0.09-0.13-0.24-0.2-0.43-0.2s-0.33,0.07-0.43,0.22c-0.09,0.15-0.14,0.45-0.14,0.89v0.96h-2.26v-0.37 c0-0.57,0.03-1.01,0.09-1.34c0.06-0.33,0.2-0.65,0.43-0.96c0.23-0.32,0.53-0.56,0.89-0.72s0.8-0.24,1.32-0.24 c1,0,1.76,0.25,2.27,0.75c0.51,0.5,0.77,1.13,0.77,1.89c0,0.58-0.14,1.19-0.43,1.83c-0.29,0.64-1.14,2.01-2.56,4.11H71.99z'/></g><g><path class='st2' d='M255.8,245.97c0.38,0.13,0.67,0.34,0.85,0.63c0.19,0.29,0.28,0.98,0.28,2.06c0,0.8-0.09,1.43-0.27,1.87 c-0.18,0.44-0.5,0.78-0.95,1.01c-0.45,0.23-1.03,0.35-1.73,0.35c-0.8,0-1.43-0.13-1.88-0.4c-0.46-0.27-0.76-0.6-0.9-0.99 c-0.14-0.39-0.22-1.06-0.22-2.02v-0.8h2.52v1.64c0,0.44,0.03,0.71,0.08,0.83c0.05,0.12,0.17,0.18,0.35,0.18 c0.2,0,0.32-0.07,0.39-0.22c0.06-0.15,0.09-0.54,0.09-1.17v-0.7c0-0.39-0.04-0.67-0.13-0.85s-0.22-0.3-0.39-0.35 c-0.17-0.06-0.5-0.09-0.99-0.1v-1.47c0.6,0,0.97-0.02,1.11-0.07s0.24-0.15,0.31-0.3c0.06-0.15,0.09-0.39,0.09-0.72v-0.56 c0-0.35-0.04-0.59-0.11-0.7c-0.07-0.11-0.19-0.17-0.34-0.17c-0.17,0-0.29,0.06-0.36,0.18c-0.06,0.12-0.1,0.37-0.1,0.76v0.83h-2.52 v-0.86c0-0.97,0.22-1.62,0.66-1.96c0.44-0.34,1.14-0.51,2.1-0.51c1.2,0,2.02,0.23,2.45,0.7c0.43,0.47,0.64,1.12,0.64,1.96 c0,0.57-0.08,0.97-0.23,1.23C256.45,245.53,256.18,245.76,255.8,245.97z'/></g><g><path class='st2' d='M312.96,262.66v6.6h0.72v1.72h-0.72v1.78h-2.52v-1.78h-3v-1.72l2.18-6.6H312.96z M310.44,269.26v-4.3 l-1.11,4.3H310.44z'/></g><g><path class='st2' d='M165.42,285.46v1.62h-3.13v1.72c0.39-0.46,0.89-0.69,1.51-0.69c0.69,0,1.22,0.19,1.58,0.58 c0.36,0.39,0.53,1.14,0.53,2.24v1.44c0,0.72-0.03,1.24-0.1,1.58c-0.07,0.33-0.22,0.64-0.44,0.93c-0.22,0.29-0.54,0.5-0.94,0.66 c-0.4,0.15-0.89,0.23-1.47,0.23c-0.65,0-1.22-0.11-1.72-0.34c-0.5-0.23-0.85-0.57-1.05-1.01c-0.21-0.45-0.31-1.14-0.31-2.08v-0.55 h2.52v0.63c0,0.65,0.02,1.12,0.07,1.39c0.05,0.27,0.2,0.41,0.47,0.41c0.12,0,0.23-0.04,0.3-0.12c0.08-0.08,0.12-0.16,0.12-0.26 c0.01-0.09,0.01-0.49,0.02-1.19v-2c0-0.38-0.04-0.64-0.12-0.77s-0.21-0.21-0.39-0.21c-0.12,0-0.21,0.04-0.29,0.11 c-0.08,0.07-0.13,0.15-0.15,0.23c-0.02,0.08-0.03,0.27-0.03,0.56h-2.5l0.11-5.09H165.42z'/></g></svg><div class='btncont'><button class='cornerbtn' data-cornernr='0' id='cbtn0'>0</button><button class='cornerbtn' data-cornernr='1' id='cbtn1'>1</button><button class='cornerbtn' data-cornernr='2' id='cbtn2'>2</button><button class='cornerbtn' data-cornernr='3' id='cbtn3'>3</button><button class='cornerbtn' data-cornernr='4' id='cbtn4'>4</button><button class='cornerbtn' data-cornernr='5' id='cbtn5'>5</button><button class='cornerbtn' data-cornernr='6' id='cbtn6'>6</button><button class='cornerbtn' data-cornernr='7' id='cbtn7'>7</button><button class='cornerbtn' data-cornernr='8' id='cbtn8'>8</button><button class='cornerbtn' data-cornernr='9' id='cbtn9'>9</button><button class='cornerbtn' data-cornernr='10' id='cbtn10'>10</button><button class='cornerbtn' data-cornernr='11' id='cbtn11'>11</button></div></div>");
                // Display current state, and ON/OFF buttons for GPIO 5  
                client.println("<div><label>LightMode</label><div class=\'selectbox\'><div class=\'select\'><select onchange=\"lightmodeChange(this);\" id=\'modeSelect\'><option value=\'0\' disabled selected>Please Select</option>");
                client.println("<option value=\'0\'>Off</option>");
                client.println("<option value=\'1\'>White</option>");
                client.println("<option value=\'2\'>Running Light</option>");
                client.println("</select><a href=\'#\' id=\'gobtn' class=\'go\'>Go</a></div></div>");
                
                client.println("<script>function lightmodeChange(target){console.log(target.value);var targetref=\'/lightmode\' + target.value;document.getElementById(\'gobtn\').href=targetref}</script>");
                
                
                client.println("<p>OTAMode - State " + otaMode + "</p>");
                // If the otaMode is off, it displays the ON button       
                if (otaMode=="off") {
                  client.println("<a href=\"/1/on\" class=\"otabutton\">OTA (off)</a>");
                } else {
                  client.println("<p><a href=\"/1/off\" class=\"otabutton red\">OTA ON!</a></p>");
                } 
                   
                client.println("</body></html>");
                
                // The HTTP response ends with another blank line
                client.println();
                // Break out of the while loop
                break;
              } else { // if you got a newline, then clear currentLine
                currentLine = "";
              }
            } else if (c != '\r') {  // if you got anything else but a carriage return character,
              currentLine += c;      // add it to the end of the currentLine
            }
          }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
      }
    }
  } 
}

//Task2code: blinks an LED every 700 ms
void Task2code( void * pvParameters ){
  // core1 loop goes here flashy lights bits

  for(;;){
    
    //knob1Val = analogRead(Knob1);
    //knob2Val = analogRead(Knob2);
    if (otaMode == "on") { // check for ota}
      greenwashfade();
    } else {
      if (lightModeInt == 0) {
        blackOut();
      }
      if (lightModeInt == 1) {
        whiteOut();
      }
      if (lightModeInt == 2) {
        runningLight();
      }
    }
  }
}

void loop() {
  // pewpew
}




void runningLight() {
  int h = 0;
  int s = 255; 
  int v = 255;
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
    v = 255;
}

void greenwashfade(){
  for (int j = 0; j < 255; j++) {
    
    for( int i = 0; i < NUM_LEDS; i++) { //9948
      leds[i] = CHSV(100, 255, j);
    }
    FastLED.show();
    delay(1);
  }
  
  for (int j = 255; j > 100; j--) {
    
    for( int i = 0; i < NUM_LEDS; i++) { //9948
      leds[i] = CHSV(100, 255, j);
    }
    FastLED.show();
    delay(1);
  }
}

void blackOut() {
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
    leds[dot] = CHSV(0, 0, 0);
    delay(1);
    FastLED.show();
  }
}
void whiteOut() {
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
    leds[dot] = CHSV(0, 0, 255);
    delay(1);
    FastLED.show();
  }
}
