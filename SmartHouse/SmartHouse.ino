/** 
 * @file SmartHouse.ino
 * 
 * @author Gavins Maragia Onsase      
 *  
 * @date 2022-10-27
 * 
 * @brief SmartHouse remote control
 * 
 * @details This project is a smart house project that uses a microcontroller to control the house 
 *          through a graphics display dashboard.
 * 
 * @version 1.0
 *   
 * PinOut:
 * 
 *  DS1307(RTC):
 *    SDA - pin 20  (SDA)
 *    SCK - pin 21  (SCK) 
 *   
 *  Graphic LCD 128*64:
 *    Data - A0 to A7
 *    EN   - pin 13    
 *    CS1  - pin 12
 *    CS2  - pin 11
 *    RS   - pin 9
 *    RW   - pin 10
 *    
 *  Kepad 4*3  
 *    CD4021   PISO for reading 
 *    74HC595  SIPO for scanning
 *    
 *  Virtual Terminal used for debug *    
 *    
 *  Appliances are represented using LEDs   
 * 
 *  The ciruit is simulated with proteus and it runs as expected. 
 */

#include <Arduino.h>    // including all required libraries 
#include <Wire.h>       // including i2c lib for RTC communication
#include <TimeLib.h>    // RTC library
#include <U8glib.h>     // u8glib display from library mannager 
#include <DS1307RTC.h>  // https://www.arduinolibraries.info/libraries/ds1307-rtc

// display constructor for u8g library with pinout for display
U8GLIB_KS0108_128 u8g(A0, A1, A2, A3, A4, A5, A6, A7, 13, 12, 11, 9, 8);     // 8Bit Com: D0..D7, en, cs1, cs2,di,rw

//keypad variables
//defining the serial data pins  -- FOR KEYPAD
//Pin connected to shift registers
int latchPin = 3;   //both shift registers
int clockPin = 2;   //both shift registers
int dataOutPin = 4; //the data out pin 74HC595 only
int dataInPin = 5;  //the data in CD4021BE only

int shiftPin[9] = {0, 1, 2, 4, 8, 16, 32, 64, 128}; // when sent to the shift register will only turn on 1 pin
byte dataIn = 72; //data coming from CD4021BE register

// values for 74HC595 shift register. control single pins by sending(0,1,2,4,8,16,32,64,128)
                              
