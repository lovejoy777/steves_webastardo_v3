//*********************************************************************************************
// DIY Webasto Controller
// Board:  Adafruit Feather Huzzah32 or Feather M0 Express.
// Add the following board link in preferences: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
// Code based on a Webasto Shower Controller by David McLuckie
// https://davidmcluckie.com/arduino-webasto-shower-project/ 
// His project was based on the work of Mael Poureau
// https://maelpoureau.com/webasto_shower/
//
// Simon changed it from a Shower to a general purpose controller which tries to regulate the 
// Temperature to a pre-defined target by adjusting the Fueling and Combustion fan
// Simon converted the code to run on an Adafruit Feather M0 SAMD21 and designed a drop-in
// replacement PCB for Webasto ThermoTop C & E Heaters.

// Steve converted the code again to run on an Adafruit Feather Huzzah32 wifi and Adafruit Feather Mo Express boards and made some edits, see below.
//
// The wiring harness is similar to the original, except:
// 6 Pin Connector - header pins:  [Changes indicated by * ]
// 1 - Clock (+12V = Heater On)/(0V = Heater Off)
// 2*- Serial1 TX pin.
// 3*- Exhaust Thermistor (100k NTC between header pin 3 & Gnd or 4.7k using original onboard thermistor) [See Note 1].
// 4*- serial1 RX pin.
// 5*- Water Thermistor (100k NTC between header pin 5 & Gnd).
// 6 - Fuel Dosing Pump

//#########################################################################################################################
// [Note 1] The PCB has holes for the original Thermistor which you can salvage from an old unit
// It has a different 25C Resistance around 4.7k.  You will need to change R11 to 4.7k
// and in get_webasto_temp, change "Nominal resistance at 25 ºC" from 100000 to 4700.
// I've found the original thermistor not to work well with this code though.

//#########################################################################################################################
// [Note 2]
// The thermistors can be problematic with this code when not using the current sensor for the flame sensor. 
// (make sure to position the thermistor carefully) inside the exhaust is too hot for my sensors to handle -
// but strapped to the outside of the exhaust does not reach the temperature soon enough -
// -(maybe when installed It will work best placed between the lagging and the exhaust).

//########################################################################################################################
// [Note 3]
// ADC2 has some restrictions on the feather huzzah32 wifi micro-controller:
// ADC2 is used by the Wi-Fi driver, so ADC2 is only usable when the Wi-Fi driver has not started.
// Three of the ADC2 pins are strapping pins and thus should be used with caution. Strapping pins -
// - are used during power-on/reset to configure the device boot mode, the operating voltage, and other initial settings
// Importantly, the official Adafruit docs for the Huzzah32 are confusing: -
// - they state that “you can only read analog inputs on ADC #1 once WiFi has started”. 
// Through experimentation, I found that Adafruit intended to say simply that "ADC#2 is unavailable once WiFi has started" (so you can only use ADC#1).

// ADC#1 pins.
// A2,A3,A4,A7,A9.
// ADC#2 pins.
// A0,A1,A5,A6,A8,A10,A11,A12.

// IMPORTANT.
// to use ADC#1 pins we have to change WebastardoV3.0 board a little, -
// - cut 3 of the traces and wire 3 new links to the new pins, see steves' part 1 video.
// Link: https://youtu.be/f0CdLafApvU
//##############################################################################################################################

// Simon Rafferty SimonSFX@Outlook.com 2022
// The V3.0 PCB's (https://oshwlab.com/SimonRafferty/webasto-controller)
// has provision for a Thermal Fuse.  This simply cuts the fueling if the heater really overheats
// to prevent it catching fire.  I used RS Part Number 797-6042 which fuses at 121C
// If you prefer the excitement of waiting for it to catch fire, you can always bridge the 
// contacts with a bit of wire.
//
// Steven Lovejoy edits sept 2023.
// reference to header pins are referencing the 6 pin webasto connector.
// 1/ Changed board to Feather esp32 Huzzah32, see Note3.
// 2/ configured for physical switch/timer or blynk on/off, -
// - now we can have a timeclock or thermostat etc to start the heater.
// 3/ Added LED code (I use it for testing atm.
// 4/ Added uart/serial1 pins to the pin header so we can connect it to another micro-controller for a remote terminal/ -
// - infomation screen or programmable room stat etc, TX = header pin 2, RX = header pin 4.
//*********************************************************************************************

