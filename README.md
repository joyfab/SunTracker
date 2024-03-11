![PicozSuntracker](https://github.com/joyfab/SunTracker/assets/29073056/d98987fa-aed0-4f49-b8ee-3eb199583ce5)
# SunTracker
Suntracker XY two axis RxTx Driver                                                               
Setting your PicozSunTrackerOledRxTx.ino file, IDE and libraries:                                                     
get the Arduino IDE1 v1.8.19:                                                                    
https://www.arduino.cc/en/software                                                              
get earlephilhower pico library:                                                                 
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json          
install Raspberry Pi RP2040 boards(v3.7.2) and Select Raspberry Pi Pico (firth line).                
drop Helios library from here into your library.                                                         
get and install the Oled library from https://github.com/olikraus/u8g2/                               
get and install the RDC DS3231 library from https://github.com/NorthernWidget/DS3231                
get and install the Adafruit_NeoPixel library from https://github.com/adafruit/Adafruit_NeoPixel      
set your Latitude and longitude end of line 336 of PicozSunTrackerOledRxTx.ino file  
replace position with 5 decimal after dot (eg. 5°37'43.66"E,44°18'58.91"N  => 5.62879,44.31636).                      
Setting L6470PD microstepper drivers for motors caracteristics:                                         
NEVER PLUG THE RP2040 BY USB IF ON BOARD. you should have to unplug it from the board to program it.      
Voltage motors will be 24 volts. (adjusted with the trimmer, don't touch it..). 
Once programmed alone, you can replug it exactly on his place. 
in dSPI_L6470PDPiconzAz or dSPI_L6470PDPiconzEl folders it wil be 4 files. Firth tab is the sketch.
Setting X (Azimuth) on tab dSPIN_L6470PDPicozAz line 
