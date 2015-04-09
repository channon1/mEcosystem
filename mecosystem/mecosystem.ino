#include <Bridge.h>
#include <Console.h>
#include <FileIO.h>
#include <HttpClient.h>
#include <Mailbox.h>
#include <Process.h>
#include <YunClient.h>
#include <YunServer.h>

/* MIT License
 *Hannon
 * channon@hawk.iit.edu
 *support@mecosystemlabs.com
 *
 *
 *
 */

#define YUN_SERVER_PORT 1993


/* Data Pins*/
#define lightT 3
#define lightB  4
#define lightF 5
#define pump 6
#define heater 7
#define other 8
#define temp 9
#define ph A0

//global variables
//data variables
//min on
int lightTmin = 0;
int lightBmin = 5;
int lightFmin = 0;
//hour on
int lightThr = 18;
int lightBhr = 18;
int lightFhr = 07;
//duration
int lightTdhr = 14;
int lightBdhr = 14;
int lightFdhr = 10;
int lightTdmin = 0;
int lightBdmin = 0;
int lightFdmin = 0;
//pump
int pumpFreq = 90; //min
int pumpD = 210; // sec
//temp
int setTemp = 78;
//time
int s = 55;
int m = 25;
int h = 10;
//flag
int sched = 0;

//server stuff
YunServer server(YUN_SERVER_PORT);
YunClient client;

void setup() {
  // put your setup code here, to run once:
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, INPUT);
  pinMode(A0, INPUT);
  Serial.begin(115200);
  // initialize timer1
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 62500;            // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts

//setup bridge
  Bridge.begin();
  server.listenOnLocalhost();
  server.begin();

}


ISR(TIMER1_COMPA_vect) {          // timer compare interrupt service routine
  s++;
  if (s == 60) {
    //mina ++
    m++;
    s = 0;
    //check sched flag
    sched = 1;
  }
  if (m == 60) {
    h++;
    m = 0;
  }
  if (h == 24) {
    h = 0;
  }
}
void loop() {
  // put your main code here, to run repeatedly:
  if (sched) {
    Serial.println("in sched");
    sched = 0;
    //check stuff
    //lightT
    if (m == lightTmin) {
      if (h == lightThr) {
        Serial.println("relaying T");
        relayOn(lightT);
      }
    }
    if (m == (lightTmin + lightTdmin)) {
      if (h == (lightThr + lightTdhr)%24) {
        relayOff(lightT);
      }
    }
    //rest ifs
    //lightB
    if (m == lightBmin) {
      if (h == lightBhr) {
        Serial.println("relaying B");
        relayOn(lightB);
      }
    }
    if (m == (lightBmin + lightBdmin)) {
      if (h == (lightBhr + lightBdhr)%24) {
        relayOff(lightB);
      }
    }
    //lightF
    if (m == lightFmin) {
      if (h == lightFhr) {
        Serial.println("relaying F");
        relayOn(lightF);
      }
    }
    if (m == (lightFmin + lightFdmin)) {
      if (h == (lightFhr + lightFdhr)%24) {
        relayOff(lightF);
      }
    }
    //pump
    long mTemp  = m + h * 60;
    int sTempP = (pumpD % 60);
    Serial.println(sTempP);//+ mTemp);
    int mTempd = (pumpD - sTempP) / 60;
    if (mTemp % pumpFreq == 0) {
      Serial.println("pumping");
      relayOn(pump);
    }
    if (mTemp % pumpFreq == mTempd) {
      relayOffwDelay(pump, sTempP);
    }
  }
  delay(500);
  
  if(client.connected()) {
     //handle client
     String data = (String(lightTmin)+','+String(lightBmin)+','+String(lightFmin)+','+
             String(lightThr)+','+String(lightBhr)+','+String(lightFhr)+','+
             String(lightTdhr)+','+String(lightBdhr)+','+String(lightFdhr)+','+
             String(lightTdmin)+','+String(lightBdmin)+','+String(lightFdmin)+','+
             String(pumpFreq)+','+String(pumpD)+','+
             String(setTemp)+','+
             String(s)+','+String(m)+','+String(h)             
             );
     client.println(data,20);
     Serial.println(data);
  }
  else {
     client = server.accept();
      if (client.connected()) {
         Serial.println("User is connected!");
      } 
  }
  
  Serial.print(h);
  Serial.print(":");
  Serial.print(m);
  Serial.print(":");
  Serial.println(s);
}
void relayOn(int Pin) {
  digitalWrite(Pin, HIGH);
}
void relayOff(int Pin) {
  digitalWrite(Pin, LOW);
}
void relayOffwDelay(int Pin, int del) {
  Serial.println("waiting to offpump");
  Serial.println(del);
  delay(del*1000);
  Serial.println("offpump");
  digitalWrite(Pin, LOW);
}
