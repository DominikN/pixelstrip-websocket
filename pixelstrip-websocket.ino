#include <WiFi.h>
#include <WiFiMulti.h>
#include <NeoPixelBus.h>
#include <Husarnet.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <PID_v1.h>

#include <Preferences.h>

/* ================ CONFIG SECTION START ==================== */
#define PIN           12   // Pin connected to PixelStrip

// default NVS partition size 0x5000 -> max buf size about 12
#define MAXNUMPIXELS  150   // How many NeoPixels are attached to the Arduino?
#define MAXBUFSIZE    150   // buffer size for storing LED strip state (each NUMPIXELS * 3 size)

#define WEBSOCKET_PORT 8001

#define NUM_NETWORKS  2   // number of Wi-Fi networks

// Add your networks credentials here
const char* ssidTab[NUM_NETWORKS] = {
  "WIFI-SSID-1",
  "WIFI-SSID-2",
};
const char* passwordTab[NUM_NETWORKS] = {
  "WIFI-PASS-1",
  "WIFI-PASS-2",
};

// Husarnet credentials
const char* hostName = "pixelstrip";  //this will be the name of the 1st ESP32 device at https://app.husarnet.com

/* to get your join code go to https://app.husarnet.com
   -> select network
   -> click "Add element"
   -> select "join code" tab
   Keep it secret!
*/
const char* husarnetJoinCode = "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/yyyyyyyyyyyyyyyyyyyyyy";

/* ================ CONFIG SECTION END   ==================== */

Preferences preferences;
String mode_s;
int numpixel_s;
int buffer_s;
int delay_s;

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 2.0, 0.5, 0.0, DIRECT);

typedef struct {
  uint8_t pixel[3 * MAXNUMPIXELS];
} LedStripState;

QueueHandle_t queue;
SemaphoreHandle_t sem = NULL;

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(MAXNUMPIXELS, PIN);

WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);
WiFiMulti wifiMulti;

StaticJsonBuffer<200> jsonBufferSettings;

bool recording = false;


void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
	LedStripState ledstrip;
	
	switch (type) {
    case WStype_DISCONNECTED: {
        Serial.printf("[%u] Disconnected\r\n", num);
      }
      break;

    case WStype_CONNECTED: {
        Serial.printf("[%u] Connected\r\n", num);
      }
      break;

    case WStype_TEXT: {
        *(payload + length) = 0; //make cstring
        Serial.printf("[%s]", (char*)payload);

        jsonBufferSettings.clear();
        JsonObject& rootSettings = jsonBufferSettings.parseObject(payload);

        if (rootSettings.success()) {
          if (rootSettings.containsKey("mode")) {
            String mode_l = rootSettings["mode"];
            if ((mode_l == "auto") || (mode_l == "sequence")) {
              mode_s = mode_l;
              xQueueReset(queue);
              if (mode_l == "sequence") {
                recording = true;
              } else {
                recording = false;
              }
            }
          }
          if (rootSettings.containsKey("numpixel")) {
            int n = rootSettings["numpixel"];
            if ((n > 0) && (n < MAXNUMPIXELS + 1)) {
              numpixel_s = n;
            }
          }
          if (rootSettings.containsKey("buffer")) {
            int n = rootSettings["buffer"];
            if ((n > 0) && (n < MAXBUFSIZE + 1)) {
              buffer_s = n;
              Setpoint = buffer_s / 2;
            }
          }
          if (rootSettings.containsKey("delay")) {
            delay_s = rootSettings["delay"];
            if (delay_s < -1)
              delay_s = -1;
          }

          preferences.begin("my-app", false);
          preferences.putString("strip_mode", mode_s);
          preferences.putInt("strip_numpixel", numpixel_s);
          preferences.putInt("strip_buffer", buffer_s);
          preferences.putInt("strip_delay", delay_s);
          preferences.end();

          Serial.printf("\r\nPixel Strip settings:\r\nmode = %s\r\nnumpixel = %d\r\nbuffer = %d\r\ndelay = %d\r\n", mode_s.c_str(), numpixel_s, buffer_s,  delay_s);
        } else {
          Serial.printf("JSON parse error\r\n");
        }
      }
      break;

    case WStype_BIN: {
        for (int i = 0; i < numpixel_s; i++) {
          ledstrip.pixel[3 * i + 0] = (uint8_t)(*(payload + (i * 3 + 0)));
          ledstrip.pixel[3 * i + 1] = (uint8_t)(*(payload + (i * 3 + 1)));
          ledstrip.pixel[3 * i + 2] = (uint8_t)(*(payload + (i * 3 + 2)));
        }

        if (mode_s == "auto") {
          xQueueSendToBack(queue, (void*)&ledstrip, 0);
        }

        if ((recording == true) && (mode_s == "sequence")) {
          Serial.printf("%d ", uxQueueMessagesWaiting(queue));
          xQueueSendToBack(queue, (void*)&ledstrip, 0);

          if (uxQueueMessagesWaiting(queue) >= buffer_s) {
            Serial.printf("\r\nWriting frames to NVM (%d x %d) ... ", numpixel_s, buffer_s);

            preferences.begin("my-app", false);
            for (int i = 0; i < buffer_s; i++) {
              String key = String("strip_frames_" + String(i));

              xQueueReceive(queue, (void*)&ledstrip, portMAX_DELAY );
              preferences.putBytes(key.c_str(), (const void*)&ledstrip, sizeof(ledstrip));
              xQueueSendToBack(queue, (void*)&ledstrip, 0);
            }
            recording = false;
            preferences.end();

            Serial.printf("done\r\n");
          }
        }
      }
      break;
    case WStype_ERROR: {
        Serial.printf("[%u] Error\r\n", num);
      }
      break;

    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;

    default:
      break;
  }
  xSemaphoreGive( sem );
}

