//Puzzle 2.30 - Beer tags and main puzzle
//By MAD Robot, Martin Agnar Dahl
//.18.08.2018
//6x RFID
//ver 0.0, RFID Read function and password collection
//ver 1.0, communication with 2nd arduino + LCD
//ver 1.1c, new tags
//ver 2.0, I2C Communication with other arduino, Master (add.13)
//ver 2.1, Buzzer
//ver 2.1c, New library for RFID
//ver 2.2, Code for RFID changed totally
//ver 3.0, Code for RFID changed totally again
//ver 3.0d, Debug
//ver 4.0, fail counter added, new library
//ver 4.0c, RFID works ok with 6 RFID and 10cm space between readers
//ver 4.1, lock
//ver 4.1a, new password
//ver 4.2, 2-ways communication
//ver 4.3b, Relay changed to NO
//ver 4.3d, New Relay type; (safe, HIGH) for turn on
//ver 4.3f, New password
//ver 5.0, Sending win information
//ver 5.0a, Relay back to (safe, LOW)


#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <rfid1.h>
#include <softspi.h>

#define DEBUG

const byte RST_PIN = 9;
const byte sdaPin = 53;
const byte sckPin = 52;
const byte mosiPin = 51;
const byte misoPins[]={23, 25, 27, 29, 31, 33};

const byte lockPower = 47;
const byte lock = 45;
const byte safe = 49;

const byte buzzerPin = 4;

const String tag1 = "8804DDA5";  //frosk
const String tag2 = "8804F2A7";  //fisk
const String tag3 = "8804D5A5";  //dådyr
const String tag4 = "8804E7A6";  //fugl
const String tag5 = "8804C6A7";  //øgle
const String tag6 = "8804FCA8";  //sommerfugl
const String startTag = "FAFDBBCA";  //Batman

const byte NR_OF_READERS = (sizeof(misoPins)/sizeof(byte));
const String password[NR_OF_READERS] = {tag1, tag2, tag3, tag4, tag5, tag6}; //Dådyr, fisk, frosk, sommerfugl, øgle, fugl


//game variables
String passwordIn[NR_OF_READERS];
boolean change[NR_OF_READERS];
boolean cardStatus[NR_OF_READERS]; 
boolean oldCardStatus[NR_OF_READERS]; 
String tempPass[NR_OF_READERS];
boolean passStatus[NR_OF_READERS];
int counter = 0;
boolean gameEnds = false;

String oldPassword[NR_OF_READERS] = {"Nan","Nan","Nan","Nan","Nan","Nan"};

boolean lockStatus = false;
int changeStat[NR_OF_READERS] = {0,0,0,0,0,0};
boolean game = true;


//RFID
RFID1 rfid;
uchar serNum[5]; 

//LCD
LiquidCrystal_I2C lcdBeer (0x3F, 16, 2);  //beer tags - 0x3F
LiquidCrystal_I2C lcdSafe (0x27, 16, 2);  //safe - 0x27

int oldTimer = 0;
int delayInterv = 320;

int keypadStatus = 0;

void setup() {

  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("Setup starts...");
    Serial.println();
  #endif

  Serial.println("LockPower pin connected - lock power OFF");
  pinMode (lockPower, OUTPUT);
  digitalWrite(lockPower, LOW);

  Serial.println("SafePower pin connected - lock power OFF (LOCKED)");
  pinMode (safe, OUTPUT);
  digitalWrite(safe, HIGH);
  
  pinMode (lock, INPUT);
  
  
  //LCD
  lcdSafe.begin();
  lcdSafe.clear();
  lcdSafe.backlight();

  lcdBeer.begin();
  lcdBeer.clear();
  lcdBeer.backlight();

  Serial.println("LCD Connected");

  
  //I2C com
  Wire.begin();
  Serial.println("I2C done");

  
  //RFID
  Serial.print("Numb. of readers: ");Serial.println(NR_OF_READERS);
  Serial.println();
  

  Serial.println("RFID done");
  

  
//---
  Serial.println("Setup done!");
  Serial.println("");
  


  lcdSafe.print("Please turn");
  lcdSafe.setCursor(0,1);
  lcdSafe.print("ON the power!");
  lcdBeer.print("Don't mess !");
  lcdBeer.setCursor(0,1);
  lcdBeer.print("with my stuff!");
  delay(1500);
  
  Serial.println("START in 2 sec.");
//---
}