//Build options
#define BLYNK_ENABLE              //Uncomment if you want to send data to Blynk (only applies to WiFi board)
#define FLAME_SENSOR_ENABLE       //Uncomment if using V3.0 board with ACS711 Current Sensor
#define ESP_WIFI_ENABLE           //Uncomment if you are using a Feather huzzah32 WiFi microcontroller
//With all three commented out, project will compile the same as the main branch

//This bit just avoids the situation of enabling blynk, without Wifi - which would be a bit dumb huh?  Can't imagine anyone would do that?? ;-)
#ifdef BLYNK_ENABLE
  #ifndef ESP_WIFI_ENABLE
    #define ESP_WIFI_ENABLE
  #endif
#endif

#ifdef BLYNK_ENABLE
//  **BLYNK Defines MUST be before Includes
  #define BLYNK_TEMPLATE_ID "<put your blynk template id here>"
  #define BLYNK_TEMPLATE_NAME "<put your blynk template name here>"
  #define BLYNK_AUTH_TOKEN "<put your blynk auth token here>"
#endif

#include <math.h> // needed to perform some calculations
//#include <arduino_secrets.h> // used for sensative code like passwords etc.

#ifdef ESP_WIFI_ENABLE
  #include <SPI.h>
  #include <WiFi.h>
  #include <WiFiClient.h>
  #define SECRET_SSID "<put your ssid here>"
  #define SECRET_PASS "<put your password here>"
#endif

#ifdef BLYNK_ENABLE
  #include <BlynkSimpleEsp32.h>
#else
  #include <WiFiMDNSResponder.h>
#endif

//Heater Config 
//*********************************************************************************
//**Change these values to suit your application **
int heater_min = 55; // Increase fuel if below
int heater_target = 60; // degrees C Decrease fuel if above, increase if below.
int water_warning = 70;// degrees C - At this temperature, the heater idles
int water_overheat = 85;// degrees C - This is the temperature the heater will shut down

int flame_threshold = 75; //Exhaust temperature above which we assume it's alight

//Fuel Mixture
//If you find the exhaust is smokey, increase the fan or reduce the fuel
float throttling_high_fuel = 1.8;
//float throttling_high_fuel = 1.6; //In summer, exhaust gets too hot on startup
float throttling_high_fan = 90;
float throttling_steady_fuel = 1.3;
float throttling_steady_fan = 65;
float throttling_low_fuel = 0.83;  //(Winter Setting)
float throttling_low_fan = 55;
//Just enough to keep it alight at idle
float throttling_idle_fuel = 0.6; //Do not reduce this value
float throttling_idle_fan = 30; 

//*********************************
// ToDo:  Winter & summer need slightly different idle settings
// I'm guessing because the air intake temperature is higher in summer, it doesn't
// need as much fuel to heat to a given temperature.
// Using the winter setting in Summer causes it to overheat & shut down before the  
// water tank has heated properly.
// * Need to find a way of switching automatically
//********************************

// LED
int led = LED_BUILTIN;

//Fuel Pump Setting
//Different after-market pumps seem to deliver different amounts of fuel
//If the exhaust is consistently smokey, reduce this number
//If you get no fuel (pump not clicking) increase this number
//Values 22,30 or 60 seem to work in most cases.

int pump_size = 22; //22,30,60 
//**********************************************************************************
 
//Prime
float prime_low_temp = 10; //Prime ratio is temperature dependent. Below this temp, prime fueling is increased
float prime_high_temp = 20; //Not used at the moment
bool Fuel_Purge = false; //Set by blynk.  Delivers fuel rapidly without running anything else

