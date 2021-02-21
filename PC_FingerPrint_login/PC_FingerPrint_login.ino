/*
* Christian Barran
* Subscribe to my Youtube Channel
* www.youtube.com/c/justbarran
*
*
*Keyboard Library: https://www.arduino.cc/reference/en/language/functions/usb/keyboard/
*FlashStorage Library: https://www.arduino.cc/reference/en/libraries/flashstorage/
*FPS_GT-521Fxx Library: https://github.com/sparkfun/Fingerprint_Scanner-TTL
*/
#include "Keyboard.h"
#include <FlashStorage.h> 
#include "FPS_GT511C3.h"

#define ON_BOARD_LED 13
#define TOUCH_SENSOR 1
#define SS_RX 7
#define SS_TX 6
#define PASS_MAX 33
#define PASS_TRYS 3
#define DEFAULT_PASS 1234
#define ENCRYPT_KEY 1234

#define UNLOCK_TIME 30000 //milliseconds

FPS_GT511C3 fps(SS_RX,SS_TX);
char devicePassTemp[PASS_MAX];
char devicePass[PASS_MAX]= {'4','3','2','1','\0'};
char password[PASS_MAX]= {'7','9','1','9','8','7','1','\0'};
byte touchStateLast = LOW;
byte touchState = LOW;

byte menuState = 0;
byte unlockState = LOW;
byte passTries = 0;


unsigned long unlockTimeLast = 0;

typedef struct {
  boolean valid;
  byte tries;
  char pass0[32];
  char name1[32];
  char pass1[32];
  char name2[32];
  char pass2[32];
  char name3[32];
  char pass3[32];
} Passwords;


void setup() {
  // put your setup code here, to run once:
  pinMode(ON_BOARD_LED, OUTPUT);
  pinMode(TOUCH_SENSOR, INPUT);
  SerialUSB.begin(9600);
  Keyboard.begin();
  fps.Open(); //send serial command to initialize fps
  delay(1000);
  SerialUSB.println("JustBarran FingerPrint Login");
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
            fps.Open();
            fps.SetLED(1);
            fps.DeleteAll();
            fps.SetLED(0);
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
        int fingerPrints = fps.GetEnrollCount();
        SerialUSB.printf("FPS Enrolled: %d \n",fingerPrints);
      }
      else if(check == '3')
      {
        Enroll();
      }
      else if (check == '4')
      {
        fps.Open();
        fps.SetLED(1);
        fps.DeleteAll();
        fps.SetLED(0);
        SerialUSB.println("All Prints Deleted");
      }
      else
      {
        SerialUSB.println(check);
      }
    }
  }
  if(unlockState == HIGH)
  {
    if((millis()-unlockTimeLast) > unlockTimeLast)
    {
      SerialUSB.println("Device Locked");
      unlockState = LOW;
    }
  }
  // put your main code here, to run repeatedly:
  touchState = digitalRead(TOUCH_SENSOR);
  if((touchState!=touchStateLast) && (touchState==1))
  {
    digitalWrite(ON_BOARD_LED,LOW);
    fps.Open();
    delay(100);
    if(fps.SetLED(1)==true)
    {
      SerialUSB.println("LED ON");
      delay(100);
      
      if (fps.IsPressFinger())
      {
        fps.CaptureFinger(false);
        int id = fps.Identify1_N();
        if (id <200)
        {
          SerialUSB.print("Verified ID:");
          SerialUSB.println(id);
          pc_login(password);
        }
        else
        {
          SerialUSB.println("Finger not found");
        }
      }
      else
      {
        SerialUSB.println("No Finger Press");
      }
    }
    fps.Close();
  }
  else if (touchState==0)
  {
    digitalWrite(ON_BOARD_LED,HIGH);
    fps.SetLED(0);// turn off the LED inside the fps
  }
  touchStateLast = touchState;
  delay(100);
}


void pc_login(char * pass)
{
  Keyboard.write(KEY_RETURN);
  delay(1000);
  Keyboard.print(pass);
  delay(500);
  Keyboard.write(KEY_RETURN);
  delay(500);
}

void firstTime()
{
  SerialUSB.println("Type in Password < 32 char to be used by Device ");
  //SerialUSB.println("Password Invalid");
  //SerialUSB.println("Password Valid");
  SerialUSB.println("Type in Password again");
}

void menu_0()
{
  SerialUSB.println("Type in Password to Unlock Device!");
  //SerialUSB.println("Password Invalid");
  //SerialUSB.println("Password Valid");
}

void menu_1()
{
  SerialUSB.println("Menu Options:");
  SerialUSB.println("1: Change Device Password");
  SerialUSB.println("2: Check FingerPrints");
  SerialUSB.println("3: Add FingerPrint");
  SerialUSB.println("4: Remove All FingerPrints");
  SerialUSB.println("5: Change a Password");
  SerialUSB.println("6: Factory Reset");
  //SerialUSB.println("Option Invalid");
}

void Enroll()
{
  // Enroll test
  fps.Open();
  fps.SetLED(1);
  // find open enroll id
  int enrollid = 0;
  bool usedid = true;
  while (usedid == true)
  {
    usedid = fps.CheckEnrolled(enrollid);
    if (usedid==true) enrollid++;
  }
  fps.EnrollStart(enrollid);

  // enroll
  SerialUSB.print("Press finger to Enroll #");
  SerialUSB.println(enrollid);
  while(fps.IsPressFinger() == false) delay(100);
  bool bret = fps.CaptureFinger(true);
  int iret = 0;
  if (bret != false)
  {
    SerialUSB.println("Remove finger");
    fps.Enroll1(); 
    while(fps.IsPressFinger() == true) delay(100);
    SerialUSB.println("Press same finger again");
    while(fps.IsPressFinger() == false) delay(100);
    bret = fps.CaptureFinger(true);
    if (bret != false)
    {
      SerialUSB.println("Remove finger");
      fps.Enroll2();
      while(fps.IsPressFinger() == true) delay(100);
      SerialUSB.println("Press same finger yet again");
      while(fps.IsPressFinger() == false) delay(100);
      bret = fps.CaptureFinger(true);
      if (bret != false)
      {
        SerialUSB.println("Remove finger");
        iret = fps.Enroll3();
        if (iret == 0)
        {
          SerialUSB.println("Enrolling Successful");
        }
        else
        {
          SerialUSB.print("Enrolling Failed with error code:");
          SerialUSB.println(iret);
        }
      }
      else SerialUSB.println("Failed to capture third finger");
    }
    else SerialUSB.println("Failed to capture second finger");
  }
  else SerialUSB.println("Failed to capture first finger");
  fps.SetLED(0);
}
