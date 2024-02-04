/*
* Christian Barran
* Subscribe to my Youtube Channel
* www.youtube.com/c/justbarran
*
*
*Keyboard Library: https://www.arduino.cc/reference/en/language/functions/usb/keyboard/
*FlashStorage Library: https://www.arduino.cc/reference/en/libraries/flashstorage/
*MY GT 521F Library: https://github.com/justbarran/Just-a-Fingerprint-Scanner-GT-521FXX
* 10/9/2021
*/

#include <FlashStorage.h> 
#include "Keyboard.h"
#include <GT_521F.h>

#define ON_BOARD_LED 13
#define TOUCH_SENSOR_PIN 1   
#define FPS_RX 7          //RX pin
#define FPS_TX 6          // TX pin
#define PASS_MAX 33      //Password Lenght
#define PRINTS_MAX 200   //200=GT_521F32 3000=GT_521F52
#define PASS_TRYS_MAX 5  //Password Tiess
#define FINGER_TRYS 3    //
 
#define SLEEP_TIME  20000 //milliseconds
#define UNLOCK_TIME 10000 //milliseconds
#define ENROLL_TIME 10000 //milliseconds
#define SerialUSB Serial

char defaultName1[PASS_MAX]= "justbarran";
char passwordTemp1[PASS_MAX];
char passwordTemp2[PASS_MAX];

uint8_t touchStateLast = LOW;
uint8_t unlockState = LOW;
uint8_t sleepState = LOW;

uint8_t menuState = 0;
uint8_t passTries = PASS_TRYS_MAX;
uint8_t serialFlag = 0;

uint32_t unlockTimeLast = 0;
uint32_t sleepTimeLast = 0;

typedef struct {
  boolean valid;
  uint8_t fpsID[16];
  char pass0[PASS_MAX];
  char name1[PASS_MAX];
  char pass1[PASS_MAX];
} Passwords;

Passwords storeNVS;
FlashStorage(my_flash_store, Passwords);
// Note: the area of flash memory reserved for the variable is
// lost every time the sketch is uploaded on the board.


//Uncomment for Software Serial
//SoftwareSerial mySerial (7, 6);
//GT_521F fps(mySerial); 

//Uncomment for Hardware Serial
GT_521F fps(Serial1); 

//Prototypes
int8_t checkFinger(void);
void checkSerial(void);
void pclogin(char * pass);
uint8_t FingerPrintEnrollment(void);
void setDefaults(void);
void menu_1(void);

/*=====================================Setup================================*/
void setup()
{
  pinMode(ON_BOARD_LED, OUTPUT);  //Set LED as output - This not going to be seen 
  pinMode(TOUCH_SENSOR_PIN, INPUT);   //Set the touch sensor pin as input
  delay(10);

  while(!fps.begin(9600)) //Check for FPS sensor
  {
    digitalWrite(ON_BOARD_LED,!digitalRead(ON_BOARD_LED)); 
    delay(100);
  }
  
  storeNVS = my_flash_store.read();  //Check the flash storage 
  if(storeNVS.valid==LOW)
  {
    setDefaults();
  }  

  fps.cmosLed(true);  //Do a FPS light test 
  delay(100);
  fps.cmosLed(false);

  sleepState = HIGH;
  Keyboard.begin();  //Start Keyboard
}

/*=====================================LOOP================================*/

void loop() 
{
  //Check Finger print to login in or to wake up from sleep
  if(unlockState == LOW)
  {
    int8_t getFinger = checkFinger();
    if(getFinger==HIGH)
    {
      pclogin(storeNVS.pass1);
    }
    else if (getFinger ==LOW)
    {
      Keyboard.end();
      delay(1);
      SerialUSB.begin(9600);  //Start uart over USB
      delay(1);
      sleepState = LOW;
      sleepTimeLast = millis(); 
      SerialUSB.println("---------------------------------------");
      SerialUSB.println("Check out \"Just Barran\" on Youtube"); 
      SerialUSB.println("---------------------------------------");
      SerialUSB.println(">>>Enter Password<<<");    
    }
  }
  
  //Check Serial once board is awake
  if(sleepState==LOW)
  {
    checkSerial();   
  }
  else
  {
    if(SerialUSB.available()> 0)  //Check if there is computer seriall information 
    {
       SerialUSB.read();     
    }
  }
   
  //Check time to Know if to Lock device 
  if(unlockState == HIGH)
  {
    if((millis()-unlockTimeLast) > UNLOCK_TIME)
    {
      SerialUSB.println("-Device Locked");
      unlockState = LOW;
    }
  }

  //Check time to Know if to put device to sleep and stop checking serial monitor 
  if(sleepState == LOW)
  {
    if((millis()-sleepTimeLast) > SLEEP_TIME)
    {
      sleepState = HIGH;
      unlockState = LOW;
      SerialUSB.println("-Device Sleep");
      delay(10);
      //SerialUSB.end();  //Start uart over USB
      delay(10);
      Keyboard.begin();  //Start Keyboard
    }    
  }
  delay(10);
}