float prime_fan_speed = 15;
float prime_low_temp_fuelrate = 3.5;
float prime_high_temp_fuelrate = 2.0;

//Inital
float start_fan_speed = 40;
float start_fuel = 1;  //Summer setting
float start_fuel_Threshold = -10; //Exhaust temperature, below which to use start_fuel_Cold
float start_fuel_Cold = 1.2;  //Winter Setting (use below 10C)
float start_fuel_Warm = 1.0;  //Winter Setting (use below 10C)

int full_power_increment_time = 30; //seconds

//Pin Connection for Adafruit Feather Huzzah32
int glow_plug_pin = 14; // 
int water_pump_pin = 15; // 
int fuel_pump_pin = 27; // header pin 6
int burn_fan_pin = 33; 
// see [Note3]
//int lambda_pin = A1;
int water_temp_pin = A2; // header pin 5
int exhaust_temp_pin = A3; // header pin 3
int flame_sensor = A4;
int push_pin = A7; // header pin 1:

/*
//Pin Connections for Adafruit Feather M0 Express
int glow_plug_pin = 5;
int water_pump_pin = 9;
int fuel_pump_pin = 11; // header pin 6
int burn_fan_pin = 10;
// see [Note3]
//int lambda_pin = A1;
int water_temp_pin = A2; // header pin 5
int exhaust_temp_pin = A3; // header pin 3
int flame_sensor = A4;
int push_pin = 6; // header pin 1:
*/

//Blynk Write Variables
int BlynkHeaterOn = 0;
int BlynkPurgeFuel = 0;

#ifndef BLYNK_ENABLE
  #ifdef ESP_WIFI_ENABLE //Setup web server if this is a wifi board and blynk not selected.

    //WiFi Setup
    //#include "Arduino_Secrets" 
    char ssid[] = SECRET_SSID;    // your network SSID (name)
    char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
    int keyIndex = 0;             // your network key Index number (needed only for WEP)
    bool WiFiACTIVE = false;
    WiFiClient client;
    
    char mdnsName[] = "webastardo"; // the MDNS name that the board will respond to
    // Note that the actual MDNS name will have '.local' after
    // the name above, so "webastardo" will be accessible on
    // the MDNS name "webastardo.local".
    
    int status = WL_IDLE_STATUS;
    
    // Create a MDNS responder to listen and respond to MDNS name requests.
    WiFiMDNSResponder mdnsResponder;
    
    WiFiServer server(80);
  #endif
#endif

//Temperature Filtering
#define filterSamples   13              // filterSamples should  be an odd number, no smaller than 3
float rawDataWater, smoothDataWater;  // variables for sensor1 data
float rawDataExhaust, smoothDataExhaust;  // variables for sensor2 data

float WaterSmoothArray [filterSamples];   // array for holding raw sensor values for sensor1 
float ExhaustSmoothArray [filterSamples];   // array for holding raw sensor values for sensor2 

float Last_Exh_T = 0;
float Last_Wat_T = 0;
float Last_Mute_T = 0;
int GWTLast_Sec;
int Last_TSec;
boolean EX_Mute = false;
float Last_Temp = 0;
float Max_Change_Per_Sec_Exh = 4;  //Used to slow down changes in temperature to remove spikes
float Max_Change_Per_Sec_Wat = 2;  //Used to slow down changes in temperature to remove spikes
//Flame Sensor workings
float Flame_Diff = 0; 
float Flame_Threshold = 1.000;
long Flame_Timer = millis(); //prevent flame sensor being called too often
float Flame_Last = 0;

//Serial Settings
String message = "Off";
//bool pushed;
//bool long_press;
bool heater_on;
bool debug_glow_plug_on = false;
int debug_water_percent_map = 999;

//Varaiables
int Ignition_Failures = 0;
float fan_speed; // percent
float water_pump_speed; // percent
float fuel_need; // percent
int glow_time; // seconds
float water_temp; // degres C
float exhaust_temp; // degres C
float exhaust_temp_sec[11]; // array of last 10 sec water temp, degres C
int water_temp_sec[181];
int glow_left = 0;
int last_glow_value = 0;
bool burn = false;
bool webasto_fail = false;
int Start_Failures = 0;
int seconds;

