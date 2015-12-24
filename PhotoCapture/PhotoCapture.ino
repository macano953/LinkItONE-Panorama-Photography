/*!< Including libraries used */
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <LGPS.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "GPS_functions.h"
#include "GPSWaypoint.h"
#include "base64.h"

/*!< Defining some macros */
#define WIFI_AP "my_network" /*!< Define your SSID network */
#define WIFI_PASSWORD "my_password" /*!< Define your password */
#define WIFI_AUTH LWIFI_WPA  /*!< LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP. */
#define SITE_URL "my_server_IP"
#define SITE_PORT "my_server_port"
#define HTTP_REQUEST "POST /api/images/upload HTTP/1.1"
#define USER_AGENT "User-Agent: LinkIt/1.0"
#define CONTENT_TYPE "Content-Type: application/x-www-form-urlencoded;"
#define CONTENT_LENGTH "Content-Length: "
#define ID "id="
#define LINKIT_ID "LNKT0001"
#define TIMESTAMP "&timestamp="
#define TIMESTAMP_EXAMPLE "1449235906"
#define LAT_VAR "&lat="
#define LON_VAR "&lon="
#define IMAGE1 "&image1="
#define IMAGE2 "&image2="
#define IMAGE3 "&image3="
#define IMAGE4 "&image4="
#define RECURRENCE_TIME 60000

/*!< Important global variables */
const int CS1 = 4; /*!< SPI Chip Select (cameras) */
const int CS2 = 5;
const int CS3 = 6;
const int CS4 = 7;
ArduCAM myCAM1(OV5642, CS1); /*!< ArduCAM instances */
ArduCAM myCAM2(OV5642, CS2);
ArduCAM myCAM3(OV5642, CS3);
ArduCAM myCAM4(OV5642, CS4);
bool cam1 = true, cam2 = true, cam3 = true, cam4 = true; /*!< SPI Chip detection */
GPSWaypoint* gpsPosition; /*!< Some instances and variables to get GPS location*/
char buff[256];
char* buffer_latitude;
char* buffer_longitude;
unsigned long previous_time = 0; /*!< Time counter init */
unsigned short int pic_checker = 0; /*!< Counter that checks if 4 pictures have been really taken or not */
unsigned long timestamp = 0;
LWiFiClient c;

/*
 * Function: setup
 * ----------------------------
 *   First executed function:
        - Initializes and test hardware to be sure it is working properly
        before operating.
        - Identifies camera model. Sets image format & resolution for the cameras.
        Erase Arduchip FIFO's content (old pics)
        - Enables low power mode on cameras.
 *
 *   receives: nothing
 *   returns: nothing
 * ----------------------------
 */

void setup() {
  uint8_t vid, pid;
  uint8_t temp;
  Serial.begin(115200);
  SPI.begin(); /*!< initializes SPI bus */
  Wire.begin(); /*!< initializes I2C bus */
  Serial.println("Serial setup...");
  pinMode(CS1, OUTPUT); /*!< set the SPI_CS as an output */
  pinMode(CS2, OUTPUT);
  pinMode(CS3, OUTPUT);
  pinMode(CS4, OUTPUT);
  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, HIGH);
  digitalWrite(CS3, HIGH);
  digitalWrite(CS4, HIGH);
  delay(100);

  /*!< Check if the ArduCAM SPI bus is OK (for each camera)*/
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

  /*!< Check the camera model taking a look at a camera register specified in the libraries*/
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

  /*!< Sets format to JPEG and resolution to 1920x1080*/
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

  LWiFi.begin();

  /*!< Trying to connect to the WiFi AP */
  Serial.println(F("Connecting to AP"));
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    /*!< Retrying until connected */
    Serial.println(F("Retrying connection to AP!"));
    delay(1000);
  }

  Serial.println(F("Connection established!"));
}

/*
 * Function: loop
 * ----------------------------
 *   Loops consecutively:
        - Powers up GPS module and gets GPS location for the device.
        - Powers up cameras, takes 4 simultaneous pictures, enables low power.
        - Powers up WiFi module and connects to the image processing server.
        - Tests if 4 pictures have been taken and stored properly.
        - Measure the amount of time needed to execute the whole process.
 *
 *   receives: nothing
 *   returns: nothing
 * ----------------------------
 */