/*=====================================Funtions================================*/

/*========Finger print sensor touched=====================*/
int8_t checkFinger(void)
{
  int8_t tempState = -1;
  uint8_t touchState = digitalRead(TOUCH_SENSOR_PIN);

  if((touchState!=touchStateLast) && (touchState==HIGH))
  {
    digitalWrite(ON_BOARD_LED,LOW);     
    uint16_t openStatus = fps.open(true);
    if(openStatus == NO_ERROR) 
    {
      uint16_t checkLED = fps.cmosLed(true);
      if(checkLED == NO_ERROR)
      { 
        uint8_t FingerCount = 0;
        uint16_t checkFinger = FINGER_IS_NOT_PRESSED;
        /*check a certain number of times if a finger is pressed else time out*/
        while((checkFinger==FINGER_IS_NOT_PRESSED) && (FingerCount<FINGER_TRYS)) 
        {
          checkFinger = fps.isPressFinger();
          FingerCount++; 
          delay(5);                   
        }
        if(checkFinger == FINGER_IS_PRESSED)
        {
          checkFinger = fps.captureFinger();
          if(checkFinger == NO_ERROR)
          {
            checkFinger = fps.identify();
            if(checkFinger < PRINTS_MAX)
            {
              tempState = HIGH;
            }
          }
        }
        else
        {
          tempState = LOW;
        }
      }
      else
      {
        SerialUSB.print("LED FAIL: ");
        SerialUSB.println(checkLED,HEX);
      }
    } 
    else 
    {
      SerialUSB.print("Initialization failed!\nstatus: ");
      SerialUSB.print(openStatus, HEX);
      SerialUSB.println();
    }    
  }
  else if((touchState!=touchStateLast) && (touchState==0))
  {
    fps.open(false);
    fps.cmosLed(false);
    digitalWrite(ON_BOARD_LED,HIGH);
  }
  touchStateLast = touchState;
  return tempState;
}


