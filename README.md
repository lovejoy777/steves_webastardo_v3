# steves_webastardo_v3.0+
code for the webastardo v3.0 board by Simon Rafferty.

I've mades some changes to the board that must be done for this code to work.
Please see my video explaining in depth.
Link: https://www.youtube.com/watch?v=f0CdLafApvU

Changes I've made (hardware & software).
1/ Changed Simon's code to work with the Feather Huzzah32 board but can be configured for the Feather M0 Express boards easily too.
2/ Changed all the pins on the board to be on adc#1 (works with wifi connected, adc#2 pins don't).
3/ Exposed RX to header pin 4 (now we have RX & TX both exposed).

Thanks to Simon & David for such an awesome project.
Here are some links of their work.
https://github.com/SimonRafferty/Webasto-Heater---Replacement-Controller/tree/Webastardo-V3
https://oshwlab.com/simonrafferty/webastardo-3-0
https://github.com/davidmcluckie/webastardov3airheater

warning:
Use this code and information at your own risk, I take no responsibility if you set fire to anything or everything.