bool lean_burn;
int delayed_period = 0;
unsigned long water_pump_started_on;
int water_pump_started = 0;
long glowing_on = 0;
int burn_mode = 0;

//PWM properties

const int glow_channel = 0;
const int water_channel = 1;
const int air_channel = 2;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
#ifdef BLYNK_ENABLE
  Serial.println("Connect to Blynk");

  Blynk.begin(BLYNK_AUTH_TOKEN, SECRET_SSID, SECRET_PASS);
  delay(1000);
  while(!Blynk.connected()) {
    Serial.print(".");
    Blynk.connect();
    delay(1000);
  }
  Serial.println("Blynk Connected");
#endif

  pinMode(led, OUTPUT);
  pinMode(glow_plug_pin, OUTPUT);
  pinMode(fuel_pump_pin, OUTPUT);
  pinMode(burn_fan_pin, OUTPUT);
  pinMode(water_pump_pin, OUTPUT);
  pinMode(water_temp_pin, INPUT);
  pinMode(exhaust_temp_pin, INPUT);
  pinMode(push_pin, INPUT); 
//  pinMode(lambda_pin, INPUT);
  pinMode(flame_sensor, INPUT); 

  digitalWrite(push_pin, LOW);
  analogWrite(water_pump_pin, 100); //Run water pump on startup for a few seconds
  //Pulse Burn fan - to test & indicate startup
  fan_speed = 70;
  burn_fan();
  delay(1000);
  fan_speed = 0;
  burn_fan();
  delay(1000);
  fan_speed = 70;
  burn_fan();
  delay(1000);
  fan_speed = 0;
  burn_fan();
  delay(3000);
  
  analogReadResolution(12);

#ifdef ESP_WIFI_ENABLE
  #ifndef BLYNK_ENABLE

    // attempt to connect to WiFi network:
    while ( status != WL_CONNECTED) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, pass);
      // wait 5 seconds for connection:
      delay(5000);
    }
    // you're connected now, so print out the status:
    printWiFiStatus();
  
    server.begin();
    
    // Setup the MDNS responder to listen to the configured name.
    // NOTE: You _must_ call this _after_ connecting to the WiFi network and
    // being assigned an IP address.
    if (!mdnsResponder.begin(mdnsName)) {
      Serial.println("Failed to start MDNS responder!");
      while(1);
    }
  
    Serial.print("Server listening at http://");
    Serial.print(mdnsName);
    Serial.println(".local/");  
  #endif
#endif 
}

void loop() { // runs over and over again, calling the functions one by one

  temp_data();
  control();
  webasto();
  
#ifdef ESP_WIFI_ENABLE
  #ifdef BLYNK_ENABLE
    Fuel_Purge_Action();
    Blynk.run();
  #else
    
    // Call the update() function on the MDNS responder every loop iteration to
    // make sure it can detect and respond to name requests.
    mdnsResponder.poll();
    // listen for incoming clients
    client = server.available();
    if (client) {
      WiFiACTIVE = true;
      WiFi_Deliver_Content();
    } else {
       WiFiACTIVE = false;  //Suspend serial logging
    }
  #endif
#endif
}

void Fuel_Purge_Action() {
//If it's safe to do so (heater & glow plug switched off), run the fuel pump rapidly to purge air
  if(!heater_on && !debug_glow_plug_on) {
    if(Fuel_Purge) {
      fuel_need = prime_ratio(prime_low_temp);
    } else {
      fuel_need = 0;
    }
    
  }
}

#ifdef BLYNK_ENABLE
  BLYNK_WRITE(V51)
  {
    //When selected from the Blynk Console, deliver fuel rapidly
    Fuel_Purge = param.asInt(); // assigning incoming value from pin V1 to a variable
  }
#endif

#ifdef ESP_WIFI_ENABLE
  void printWiFiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
  
    // print your WiFi IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
  
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
  }
#endif
