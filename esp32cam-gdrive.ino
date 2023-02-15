#include <EEPROM.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "SD.h"
#include "FS.h"
#include "esp_camera.h"


const char* ssid     = "";   //your network SSID
const char* password = "";   //your network password
const char* myDomain = "script.google.com";
String myScript = "/macros/s/AKfycbzS940sSyttC3LUaEtSy43yhiii5M_dCu6rXcESedA97wXQi6a_jOdiwj3zjHEhOHm7/exec";    //Replace with your own url
String myFilename = "filename=ESP32-CAM.jpg";
String mimeType = "&mimetype=image/jpeg";
String myImage = "&data=";

int waitingTime = 30000; //Wait 30 seconds to google response.


#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//SD CARD Pins
#define sd_sck 14
#define sd_mosi 15
#define sd_ss 13
#define sd_miso 2

//read memory forever

#include <EEPROM.h>

//definen the number ob bytes you want to access
#define EEPROM_SIZE 1

int pictureNumber = 0;
int mySN = 0;
char myFname1[4];
char myFname2[4];

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(10);




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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;
  EEPROM.write(0, pictureNumber);
  EEPROM.commit();

  // LED Pin Mode
  pinMode(33, OUTPUT);

}
boolean enviar = true;

//Auto Reset
//void(* resetFunc) (void) = 0; //declare reset function @ address 0

void loop() {

  if (digitalRead(0) == 1) {
    for (int resetnum = 1; resetnum == 1; resetnum = resetnum + 1) {
      WiFi.mode(WIFI_STA);

      Serial.println("");
      Serial.print("Connecting to ");
      Serial.println(ssid);
      WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(50);

      }

      Serial.println("");
      Serial.println("STAIP address: ");
      Serial.println(WiFi.localIP());

      Serial.println("");


    }
    //if(enviar) {
    saveCapturedImage();
    enviar = false;
    delay(20);
    //}

  }


  if (digitalRead(0) == 0) {
    digitalWrite(33, LOW);
    delay (200);
    saveCapturedImage2();

    digitalWrite(33, HIGH);

    delay(500); //待機間隔 秒
  }

 if(digitalRead(4)==1){
  digitalWrite(4, HIGH);
  delay(1000);
 }
 
}

void saveCapturedImage() {
  Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client;
  client.setInsecure();

  if (client.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      delay(1000);
      ESP.restart();
      return;
    }

    char *input = (char *)fb->buf;
    char output[base64_enc_len(3)];
    String imageFile = "";
    for (int i = 0; i < fb->len; i++) {
      base64_encode(output, (input++), 3);
      if (i % 3 == 0) imageFile += urlencode(String(output));
    }
    String Data = myFilename + mimeType + myImage;

    esp_camera_fb_return(fb);

    Serial.println("Send a captured image to Google Drive.");

    client.println("POST " + myScript + " HTTP/1.1");
    client.println("Host: " + String(myDomain));
    client.println("Content-Length: " + String(Data.length() + imageFile.length()));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();

    client.print(Data);
    int Index;
    for (Index = 0; Index < imageFile.length(); Index = Index + 1000) {
      client.print(imageFile.substring(Index, Index + 1000));
    }

    Serial.println("Waiting for response.");
    long int StartTime = millis();
    while (!client.available()) {
      Serial.print(".");
      delay(500);
      if ((StartTime + waitingTime) < millis()) {
        Serial.println();
        Serial.println("No response.");
        //If you have no response, maybe need a greater value of waitingTime
        break;
      }
    }
    Serial.println();
    while (client.available()) {
      Serial.print(char(client.read()));
    }
  } else {
    Serial.println("Connected to " + String(myDomain) + " failed.");
  }
  client.stop();
}


//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/
String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      //encodedString+=code2;
    }
    yield();
  }
  return encodedString;
}



void saveCapturedImage2() {

  camera_fb_t * fb = NULL;

  // Take Picture with Camera
  Serial.println(" ");
  Serial.println("Take Picture ");
  fb = esp_camera_fb_get();

  // Path where new picture will be saved in SD Card
  Serial.println("Save SD Card");

  // Get File Name
  if (mySN == 999) {

    EEPROM.begin(EEPROM_SIZE);
    pictureNumber = EEPROM.read(0) + 1;
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();

    mySN = 1;
  } else {
    mySN = mySN + 1;
  }

  sprintf(myFname1, "%03d", pictureNumber);
  sprintf(myFname2, "%03d", mySN);

  String path = "/p" + String(myFname1) + "_" + String(myFname2) + ".jpg";

  Serial.printf("Picture file name: %s\n", path.c_str());

  SPI.begin(sd_sck, sd_miso, sd_mosi, sd_ss);
  SD.begin(sd_ss);

  File file = SD.open(path.c_str(), FILE_WRITE);
  file.write(fb->buf, fb->len);

  file.close();

  esp_camera_fb_return(fb);
  Serial.println("complete");
}
