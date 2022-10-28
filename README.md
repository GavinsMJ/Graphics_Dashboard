# Graphics_Dashboard
Multiple device controller with a 128*64 Graphics display

Controller for multiple devices with a 128*64 Graphics display. The controller is based on the Arduino Mega and the LCD display. The controller is able to control as many devices as can be added. The controller is currently able to control 3 rooms with 3 devices each, this can be expanded or reduced in the code.

Main Menu:

![Main Menu](/Assets/menu.png)
Room Menu:

![Room Menu](/Assets/room.png)

Device Menu:

![Device Menu](/Assets/lampTwo.png)
 

## Pinout
 
DS1307(RTC):

SDA - pin 20  (SDA), 
SCK - pin 21  (SCK)
  
Graphic LCD 128*64:

Data - A0 to A7, 

EN   - pin 13, 
CS1  - pin 12,  
CS2  - pin 11, 
RS   - pin 9, 
RW   - pin 10
 
Kepad 4*3 : 

CD4021   PISO for reading,  
74HC595  SIPO for scanning
 

## Features

Three rooms with three devices each,
select option using keypad :  

'*' to go back to the previous page
'0' and '#' to go back to the main menu

## To do
Make it run more efficient. Add animations to UI.