void loop() {
  

while(game){

  int timerStatus = checkTimerStatus();
  if (timerStatus==0){ //0=game over
    Serial.print("TimerStatus: ");Serial.println(timerStatus);
	  gameOver();
  }

    if (timerStatus==3){ //3=no communication
    Serial.println("No communication");

  }

  if (timerStatus==2){ //2=setup STATUS, 1=game running
    Serial.println("SETUP MODE");
    rfid.begin(2,sckPin,mosiPin,misoPins[0],sdaPin,RST_PIN);
    delay(2);
    rfid.init();
    delay(2);
    uchar status;
    uchar str[MAX_LEN];
    status = rfid.request(PICC_REQIDL, str);
    status = rfid.anticoll(str);
    if (status == MI_OK)                  //read ID success
    {
      memcpy(serNum, str, 5);
      String decod = decodeID(serNum);
      decod.toUpperCase();
      Serial.print("CardID: ");Serial.print(decod);
      Serial.println();
      rfid.halt();

      if (decod.equals(startTag)){
        Serial.println("CardID (BATMAN) correct! SETUP MODE ON");
        digitalWrite(safe, LOW);
        delay(2000);
        digitalWrite(safe, HIGH);
      }
    }
  }
  
  int timer = millis();
    

  for (uint8_t i = 0; i < NR_OF_READERS; i++) {
    change[i] = false;
    passwordIn[i]="Nan"; //reset passwordIn
    rfid.begin(2,sckPin,mosiPin,misoPins[i],sdaPin,RST_PIN);
    Serial.print("RFID nr: ");Serial.print(i);Serial.print(" ");
    delay(2);
    rfid.init();
    delay(2);
    uchar status;
    uchar str[MAX_LEN];
    

    // Search card
    status = rfid.request(PICC_REQIDL, str);
    
    if (status != MI_OK)            //NO CARD
    {
      Serial.print("########");
      Serial.println();
      cardStatus[i] = false;                                //if card not present = false
      passwordIn[i]="Nan";
      
    }
    
    else{                         //Card present
      Serial.print(", Card present on reader ");Serial.print(i);Serial.print(": ");
      cardStatus[i] = true;
    }

    status = rfid.anticoll(str);
    
    if (status == MI_OK)                  //read ID success
    {
      memcpy(serNum, str, 5);
      String decod = decodeID(serNum);
      decod.toUpperCase();
      tempPass[i]=decod;
      Serial.print(tempPass[i]);
      Serial.println();
    }

    else if (status != MI_OK && cardStatus[i] == true)                      //cannot read ID
    {             
      tempPass[i]= "Nan";                       
      Serial.print(" ** Nan");
      Serial.println(" ** ");
    }
    //delay(10);
    rfid.halt(); //command the card into sleep mode 




    if(oldCardStatus[i]!=cardStatus[i] && changeStat[i]==0){
      Serial.println("*CHANGE*");
      oldCardStatus[i]= cardStatus[i];  //Update old card status
      changeStat[i]=1;
    }
    
    else if(oldCardStatus[i]!=cardStatus[i] && changeStat[i]==1){
      Serial.print("*CONFIRMING CHANGE (neg)*");Serial.println(" *!CHANGE NOT CONFIRMED!* ");
      oldCardStatus[i]= cardStatus[i];
      changeStat[i] = 0;
    }
  
    else if(oldCardStatus[i]==cardStatus[i] && changeStat[i]==1){
      Serial.println("*CONFIRMING CHANGE (pos)*");
      oldCardStatus[i]= cardStatus[i];
      changeStat[i] = 2;
    }
    
    else if(oldCardStatus[i]==cardStatus[i] && changeStat[i] == 2){
      Serial.println("*CHANGE CONFIRMED*");
      oldCardStatus[i]= cardStatus[i];
      changeStat[i]=0;
      change[i]=true;
    }
    
    else if(oldCardStatus[i]!=cardStatus[i] && changeStat[i] == 2) {
      Serial.println("*CHANGE NOT CONFIRMED*");
      oldCardStatus[i]= cardStatus[i];
      changeStat[i] =0;
    }
  }
  
  //Read ID on present card
  for (uint8_t i = 0; i < NR_OF_READERS; i++) { 
    if (change[i]){
      if(cardStatus[i]){                                                              //if there is a card present...
        Serial.print("Card status ");Serial.print(i);Serial.print(": ");Serial.println(cardStatus[i]);
        if(tempPass[i].length()>1){                                         //...try to read and...
          passwordIn[i]=tempPass[i]; // ...check ID and register it in array

          Serial.print("Reg. input on reader: ");Serial.print(i);Serial.print(": ");Serial.print(passwordIn[i]);Serial.print(", ");
          
          //check if same card is used
          if (!oldPassword[i].equals(passwordIn[i]) || change[i]){ //new card or change

            oldPassword[i]=passwordIn[i];
          
            //check password for one card**************************************************
            
            //Correct password for one card
            if(passwordIn[i].equals(password[i])){
              passStatus[i]=true;
              lcdBeer.clear();
              
              if(counter<5){
                lcdBeer.print("Correct!");
                lcdBeer.setCursor(0,1);
                lcdBeer.print("Please proceed...");
                tone(buzzerPin, 650);
                delay(1000);
                noTone(buzzerPin);
              }

              Serial.println();
              Serial.print("***************Tag on reader: ");Serial.print(i);Serial.print(" correct!");Serial.print(" passStatus (check one card): ");Serial.println(passStatus[i]);
              
            }
            
            //WRONG PASSWORD, LOSE 5 minutes, sound and information
            else{         
            Serial.println();
            Serial.print("Tag on reader: ");Serial.print(i);Serial.println(" WRONG!");  
            passStatus[i]=false;
            lcdBeer.clear();
            lcdBeer.print("WRONG, You lost");
            lcdBeer.setCursor(0,1);
            lcdBeer.print("five minutes!");
            tone(buzzerPin, 50);
            delay(3000);
            noTone(buzzerPin);
            lcdBeer.clear();
            lcdBeer.setCursor(0,1);
            lcdBeer.print("...Proceed...");
            sendData();
            }
  
          }
          else{   //old card, nothing
  
          }
        }
        else{                                                                                //...unable to read
          Serial.println("UNABLE TO READ CARD!");
          cardStatus[i]=false;
          oldCardStatus[i]=false;
        }
      }
      else if(!cardStatus[i]){ // No card
        passwordIn[i]="Nan";
        passStatus[i]=false;
      }
    }
  }
      
  counter = 0;

  //check whole password**************************************************
  for (uint8_t i = 0; i < NR_OF_READERS; i++) { 
    if (passStatus[i]){
      counter++;
    }
    if(counter == 6){ //if password correct
      Serial.println("*****************  YOU WON!  ******************************");

      sendWonData(); //sending won data to dartboard
      
      lcdBeer.clear();
      lcdBeer.print("Thank You!");
      lcdBeer.setCursor(0,1);
      lcdBeer.print("Power is now ON");
      
      lcdSafe.clear();
      lcdSafe.setCursor(0,0);
      lcdSafe.print("POWER IS ON!");
      lcdSafe.setCursor(0,1);
      lcdSafe.print("Enter the code:");
      game = false;
      tone(buzzerPin, 300);
      delay(500);
      noTone(buzzerPin);
      tone(buzzerPin, 400);
      delay(500);
      noTone(buzzerPin);
      tone(buzzerPin, 500);
      delay(500);
      noTone(buzzerPin);

      delay(1500);
      digitalWrite(lockPower, HIGH); //Keypad power ON
      gameEnds = true;
    }
  }
  Serial.print("Counter (whole pass): ");Serial.println(counter);
  Serial.println();
 }

  while (gameEnds)
  {
    if(digitalRead(lock)==1){
      lockStatus = true;
    }
  
    if (lockStatus){
      digitalWrite(safe, LOW);
      delay(2000);
      digitalWrite(safe, HIGH);
      Serial.println("Drink and have fun!");
      lockStatus = false;
      gameEnds = false;
    }
  }
 
}

void sendData(){
  Wire.beginTransmission(13);
  Wire.write('H');
  Wire.endTransmission(13);
  Serial.println("Data send!");
}

void sendWonData(){
  Wire.beginTransmission(13);
  Wire.write('W');
  Wire.endTransmission(13);
  Serial.println("Data send!");
}

String decodeID(uchar *serNum){
  String id = "";
  int IDlen=4;
  for(int i=0; i<IDlen; i++){
    id += String(0x0F & (serNum[i]>>4), HEX);
    id += String(0x0F & serNum[i],HEX);
  }
  return id;
}

void gameOver(){
  while(true){
    lcdBeer.clear();
    lcdBeer.setCursor(0,0);
    lcdBeer.print("GAME OVER!");
  
    lcdSafe.clear();
    lcdSafe.setCursor(0,0);
    lcdSafe.print("GAME OVER!");
  }
}

int checkTimerStatus(){
  int c;
  Wire.requestFrom(13, 1);
  while(Wire.available())
  {
    c = Wire.read();
    Serial.print("C_value: ");Serial.println(c);
  }
  if (c > 2 or c<0){
    Serial.println("Communication problems during checking timer status");
    c = 3; //3 = no communication
  }
  return c;
}
