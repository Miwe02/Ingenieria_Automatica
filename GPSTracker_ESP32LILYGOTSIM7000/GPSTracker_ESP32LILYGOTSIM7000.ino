#define TINY_GSM_MODEM_SIM7070
#define TINY_GSM_RX_BUFFER 1024
#define SerialAT Serial1
#include "driver/rtc_io.h"
#define WAKEUP_GPIO              GPIO_NUM_26

#define uS_TO_S_FACTOR 1000000ULL 
#define TIME_TO_SLEEP  120


#define GSM_PIN ""

const char apn[]  = "nauta";     
const char gprsUser[] = "nauta";
const char gprsPass[] = "nauta";

#define SMS_TARGET  "+5353773736"

#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
#include <Ticker.h>
#include<Wire.h>

#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, Serial);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif


#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4

#define SD_MISO     2
#define SD_MOSI     15
#define SD_SCLK     14
#define SD_CS       13
#define LED_PIN     12


#define I2C_SDA 21
#define I2C_SCL 22

const char str[] = "temp here: ";
char valueStr[25]; 


int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input;
String msg;
char message[] = "";




void setup(){
 
  Serial.begin(115200);
  delay(10);

  Wire.begin(I2C_SDA,I2C_SCL);

  pinMode(WAKEUP_GPIO, INPUT);


  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(300);
  digitalWrite(PWR_PIN, LOW);

  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SDCard MOUNT FAIL");
  } else {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    String str = "SDCard Size: " + String(cardSize) + "MB";
    Serial.println(str);
  }

  Serial.println("\nWait...");

  delay(1000);

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);

  rtc_gpio_pullup_dis(WAKEUP_GPIO);
  rtc_gpio_pulldown_en(WAKEUP_GPIO);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    
  Serial.println("Initializing modem...");
  if (!modem.restart()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  Serial.println("Initializing modem...");
  if (!modem.init()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  String name = modem.getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  delay(500);
  Serial.println("Modem Info: " + modemInfo);
  
  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
      modem.simUnlock(GSM_PIN);
  }
  modem.sendAT("+CFUN=0 ");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" +CFUN=0  false ");
  }
  delay(200);

  String res;

  res = modem.setNetworkMode(2);
  if (res != "1") {
    DBG("setNetworkMode  false ");
    return ;
  }
  delay(200);

  res = modem.setPreferredMode(1);
  if (res != "1") {
    DBG("setPreferredMode  false ");
    return ;
  }
  delay(200);

  modem.sendAT("+CFUN=1 ");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" +CFUN=1  false ");
  }
  delay(200);

  SerialAT.println("AT+CGDCONT?");
  delay(500);
  if (SerialAT.available()) {
   
    input = SerialAT.readString();
    for (int i = 0; i < input.length(); i++) {
      if (input.substring(i, i + 1) == "\n") {
        pieces[counter] = input.substring(lastIndex, i);
        lastIndex = i + 1;
        counter++;
       }
        if (i == input.length() - 1) {
          pieces[counter] = input.substring(lastIndex, i);
        }
      }
    
      input = "";
      counter = 0;
      lastIndex = 0;

      for ( int y = 0; y < numberOfPieces; y++) {
        for ( int x = 0; x < pieces[y].length(); x++) {
          char c = pieces[y][x];  //gets one byte from buffer
          if (c == ',') {
            if (input.indexOf(": ") >= 0) {
              String data = input.substring((input.indexOf(": ") + 1));
              if ( data.toInt() > 0 && data.toInt() < 25) {
                modem.sendAT("+CGDCONT=" + String(data.toInt()) + ",\"IP\",\"" + String(apn) + "\",\"0.0.0.0\",0,0,0,0");
              }
              input = "";
              break;
            }
          // Reset for reuse
          input = "";
         } else {
          input += c;
         }
      }
    }
  } else {
    Serial.println("Failed to get PDP!");
  }

  Serial.println("\n\n\nWaiting for network...");
  if (!modem.waitForNetwork()) {
    delay(10000);
    return;
  }

  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }
  
  modem.disableGPS();

  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" SGPIO=0,4,1,0 false ");
  }
  Serial.println("\n---End of GPRS TEST---\n");


  // --------SMS--------
  res = modem.sendSMS(SMS_TARGET, String("Bienvenido, envie 'GPS' para conocer ubicacion"));
  DBG("SMS:", res ? "OK" : "fail");
  Serial.println("Bienvenido, envie 'GPS' para conocer ubicacion");


  modem.sendAT("+CMGF=1");                                                  
  modem.waitResponse(1000L);
  modem.sendAT("+CNMI=2,2,0,0,0\r");
  delay(100);


}

void Read_SMS() { 

  
      
 
      modem.sendAT("+CMGL=\"ALL\"");  
      modem.waitResponse(5000L);
      

      String data = SerialAT.readString();
      Serial.println(data);

 
      
      if(data.indexOf("+CMT:") > 0)

      {

      parseSMS(data);

      
       
      }     
      
      modem.sendAT("+cmgd=,4");
      modem.waitResponse(1000L);
}


void parseSMS(String data) {


data.replace(",,", ",");
data.replace("\r", ",");
data.replace("\"", "");
Serial.println(data);

String for_mess = data;

char delimiter = ',';
String date_str =  parse_SMS_by_delim(for_mess, delimiter,2);
String time_str =  parse_SMS_by_delim(for_mess, delimiter,3);
String message_str =  parse_SMS_by_delim(for_mess, delimiter,4);

Serial.println("************************");
Serial.println(date_str);
Serial.println(time_str);
Serial.println(message_str);
Serial.println("************************");

if(message_str== "GPS"){
  Serial.println("Buscando direccion GPS...");

  modem.sendAT("+SGPIO=0,4,1,1");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" SGPIO=0,4,1,1 false ");
  }
  modem.enableGPS();
  float lat,  lon;
  while (1) {
    if (modem.getGPS(&lat, &lon)) {
      Serial.printf("lat:%f lon:%f\n", lat, lon);
      break;
    } else {
      Serial.print("getGPS ");
      Serial.println(millis());
    }
    delay(2000);
  }

  modem.sendSMS(SMS_TARGET, String("Latitud: ")+lat+String(" Longitud: ")+lon);
  Serial.println("Se envio la latitud y la longitud ");

Serial.print("Entrando en modo DeepSleep");
esp_deep_sleep_start();

  
  }else{
    modem.sendSMS(SMS_TARGET, "Mensaje incorrecto, escriba GPS");
    }

}

String parse_SMS_by_delim(String sms, char delimiter, int targetIndex) {

  int delimiterIndex = sms.indexOf(delimiter);
  int currentIndex = 0;

  while (delimiterIndex != -1) {
    if (currentIndex == targetIndex) {
    
    String targetToken = sms.substring(0, delimiterIndex);
    targetToken.replace("\"", "");
    targetToken.replace("\r", "");
    targetToken.replace("\n", "");
    return targetToken;
    }

    // Move to the next token
    sms = sms.substring(delimiterIndex + 1);
    delimiterIndex = sms.indexOf(delimiter);
    currentIndex++;
  }

  return "";
}



void loop()
{
 
Read_SMS();


}
