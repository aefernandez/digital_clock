/* Digital Alarm Clock by Alan Fernandez
   Using Time and DS1307RTC libraries.
   Version 1.9 (Brightness Automatic Adjust Finalized) - 6/30/17

   Remarks:
    Working:
      Time Setting
      Date Setting
      Buzz on click
    Not Working:
      Button navigation is glitchy

    Fixed:
      [1.2.0] Back to display time -> Nothing shows up
      [1.2.1] Setting time or date -> Nothing shows up (Was checking for buttons to be LOW instead of HIGH)
      [1.3.0] Time Setting: Digits do not blink making navigation hard & this function lags, hours place holder shows up after minutes and seconds
      [1.3.1] Implemented const int BUTTON_DEBOUNCE to standardize debouncing time
      [1.4.0] Designed blink function that can be utilized both for time setting and date setting. Makes use of pointers. Streamlined code for timeSetting.
      [1.4.1] Back to Main display shows garbled junk: Fixed itself with the streamlining.
      [1.4.2] Implemented blinking function to dateChange(). The function works beautifully :)
      [1.5.0] Improved readability by changing if statement ladders for switch cases
      [1.5.1] blinkChar(): When the number is above 10 it blinks one more space to the right than it should.
      [1.5.2] Finished blinking function and implementation.
      [1.5.3] Setting Date: Clears the screen intead of just the digit being adjusted when looping around (ie 0 to 31)
      [1.5.4] Optimized displayTime() function using the printDigits() function.
      {1.5.5] Added comments describing parameter function and function description.
      [1.6] Removed redundant code from menu & fixed garbage on screen after cycling through all options. Garbage seems to come from printing to the serial port.
      [1.7] Half implemented PWM LCD backlight brightness control. Decreasing doesn't work. Looping does not show squares anymore for both increase and decrease.
      [1.8] Brightness adjust fully implemented.
      [1.9] Automatic brightness adjust 
      [2.0] Menu time-out
      [2.1] Fixed Alarm - missing bracket and bad choice of parameters
      
    To Be Implemented:
      Pillow Sensor - Probably will not implement given inconveniences. Extremely long cables mainly. And adding a port to connect the sensor.
      Automatic Brightness adjust with PWM + LDR - Probably will not implement 
      Make the date be changed once a day instead of every second
*/

/*Pin configuration */
#define LCDBacklight 6
#define decreaseBtn 7
#define increaseBtn 8
#define modeBtn 9
#define setBtn 10
#define buzzer 13

#include <math.h>
#include <Wire.h>
#include <TimeAlarms.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <LiquidCrystal.h>
#include <customInvertedCharacters.h>
#include <clockHelperFunctions.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int BUTTON_DEBOUNCE = 200; //200ms delay after button press to debounce

/* Time Settings */
int HOUR, MINUTE, SECOND;
unsigned long startDate = 1416614400; //Time between Jan 1, 1970 and Nov 22, 2014.
bool showingDate = false;
unsigned long timeUpdated = millis();
unsigned long dateUpdated = -60000;

/* Menu Settings */
const char *menuItems[] = {"Set Time?", "Set Date?", "Set Alarm?", "Set Brightness?", "Set Features"};
const int TOTAL_MENU_ITEMS = 5;

/* Blink Function variables and settings */
const int POSITION_ARRAY[] = {0, 3, 6, 0, 3, 6}; //Position of the first character of each pair on the LCD for dd/mm/yyyy and hh:mm:ss
const char SELECTED_ARRAY[] = {'D', 'M', 'Y', 'h', 'm', 's'}; //Case is important to avoid confusion between minute and MONTH. Array holds posible selected pair.
const int BLINK_ARRAY_LENGTH = 6; //Array length is necessary to avoid running off the edge of the array

/* Features Setup */
boolean featuresArray[] = {true};
const int FEATURES_ARRAY_LENGTH = 1;

/* Create a LCDSettings Structure that holds all the settings for the LCD and initialize its members*/

time_t t = now();

void setup() {
  /*Begin Communications */
  Wire.begin();
  lcd.begin(16, 2);
  Serial.begin(9600);

  /*RTC Configuration */
  setSyncProvider(RTC.get); //When syncing, get the time from the RTC
  setSyncInterval(10); //Time will be synced with the RTC every n seconds

  /*Pin configuration */
  pinMode(modeBtn, INPUT);
  pinMode(setBtn, INPUT);
  pinMode(increaseBtn, INPUT);
  pinMode(decreaseBtn, INPUT);
  pinMode(buzzer, OUTPUT);
  
  /*LCD Configuration */
  pinMode(LCDBacklight, OUTPUT);
  analogWrite(LCDBacklight, LCDProfile.currentBrightness);
  lcd.setCursor(0, 1);
  lcd.print("    Hi Sabs!    "); 
  delay(1000);
 
}

