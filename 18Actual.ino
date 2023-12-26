/*
rH & Tempurature data logging system for 18 composte bins, a part of a postgraduate research project, USJ
L.Swarnajith
Last Update - 2022-10-29
*/

#include <SPI.h>
#include <SD.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <Adafruit_AHT10.h>

#define INTERVAL 5

#define SD_ERR  0xE0
#define AHT_ERR 0xE1
#define CSV_ERR 0xE2

#define SIPO_NUM 0x03
#define SIPO_DAT 0x03
#define SIPO_CLK 0x04
#define SIPO_LAT 0x05
#define SIPO_OEN 0x06

#define RTC_SDA 0x07
#define RTC_SCL 0x08
#define RTC_RST 0x09

#define countof(a) (sizeof(a) / sizeof(a[0]))

ThreeWire myWire(RTC_SDA, RTC_SCL, RTC_RST);
RtcDS1302<ThreeWire> rtc(myWire);
Adafruit_AHT10 aht;

uint8_t * dValues;
char timeBuf[0x20];
String outputBuf;

void setup() {
  
  outputBuf.reserve(0x20);

  Serial.begin(9600);
  initSIPO();
  rtc.Begin();
  SD.begin(0x0A);
  if (!rtc.IsDateTimeValid()) rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  
}

void loop() {
  
  getDateTime();
  Serial.println(rtc.GetDateTime().Minute());
  if (rtc.GetDateTime().Minute()%INTERVAL==0){
    if (rtc.GetDateTime().Second() <= 10) for (int i=1; i<=18; i++) readBin(i);
  }
  
}

void initSIPO(){
  
  pinMode(SIPO_CLK, OUTPUT);
  pinMode(SIPO_DAT, OUTPUT);
  pinMode(SIPO_LAT, OUTPUT);
  pinMode(SIPO_OEN, OUTPUT);

  dValues = (uint8_t *)malloc(SIPO_NUM * sizeof(uint8_t));
  memset(dValues, 0, SIPO_NUM * sizeof(uint8_t));
  setAllSIPO();
  
}

void setAllSIPO(){
  
  for (int i=SIPO_NUM-1; i>=0; i--) shiftOut(SIPO_DAT, SIPO_CLK, LSBFIRST, dValues[i]);
  digitalWrite(SIPO_LAT, HIGH);
  digitalWrite(SIPO_LAT, LOW);
  
}

void readBin(uint8_t binNumber){
  
  for (int i=0; i<SIPO_NUM; i++) dValues[i] = 0;
  setAllSIPO();
  binNumber--;
  dValues[binNumber/8] |= 1 << (binNumber%8);
  setAllSIPO();
  
  delay(2000);
  aht.begin();
  sensors_event_t humidity, temp;
  
  aht.getEvent(&humidity, &temp);
  outputBuf = String(timeBuf) + ", " + String(binNumber) + ", " + String(temp.temperature) + " C, " + String(humidity.relative_humidity) + " %\n";
  Serial.println(outputBuf);
  
  File csv = SD.open("logFile.csv", FILE_WRITE);
  if (csv){
    csv.print(outputBuf);
    csv.close();
  }
  
}

void getDateTime(){
  
  RtcDateTime dt = rtc.GetDateTime();

  snprintf_P(timeBuf, 
          countof(timeBuf),
          PSTR("%02u/%02u/%04u %02u:%02u"),
          dt.Month(),
          dt.Day(),
          dt.Year(),
          dt.Hour(),
          dt.Minute() );
          
}