void loop() {
  Serial.println("Everything configured properly. Let's take some pictures...");
  Serial.println("Getting GPS Data");
  pic_checker = 0; /*!< Initialize pictures counter to 0 */
  LGPS.powerOn(); /*!< Get GPS data and upload location and speed*/
  char GPS_formatted[] = "GPS fixed";
  gpsPosition = new GPSWaypoint();
  gpsSentenceInfoStruct gpsDataStruct;
  getGPSData(gpsDataStruct, GPS_formatted, gpsPosition);
  buffer_latitude = new char[30];
  sprintf(buffer_latitude, "%2.6f", gpsPosition->latitude);
  buffer_longitude = new char[30];
  sprintf(buffer_longitude, "%2.6f", gpsPosition->longitude);
  LGPS.powerOff();
  Serial.print(F("latitude: "));
  Serial.println(buffer_latitude);
  Serial.print(F("longitude: "));
  Serial.println(buffer_longitude);

  Serial.println(F("ArduCAM start-up..."));
  /*!< Powering up cameras*/
  delay(1000);
  myCAM1.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  myCAM2.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  myCAM3.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  myCAM4.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  delay(1000);

  /*!< Clear old pictures and start a new capture*/
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

  /*!< Pictures correctly taken?*/
  while (!myCAM1.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) && cam1);
  while (!myCAM2.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) && cam2);
  while (!myCAM3.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) && cam3);
  while (!myCAM4.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) && cam4);

  /*!< Enables low power mode */
  myCAM1.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  myCAM2.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  myCAM3.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  myCAM4.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);

  Serial.println(F("Pictures taken!"));
  previous_time = millis();

  /*!< Trying to connect to server_IP:3000 */
  Serial.println("Connecting to the server...");
  while (0 == c.connect(SITE_URL, 3000))
  {
    Serial.println("Re-Connecting to the server");
    delay(1000);
  }
  Serial.println("Connected to the server");
  timestamp = get_timestamp(c);

  Serial.println("Connecting to the server...");
  while (0 == c.connect(SITE_URL, 3000))
  {
    Serial.println("Re-Connecting to the server");
    delay(1000);
  }
  Serial.println("Connected to the server");
  /*!< If there is a camera attached to CS 4, read image bytes */
  if (cam1 == true)
  {
    read_fifo_burst_encode(myCAM1, c, IMAGE1);
    get_http_response_pics(c);
    myCAM1.clear_fifo_flag();
  }
  Serial.println(F("CAM1: capture done!"));
  delay(1000);

  /*!< If there is a camera attached to CS 5, read image bytes */
  Serial.println("Connecting to the server...");
  while (0 == c.connect(SITE_URL, 3000))
  {
    Serial.println("Re-Connecting to the server");
    delay(1000);
  }
  if (cam2 == true)
  {
    read_fifo_burst_encode(myCAM2, c, IMAGE2);
    get_http_response_pics(c);
    myCAM2.clear_fifo_flag();
  }
  Serial.println(F("CAM2: capture done!"));
  delay(1000);

  /*!< If there is a camera attached to CS 6, read image bytes */
  Serial.println("Connecting to the server...");
  while (0 == c.connect(SITE_URL, 3000))
  {
    Serial.println("Re-Connecting to the server");
    delay(1000);
  }
  if (cam3 == true)
  {
    read_fifo_burst_encode(myCAM3, c, IMAGE3);
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

  /*!< If there is a camera attached to CS 7, read image bytes */
  if (cam4 == true)
  {
    read_fifo_burst_encode(myCAM4, c, IMAGE4);
    get_http_response_pics(c);
    myCAM4.clear_fifo_flag();
  }
  Serial.println(F("CAM4: capture done!"));

  /*!< measure how long it took to upload the pictures */
  if (pic_checker == 4) {
    Serial.println(F("4 pictures uploaded"));
    Serial.print(F("Elapsed time: "));
    Serial.println(millis() - previous_time);
  }
  else {
    Serial.println(F("Error: couldn't take 4 pictures"));
  }
  /*!< PICTURES_RECURRENCY = TIME TO UPLOAD 4 PICTURES + RECURRENCE_TIME */
  delay(RECURRENCE_TIME);
}