/**Main loop */
void loop() {
  /* Check if the modeBtn has been pressed */
  if (digitalRead(modeBtn) == HIGH) {
    beep();
    lcd.clear();
    menu();
  }

  /* Update the time every second. Check alarms in between seconds */
  if (millis() - timeUpdated > 1000) {
    displayTime();
    displayDate(t, false);
    showingDate = false;
  } else {                           
    displayDate(t, true);
    t = now();
    timeUpdated = millis();
    displayTime();
    Alarm.delay(0);
  }

  /* Update the date every 60 seconds */
  if (millis() - dateUpdated > 60000) {
    t = now();
    dateUpdated = millis();
    if (showingDate) {
      showingDate = true;
    }
  }

  /* Start or stop the Automatic Brightness Adjust */
  if(hour() >= LCDProfile.startTime || hour() == LCDProfile.minBrightnessTime){
    LCDBrightnessAutoAdjust();
  }
  
  /* If sync fails display error on the right end of the first row */
  if (timeStatus() != timeSet) {
    timeSyncFail();
  }
}

/* Main menu of the clock */
void menu() {
  int selectedItem = 0;
  const int TIME_OUT = 10000; //Time out after 10 seconds of inactivity
  unsigned long runningTime = millis();
  
  lcd.clear();
  lcd.print(menuItems[selectedItem]);
  delay(BUTTON_DEBOUNCE);

  while ((selectedItem < TOTAL_MENU_ITEMS) && (runningTime + TIME_OUT >= millis())) { //Once the user loops through all the items or timeOut go back to the main menu
    lcd.setCursor(9, 0);
    
    if(digitalRead(modeBtn) == HIGH){
       beep();
       selectedItem++;
       lcd.clear();
       lcd.print(menuItems[selectedItem]);
       delay(BUTTON_DEBOUNCE);
       runningTime = millis();
    }
    
    if (digitalRead(setBtn) == HIGH) {
      beep();
      switch (selectedItem) {
        case 0:
          changeTime(false);
          break;
        case 1:
          changeDate();
          break;
        case 2:
          changeTime(true);
          break;
        case 3:
          setLCDBrightness();
          break;
        case 4:
          configureFeatures();
          break;
      }
     selectedItem = TOTAL_MENU_ITEMS; //Quit the while loop and return to main screen
    }
  }

  lcd.clear();
  displayTime();
  displayDate(t, false);
  timeUpdated = millis();
}

/**Specifies what to do when the Time library fails to sync with the RTC */
void timeSyncFail() {
  while (timeStatus() != timeSet) {

    /*Update the time every second */
    if (millis() - timeUpdated > 1000) {
      t = now();
      timeUpdated = millis();
      displayTime();
      Alarm.delay(0);
      lcd.setCursor(12, 0);
      lcd.print("!RTC"); //Display error msg on the far right of the first row
    }

    /* Update the date every 60 seconds */
    if (millis() - dateUpdated > 60000) {
      t = now();
      dateUpdated = millis();
      if (showingDate) {
        displayDate(t, false);
        showingDate = false;
      } else {
        displayDate(t, true);
        showingDate = true;
      }
    }
  }
  lcd.clear();
}

/**Interrupt Service Routine for the dailyAlarm 
*  If statement inside checks to see if the alarm is enabled
*/
void dailyAlarm() {                         //Alarm was triggered
  if(featuresArray[ALARMONINDEX]){          //Check to see if the alarm is enabled
    int triggerTime = minute();             //When the alarm was triggered
    lcd.setCursor(14, 0);
    lcd.print("A");                         //Indicate that the alarm has been triggered
    alarmBeep();
    while (minute() - triggerTime < 5) {    //Check if the pillow sensor is triggered for 5 minutes
      if (millis() - timeUpdated > 1000) {  //Update the time on screen as usual
        t = now();
        timeUpdated = millis();
        displayDate(t, true);
        displayTime();
      }
    }
    lcd.clear();
  }
}

/*************************************************************************************************************************************************/
/*********************************************************Time and Date Change Functions**********************************************************/
/*************************************************************************************************************************************************/

