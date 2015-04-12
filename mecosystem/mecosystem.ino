/* MIT License
 * Hannon
 * channon@hawk.iit.edu
 * help? support@mecosystemlabs.com
 * woot! go food
 *
 *
 */

//type 1 for yun 0 for other
#define YUN 1
//temp
#include <OneWire.h>
//humidity
#include <DHT.h>
//4char
//#include <SoftwareSerial.h>
//yun
#include <Bridge.h>
#include <Console.h>
#include <FileIO.h>
#include <HttpClient.h>
#include <Mailbox.h>
#include <Process.h>
#include <YunClient.h>
#include <YunServer.h>

#define YUN_SERVER_PORT 5678

//ph
#define phOFF 0.00
#define ArrayLenth  40    //times of collection
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;
//what do these do?
#define samplingInterval 20
#define printInterval 800
//end ph

/* Data Pins*/
#define lightT 3
#define lightB  4
#define lightF 5
#define pump 6
#define heater 7
#define other 8
#define temp 9
#define tempPlant 10
#define tempMush 11

#define ph A0


////////////////////////////////   humidity  /////////////////////////////
#define DHTPIN 12     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
// NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
// might need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// Example to initialize DHT sensor for Arduino Due:
//DHT dht(DHTPIN, DHTTYPE, 30);
/////////////////////////////    end humidity   //////////////////////////

//TODO: flippin organize this better

//who made these names???? satan, I hope i didnt
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
////////////////    time   //////////////////
int s = 55;
int m = 25;
int h = 10;
////////////////   END time    ///////////////

//flag
int sched = 0;

/////////////////////   server stuff   /////////////////
#ifdef YUN
YunServer server(YUN_SERVER_PORT);
YunClient client;
#endif /* YUN */
////////////////////  End Server Stuff //////////////////

////////////////////// SETUP STUFF  ///////////////////////
void setup() {
  // put your setup code here, to run once:
  pinMode(lightT, OUTPUT);
  pinMode(lightB, OUTPUT);
  pinMode(lightF, OUTPUT);
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

#if YUN  //setup bridge
  Bridge.begin();
  server.listenOnLocalhost();
  server.begin();
#endif

  //humidity
  dht.begin();
}
/////////////////////////////////   END SETUP   //////////////////////////

//////////////////////////////// TIMER 1 INTERRUPT  ///////////////////////
ISR(TIMER1_COMPA_vect) {          // timer compare interrupt service routine
  //This happens every 1 second (earth time)
  s++;
  if (s == 60) { //aka 1 minute
    m++;
    s = 0;       // reset second count
    //check sched flag
    sched = 1;  // this runs the scheduling routine to check if any relays need to be checked
  }
  if (m == 60) { // aka 1 hour
    h++;
    m = 0;       // reset min count
  }
  if (h == 24) {
    h = 0;
  }
  /* should we count days?
   * do plants count days?
   * basil - "Hey guys its friday, anyone wanna go to the pub?
   * hops - "Yo, I'm in"
   */
}
//////////////////////////  END TIMER 1 INTURRUPT /////////////////

//////////////////////////  main loop  ////////////////////////////
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
      if (h == (lightThr + lightTdhr) % 24) {
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
      if (h == (lightBhr + lightBdhr) % 24) {
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
      if (h == (lightFhr + lightFdhr) % 24) {
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
#if (YUN)
  if (client.connected()) {
    //handle client
    String data = (String(lightTmin) + ',' + String(lightBmin) + ',' + String(lightFmin) + ',' +
                   String(lightThr) + ',' + String(lightBhr) + ',' + String(lightFhr) + ',' +
                   String(lightTdhr) + ',' + String(lightBdhr) + ',' + String(lightFdhr) + ',' +
                   String(lightTdmin) + ',' + String(lightBdmin) + ',' + String(lightFdmin) + ',' +
                   String(pumpFreq) + ',' + String(pumpD) + ',' +
                   String(setTemp) + ',' +
                   String(s) + ',' + String(m) + ',' + String(h)
                  );
    client.println(58.123456, 5);
    Serial.println(data);
  }
  else {
    client = server.accept();
    if (client.connected()) {
      Serial.println("User is connected!");
    }
  }

#endif

  printTime();
}
/////////////////////////////// END MAIN LOOP //////////////////////////////

///////////////////////////////  HELPER FUNCTIONS   ////////////////////////
void printTime(void) {
  Serial.print(h);
  Serial.print(":");
  Serial.print(m);
  Serial.print(":");
  Serial.println(s);
}

//self explanitory
void relayOn(int Pin) {
  digitalWrite(Pin, HIGH);
}
void relayOff(int Pin) {
  digitalWrite(Pin, LOW);
}

// this is tricky, we need to wait maybe 90 seconds for a pump load
// but the scheduler can only relay off every min, so hence
void relayOffwDelay(int Pin, int del) {
  Serial.println("waiting to offpump");
  Serial.println(del);
  delay(del * 1000);
  Serial.println("offpump");
  digitalWrite(Pin, LOW);
}

// make-belive instance:
// so the clock got set for 5pm... but the scheduler turns the lights on at 4pm
// We dont want to wait 23 hours so we need to check all the lights
void checkIfShouldBeOn() {
  //TODO
}

////////////////////////////////////// TEMPS  ///////////////////////////////
//temp is pin 9 (temp)
//tempPlant, 
// I think One-Wire sensors should be replaced. 
// They just make everything difficult with addresses and whatnot
//but they work
float getTempPlants() {
  OneWire ds(tempPlant);

  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    //ds.reset_search();
    delay(250);
    return 0;
  }


  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  //delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
  return fahrenheit;

}

float getTempMush() {
  OneWire ds(tempMush);

  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    //ds.reset_search();
    delay(250);
    return 0;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  //delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
  return fahrenheit;

}

float getTempFish() {
  OneWire ds(temp);

  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    //ds.reset_search();
    //delay(250);
    return 0;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  //delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
  return fahrenheit;
}
// Thank satan thats over...
////////////////////////////////////// END TEMP //////////////////////////////

float getPH() {
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;
  if(millis()-samplingTime > samplingInterval)
  {
    pHArray[pHArrayIndex++]=analogRead(ph);
    if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
    voltage = avergearray(pHArray, ArrayLenth)*5.0/1024;
    pHValue = 3.5*voltage+phOFF;
    samplingTime=millis();
  }
  if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    Serial.print("Voltage:");
    Serial.print(voltage,2);
    Serial.print("    pH value: ");
    Serial.println(pHValue,2);
    printTime=millis();
    return pHValue;
  }
}

//////////////////////////////////// HUMIDITY ///////////////////////////////
float getHumidityPlants() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return 0;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
  float hi = dht.computeHeatIndex(f, h);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hi);
  Serial.println(" *F");
  return h;
}
///////////////////// END HUMIDITY ////////////////////////////

//TODO: WE need a mushroom Humidity 

// ph helper .. ikr 
double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n"); //wow who wrote this?
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }
  else{
    if(arr[0]<arr[1]){
      min = arr[0];
      max=arr[1];
    }
    else{
      min=arr[1];
      max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }
      else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }
        else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}

////////////////////////  END HELPERs  ///////////////////////////////


