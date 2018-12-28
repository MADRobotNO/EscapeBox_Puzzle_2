//Puzzle 2.20 - Mixer table
//By MAD Robot, Martin Agnar Dahl
//ver 2.20 - MP3 module added
//ver 2.21 - debug
//ver 2.23 - new final mp3, final ver.

#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
//#define DEBUG

const int buttons[8] = {2, 3, 4, 5, 6, 7, 8, 9};

const int leds[16]= {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37}; //even numbers = green, odd numbers = red, new led every second number (22,24,26...)
int butValues[8] = {};
const int correctCode[8] = {7, 5, 3, 8, 1, 6, 4, 2};
int inputCode[8] = {0, 0, 0, 0, 0, 0, 0 , 0};
int countInput = 0;
int lock = 40;
boolean game = true;
int buttonStatus = 0;

///Mp3
DFRobotDFPlayerMini mp3;

//Software
SoftwareSerial softSerial(10, 11); //RX, TX

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Setup starts...");
  Serial.println();
  #endif

  
  softSerial.begin(9600);  //Communication with MP3
 
  //MP3 setup
  mp3.begin(softSerial);
  mp3.outputDevice(DFPLAYER_DEVICE_SD);
  mp3.volume(25);
  
  pinMode(lock, OUTPUT);
  digitalWrite(lock, HIGH);
  
  for (int i = 0; i<8; i++){
    pinMode(buttons[i], INPUT_PULLUP);
  }

  for (int i = 0; i<16; i++){
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }
}

void loop() {

while(game){

  if (checkButtons()!=0){
    regButton(checkButtons()-1);
    playSound(checkButtons());
  }
  
  if (countInput > 0){    
    for (int i = 0; i<countInput; i++){
      
      if (inputCode[i]==correctCode[i]){
        int j=i;
        Serial.print(j+1);Serial.println(". nr correct!");   
        j=j*2;
        Serial.print("Pin Led: ");Serial.print(leds[j]);Serial.println(" lights");
        digitalWrite(leds[j], HIGH);
        if(countInput==8 && i==7){
          Serial.println("CODE CORRECT! LOCK IS OPEN NOW!");          

          delay(2100);
          
          mp3.play(9);
          
          delay(2691);

          resetLEDwin();
          digitalWrite(lock, LOW);
          game=false;
          endGame();
          break;
        }
      }
      
      else{
        
        int l = i*2+1;
        digitalWrite(leds[l], HIGH);
        Serial.print("Pin Led: ");Serial.print(leds[l]);Serial.println(" lights");
        Serial.println("Code incorrect! Starting from the beginning");
        countInput = 0;
        for (int k= 0; k<sizeof(inputCode)/2; k++){
          inputCode[k]=0;
        }
        delay(1000);
        resetLED();
        break;
      }
    }
    
    while (buttonStatus!=0){
  
      if (checkButtons()==0){
        buttonStatus=0;
      } 
      delay(5);
    }
    
  }

  
  Serial.print("User code: ");
  for (int i = 0; i<(sizeof(inputCode)/2); i++){
    Serial.print(inputCode[i]);
    Serial.print(" ");
  }
  
  Serial.println("");
  
}
}

void resetLED(){
  for (int i = 0; i<(sizeof(leds)/2); i++){
    digitalWrite(leds[i], LOW);
  }
}

void resetLEDwin(){
  for (int i = 0; i<(sizeof(leds)/2); i++){
    digitalWrite(leds[i], LOW);
  }
  
  for(int i=0; i<18; i++){
    for (int i = 0; i<(sizeof(leds)/2)/2; i++){
      digitalWrite(leds[(i*2)], HIGH);
   
      delay(70); //70*2*8*6=6720-2691 =9411
      digitalWrite(leds[(i*2)], LOW);
    }
    
    for (int i = 0; i<(sizeof(leds)/2)/2; i++){
      digitalWrite(leds[((i*2)+1)], HIGH);
  
      delay(70);
      digitalWrite(leds[((i*2)+1)], LOW);
    }
  }
    
  for (int i = 0; i<(sizeof(leds)/2); i++){
    digitalWrite(leds[i], LOW);
  }
    
}

int checkButtons(){
  int val = 0;
    for (int i = 0; i<8; i++){
    butValues[i]=digitalRead(buttons[i]);
    if (butValues[i]==0){
      val = i+1;
      buttonStatus= i+1;
    }
    }
  return val;
}

void regButton(int i){
  countInput++;
    for (int j = 0; j<(sizeof(inputCode)/2); j++){
    
      if (inputCode[j] ==0){
        inputCode[j]=i+1;
        break;
      }
    }
}

void playSound(int buttonNr){
  mp3.play(buttonNr);
}

void endGame(){
  while(!game){
    //digitalWrite(lock, LOW);
    //Game ends
  }
}