/**Allows the user to change the time.
   @param MODE Specifies if the function is being used to change the time for the clock or the time for the alarm.
*/
void changeTime(bool settingAlarm) {
  int hours = 1, minutes = 0, seconds = 0; //Temporary storage for the desired time

  unsigned long lastBlink = millis();
  bool blinkBlank = false;
  char selected = 'h';//What to adjust
  int* updateArray[] = {NULL, NULL, NULL, &hours, &minutes, &seconds};

  delay(BUTTON_DEBOUNCE);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Setting Time");
  lcd.setCursor(0, 1);
  lcd.print("01:00:00");

  while (digitalRead(setBtn) == LOW) { //Setting time will quit once the user presses the set button

    /*Blinking the selected setting*/
    if (millis() - lastBlink >= 500) {
      lastBlink = blinkChar(blinkBlank, selected, updateArray);
      blinkBlank = !blinkBlank;
    }

    /*Reacting to user button presses */
    if (digitalRead(increaseBtn) == HIGH) {
      switch (selected) {
        case 'h':
          if (hours == 24) { //Loop back to one
            hours = 1;
          } else {
            (hours)++;
          }
          break;
        case 'm':
          if (minutes == 59) { //Loop back to zero
            minutes = 0;
          } else {
            (minutes)++;
          }
          break;
        case 's':
          if (seconds == 59) { //Loop back to zero
            seconds = 0;
          } else {
            seconds++;
          }
          break;
      }
      delay(BUTTON_DEBOUNCE);
    }

    if (digitalRead(decreaseBtn) == HIGH) {
      switch (selected) {
        case 'h':
          if (hours == 1) { //Loop to 24
            hours = 24;
          } else {
            (hours)--;
          }
          break;
        case 'm':
          if (minutes == 0) { //Loop to 59
            minutes = 59;
          } else {
            (minutes)--;
          }
          break;
        case 's':
          if (seconds == 0) { //Loop to 59
            seconds = 59;
          } else {
            seconds--;
          }
          break;
      }
      delay(BUTTON_DEBOUNCE);
    }

    if (digitalRead(modeBtn) == HIGH) {
      beep();
      switch (selected) {
        case 'h':
          printDigits(hours, 0, 1);
          selected = 'm';
          break;
        case 'm':
          printDigits(minutes, 3, 1);
          selected = 's';
          break;
        case 's':
          printDigits(seconds, 6, 1);
          selected = 'h';
          break;
      }
      delay(BUTTON_DEBOUNCE);
    }
  }
    /*If the function is being used to change the time of the clock then do this, else it is being used to change the alarm time, do that. */
    if (settingAlarm) { //Changing the alarm time
      lcd.setCursor(0, 0);
      lcd.print("Setting Alarm");
      Alarm.alarmRepeat( hours,  minutes,  seconds, dailyAlarm);
      delay(1000);
    } else { //Changing the clock time
      lcd.setCursor(0, 0);
      lcd.print("Setting Time");
      setTime( hours,  minutes,  seconds, day(), month(), year()); //adjust local time
      t = now(); //convert time to time_t form
      RTC.set(t); //adjust time in the RTC
      delay(1000);
    }
  
  delay(BUTTON_DEBOUNCE);
  lcd.clear();
}

