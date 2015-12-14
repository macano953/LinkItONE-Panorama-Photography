#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <LGPS.h>
#include "GPS_functions.h"
#include "GPSWaypoint.h"
#include "memorysaver.h"
#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "base64.h"
#include <avr/pgmspace.h>
#if defined(__arm__)
#include <itoa.h>
#endif

#define WIFI_AP "my_network"
#define WIFI_PASSWORD "my_password"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.
#define SITE_URL "my_server_IP"
#define HTTP_REQUEST "POST /api/images/upload HTTP/1.1"
#define USER_AGENT "User-Agent: LinkIt/1.0"
#define CONTENT_TYPE "Content-Type: application/x-www-form-urlencoded;"
#define CONTENT_LENGTH "Content-Length: "
#define DRONE "drone="
#define DRONE_ID "D0001"
#define TIMESTAMP "&timestamp="
#define timestamp_example "1449235906"
#define LAT_VAR "&lat="
#define LON_VAR "&lon="
#define LAT "52.222401"
#define LON "-0.075763"
#define IMAGE1 "&image1="
#define IMAGE2 "&image2="
#define IMAGE3 "&image3="
#define IMAGE4 "&image4="
LWiFiClient c;
unsigned long previous_time = 0;
GPSWaypoint* gpsPosition;
char buff[256];
const int CS1 = 4;
const int CS2 = 5;
const int CS3 = 6;
const int CS4 = 7;
bool cam1 = true, cam2 = true, cam3 = true, cam4 = true;
int pic_checker = 0; //Checks if 4 pictures have been really taken or not
double lat;
double lon;
char* buffer_latitude;
char* buffer_longitude;
int quality = 0;
int satellites = 0;
ArduCAM myCAM1(OV5642, CS1);
ArduCAM myCAM2(OV5642, CS2);
ArduCAM myCAM3(OV5642, CS3);
ArduCAM myCAM4(OV5642, CS4);

void setup() {
  uint8_t vid, pid;
  uint8_t temp;
  Serial.begin(115200);
  // set the SPI_CS as an output:
  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);
  pinMode(CS3, OUTPUT);
  pinMode(CS4, OUTPUT);
  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, HIGH);
  digitalWrite(CS3, HIGH);
  digitalWrite(CS4, HIGH);

  // initialize SPI:
  SPI.begin();
  Wire.begin();
  delay(100);
  //Check if the ArduCAM SPI bus is OK
  myCAM1.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM1.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55)
  {
    Serial.println("SPI1 interface Error!");
    cam1 = false;
  }

  myCAM2.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM2.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55)
  {
    Serial.println("SPI2 interface Error!");
    cam2 = false;
  }

  myCAM3.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM3.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55)
  {
    Serial.println("SPI3 interface Error!");
    cam3 = false;
  }

  myCAM4.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM4.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55)
  {
    Serial.println("SPI4 interface Error!");
    cam4 = false;
  }
  delay(100);
  //Check if the camera module type is OV5642
  myCAM1.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM1.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x42))
    Serial.println("Can't find OV5642 module 1!");
  else
    Serial.println("OV5642 1 detected.");

  myCAM2.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM2.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x42))
    Serial.println("Can't find OV5642 module 2!");
  else
    Serial.println("OV5642 2 detected.");

  myCAM3.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM3.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x42))
    Serial.println("Can't find OV5642 module 3!");
  else
    Serial.println("OV5642 3 detected.");

  myCAM4.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM4.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  if ((vid != 0x56) || (pid != 0x42))
    Serial.println("Can't find OV5642 module 4!");
  else
    Serial.println("OV5642 4 detected.");
  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM1.set_format(JPEG);
  myCAM2.set_format(JPEG);
  myCAM3.set_format(JPEG);
  myCAM4.set_format(JPEG);
  myCAM1.InitCAM();
  myCAM2.InitCAM();
  myCAM3.InitCAM();
  myCAM4.InitCAM();
  myCAM1.OV5642_set_JPEG_size(OV5642_1920x1080);
  myCAM2.OV5642_set_JPEG_size(OV5642_1920x1080);
  myCAM3.OV5642_set_JPEG_size(OV5642_1920x1080);
  myCAM4.OV5642_set_JPEG_size(OV5642_1920x1080);

  myCAM1.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM2.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM3.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM4.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);

  myCAM1.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power
  myCAM2.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power
  myCAM3.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power
  myCAM4.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power

  myCAM1.clear_fifo_flag();
  myCAM2.clear_fifo_flag();
  myCAM3.clear_fifo_flag();
  myCAM4.clear_fifo_flag();

  myCAM1.write_reg(ARDUCHIP_FRAMES, 0x00);
  myCAM2.write_reg(ARDUCHIP_FRAMES, 0x00);
  myCAM3.write_reg(ARDUCHIP_FRAMES, 0x00);
  myCAM4.write_reg(ARDUCHIP_FRAMES, 0x00);
}