// matrix keypad array
char btMatrix[4][3] = { //keyboard rows are pins(1,15,14,13) on CD4021BE columns are 74HC595
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// 'X' is the default value to show no key was pressed
char userIn = 'X';//holds the user inputed letter
char oldIn = 'X'; //holds that last value of userIn. for comparison
bool noInput = true; // used to reset and be able to press the same button a second time(true = no input, false = input)

// appliance names for all the rooms
String app1 = "LAMP"; 
String app2 = "TV";
String app3 = "FAN";

// THREE ROOMS each WITH 3 appliances
                        // RM1      // RM2        // RM3
int appliance[9]  = {23, 25, 27,  39, 41, 43,   49, 51, 53}; // APPLIANCE PINS
int appliance_Hold_state[9];

// RTC variables
tmElements_t tm;
int Previous_Hour = 0;              // prev hour
int Previous_Minute = 0;            // prev minute
int Previous_Second = 0;            // prev second

long Currtime;             //store current time SUN

unsigned long previous;    // previous snapshot of RTC
long interval = 120;       // Interval in ms at which to update time on LCD

// buffer arrays 
char Buf1[15];             // buffer to hold string being displayed for time
char Buf2[4];              // buffer to hold string being displayed for day
char Buf3[15];             // buffer to hold string being displayed for date

// variables for state machine to navigate main display
int select_menu   = 0;     // menu state machine
int r_menu = 0;            // holds current room variable
int n_value;               // current read value 
int state = 0;             // holds state of room selected
int pin;                   // holds 

void setup() {

  Serial.begin(9600);    // Start serial monitor for debugging
  
  u8g.begin();           // start display
  u8g.setColorIndex(1);  // select display colour

  for (int i = 0; i < 9; i++)        // All appliances as output and off
  { 
    pinMode(appliance[i], OUTPUT);   // loop through all appliance pins, set them as OUTPUT
    digitalWrite(appliance[i], LOW); // set each appliance  off
  }

  pinMode(latchPin, OUTPUT);   // latch pin shift register
  pinMode(clockPin, OUTPUT);   // clk pin shift register
  pinMode(dataOutPin, OUTPUT); //data that is being sent to the shift registers
  pinMode(dataInPin, INPUT);   //data that is read from the CD4021BE shift registers

  delay(100);                  // wait for RTC to initialize   
  readRTC();                   // read time at startup
  
}

void loop() {
  u8g.firstPage();          // draw first page on screen
  do {               
    draw();                 // display draw function 
  }
  while ( u8g.nextPage());  // waiting to finish draawing the things being displayed

}

// function to draw display
void draw() {

  READ_KEYPAD();

  u8g.setFont(u8g_font_5x7);

  if (millis() > previous + interval ) {
    previous = millis();
    Displaytime();
  }

  u8g.drawStr(76, 7, Buf1);    // date
  u8g.drawStr(55, 7, Buf2);    // day 50
  u8g.drawStr(76, 14, Buf3 );   // time


  switch (select_menu) {

    case 0:
      if (noInput == false) {
        if (r_menu > 0 && r_menu < 4 ) {
          select_menu  = n_value;
        } else {
          state = n_value;
        }
      }

      if (state == 0) {
        
        menu();               // main menu 
        r_menu = 0;           // room 0 is main menu
      
      } else if (state == 1) {   
       
        r_menu = 1;           // room 1 selected
        room();               // Draw room controls
        BackandHome();
        
      } else if (state == 2) {
        
        r_menu = 2;           // room 2 selected
        room();               // Draw room controls
        BackandHome();
        
      } else if (state == 3) {
       
        r_menu = 3;           // room 3 selected
        room();               // Draw room controls
        BackandHome();
        
      } else {
        menu();   // still show menu
        r_menu = 0; 
        u8g.setFont(u8g_font_5x7);
        u8g.drawStr(1, 57, "** Invalid Key selected**");//print symbol          // report invalid number choosen   
      }
      break;

    case 1:
      // if appliance 1 in any room is selected
      toggle(app1, appliance_state(select_menu), r_menu);  
      litsenButton();
      break;

    case 2:
      // if appliance 2 in any room is selected
      toggle(app2, appliance_state(select_menu), r_menu);
      litsenButton();
      break;

    case 3:
      // if appliance 3 in any room is selected
      toggle(app3, appliance_state(select_menu), r_menu);
      litsenButton();
      break;

    default :
      reset();
      menu();                                          // still show menu
      u8g.setFont(u8g_font_5x7);
      u8g.drawStr(1, 57, "** Invalid Key selected**");//print symbol          // report invalid number choosen   
  }

}

// check previous appliance state before display
bool appliance_state(int x) {

  // select pin for appliance depending on array location
  if (r_menu == 1) {
    x = x - 1;                // first room appliances
  } else if (r_menu == 2) {
    x = x + 2;               // second room appliances
  } else if (r_menu == 3) {
    x = x + 5;               // third room appliances
  }

  pin = x;
  bool on_off = false;
  if (appliance_Hold_state[pin] == 1)
    on_off = true;
    
  return on_off;//digitalRead(pin);
}

// Check pressed buttons
void litsenButton() {

  if (noInput == false) {
    if (n_value == 0 || userIn == '#' ) { // RESET TO HOME SCREEN if EITHER '0' or '#'  is detected
      reset();
      return;
      
    } else if (userIn == '*')  {         // head back to selecting appliance
      select_menu  = 0;
    }
    if (n_value == 1 || n_value == 2) {
      toggleState((n_value - 1));      // either 1 or 0 to turn off or on appliance
    }
  }
}

//back and gome keys
void BackandHome() {

  if (noInput == false) {
    if (n_value == 0 || userIn == '#' ) { // RESET TO HOME SCREEN if EITHER '0' or '#'  is detected
      reset();
      return;
      
    } else if (userIn == '*')  {         // head back to selecting appliance
      
      r_menu       = 0;
      select_menu  = 0;
    } 
  }
}

// reset back to home screen 
void reset(){
   select_menu  = 0;
   state        = 0;
   r_menu       = 0;
   noInput      = true;
  }


// set appliance state
void toggleState(int state__) {
  appliance_Hold_state[pin] = state__;
  digitalWrite(appliance[ pin ], state__);
}


// toggle appliance state
void togglePin() {
  appliance_Hold_state[pin] = !digitalRead(pin);
  digitalWrite(appliance[ pin ], appliance_Hold_state[pin]);
}

 
// print out passed text to LCD
void msg(const char *txt)
{
  u8g.print(txt);
}

// trancate two digits for dates
String print2digits(int number) {
  String f = "";
  if (number >= 0 && number < 10) {
    f = "0";
  }
  f = f + String(number);
  return f;
}

// read RTC date, time and day and store to buffer character arrays
void readRTC() {
  String BUFFER =  String(tm.Day) + "/" + String(tm.Month) + "/" + String(tmYearToCalendar(tm.Year)) ;  // read date
  BUFFER.toCharArray(Buf1, 15);
  
  int WeekDay = tm.Wday;                                                                                    // read day
  BUFFER =  String(DayHold(WeekDay)) ;
  BUFFER.toCharArray(Buf2, 4); 
  
  BUFFER =  String(print2digits(tm.Hour)) + ":" + String(print2digits(tm.Minute)) + ":" + String(print2digits(tm.Second)) ; // read time
  BUFFER.toCharArray(Buf3, 15);
}

void Displaytime() {

  if (RTC.read(tm)) {
    if ((Previous_Hour != tm.Hour) || (Previous_Minute != tm.Minute) || (Previous_Second != tm.Second)) {
      // if the current time is not equal to the last read time, update time
      readRTC();

      Previous_Hour = tm.Hour;                                // store last read hour
      Previous_Minute = tm.Minute;                              // store last read minute
      Previous_Second = tm.Second;                              // store last read secondr
    }
  }
  else {
    if (RTC.chipPresent()) {                         // if TRC chip is detected but no time availale
      u8g.drawStr(13, 13, "RTC Clock init failure"); // report clock initialization failed
      u8g.drawStr(13, 26, "Please set Time ");       // set time feedback 
    } else {
      u8g.drawStr(13, 13, "RTC Clock init failure"); // if TRC chip is not detected - report clock initialization failed
      u8g.drawStr(13, 26, "Check wiring     ");      // and check wiring 
    }
    delay(100);
  }
}

// returns day currently selected
String DayHold(int day_number) {   
  if (day_number == 1) 
    return "SUN";
  if (day_number == 2) 
    return "MON";
  if (day_number == 3) 
    return "TUE";
  if (day_number == 4) 
    return "WED";
  if (day_number == 5) 
    return "THU";
  if (day_number == 6) 
    return "FRI";
  if (day_number == 7)
    return "SAT";
  if (day_number == 0) 
    return "   ";
}


void menu() {
  //  print main menu on lcd screen
  
  u8g.setFont(u8g_font_courB10);  //set font 10 pixels

  u8g.drawStr(1, 10, "MENU");   //value print on LCD screen

  u8g.setFont(u8g_font_6x10);   // set font 6 pixels

  u8g.drawStr(5, 25, "SMART HOUSE CONTROL");   //title print on LCD screen

  u8g.setFont(u8g_font_5x8);
  u8g.drawStr(4, 33, "1. Room One");   //print room 1

  u8g.drawStr(4, 42, "2. Room two");   //print room 2

  u8g.drawStr(4, 50, "3. Room three"); //print room 3 


  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(1, 64, "*Please select on keypad*");//print symbol
}

void room() {
  //  print on room options lcd screen
  u8g.setFont(u8g_font_6x10);  //set font 10 pixels

  char buffer_[18];
  String BUFFER =  "ROOM " + String(r_menu) + " CONTROLS:" ;  // concatenate title
  BUFFER.toCharArray(buffer_, 18);

  u8g.drawStr(15, 25, buffer_);            //value print on LCD screen

  u8g.setFont(u8g_font_5x8);
  u8g.drawStr(4, 33, "1. Control Lights");//toggle lights expression

  u8g.drawStr(4, 42, "2. Control TV");    //toggle TV expression

  u8g.drawStr(4, 50, "3. Control FAN");   //toggle fan expression

  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(1, 64, "*        (#) HOME       *"); //print symbol 
}

// Turn appliances function - displays rectangles and turn off/on
void toggle(String name, bool state, int roomNumber) {
 // [parameters here 
  
  String val = "OFF";          // variable with state of appliance - off 
  if (state)                   // if state is true 
    val = "ON";                // appliance is on

  u8g.setFont(u8g_font_courB10);  // select 10 pixels fond

  char buffer__[8];
  String BUFFER_ =  "ROOM " + String(roomNumber) ;   //show current room being worked on
  BUFFER_.toCharArray(buffer__, 8);

  u8g.drawStr(1, 10, buffer__);   // Print room number

  u8g.setFont(u8g_font_6x10);     // set font

  char buffer_[22];
  String BUFFER =  "Toggle " + name + " state:" + val ;   // concatenate title - e.g Toggle Lamp state
  BUFFER.toCharArray(buffer_, 22);

  u8g.drawStr(3, 25, buffer_);   //value print on LCD screen

  u8g.drawFrame(0, 27, 128, 13); // display upper frame
  u8g.drawFrame(0, 42, 128, 13); // display lower frame

  u8g.setFont(u8g_font_5x8);
  u8g.drawStr(4, 36, "1. TURN OFF");  //toggle appliance ON
  u8g.drawStr(4, 51, "2. TURN ON"); //toggle appliance OFF


  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(1, 64, "SET: (*) BACK OR (#) HOME");//print symbol
}


// keypad reading with help of shift register 
void READ_KEYPAD() {
  
  // turns on then off pins 1,2 and 3 of the 74HC595 one by one
  for (int i = 1; i < 4; i++) { 
   
    //sending data to 74HC595
    digitalWrite(latchPin, LOW); //needs to be set low to send data
    shiftOut(dataOutPin, clockPin, MSBFIRST, shiftPin[0]); //sending the data to second shift register and it should be off
    shiftOut(dataOutPin, clockPin, MSBFIRST, shiftPin[i]); //turns one pin high on first shift register
    digitalWrite(latchPin, HIGH); //sending data to shift register need to shift in while latch is high

    //reading data from CD4021BE
    digitalWrite(clockPin, HIGH); //This is set high before latch is pulled low. Otherwise pin 1 on CD4021BE will not be counted
    digitalWrite(latchPin, LOW); //latch pin set low to send data
    dataIn = shiftIn(dataInPin, clockPin, MSBFIRST); //reading data from CD4021BE
    digitalWrite(latchPin, HIGH); //sending data to arduino

    //finding the the button that was pressed
    switch (dataIn) {
      case 128://pin 1 on CD4021BE
        userIn = btMatrix[0][i - 1]; // setting userIn to the letter that was pressed. i starts at val 1 but matrix starts at val 0
        noInput = false; // a button was pressed
        break;
      case 64://pin 15 on CD4021BE
        userIn = btMatrix[1][i - 1];
        noInput = false; // a button was pressed
        break;
      case 32://pin 14 on CD4021BE
        userIn = btMatrix[2][i - 1];
        noInput = false; // a button was pressed
        break;
      case 16://pin 13 on CD4021BE
        userIn = btMatrix[3][i - 1];
        noInput = false; // a button was pressed
        break;
      default:
        noInput = true; //a button was not pressed
        break;
    }

    if (noInput == false) break; //if a button was pressed no need to continue the loop
  }

  //checking the input ***note: can not hold down button***
  if (userIn != 'X' && userIn != oldIn) //if a button is pressed and not being held down
  {
    // to print the pressed value to debug monitor
    Serial.print(userIn);    // printing the pressed letter on serial 

    if (userIn != '*' && userIn != '#' ) { // convert character to integer
      //n_value = atoi(userIn);     // This doesnt work for some reason
      n_value = userIn - '0';       // I use this to convert a single character into an integer -- this works 
      delay(50);                    // wait 50ms 
    }

    oldIn = userIn;            //stops it from printing that same letter over and over during press and hold
  }
  else if (userIn == oldIn) {  // nobuttons were pressed reset the ability to press the same button
    if (noInput == true) {     // no button was pressed
      userIn =  'X';           // reset user input
      oldIn  =  'X';           // reset old input
    }
  }

}
