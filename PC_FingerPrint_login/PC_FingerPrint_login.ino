/*
* Christian Barran
* Subscribe to my Youtube Channel
* www.youtube.com/c/justbarran
*
*
*Keyboard Library: https://www.arduino.cc/reference/en/language/functions/usb/keyboard/
*FlashStorage Library: https://www.arduino.cc/reference/en/libraries/flashstorage/
*
*
*/


#include "Keyboard.h"
#include <FlashStorage.h> 
#include <GT_521F.h>

#define ON_BOARD_LED 13
#define TOUCH_SENSOR 1
#define SS_RX 7
#define SS_TX 6
#define PASS_MAX 33
#define PRINTS_MAX 200 //200=GT_521F32 3000=GT_521F52
#define PASS_TRYS 3
#define FINGER_TRYS 3
#define DEFAULT_PASS 1234
#define ENCRYPT_KEY 1234

#define UNLOCK_TIME 10000 //milliseconds
#define ENROLL_TIME 5000 //milliseconds


char devicePassTemp[PASS_MAX];
char devicePass[PASS_MAX]= {'4','3','2','1','\0'};
char password[PASS_MAX]= {'7','9','1','9','8','7','1','\0'};

uint8_t touchStateLast = LOW;
uint8_t touchState = LOW;
uint8_t menuState = 0;
uint8_t unlockState = LOW;
uint8_t passTries = 0;


uint32_t unlockTimeLast = 0;

typedef struct {
  uint8_t valid;
  uint8_t tries;
  uint8_t fpsID[16];
  char pass0[32];
  char name1[32];
  char pass1[32];
  char name2[32];
  char pass2[32];
  char name3[32];
  char pass3[32];
} Passwords;

GT_521F fps(Serial1);

void setup() {
  // put your setup code here, to run once:
  SerialUSB.begin(9600);
  pinMode(ON_BOARD_LED, OUTPUT);
  pinMode(TOUCH_SENSOR, INPUT);  
  Keyboard.begin();
  SerialUSB.println("fps.begin");
  while(!fps.begin(9600))
  {
    SerialUSB.print(".");
    digitalWrite(ON_BOARD_LED,!digitalRead(ON_BOARD_LED)); 
  }
  digitalWrite(ON_BOARD_LED,HIGH);
}

void loop() {
 
  if(SerialUSB.available()!=0)
  {        
    if(unlockState ==LOW)
    {
      byte i = 0;
      while(SerialUSB.available()>0)
      {
        devicePassTemp[i]=SerialUSB.read();
        i++;
      }
      devicePassTemp[i] = '\0';
      if(i>=4)
      {
        byte j = 0;
        byte check = 1;
        while((check == 1)&&(j<=PASS_MAX))
        {
          if(devicePassTemp[j]!=devicePass[j])
          {
            check = 0;
          }
          else
          {
            if(devicePass[j]=='\0')
            {
              passTries = 0;
              unlockState = HIGH;
              unlockTimeLast = millis();
              check = 0;
              SerialUSB.println("Device Unlocked");
              menu_1();
            }
          }
          j++;
        }
        if(unlockState == LOW)
        {
          passTries++;
          SerialUSB.println("Invalid Device Password");
          if(passTries==PASS_TRYS)
          {
            SerialUSB.println("MAX TRIES");
            SerialUSB.println("Factory Clear Done");
          }
          else 
          {
            SerialUSB.print("Tries Left: ");
            SerialUSB.println((PASS_TRYS-passTries));
            SerialUSB.println("Please Enter Device Password");
          }
          
        }
      }
    }
    else 
    {
      char check = SerialUSB.read();
      unlockTimeLast = millis();
      if(check == '0')
      {
        menu_1();
      }  
      else if(check == '1')
      {
        
      }     
      else if(check == '2')
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
      }
      else if(check == '3')
      {
        uint16_t State = FingerPrintEnrollment();
        if(State==NO_ERROR)
        {
          SerialUSB.print("Add Finger Success ID: ");
          SerialUSB.println(fps.getEnrollCount());
        }
        else
        {
          SerialUSB.print("Add Finger Fail ERROR: ");
          SerialUSB.println(State);
        }

      }
      else if (check == '4')
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
          } 
          else 
          {
            SerialUSB.print("-Initialization failed!\nstatus: ");
            SerialUSB.print(openStatus, HEX);
            SerialUSB.println();
          }
      }
      else if (check == '5')
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
      }
      else
      {
        //SerialUSB.println(check);
        SerialUSB.println("-Option Invalid");
      }
    }
  }
  if(unlockState == HIGH)
  {
    if((millis()-unlockTimeLast) > UNLOCK_TIME)
    {
      SerialUSB.println("-Device Locked");
      unlockState = LOW;
    }
  }
  // put your main code here, to run repeatedly:
  touchState = digitalRead(TOUCH_SENSOR);
  if((touchState!=touchStateLast) && (touchState==1) && (unlockState ==LOW))
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
        while((checkFinger==FINGER_IS_NOT_PRESSED) && (FingerCount<FINGER_TRYS))
        {
          delay(5); 
          checkFinger = fps.isPressFinger();
          FingerCount++;                   
        }
        if(checkFinger == FINGER_IS_PRESSED)
        {
          checkFinger = fps.captureFinger();
          if(checkFinger == NO_ERROR)
          {
            checkFinger = fps.identify();
            if(checkFinger < PRINTS_MAX)
            {
              pc_login(password);
            }
          }
        }
        else
        {
          SerialUSB.print("FINGER FAIL: ");
          SerialUSB.println(checkFinger,HEX);
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
  delay(100);
}


void pc_login(char * pass)
{
  Keyboard.write(KEY_RETURN);
  delay(250);
  Keyboard.print(pass);
  delay(250);
  Keyboard.write(KEY_RETURN);
}

uint8_t FingerPrintEnrollment()
{
  touchState = HIGH;
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
              enrollState = FINGER_IS_PRESSED;
              while((enrollState == FINGER_IS_PRESSED) && ((millis()-enrollTimeOut)<ENROLL_TIME))
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

void menu_1()
{
  SerialUSB.println("Menu Options:");
  SerialUSB.println("1: Change Device Password");
  SerialUSB.println("2: Check FingerPrint Count");
  SerialUSB.println("3: Add FingerPrint");
  SerialUSB.println("4: Check if Finger is enrolled");
  SerialUSB.println("5: Remove All FingerPrints");

  //SerialUSB.println("Option Invalid");
}