void loop() {
  pic_checker = 0;
  Serial.print("Started");
  LGPS.powerOn();
  char GPS_formatted[] = "GPS fixed";
  gpsPosition = new GPSWaypoint();
  gpsSentenceInfoStruct gpsDataStruct;
  // Get GPS data and upload location and speed
  getGPSData(gpsDataStruct, GPS_formatted, gpsPosition);

  buffer_latitude = new char[30];
  sprintf(buffer_latitude, "%2.6f", gpsPosition->latitude);
  buffer_longitude = new char[30];
  sprintf(buffer_longitude, "%2.6f", gpsPosition->longitude);
  LGPS.powerOff();
  
  delay(1000);
  myCAM1.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //Power up Camera
  myCAM2.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //Power up Camera
  myCAM3.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //Power up Camera
  myCAM4.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //Power up Camera
  delay(1000);

  myCAM1.flush_fifo();
  myCAM2.flush_fifo();
  myCAM3.flush_fifo();
  myCAM4.flush_fifo();
  myCAM1.clear_fifo_flag();
  myCAM1.start_capture();
  myCAM2.clear_fifo_flag();
  myCAM2.start_capture();
  myCAM3.clear_fifo_flag();
  myCAM3.start_capture();
  myCAM4.clear_fifo_flag();
  myCAM4.start_capture();

  while (!myCAM1.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) && cam1);
  while (!myCAM2.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) && cam2);
  while (!myCAM3.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) && cam3);
  while (!myCAM4.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) && cam4);

  myCAM1.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power
  myCAM2.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power
  myCAM3.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power
  myCAM4.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power

  previous_time = millis();

  LWiFi.begin();

  // keep retrying until connected to AP
  Serial.println(F("Connecting to AP"));
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    Serial.println(F("Retrying connection to AP!"));
    delay(1000);
  }

  Serial.println(F("Connected!"));
  Serial.println(F("ArduCAM Start!"));

  // keep retrying until connected to website
  Serial.println("Connecting to the server...");
  while (0 == c.connect(SITE_URL, 3000))
  {
    Serial.println("Re-Connecting to the server");
    delay(1000);
  }
  Serial.println("Connected to the server");
  //long timestamp = get_timestamp(c);
  if (cam1 == true)
  {
    read_fifo_burst_encode(myCAM1, c, IMAGE1);
    pic_checker++;
    get_http_response_pics(c);
    myCAM1.clear_fifo_flag();
  }
  Serial.println(F("CAM1: capture done!"));
  Serial.println(F("CAM1: couldn't capture!"));
  delay(1000);
  //image2
  Serial.println("Connecting to the server...");
  while (0 == c.connect(SITE_URL, 3000))
  {
    Serial.println("Re-Connecting to the server");
    delay(1000);
  }
  if (cam2 == true)
  {
    read_fifo_burst_encode(myCAM2, c, IMAGE2);
    pic_checker++;
    get_http_response_pics(c);
    myCAM2.clear_fifo_flag();
  }
  Serial.println(F("CAM2: capture done!"));
  delay(1000);
  //image3
  Serial.println("Connecting to the server...");
  while (0 == c.connect(SITE_URL, 3000))
  {
    Serial.println("Re-Connecting to the server");
    delay(1000);
  }
  if (cam3 == true)
  {
    read_fifo_burst_encode(myCAM3, c, IMAGE3);
    pic_checker++;
    get_http_response_pics(c);
    myCAM3.clear_fifo_flag();
  }
  Serial.println(F("CAM3: capture done!"));
  delay(1000);
  Serial.println("Connecting to the server...");
  while (0 == c.connect(SITE_URL, 3000))
  {
    Serial.println("Re-Connecting to the server");
    delay(1000);
  }
  //image4
  if (cam4 == true)
  {
    read_fifo_burst_encode(myCAM4, c, IMAGE4);
    pic_checker++;
    get_http_response_pics(c);
    myCAM4.clear_fifo_flag();
  }
  Serial.println(F("CAM4: capture done!"));
  //If we were able to take 4 pictures, upload them to the server
  if (pic_checker == 4) {
    Serial.println(F("4 pictures uploaded"));
    Serial.print(F("Elapsed time: "));
    Serial.println(millis() - previous_time);
  }
  else {
    Serial.println(F("Error: couldn't take 4 pictures"));
  }
  LWiFi.end();
  delay(10000);
}

