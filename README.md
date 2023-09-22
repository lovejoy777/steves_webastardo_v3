#############################
## steves_webastardo_v3.0+ ##
#############################

Code for the webastardo v3.0 board by Simon Rafferty.
SimonSFX@Outlook.com 2022

I've made some changes to the board that must be done for this code to work.
Please see my video explaining in depth.
Link: https://www.youtube.com/watch?v=f0CdLafApvU

Changes I've made (hardware & software).
1/ Changed Simon's code to work with the Feather Huzzah32 board but can be configured for the Feather M0 Express boards easily too.
2/ Changed all the pins on the board to be on adc#1 (works with wifi connected, adc#2 pins don't).
3/ Exposed RX to header pin 4 (now we have RX & TX both exposed).

ADC2 has some restrictions on the feather huzzah32 wifi micro-controller:
ADC2 is used by the Wi-Fi driver, so ADC2 is only usable when the Wi-Fi driver has not started.
Three of the ADC2 pins are strapping pins and thus should be used with caution. Strapping pins -
- are used during power-on/reset to configure the device boot mode, the operating voltage, and other initial settings
Importantly, the official Adafruit docs for the Huzzah32 are confusing: -
- they state that “you can only read analog inputs on ADC #1 once WiFi has started”. 
Through experimentation, I found that Adafruit intended to say simply that "ADC#2 is unavailable once WiFi has started" (so you can only use ADC#1).

ADC#1 pins.
A2,A3,A4,A7,A9.
ADC#2 pins.
A0,A1,A5,A6,A8,A10,A11,A12.

IMPORTANT.
To use ADC#1 pins we have to change WebastardoV3.0 board a little, -
- cut 3 of the traces and wire 3 new links to the new pins, see Steves' part 1 video.
Link: https://www.youtube.com/watch?v=f0CdLafApvU

New Pinout for Adafruit Feather Huzzah32 board (wifi):
int glow_plug_pin = 14; // 
int water_pump_pin = 15; // 
int fuel_pump_pin = 27; // header pin 6
int burn_fan_pin = 33; 
//int lambda_pin = A1;
int water_temp_pin = A2; // header pin 5
int exhaust_temp_pin = A3; // header pin 3
int flame_sensor = A4;
int push_pin = A7; // header pin 1:

New Pinout for Adafruit Feather M0 Express board (none wifi):
int glow_plug_pin = 5;
int water_pump_pin = 9;
int fuel_pump_pin = 11; // header pin 6
int burn_fan_pin = 10;
//int lambda_pin = A1;
int water_temp_pin = A2; // header pin 5
int exhaust_temp_pin = A3; // header pin 3
int flame_sensor = A4;
int push_pin = 6; // header pin 1:

The wiring harness is similar to the original, except:
6 Pin Connector - header pins:  [Changes indicated by * ]
1 - Clock (+12V = Heater On)/(0V = Heater Off)
2*- Serial1 TX pin.
3*- Exhaust Thermistor (100k NTC between header pin 3 & Gnd or 4.7k using original onboard thermistor).
4*- serial1 RX pin.
5*- Water Thermistor (100k NTC between header pin 5 & Gnd).
6 - Fuel Dosing Pump

Thanks to Simon & David for such an awesome project.
Here are some links of their work.
Link: https://github.com/SimonRafferty/Webasto-Heater---Replacement-Controller/tree/Webastardo-V3 \n
Link: https://oshwlab.com/simonrafferty/webastardo-3-0 \n
Link: https://github.com/davidmcluckie/webastardov3airheater \n

warning:
Use this code and information at your own risk, I take no responsibility if you set fire to anything or everything.