/*
 * Function: read_fifo_burst_encode
 * ----------------------------
 *   Reads byte by byte from FIFO every camera has the picture stored on it.
 *   Then performs a HTTP POST Request to the server.
 *
 *   To avoid memory issues on board, images have to be transmitted splitted
 *   into 5kB chunks approximately. We store them on the heap, dinamically
 *   allocating every image packet. The consecutive data chunks are converted
 *   to Base64 (https://en.wikipedia.org/wiki/Base64) to transmit them over
 *   a channel that is not prepared to deal with binary data itself (non-printable
 *   characters). It adds almost a 33% of overload per image in total.
 *
 *   receives:
 *      myCAM: ArduCAM camera class. Gives us access to control functions.
 *      client: WiFi client opened -> |Already connected to the server|
 *      image: declared on top (pre-processor definitions) to identify each
 *             image when they are transmitted via HTTP.
 *   returns: nothing
 * ----------------------------
 */

void read_fifo_burst_encode(ArduCAM myCAM, LWiFiClient client, char * image) {
  char temp, temp_last; /*!< JPEG tail is composed by FF D9. Knowing which byte
                            was the last one helps us to identify the end of an image
                            to begin transmissions */
  uint32_t length = 0;
  char * image_chunk;
  char * encoded_chunk;
  char * encoded;
  size_t total_encoded_len;
  size_t encodedLen;

  length = myCAM.read_fifo_length();
  if (length >= 524288 ) /*!< 524 kB at most */
  {
    Serial.println("Over size.");
    return;
  }
  else if (length == 0 ) /*!< 0 kB at least */
  {
    Serial.println("Size is 0.");
    return;
  }
  else { /*!< Length OK */
    int VM_TCP_RESULT0 = 0; /*!< WiFi module response: bytes transmitted or error code */
    int index = 0; /*!< Iterator */
    size_t encodedLen; /*!< Encoded image length */
    char * ptr_encoded_chunk; /*!< Auxiliar pointer to image chunks */
    /*!< Dynamically allocating memory space to image chunks (both raw and decoded) */
    image_chunk = (char*) calloc (4000, sizeof(char));
    if (image_chunk == NULL) return;
    encoded_chunk = (char*) calloc (5500, sizeof(char));
    if (encoded_chunk == NULL) return;
    total_encoded_len = 4 * ((length + 2) / 3); /*!< To calculate Base64 overload */

    myCAM.CS_LOW(); /*!< Selects the camera chip (SPI) - Active low */
    myCAM.set_fifo_burst(); /*!< Set fifo burst mode */

    Serial.println("Transmitting data SPI - WiFi client...");
    client.println(F("POST /api/images/upload HTTP/1.1"));
    client.println("Host: " SITE_URL);
    client.println(F("User-Agent: LinkIt/1.0"));
    client.println(F("Connection: keep-alive"));
    client.println(F("Content-Type: application/x-www-form-urlencoded"));
    client.print(F("Content-Length: "));  /*!< Mandatory parameter (really important
                                          to calculate it well). Otherwise requests won't
                                          reach the server */
    client.println(strlen(ID) + strlen(LINKIT_ID) + strlen(LAT_VAR) + strlen(buffer_latitude) + strlen(LON_VAR) + strlen(buffer_longitude) + strlen(TIMESTAMP) + strlen(timestamp) + strlen(image) + total_encoded_len);
    client.println();
    client.print(ID);
    client.print(LINKIT_ID);
    client.print(LAT_VAR);
    client.print(buffer_latitude);
    client.print(LON_VAR);
    client.print(buffer_longitude);
    client.print(TIMESTAMP);
    client.print(timestamp);
    client.print(image);
    /*!< While we have bytes to be read... */
    length--;
    while ( length--) {
      temp_last = temp; /*!< we stored the new byte and set the last one too */
      temp =  SPI.transfer(0x00); /*!< Reading a byte from SPI */
      image_chunk[index] = temp; /*!< storing the new byte */
      index++;
      /*!< If we took 3750 bytes (results in 5000 Base64 characters) or we finished reading, upload image chunk.
        3750 is the value calculated to avoid new dummy bytes introduced by Base64 encoding */
      if ((temp == 0xD9) && (temp_last == 0xFF))
        length = 0;
      if (index == 3750 || length == 0) {
        encoded_chunk = base64_encode(image_chunk, index, &encodedLen);
        ptr_encoded_chunk = encoded_chunk;
        while (encodedLen > 0) {
          VM_TCP_RESULT0 = client.write((uint8_t *)ptr_encoded_chunk, encodedLen);
          if (VM_TCP_RESULT0 > 0 && encodedLen > 0) {
            ptr_encoded_chunk += VM_TCP_RESULT0; /*!< advancing pointer until next first byte of the chunk */
            encodedLen -= VM_TCP_RESULT0;
          } else {
            Serial.print(VM_TCP_RESULT0);
            Serial.println(" -> value ignored");
          }
          delay(10); /*!< Needed to give the module time to transmit what it has in the queue */
        }
        index = 0;
      }
      delayMicroseconds(12);
    }
    client.println();
  }
  free(image_chunk); /*!< Freeing dynamically allocated variables */
  free (encoded_chunk);
  myCAM.CS_HIGH();
  return;
}