void read_fifo_burst_encode(ArduCAM myCAM, LWiFiClient client, char * image) {
  char temp, temp_last;
  uint32_t length = 0;
  char * image_chunk;
  char * encoded_chunk;
  char * encoded;
  size_t total_encoded_len;
  size_t encodedLen;

  length = myCAM.read_fifo_length();
  length--;
  image_chunk = (char*) calloc (4000, sizeof(char));
  if (image_chunk == NULL) return;
  encoded_chunk = (char*) calloc (5500, sizeof(char));
  if (encoded_chunk == NULL) return;
  total_encoded_len = 4 * ((length + 2) / 3);

  Serial.println(length);
  Serial.println(total_encoded_len);
  if (length >= 524288 ) // 524kb
  {
    Serial.println("Over size.");
    return;
  }
  else if (length == 0 ) //0 kb
  {
    Serial.println("Size is 0.");
    return;
  }
  else {
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();//Set fifo burst mode
    Serial.println("Transmitting data SPI - WiFi client...");
    client.println(F("POST /api/images/upload HTTP/1.1"));
    client.println("Host: " SITE_URL);
    client.println(F("User-Agent: LinkIt/1.0"));
    client.println(F("Connection: keep-alive"));
    client.println(F("Content-Type: application/x-www-form-urlencoded"));
    client.print(F("Content-Length: "));
    client.println(strlen(DRONE) + strlen(DRONE_ID) + strlen(LAT_VAR) + strlen(buffer_latitude) + strlen(LON_VAR) + strlen(buffer_longitude) + strlen(TIMESTAMP) + strlen(timestamp_example) + strlen(image) + total_encoded_len);
    client.println();
    client.print(DRONE);
    client.print(DRONE_ID);
    client.print(LAT_VAR);
    client.print(buffer_latitude);
    client.print(LON_VAR);
    client.print(buffer_longitude);
    client.print(TIMESTAMP);
    client.print(timestamp_example);
    client.print(image);
    int VM_TCP_RESULT0 = 0;

    int index = 0;
    int lenToSend;
    size_t encodedLen;
    char * ptr_encoded_chunk;
    while ( length--) {
      temp_last = temp;
      temp =  SPI.transfer(0x00); //read a byte from spi
      image_chunk[index] = temp;
      index++;/*
      if ( (temp == 0xD9) && (temp_last == 0xFF) ) {
        Serial.println("End detected");
        while(length--){
          image_chunk[index] = '0';
          index++;
        }
      }*/
      if (index == 3750 || length == 0) {
        encoded_chunk = base64_encode(image_chunk, index, &encodedLen);
        ptr_encoded_chunk = encoded_chunk;
        while (encodedLen > 0) {
          VM_TCP_RESULT0 = client.write((uint8_t *)ptr_encoded_chunk, encodedLen);
          if (VM_TCP_RESULT0 > 0 && encodedLen > 0) {
            //Serial.println(VM_TCP_RESULT0);
            ptr_encoded_chunk += VM_TCP_RESULT0;
            encodedLen -= VM_TCP_RESULT0;
          } else {
            Serial.print(VM_TCP_RESULT0);
            Serial.println(" value ignored");
          }
          delay(10);
        }
        index = 0;
      }
      delayMicroseconds(12);
    }
    client.println();
  }
  free(image_chunk);
  free (encoded_chunk);
  myCAM.CS_HIGH();
  return;
}

char * get_http_response_pics(LWiFiClient client) {
  boolean disconnectedMsg = false;
  // waiting for server response
  while (!client.available())
  {
    delay(100);
  }
  // Make sure we are connected, and dump the response content to Serial
  while (client)
  {
    int v = client.read();
    if (v != -1)
    {
      Serial.print((char)v);
    }
    else
    {
      client.stop();
    }
  }
  if (!disconnectedMsg)
  {
    Serial.println(F("Disconnected by server"));
    disconnectedMsg = true;
  }
}

long get_timestamp(LWiFiClient client) {
  boolean disconnectedMsg = false;
  // waiting for server response
  while (!client.available())
  {
    delay(100);
  }
  // Make sure we are connected, and dump the response content to Serial
  while (client)
  {
    int v = client.read();
    if (v != -1)
    {
      Serial.print((char)v);
    }
    else
    { 
      client.stop();
    }
  }
  if (!disconnectedMsg)
  {
    Serial.println(F("Disconnected by server"));
    disconnectedMsg = true;
  }
}

