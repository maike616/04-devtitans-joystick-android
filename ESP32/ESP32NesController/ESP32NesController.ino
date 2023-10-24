const int xPin = A4;
const int yPin = A7;
const int zPin = A6;

int xValue = 0;
int yValue = 0;
int zValue = 0;
//
//void setup()
//{
//  Serial.begin(115200);
//  delay(500);
//}
//
//
//void loop()
//{
//  xValue = analogRead(xPin);
//  yValue = analogRead(yPin);
//  zValue = analogRead(zPin);
//  Serial.print("Analog value: X:");
//  Serial.print(xValue);
//  Serial.print(" Y:");
//  Serial.print(yValue);
//  Serial.print(" Z:");
//  Serial.println(zValue);
//  delay(1000);
//}
volatile int nesData       = 12;    
volatile int nesClock      = 14;    
volatile int nesLatch      = 13;

volatile int nesUp         = 4;
volatile int nesDown       = 19;
volatile int nesLeft       = 5;
volatile int nesRight      = 18;
volatile int nesStart      = 22;
volatile int nesSelect     = 21;
volatile int nesA          = 27;
volatile int nesB          = 26;
volatile int nesC          = 25;
volatile int nesD          = 33;

volatile int ClockCount=1;

volatile int Button_A_State=0;
volatile int Button_B_State=0;
volatile int Button_C_State=0;
volatile int Button_D_State=0;
volatile int Button_START_state=0;
volatile int Button_SELECT_State=0;
volatile int Button_UP=0;
volatile int Button_LEFT=0;
volatile int Button_RIGHT=0;
volatile int Button_DOWN=0;

volatile int IsLatched=0;

void IRAM_ATTR Latched();
void IRAM_ATTR Clocked();

void setup() 
{
  Serial.begin(115200);
  pinMode(nesData, OUTPUT);

  digitalWrite(nesData,HIGH);

  pinMode(nesUp, INPUT_PULLUP); //UP
  pinMode(nesLeft, INPUT_PULLUP); //LEFT
  pinMode(nesRight, INPUT_PULLUP); //RIGHT
  pinMode(nesDown, INPUT_PULLUP); //DOWN
  pinMode(nesSelect, INPUT_PULLUP); //SELECT
  pinMode(nesStart, INPUT_PULLUP); //START
  pinMode(nesA, INPUT_PULLUP); //A
  pinMode(nesB, INPUT_PULLUP); //B
  pinMode(nesC, INPUT_PULLUP); //C
  pinMode(nesD, INPUT_PULLUP); //D
  
  pinMode(nesLatch, INPUT_PULLUP); 
  pinMode(nesClock, INPUT_PULLUP);

  attachInterrupt(nesLatch, Latched, HIGH);
  attachInterrupt(nesClock, Clocked, HIGH);
}

void loop()
{
  / /xValue = analogRead(xPin);
  //yValue = analogRead(yPin);
  //zValue = analogRead(zPin);
  /*
  Button_B_State=digitalRead(19);
  Button_A_State=digitalRead(21);
  Button_SELECT_State=digitalRead(5);
  Button_START_state=digitalRead(18);
  Button_UP=digitalRead(4);
  Button_DOWN=digitalRead(17);
  Button_LEFT=digitalRead(16);
  Button_RIGHT=digitalRead(3);
*/
  
  //Serial.print("IsLatched:");
  //Serial.println(IsLatched);
  //Serial.print("ClockCount:");
  //Serial.println(ClockCount);
  /*
  if(Button_B_State==0){Serial.println("Button B Pressed");}
  if(Button_A_State==0){Serial.println("Button A Pressed");}
  if(Button_SELECT_State==0){Serial.println("Button Select Pressed");}
  if(Button_START_state==0){Serial.println("Button Start Pressed");}
  if(Button_UP==0){Serial.println("Button Up Pressed");}
  if(Button_DOWN==0){Serial.println("Button Down Pressed");}
  if(Button_LEFT==0){Serial.println("Button Left Pressed");}
  if(Button_RIGHT==0){Serial.println("Button Right Pressed");}

 */

}

void IRAM_ATTR Latched()
{
  IsLatched=1;

  xValue = analogRead(xPin);
  yValue = analogRead(yPin);
  //zValue = analogRead(zPin);
  Button_A_State=digitalRead(nesA);
  Button_B_State=digitalRead(nesB);
  Button_C_State=digitalRead(nesC);
  Button_D_State=digitalRead(nesD);
  Button_START_state=digitalRead(nesStart);
  Button_SELECT_State=digitalRead(nesSelect);
  Button_UP=digitalRead(nesUp);
  Button_LEFT=digitalRead(nesLeft);
  Button_RIGHT=digitalRead(nesRight);
  Button_DOWN=digitalRead(nesDown);
  /*
  Serial.print("up ");
  Serial.print(Button_UP);
  Serial.print(", down ");
  Serial.print(Button_DOWN);
  Serial.print(", left ");
  Serial.print(Button_LEFT);
  Serial.print(", right ");
  Serial.println(Button_RIGHT); 
  

  
  if(Button_A_State==1){Button_A_State=0;}else{Button_A_State=1;}
if(Button_B_State==1){Button_B_State=1;}else{Button_B_State=0;}
  if(Button_SELECT_State==1){Button_SELECT_State=1;}else{Button_SELECT_State=0;}
  if(Button_START_state==1){Button_START_state=1;}else{Button_START_state=0;}
  if(Button_UP==1){Button_UP=1;}else{Button_UP=0;}
  if(Button_DOWN==1){Button_DOWN=1;}else{Button_DOWN=0;}
  if(Button_LEFT==1){Button_LEFT=1;}else{Button_LEFT=0;}
  if(Button_RIGHT==1){Button_RIGHT=1;}else{Button_RIGHT=0;}
  */
  
  digitalWrite(nesData,Button_A_State);
  ClockCount=1;

}
void IRAM_ATTR Clocked()
{
  if(IsLatched==1){
      if(digitalRead(nesClock)){
        if(ClockCount==1){digitalWrite(nesData,Button_B_State);}
        if(ClockCount==2){digitalWrite(nesData,Button_C_State);}
        if(ClockCount==3){digitalWrite(nesData,Button_D_State);}
        if(ClockCount==4){digitalWrite(nesData,Button_SELECT_State);}
        if(ClockCount==5){digitalWrite(nesData,Button_START_state);}
        if(ClockCount==6)
        {
          if(yValue >= 2000)
          {
            digitalWrite(nesData,LOW);
          }
          else
          {
            digitalWrite(nesData,Button_UP);
          }
        } //UP
        if(ClockCount==7)
        {
          if(yValue <= 1620)
          {
            digitalWrite(nesData,LOW);
          }
          else
          {
            digitalWrite(nesData,Button_DOWN);
          }
        } //Down
        if(ClockCount==8)
        {
          if(xValue <= 1650)
          {
            digitalWrite(nesData,LOW);
          }
          else
          {
            digitalWrite(nesData,Button_LEFT);  
          }
        } //Left
        if(ClockCount==9)
        {
          if(xValue >= 2000)
          {
            digitalWrite(nesData,LOW);
          }
          else
          {
            digitalWrite(nesData,Button_RIGHT); 
          }
        }//Right
        
        //Serial.print("Clock Cycle:"); Serial.println(ClockCount);
        ClockCount++;
      }
  }
  
}