/*========Check Serial for Data=====================*/
void checkSerial(void)
{
  if(SerialUSB.available()> 0)  //Check if there is computer seriall information 
  {  
    if(unlockState==LOW && sleepState==LOW)
    {
      byte i = 0;
      delay(1); //give a ms to makesure the data is in the buffer
      while(SerialUSB.available()>0)
      {
        passwordTemp1[i]=SerialUSB.read();
        i++;
      }
      passwordTemp1[i] = '\0'; //Turns it into a string with a null at the end 

      if(i>=4)
      {
        byte j = 0;
        byte checks = 1;
        while((checks == 1)&&(j<=PASS_MAX))
        {
          if(passwordTemp1[j]!=storeNVS.pass0[j])
          {
            checks = 0;
          }
          else
          {
            if(storeNVS.pass0[j]=='\0')
            {
              passTries = PASS_TRYS_MAX;
              unlockState = HIGH;
              sleepState = LOW;
              unlockTimeLast = millis();
              sleepTimeLast = millis(); 
              checks = 0;
              SerialUSB.println("Device Unlocked");
              menu_1();
            }
          }
          j++;
        }
        if(unlockState == LOW)
        {
          //Check for any data errors with the number of tries 
          if(passTries == 0 || passTries > PASS_TRYS_MAX)
          {
            passTries = PASS_TRYS_MAX;
          }
          passTries--;
          SerialUSB.println("Invalid Device Password");
          if(passTries==0)
          {
            SerialUSB.println("MAX TRIES");
            setDefaults();
            SerialUSB.println("Factory Clear Done");
            passTries = PASS_TRYS_MAX;
          }
          else 
          {
            SerialUSB.print("Tries Left: ");
            SerialUSB.println(passTries);
            SerialUSB.println(">>>Please Enter \"Device\" Password<<<");
          }   
        }
      }
    }
    else if (unlockState ==HIGH)
    {
      char getSerial = SerialUSB.read();
      sleepTimeLast = millis(); 
      unlockTimeLast = millis();
      if(getSerial == '0')
      {
        menu_1();
      }  
      else if(getSerial == '1')
      {
        SerialUSB.println("Enter New Device Password");
        uint32_t passwordWaitTime = millis();
        uint8_t words1=0;
        uint8_t words2=0;
        while(SerialUSB.available()>0 || ((millis()-passwordWaitTime)<ENROLL_TIME))
        {
          if(SerialUSB.available()>0)
          {
            passwordTemp1[words1]=SerialUSB.read();
            words1++;
          }          
        }
        if(words1<4 || words2>(PASS_MAX-1))
        {
          SerialUSB.println("TimeOut/ Invalid");
        }
        else
        {
          passwordTemp1[words1]='\0';
          SerialUSB.println("Enter Password Again");
          passwordWaitTime = millis();
          words2=0;
          while(SerialUSB.available()>0 || ((millis()-passwordWaitTime)<ENROLL_TIME))
          {
            if(SerialUSB.available()>0)
            {
              passwordTemp2[words2]=SerialUSB.read();
              words2++;
            }          
          }
          if(words2<4 || words2>(PASS_MAX-1))
          {
            SerialUSB.println("TimeOut / Invalid");
          }
          else
          {
          if(words2==words1)
          {
            uint8_t test = 1;
            for(int i=0;i<words1;i++)
            {
              if(passwordTemp1[i]!=passwordTemp2[i])
              {
                SerialUSB.println("Passwords dont match");
                test = 0;
                break;
              }
            }
            if(test ==1)
            {
              for(int i=0;i<PASS_MAX;i++)
              {
                storeNVS.pass0[i] = passwordTemp1[i];
              }
            my_flash_store.write(storeNVS);
            SerialUSB.println("Device Password Updated");
            }
            
          }
          else
          {
            SerialUSB.println("Passwords dont match");
          }
          }
        } 
        menu_1();       
      }     
      else if(getSerial == '2')
      {
        SerialUSB.println("Enter New Password 1");
        uint32_t passwordWaitTime = millis();
        uint8_t words1=0;
        uint8_t words2=0;
        while(SerialUSB.available()>0 || ((millis()-passwordWaitTime)<ENROLL_TIME))
        {
          if(SerialUSB.available()>0)
          {
            passwordTemp1[words1]=SerialUSB.read();
            words1++;
          }          
        }
        if(words1<4|| words2>(PASS_MAX-1))
        {
          SerialUSB.println("TimeOut/ Invalid");
        }
        else
        {
          passwordTemp1[words1]='\0';
          SerialUSB.println("Enter Password Again");
          passwordWaitTime = millis();
          words2=0;
          while(SerialUSB.available()>0 || ((millis()-passwordWaitTime)<ENROLL_TIME))
          {
            if(SerialUSB.available()>0)
            {
              passwordTemp2[words2]=SerialUSB.read();
              words2++;
            }          
          }
          if(words2<4 || words2>(PASS_MAX-1))
          {
            SerialUSB.println("TimeOut/ Invalid");
          }
          else
          {
          if(words2==words1)
          {
            uint8_t test = 1;
            for(int i=0;i<words1;i++)
            {
              if(passwordTemp1[i]!=passwordTemp2[i])
              {
                SerialUSB.println("Passwords dont match");
                test = 0;
                break;
              }
            }
            if(test ==1)
            {
              for(int i=0;i<PASS_MAX;i++)
              {
                storeNVS.pass1[i] = passwordTemp1[i];
              }

            my_flash_store.write(storeNVS);
            SerialUSB.println("Password 1 Updated");
            }            
          }
          else
          {
            SerialUSB.println("Passwords dont match");
          }
          }
        }     
        menu_1();    
      }  
      else if(getSerial == '3')
      {
        fps.open(false);
        uint16_t fingerPrintCount = fps.getEnrollCount();
        if(fingerPrintCount<NACK_TIMEOUT)
        {
          SerialUSB.print("EnrollCount: ");
          SerialUSB.println(fingerPrintCount);
        }
        else
        {
          SerialUSB.print("EnrollCount FAIL: ");
          SerialUSB.println(fingerPrintCount);
        }
        menu_1(); 
      }
      else if(getSerial == '4')
      {
        uint16_t State = FingerPrintEnrollment();
        if(State==NO_ERROR)
        {
          SerialUSB.println("Add Finger Success");
        }        
        else
        {
          SerialUSB.print("Add Finger Fail ERROR: ");
          SerialUSB.println(State);
        }
        delay(2000);
        menu_1(); 
      }
      else if (getSerial == '5')
      {
          uint16_t openStatus = fps.open(true);
          if(openStatus == NO_ERROR) 
          {
            uint16_t checkLED = fps.cmosLed(true);
            if(checkLED == NO_ERROR)
            { 
              SerialUSB.println("Place finger you want to check on Sensor");
              uint32_t FingerCountTime = millis();
              uint16_t checkFinger = FINGER_IS_NOT_PRESSED;
              while((checkFinger==FINGER_IS_NOT_PRESSED) && ((millis() - FingerCountTime)<ENROLL_TIME))
              {
                delay(100); 
                checkFinger = fps.isPressFinger();                 
              }
              if(checkFinger == FINGER_IS_PRESSED)
              {
                SerialUSB.println("-FINGER IS PRESSED");
                checkFinger = fps.captureFinger();
                if(checkFinger == NO_ERROR)
                {
                  SerialUSB.println("-FINGER CAPTURED");
                  checkFinger = fps.identify();
                  if(checkFinger < PRINTS_MAX)
                  {
                    fps.cmosLed(false);
                    SerialUSB.print("-FINGER FOUND ID: ");
                    SerialUSB.println(checkFinger);
                  }
                  else
                  {
                    SerialUSB.println("-FINGER NOT FOUND");
                  }
                }
                else
                {
                  SerialUSB.println("-FINGER CAPTURE FAILED");
                }
              }
              else
              {
                SerialUSB.print("-FINGER FAIL: ");
                SerialUSB.println(checkFinger,HEX);
              }
            }
            else
            {
              SerialUSB.print("-LED FAIL: ");
              SerialUSB.println(checkLED,HEX);
            }
            fps.cmosLed(false);
            fps.cmosLed(false);
          } 
          else 
          {
            SerialUSB.print("-Initialization failed!\nstatus: ");
            SerialUSB.print(openStatus, HEX);
            SerialUSB.println();
          }
          menu_1(); 
      }
      else if (getSerial == '6')
      {
        uint16_t State = fps.deleteAll();
        if(State==NO_ERROR)
        {
          SerialUSB.println("-All Prints Deleted");
        }  
        else
        {
          SerialUSB.print("-Delete Failed: "); 
          SerialUSB.println(State,HEX);    
        }
        menu_1(); 
      }
      else
      {
        //SerialUSB.println(check);
        SerialUSB.println();
        SerialUSB.println("-Option Invalid");
        SerialUSB.println();
        menu_1(); 
      }
      sleepState = LOW;
      sleepTimeLast = millis(); 
      unlockTimeLast = millis();
    }
  }
}

