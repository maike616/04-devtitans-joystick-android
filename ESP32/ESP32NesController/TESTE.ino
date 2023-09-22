volatile int nesData       = 12;    
volatile int nesClock      = 14;    
volatile int nesLatch      = 13;

volatile int nesUp         = 1;
volatile int nesDown       = 16;
volatile int nesLeft       = 3;
volatile int nesRight      = 17;
volatile int sw_pin        = 34;
volatile int x_pin         = 4;
volatile int y_pin         = 13;

volatile int ClockCount=1;

volatile int Button_B_State=0;
volatile int Button_A_State=0;
volatile int Button_SELECT_State=0;
volatile int Button_START_state=0;
volatile int Button_UP=0;
volatile int Button_DOWN=0;
volatile int Button_LEFT=0;
volatile int Button_RIGHT=0;

volatile int IsLatched=0;





void setup() 
{
  Serial.begin(115200);
  pinMode(nesData, OUTPUT);
  pinMode(sw_pin, INPUT);

  digitalWrite(nesData,HIGH);
  digitalWrite(SW_pin, HIGH);

  pinMode(nesUp, INPUT_PULLUP); //UP
  pinMode(nesLeft, INPUT_PULLUP); //LEFT
  pinMode(nesDown, INPUT_PULLUP); //DOWN
  pinMode(nesRight, INPUT_PULLUP); //RIGHT
  pinMode(5, INPUT_PULLUP); //SELECT
  pinMode(18, INPUT_PULLUP); //START
  pinMode(19, INPUT_PULLUP); //B
  pinMode(21, INPUT_PULLUP); //A
  
  
  

  
  pinMode(nesLatch, INPUT_PULLUP); 
  pinMode(nesClock, INPUT_PULLUP);

  attachInterrupt(13, Latched, HIGH);
  attachInterrupt(14, Clocked, HIGH);
  
 
  
  

}

void loop()
{
  /*
  Button_B_State=digitalRead(19);
  Button_A_State=digitalRead(21);
  Button_SELECT_State=digitalRead(5);
  Button_START_state=digitalRead(18);
  Button_UP=digitalRead(4);
  Button_DOWN=digitalRead(17);
  Button_LEFT=digitalRead(16);
  Button_RIGHT=digitalRead(3);

  
  if(Button_B_State==0){Serial.println("Button B Pressed");}
  if(Button_A_State==0){Serial.println("Button A Pressed");}
  if(Button_SELECT_State==0){Serial.println("Button Select Pressed");}
  if(Button_START_state==0){Serial.println("Button Start Pressed");}
  if(Button_UP==0){Serial.println("Button Up Pressed");}
  if(Button_DOWN==0){Serial.println("Button Down Pressed");}
  if(Button_LEFT==0){Serial.println("Button Left Pressed");}
  if(Button_RIGHT==0){Serial.println("Button Right Pressed");}
  */
 
  Serial.print("Switch:  ");
  Serial.print(digitalRead(SW_pin));
  Serial.print(" | ");
  Serial.print("X-axis: ");
  Serial.print(analogRead(X_pin));
  Serial.print(" | ");
  Serial.print("Y-axis: ");
  Serial.print(analogRead(Y_pin));
  Serial.println(" | ");
  delay(200);
}

void Latched()
{
  IsLatched=1;
  
  Button_B_State=digitalRead(19);
  Button_A_State=digitalRead(21);
  Button_SELECT_State=digitalRead(5);
  Button_START_state=digitalRead(18);
  Button_UP=digitalRead(nesUp);
  Button_DOWN=digitalRead(nesDown);
  Button_LEFT=digitalRead(nesLeft);
  Button_RIGHT=digitalRead(nesRight);
  /*
  Serial.print("up ");
  Serial.print(Button_UP);
  Serial.print(", down ");
  Serial.print(Button_DOWN);
  Serial.print(", left ");
  Serial.print(Button_LEFT);
  Serial.print(", right ");
  Serial.println(Button_RIGHT); 
  */

  /*
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
void Clocked()
{
  if(IsLatched==1){
      
      if(ClockCount==1){digitalWrite(nesData,Button_B_State);}
      if(ClockCount==2){digitalWrite(nesData,Button_SELECT_State);}
      if(ClockCount==3){digitalWrite(nesData,Button_START_state);}
      if(ClockCount==4){digitalWrite(nesData,Button_UP);}
      if(ClockCount==5){digitalWrite(nesData,Button_DOWN);}
      if(ClockCount==6){digitalWrite(nesData,Button_LEFT);} //Left
      if(ClockCount==7){digitalWrite(nesData,Button_RIGHT);}//Right
      //Serial.print("Clock Cycle:"); Serial.println(ClockCount);
      ClockCount++;
      
  }
  
}
