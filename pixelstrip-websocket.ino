#include <WiFi.h>
#include <WiFiMulti.h>
#include <NeoPixelBus.h>
#include <Husarnet.h>
#include <WebSocketsServer.h>
#include <PID_v1.h>

/* ================ CONFIG SECTION START ==================== */
#define PIN           12   // Which pin on the Arduino is connected to the NeoPixels?
#define NUMPIXELS     60   // How many NeoPixels are attached to the Arduino?

#define BUFSIZE       40  // buffer size for storing LED strip state (each NUMPIXELS * 3 size)

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

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 1.0, 0.2, 0.0, DIRECT);

typedef struct {
  uint8_t pixel[3 * NUMPIXELS];
} LedStripState;

QueueHandle_t queue;

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUMPIXELS, PIN);
WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);
WiFiMulti wifiMulti;

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  LedStripState ledstrip;

  Serial.printf("%d\r\n", (uint32_t)type);

  switch (type) {
    case WStype_DISCONNECTED: {
        Serial.printf("[%u] Disconnected\r\n", num);
      }
      break;

    case WStype_CONNECTED: {
        Serial.printf("\r\n[%u] Connection from Husarnet \r\n", num);
      }
      break;

    case WStype_TEXT: {
        Serial.printf("[%u] Text:\r\n", num);
        Serial.println();
        for (int i = 0; i < length; i++) {
          Serial.printf("%c", (char)(*(payload + i)));
        }
        Serial.println();
      }
      break;

    case WStype_BIN: {
        for (int i = 0; i < NUMPIXELS; i++) {
          ledstrip.pixel[3 * i + 0] = (uint8_t)(*(payload + (i * 3 + 0)));
          ledstrip.pixel[3 * i + 1] = (uint8_t)(*(payload + (i * 3 + 1)));
          ledstrip.pixel[3 * i + 2] = (uint8_t)(*(payload + (i * 3 + 2)));
        }

        xQueueSendToBack(queue, (void*)&ledstrip, 0);
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
}

void setup() {
  Serial.begin(115200);

  strip.Begin();
  strip.Show();

  queue = xQueueCreate( BUFSIZE, sizeof(LedStripState) );
  if ( queue == NULL ) {
    Serial.printf("Queue error\r\n");
  }

  for (int i = 0; i < NUM_NETWORKS; i++) {
    wifiMulti.addAP(ssidTab[i], passwordTab[i]);
    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, ssidTab[i], passwordTab[i]);
  }

  //turn the PID on
  myPID.SetOutputLimits(10, 200);
  myPID.SetMode(AUTOMATIC);
  Setpoint = BUFSIZE / 2;

  xTaskCreatePinnedToCore(
    taskWifi,          /* Task function. */
    "taskWifi",        /* String with name of task. */
    20000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    2,                /* Priority of the task. */
    NULL,             /* Task handle. */
    1);               /* Core where the task should run */

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
    xQueueReceive(queue, (void*)&ls, portMAX_DELAY );

    for (int i = 0; i < NUMPIXELS; i++) {
      red = ls.pixel[3 * i + 0];
      green = ls.pixel[3 * i + 1];
      blue = ls.pixel[3 * i + 2];
      strip.SetPixelColor(i, RgbColor(red, green, blue));
    }
    strip.Show();

    Input = uxQueueMessagesWaiting(queue);
    myPID.Compute();

    Serial.printf("%d;%f\r\n", uxQueueMessagesWaiting(queue), Output);
    delay(int(Output));
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
        delay(10);
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