/*
 * Function: get_http_response_pics
 * ----------------------------
 *   Used to parse server response in order to know if pictures have been transmitted
 *   correctly. A counter is incremented if succesfully transmitted. Once we have 4
 *   pictures transmitted, we are allowed to take another 4 pictures more.
 *
 *
 *   receives:
 *      client: WiFi client opened waiting for server response
 *   returns: nothing
 * ----------------------------
 */

void get_http_response_pics(LWiFiClient client) {
  boolean disconnectedMsg = false;
  char inBuffer [250];
  int bufferIdx = 0;
  String buff;
  String aux;

  // waiting for server response
  Serial.println("waiting HTTP response:");
  // waiting for server response
  while (!client.available())
  {
    delay(100);
  }
  while (client)
  {
    int v = client.read();
    if (v != -1)
    {
      Serial.print((char)v);
      inBuffer[bufferIdx++] = (char)v;
      aux += (char)v;
    }
    else
    {
      client.stop();
    }
  }
  int json_array_start = aux.indexOf("{") + 11;
  int json_array_end = aux.indexOf("}") - 1;
  for (int i = json_array_start; i < json_array_end; i++) {
    buff += inBuffer[i];
  }
  if (buff.equals("OK")){ 
    pic_checker++;  
    Serial.println(F(""));
    Serial.println(F("Successfully uploaded!"));
  }
  else 
    Serial.println(F("Error uploading picture"));
    
  if (!disconnectedMsg)
  {
    Serial.println(F(""));
    Serial.println(F("Disconnected by server"));
    disconnectedMsg = true;
  }
}

/*
 * Function: get_timestamp
 * ----------------------------
 *   Used to get a timestamp needed to identify photographs on the server.
 *   Performs an HTTP GET request to the server in order to obtain it.
 *
 *   receives:
 *      client: WiFi client opened waiting for server response
 *   returns: timestamp (converted to String)
 * ----------------------------
 */

String get_timestamp(LWiFiClient client) {
  boolean disconnectedMsg = false;
  char inBuffer [300];
  int bufferIdx = 0;
  String buff;
  String aux;

  Serial.println("send HTTP GET request");
  client.println("GET /api/timestamp HTTP/1.1");
  client.println("Host: " SITE_URL);
  client.println();

  // waiting for server response
  Serial.println("waiting HTTP response:");
  // waiting for server response
  while (!client.available())
  {
    delay(100);
  }
  while (client)
  {
    int v = client.read();
    if (v != -1)
    {
      inBuffer[bufferIdx++] = (char)v;
      aux += (char)v;
    }
    else
    {
      client.stop();
    }
  }
  int json_array_start = aux.indexOf("{") + 13;
  int json_array_end = aux.indexOf("}");
  for (int i = json_array_start; i < json_array_end; i++) {
    buff += inBuffer[i];
  }

  if (!disconnectedMsg)
  {
    Serial.println(F("Disconnected by server"));
    disconnectedMsg = true;
  }

  return buff;
}

