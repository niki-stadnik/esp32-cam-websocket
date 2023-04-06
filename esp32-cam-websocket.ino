#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>


//debug
#define DEBUG 0 //1 = debug messages ON; 0 = debug messages OFF

#if DEBUG == 1
#define debugStart(x) Serial.begin(x)
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debugStart(x)
#define debug(x)
#define debugln(x)
#endif


#include "config.h"
const char* wlan_ssid = WIFI;
const char* wlan_password =  PASS;
const char * ws_host = HOSTPI;
const uint16_t ws_port = PORT;
const char* ws_baseur = URL;

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#include "camera_pins.h"

using namespace websockets;
WebsocketsClient client;

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        debugln("Connection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        debugln("Connection Closed");
        ESP.restart();
    } else if(event == WebsocketsEvent::GotPing) {
        debugln("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        debugln("Got a Pong!");
    }
}

void onMessageCallback(WebsocketsMessage message) {
    
}

void setup() {
  // setup serial
  debugStart(115200);
  // flush it - ESP Serial seems to start with rubbish
  debugln();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  config.frame_size = FRAMESIZE_XGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;



  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    debug("Camera init failed with error: " + err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_XGA);


  WiFi.begin(wlan_ssid, wlan_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    debug(".");
  }
  debugln("");
  debugln("WiFi connected");

  //startCameraServer();

  debug("Camera Ready! Use 'http://");
  debug(WiFi.localIP());
  debugln("' to connect");

  
}

void loop() {
  // put your main code here, to run repeatedly:

  while(!client.connect(ws_host, ws_port, ws_baseur)) { delay(500); }

  
  client.poll();
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb)
  {
    esp_camera_fb_return(fb);
    return;
  }

#if DEBUG == 1
  int i = (int)fb->len;
  debugln(i);
#endif

  //if(fb->format != PIXFORMAT_JPEG) { return; }

  client.sendBinary((const char*) fb->buf, fb->len);
  esp_camera_fb_return(fb);
}