void setup() {
  Serial.begin(460800);

  Serial.printf("\r\nMAXBUFSIZE = %d\r\nMAXNUMPIXELS = %d\r\n", MAXBUFSIZE, MAXNUMPIXELS);

  /* Init queue for pixel strip frames (for "auto" mode only) */
  queue = xQueueCreate( MAXBUFSIZE, sizeof(LedStripState) );
  if ( queue == NULL ) {
    Serial.printf("Queue error\r\n");
    while (1);
  }
  xQueueReset(queue);

  sem = xSemaphoreCreateMutex();

  /* Load settings from NV memory */
  preferences.begin("my-app", false);
  mode_s = preferences.getString("strip_mode");  // "auto" or "sequence"
  if (mode_s == "") {
    preferences.putString("strip_mode", "auto");
    preferences.putInt("strip_numpixel", MAXNUMPIXELS);
    preferences.putInt("strip_buffer", MAXBUFSIZE);
    preferences.putInt("strip_delay", -1);
  }
  numpixel_s = preferences.getInt("strip_numpixel");
  if (numpixel_s > MAXNUMPIXELS) {
    numpixel_s = MAXNUMPIXELS;
  }
  buffer_s = preferences.getInt("strip_buffer");
  if (buffer_s > MAXBUFSIZE) {
    buffer_s = MAXBUFSIZE;
  }

  delay_s = preferences.getInt("strip_delay");

  Serial.printf("\r\nPixel Strip settings:\r\nmode = %s\r\nnumpixel = %d\r\nbuffer = %d\r\ndelay = %d\r\n", mode_s.c_str(), numpixel_s, buffer_s,  delay_s);

  if (mode_s == "sequence") {
    LedStripState ledstrip;

    Serial.printf("\r\nLoading frames from NVM (%d x %d) ... ", numpixel_s, buffer_s);
    for (int i = 0; i < buffer_s; i++) {
      String key = String("strip_frames_" + String(i));
      preferences.getBytes(key.c_str(), (void*)&ledstrip, sizeof(ledstrip));
      xQueueSendToBack(queue, (void*)&ledstrip, 0);
    }
    Serial.printf("done\r\n");
  }

  preferences.end();


  /* Add Wi-Fi network credentials */
  for (int i = 0; i < NUM_NETWORKS; i++) {
    wifiMulti.addAP(ssidTab[i], passwordTab[i]);
    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, ssidTab[i], passwordTab[i]);
  }


  /* Turn the PID on (for "auto" mode only) */
  myPID.SetOutputLimits(20, 200);
  myPID.SetMode(AUTOMATIC);
  Setpoint = buffer_s / 2;


  /* Initialize a pixel strip */
  strip.Begin();
  strip.Show();


  xTaskCreatePinnedToCore(
    taskWifi,          /* Task function. */
    "taskWifi",        /* String with name of task. */
    20000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL,             /* Task handle. */
    1);               /* Core where the task should run (z 0 działało) */ 

  xTaskCreate(
    taskDisplay,          /* Task function. */
    "taskDisplay",        /* String with name of task. */
    20000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL);             /* Task handle. */
}

void taskDisplay( void * parameter ) {
  LedStripState ls;

  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;

  while (1) {
    if (mode_s == "auto") {
      xQueueReceive(queue, (void*)&ls, portMAX_DELAY );

      Input = uxQueueMessagesWaiting(queue);
      myPID.Compute();
      myPID.SetSampleTime(int(Output));

      for (int i = 0; i < numpixel_s; i++) {
        red = ls.pixel[3 * i + 0];
        green = ls.pixel[3 * i + 1];
        blue = ls.pixel[3 * i + 2];
        strip.SetPixelColor(i, RgbColor(red, green, blue));
      }
      strip.Show();

      Serial.printf("%d;%f\r\n", uxQueueMessagesWaiting(queue), Output);
      delay(int(Output));
    } else {
      if (mode_s == "sequence" && recording == false) {
        xQueueReceive(queue, (void*)&ls, portMAX_DELAY );
        xQueueSendToBack(queue, (void*)&ls, 0);

        for (int i = 0; i < numpixel_s; i++) {
          red = ls.pixel[3 * i + 0];
          green = ls.pixel[3 * i + 1];
          blue = ls.pixel[3 * i + 2];
          strip.SetPixelColor(i, RgbColor(red, green, blue));
        }
        strip.Show();
        delay(delay_s);
      } else {
        delay(10);
      }
    }
  }
}

void taskWifi( void * parameter ) {
  while (1) {
    uint8_t stat = wifiMulti.run();
    if (stat == WL_CONNECTED) {
      Husarnet.join(husarnetJoinCode, hostName);
      Husarnet.start();

      webSocket.begin();
      webSocket.onEvent(onWebSocketEvent);

      while (WiFi.status() == WL_CONNECTED) {
        webSocket.loop();
        xSemaphoreTake(sem, ( TickType_t)1);
      }
    } else {
      Serial.printf("WiFi error: %d\r\n", (int)stat);
      delay(500);
    }
  }
}

void loop() {
  delay(50);
}