/*========Start a PC Login=====================*/
void pclogin(char * pass)
{
  delay(50);
  Keyboard.write(KEY_UP_ARROW);
  delay(300);
  Keyboard.print(pass);
  delay(300);
  Keyboard.write(KEY_RETURN);
  delay(100);
}

/*========Start Finger print enrollment=====================*/
uint8_t FingerPrintEnrollment(void)
{
  uint8_t enrollState = NO_ERROR;
  uint16_t enrollid = 0;
  uint32_t enrollTimeOut = 0;
  enrollState = fps.open(false);
  if(enrollState == NO_ERROR)
  {
    SerialUSB.println("Starting Enrollment");
    enrollState = ID_IS_ENROLLED;
    while (enrollState == ID_IS_ENROLLED)
    {
      enrollState = fps.checkEnrolled(enrollid);
      if (enrollState==ID_IS_ENROLLED) 
      {
        enrollid++;
      }
      else if(enrollState != ID_IS_NOT_ENROLLED)
      {
        SerialUSB.print("ID Error: "); 
        SerialUSB.print(enrollid); 
        SerialUSB.print(" : ");
        SerialUSB.println(enrollState,HEX); 
      }
      delay(1);
    }
    if(enrollState == ID_IS_NOT_ENROLLED)
    {
      SerialUSB.print("ID Cleared: "); 
      SerialUSB.println(enrollid);
      enrollState = fps.enrollStart(enrollid);
      if(enrollState == NO_ERROR)
      {
        enrollState = fps.cmosLed(true);
        if(enrollState==NO_ERROR)
        {
          for(int i = 1;i<4;i++)
          {
            enrollState = fps.cmosLed(true);
            enrollTimeOut = millis();
            SerialUSB.print(i);
            SerialUSB.println(" - Place Same Finger on sensor");
            enrollState = FINGER_IS_NOT_PRESSED;
            while((enrollState != FINGER_IS_PRESSED) && ((millis()-enrollTimeOut)<ENROLL_TIME))
            {
              delay(100);
              enrollState = fps.isPressFinger();
            }                    
            if(enrollState == FINGER_IS_PRESSED)
            {
              SerialUSB.println("FINGER IS PRESSED");
              enrollState = NO_ERROR;
              enrollState = fps.captureFinger(1);
              if(enrollState==NO_ERROR)
              {
                SerialUSB.println("Finger Captured");
                enrollState = fps.enrollFinger(i);
                if(enrollState == NO_ERROR)
                {  
                  SerialUSB.println("Finger Enrolled");
                }
                else
                {
                  SerialUSB.print("Enroll Failed: ");
                  SerialUSB.println(enrollState,HEX);
                  break;
               }                
              }
              else
              {
                SerialUSB.print("Capture Failed: ");
                SerialUSB.println(enrollState,HEX);
                break;
              }
              enrollTimeOut = millis();
              SerialUSB.println("Remove Finger");
              while((enrollState == NO_ERROR) && ((millis()-enrollTimeOut)<ENROLL_TIME))
              {
                enrollState = fps.isPressFinger();
                delay(5);
              } 
              fps.cmosLed(false);
              if((millis()-enrollTimeOut)>ENROLL_TIME)
              {
                SerialUSB.println("Did not Remove Finger: TimeOut");
                break;
              }
              enrollState = NO_ERROR;              
            }
            else
            {
              SerialUSB.println("Finger pressed TimeOut");
              break;
            }
            fps.cmosLed(false);
            delay(1000);
          } //End of For loop
          
          if(enrollState == NO_ERROR)
          {
            SerialUSB.println("DONE");
          }
        }
      }
      else
      {
        SerialUSB.print("Enrolling Start Failed: "); 
        SerialUSB.println(enrollState,HEX);
      }
    }
    else
    {
      SerialUSB.print("Enrolling ID Fail: "); 
      SerialUSB.println(enrollState,HEX); 
    }
  }
  else
  {
    SerialUSB.print("Enrolling Fail: "); 
    SerialUSB.println(enrollState,HEX); 
  }
  fps.cmosLed(false);
  return enrollState;
}

/*========Enter Defaults if there is an error=====================*/
void setDefaults(void)
{
    char one = '1';
    for(int i=0;i<8;i++)
    {
      storeNVS.pass0[i]=one;
      storeNVS.pass1[i]=one;
      storeNVS.pass0[i+1]='\0';
      storeNVS.pass0[i+1]='\0';
      one++;
    }    
    for(int i=0;i<PASS_MAX;i++)
    {
      storeNVS.name1[i]=defaultName1[i];
    }
    fps.open(false);
    uint16_t State = fps.deleteAll();
    storeNVS.valid = HIGH;
    my_flash_store.write(storeNVS);
    delay(1000);
}

/*========Show Menu Options=====================*/
void menu_1(void)
{
  SerialUSB.println();
  SerialUSB.println("===================================");
  SerialUSB.println("Menu Options:");
  SerialUSB.println("1: Change Device Password");
  SerialUSB.println("2: Change Stored Password 1");
  SerialUSB.println("3: Check FingerPrint Count");
  SerialUSB.println("4: Add FingerPrint");
  SerialUSB.println("5: Check if Finger is enrolled");
  SerialUSB.println("6: Remove All FingerPrints");
  SerialUSB.println("===================================");
  SerialUSB.println();
}





