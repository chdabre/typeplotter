#include <Stepper.h>

/* BEGIN PIN DECLARATIONS */

//Pen movement motors
int penPlusA = 10;
int penPlusB = 11;
int penMinusA = 12;
int penMinusB = 13;

//XY movement steppers
Stepper stepperX(96,0,1,2,3,1);
Stepper stepperY(96,4,5,6,7,1);

//switches
int limitPin = 8;
int modeSwitch = 9;

//light sensor
int sensorPin = A0;

//beeper
int beepPin = A1;

/* END PIN DECLARATIONS */

int xPos = 0;
int yPos = 0;
boolean isPenDown = false;

int width = 0;
int height = 0;

int stepSizeX = 0;
int stepSizeY = 0;

int maxSteps = 2000;

int motorSpeed = 300;
int penSpeed = 150;

int sensorCalib = 0;

void setup()
{
  /* PIN MODE DECLARATIONS*/
  stepperX.setSpeed(motorSpeed);
  stepperY.setSpeed(motorSpeed);

  pinMode(penPlusA, OUTPUT);     
  pinMode(penPlusB, OUTPUT);     
  pinMode(penMinusA, OUTPUT);     
  pinMode(penMinusB, OUTPUT);    

  pinMode(limitPin, INPUT);
  digitalWrite(limitPin, HIGH);

  pinMode(modeSwitch, INPUT);
  digitalWrite(modeSwitch, HIGH);

  digitalWrite(sensorPin, HIGH);
  /* PIN MODE DECLARATIONS*/

  Serial.begin(9600);

  beep(100,1,false);
  beep(100,1,true);

  while(!Serial){
  }
  
  if(digitalRead(modeSwitch) == LOW){
    Serial.println("Scan mode!");
    beep(100,2,false);  
    scan_mode();
    return;
  }

  Serial.println("Print mode!");

  beep(100,1,false);
  
  goNull();
  initPen();
  goHome();
  
  //find_paper_width();
  getData();

  Serial.println(1);
  beep(100,1,false);

  draw();
}

void getData(){
  while(Serial.available()<1){
  }
  String command = Serial.readString();

  int data[4];
  int numArgs = 0;

  int beginIdx = 0;
  int idx = command.indexOf(",");

  String arg;
  char charBuffer[16];

  while (idx != -1)
  {
    arg = command.substring(beginIdx, idx);
    arg.toCharArray(charBuffer, 16);

    // add error handling for atoi:
    data[numArgs++] = atoi(charBuffer);
    beginIdx = idx + 1;
    idx = command.indexOf(",", beginIdx);
  }

  data[numArgs++] = command.substring(beginIdx).toInt();

  width = data[0];
  height = data[1];
  stepSizeX = data[2];
  penSpeed = data[3];

  stepSizeY = round(stepSizeX*1.6);
}

void draw()
{
  for(int row = 0; row < height; row++) { 
    penUp();
    stepperX.step(-xPos);
    xPos = 0;  

    for(int column = 0; column < width; column++){ 

      while(Serial.available()<1){
      }

      int pixel = byte(Serial.read());

      if(pixel == 1){
        penDown();
        stepperX.step(stepSizeX);
        xPos += stepSizeX;
      }
      else if(pixel == 0){
        penUp();
        stepperX.step(stepSizeX);
        xPos += stepSizeX;
      }

      Serial.println(1);
    }

    penUp();
    stepperY.step(stepSizeY);
    yPos += stepSizeY;
    delay(15);

  }

  penUp();

  stepperX.step(-xPos);
  stepperY.step(-yPos);
  goHome();
  beep(150, 4,false);
}

void initPen(){
  pen_forward();
  delay(1000);
  pen_stop();
  isPenDown = true;
  penUp();
}

void penDown()
{

  if (!isPenDown){
    pen_forward();
    delay(penSpeed);
    pen_stop();

    isPenDown = true;
  }
}
void penUp()
{
  if(isPenDown){
    pen_backward();
    delay(penSpeed);
    pen_stop();

    isPenDown = false; 
  }
}
void pen_forward(){
  digitalWrite(penPlusA, HIGH);
  digitalWrite(penPlusB, HIGH);
  digitalWrite(penMinusA, LOW);
  digitalWrite(penMinusB, LOW);
}
void pen_backward(){
  digitalWrite(penPlusA, LOW);
  digitalWrite(penPlusB, LOW);
  digitalWrite(penMinusA, HIGH);
  digitalWrite(penMinusB, HIGH);
}
void pen_stop(){

  digitalWrite(penPlusA, LOW);
  digitalWrite(penPlusB, LOW);
  digitalWrite(penMinusA, LOW);
  digitalWrite(penMinusB, LOW);

}

void mark(){
  penDown();
  stepperY.step(10);
  penUp();
  stepperY.step(-10);
}

void goHome(){
  while(digitalRead(limitPin) == HIGH){
    stepperX.step(-1);
  }
  int black = analogRead(sensorPin);
  xPos=0;
  while(analogRead(sensorPin) >= black-30 && xPos <= maxSteps-75){
    stepperX.step(1);
    xPos++;
  }
  stepperX.step(200);
  xPos=0;
}

void goNull(){
   while(digitalRead(limitPin) == HIGH){
    stepperX.step(-1);
  }
}

void calibrate(){
  while(digitalRead(limitPin) == HIGH){
    stepperX.step(-1);
  }
  int black = analogRead(sensorPin);
  xPos=0;
  while(analogRead(sensorPin) >= black-10 && xPos <= maxSteps-75){
    stepperX.step(1);
    xPos++;
  }
  stepperX.step(200);
  xPos=0;

  sensorCalib = analogRead(sensorPin);
}

void beep(int duration, int it, boolean high){
  for(int i = 0;i<it;i++){
    if(!high) tone(beepPin,1500);
    if(high) tone(beepPin,1800);
    delay(duration);
    noTone(beepPin);
    delay(duration);
  }
}

void scan_mode(){
  penDown();
  while(Serial.available()<1){
    delay(1);
  }
  if(Serial.read() == 2)return;

  ser_flush();

  calibrate();

  String line;

  for(int y = 0;y<360;y++){
    for(int x=0; x<360;x++){
      if(Serial.available()>0){
        if(Serial.read() == 2)return;
      }

      line += analogRead(A0)-sensorCalib;

      stepperX.step(5);
      xPos+=5;

      if(x!=359) line+=",";
    }
    Serial.println(line);
    line="";

    stepperY.step(8);
    yPos+=8;

    for(int x=360; x>0;x--){
      if(Serial.available()>0){
        if(Serial.read() == 2)return;
      }

      line += analogRead(A0)-sensorCalib;

      stepperX.step(-5);
      xPos-=5;

      if(x!=1) line+=",";
    } 

    Serial.println(line);
    line="";

    stepperY.step(8);
    yPos+=8;
  }
}

void ser_flush(){
  while(Serial.available()>0){
    Serial.read();
  }
}

void find_paper_width(){
  mark();
  int white = analogRead(sensorPin);
  xPos=0;
  while(analogRead(sensorPin) <= white+100){
    stepperX.step(1);
    xPos++;
  }
  stepperX.step(0);
  xPos+=0;
  mark();
  Serial.print("Paper Width: ");
  Serial.println(xPos);
  goHome();
}

void loop(){
}