/**Allows the user to change the date.*/
void changeDate() {
  int years = 2016, months = 1, days = 1;
  char selected = 'D';//Start by modifying the day
  int* updateArray[] = {&days, &months, &years, NULL, NULL, NULL};
  unsigned long lastBlink = millis();
  bool blinkBlank = false;

  delay(BUTTON_DEBOUNCE);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Setting Date");
  lcd.setCursor(0, 1);
  lcd.print("01/01/2016");
  while (digitalRead(setBtn) == LOW) { //Set the date and quit back to menu when the set button is pressed

    /*Blinking the selected setting*/
    if (millis() - lastBlink >= 500) {
      lastBlink = blinkChar(blinkBlank, selected, updateArray);
      blinkBlank = !blinkBlank;
    }
    /*Reacting to button presses */
    if (digitalRead(increaseBtn) == HIGH) {
      switch (selected) {
        case 'D':
          if (days == 31) {
            days = 1;
          } else {
            days++;
          }
          break;
        case 'M':
          if (months == 12) {
            months = 1;
          } else {
            months++;
          }
          break;
        case 'Y':
          if (years == 2050) {
            years = 2016;
          } else {
            years++;
          }
          break;
      }
      delay(BUTTON_DEBOUNCE);
    }

    if (digitalRead(decreaseBtn) == HIGH) {
      switch (selected) {
        case 'D':
          if (days == 1) {
            days = 31;
          } else {
            days--;
          }
          break;
        case 'M':
          if (months == 1) {
            months = 12;
          } else {
            months--;
          }
          break;
        case 'Y':
          if (years == 2016) {
            years = 2050;
          } else {
            years--;
          }
          break;
      }
      delay(BUTTON_DEBOUNCE);
    }

    if (digitalRead(modeBtn) == HIGH) {
      beep();
      switch (selected) {
        case 'D':
          printDigits(days, 0, 1);
          selected = 'M';
          break;
        case 'M':
          printDigits(months, 3, 1);
          selected = 'Y';
          break;
        case 'Y':
          printDigits(years, 6, 1);
          selected = 'D';
          break;
      }
      delay(BUTTON_DEBOUNCE);
    }

  }
  setTime(hour(), minute(), second(), days, months, years); //adjust local date
  t = now(); //convert time to time_t form
  RTC.set(t); //adjust time in the RTC
  lcd.clear();
  delay(BUTTON_DEBOUNCE);
}

//*************************************************************************************************************************************************
////**************************************************************Display Functions****************************************************************
//*************************************************************************************************************************************************

/* Prints the time to the screen. */
void displayTime() {
  printDigits(hourFormat12(), 0, 0);

  lcd.print(":");
  printDigits(minute(), 3, 0);

  lcd.print(":");
  printDigits(second(), 6, 0);

  lcd.setCursor(9, 0);
  if (isAM()) {
    lcd.print("AM");
  } else {
    lcd.print("PM");
  }
}

/**Function to display the current date.
   @param showDate Boolean to indicate whether the current date is shown or the number of days since Sabrina and I got together.
   @param t Current time given by the Time library
*/
void displayDate(time_t t, bool showDate) {
  if (showDate == true) {
    lcd.setCursor(0, 1);
    lcd.print(dayShortStr(weekday(t)));
    lcd.print(" ");
    lcd.print(monthShortStr(month(t)));
    lcd.print(" ");
    lcd.print(day(t));
    lcd.print(", ");
    lcd.print(year(t));
  } else {
    lcd.setCursor(0, 1);
    lcd.print((t - startDate) / 86400); //Convert time between today and 22 Nov, 2015 to days. (Days we've been together)
    lcd.print(" Days");
  }
}

/**

*/
void defaultTime() {
  lcd.setCursor(0, 1);
  lcd.print(HOUR);
  lcd.print(":");
  lcd.print(MINUTE);
  lcd.print(":");
  lcd.print(SECOND);
}

//*************************************************************************************************************************************************
//************************************************************Haptic FeedBack Functions************************************************************
//*************************************************************************************************************************************************

void beep() {
  tone(13, 300, 10);
}

void alarmBeep() {
  tone(13, 1480, 50);
}

//*************************************************************************************************************************************************
//******************************************************General Configuration Functions************************************************************
//*************************************************************************************************************************************************

/**Function to control the settings for any features
*  Currently controls alarm ON/OFF
*/
void configureFeatures(){
  boolean updateScreen = false;
  lcd.clear();
  lcd.print("Feature Settings");
  lcd.setCursor(14,0);
  lcd.setCursor(0,1);
  lcd.print("Alarm: ");
  
  /*Update the screen with the current setting */
  if(featuresArray[0]){
    lcd.print("ON");
  }else{
    lcd.print("OFF");
  }

  /*Enter loop to change the settings */
  while(digitalRead(setBtn) == LOW) {
    
    /*Controls for the alarm settings */
    if(digitalRead(increaseBtn) == HIGH){
      featuresArray[0] = !featuresArray[0]; //Flip the current setting
      updateScreen = true;                  //Flag that the screen needs update
    }
    if(digitalRead(decreaseBtn) == HIGH){
      featuresArray[0] = !featuresArray[0]; //Flip the current setting
      updateScreen = true;
    }
    
    /*Update the screen if changes are made to the alarm settings */
    if(updateScreen){
      lcd.setCursor(7,1);
      if(featuresArray[0]){
        lcd.print("ON");
      }else{
        lcd.print("OFF");
      }
      updateScreen = false;
      delay(BUTTON_DEBOUNCE);
    }
  }
  lcd.clear(); //Clean the screen and return 
}







