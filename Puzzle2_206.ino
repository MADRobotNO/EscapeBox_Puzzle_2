 // Puzzle 2
//By MAD Robot, Martin Agnar Dahl
//28.07.2018
//LCD: TM1637
//RFID: Start/Pause function
//ver 1.0, Timer + LCD
//ver 1.1, RFID
//ver 1.2, Buzzer + light relay
//ver 1.2a, Mellody added + relay corrections.
//ver 1.2b, 2 relays (pin 30 green-NC, pin 32 red-NO);
//ver 1.2c, debug
//ver 1.3, new library for RFID
//ver 1.3a, magnetic sensor for doors added
//ver 1.4, New RFID
//ver 1.5, I2C Communication with other arduino, Slave (add.13)
//ver 1.6, No light relay
//ver 1.7, Game over sound
//ver 1.8, 2-ways communication
//ver 1.9, Final

#include <Wire.h>
#include <Wiegand.h>
#include <TM1637.h>

//SD
#include <SPI.h> //SD Card uses SPI
#include <SD.h>  //SD Card

#include <SPI.h>

#define CLK 4
#define DIO 5

#define DEBUG

//sd
int chipSelect = 53;
File dataFile;

//Rounds counter
int counter1 = 0; //starts
int counter2 = 0; //wins
int counter3 = 0; //loses
    
//Magnetsensor
boolean magSensor = false;

//Timer variables
int sec = 1000;
long oldTimeS = 0;
int secCounter = 00;
int minCounter = 60;
long nowTime;
char secChar[2];

//LCD
int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
TM1637 lcd(CLK,DIO);

//Game variables
boolean gameStart = false;
boolean gamePause = false;
String tagID = "";
boolean firstStart = true;
int gamePausedBy = 0; //card = 1, door = 2

int gameStatus = 2; //2=setup, 1=game running, 0=game over

//RFID
WIEGAND wg; //D0 - PIN2, D1 - PIN3

const char* startTag = "CABBFDFA";  //Batman
const char* pauseTag = "73CA2BF3";  //Red man
const char* readTag = "638ECCA3";
const char* resetTag = "83A57EC6";

String toLogg = ""; // to logg variable
  
//Buzzer
const int buzzerPin = 10;

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Setup starts...");
  Serial.println();
  #endif
  
  //LCD
  lcd.init();
  lcd.set(BRIGHT_TYPICAL);
  //lcd.point(POINT_OFF);
  Serial.println("LCD done");

  //SPI
  SPI.begin();
  Serial.println("SPI done");
    
  //RFID
  //rfid.begin();
  wg.begin();
  Serial.println("RFID done");
  
  //Timer
  long timer = millis();

  //SD card and file
  Serial.println("Initializing SD card... ");
  if (!SD.begin(chipSelect)) {
    Serial.println("*****ERROR**********Card failed, or not present*****ERROR**********");
    Serial.println(' ');  
    delay(2000);
  }
  else{
    Serial.println("SD Card initialized successfully!");  
    Serial.println(' ');
  }

  //check if file exists
  if (SD.exists("data.txt")) {
    Serial.println("File data.txt exists....");
  } else {
    dataFile = SD.open("data.txt", FILE_WRITE);
    Serial.println("File data.txt is now created...");
    dataFile.close();
    String startLogg = "0,0,0";
    logg(startLogg);
  }
  Serial.println("SD card and file are ready!");
  Serial.println("*************************************");
  
  String firstRead = readFile(); 
  readValues(firstRead);

  Serial.println("Counters are set up to:");
  Serial.println(counter1);
  Serial.println(counter2);
  Serial.println(counter3);
  Serial.println("*************************************");
  
  //I2C com
  Wire.begin(13);
  Wire.onReceive(recData);
  Wire.onRequest(reqData);
  Serial.println("I2C done");

  pinMode (42, INPUT_PULLUP); //magSensor Pin
  
  pinMode (buzzerPin, OUTPUT);
  Serial.println("Setp done!");
  Serial.println("");
}

void loop() {
  
  toLogg = "";
  
  //Check door
  magSensor = digitalRead(42);
  
  nowTime = millis();

  //Read card
  if(wg.available()){
    delay(10);
    Serial.print("TagID: ");Serial.println(wg.getCode(), HEX); //debug - cardID
    tagID=String(wg.getCode(), HEX);     
    tagID.toUpperCase();  
  }


  //check Counters
  if (tagID.equals(readTag)){
	Serial.println("Check tag registered, showing counters on LCD");
    showCounters(counter1);
    delay(3000);
    showCounters(counter2);
    delay(3000);
    showCounters(counter3);
    delay(3000);
	tagID="";
  }

	//reset Counters
  if (tagID.equals(resetTag)){
	Serial.println("Reset tag registered, Reseting all counters to 0");
	toLogg = "0,0,0";
	logg(toLogg );
    	tagID="";
  }


  
  //Game starts
  if (tagID.equals(startTag) && !gameStart && !gamePause){
    
    Serial.print("TagID: ");
    Serial.println(tagID);
    Serial.println("Game Starts");
    counter1++;
    toLogg += counter1 + counter2 + counter3;
    logg(toLogg); 
    Serial.print("Game Starts counter is now: ");Serial.println(counter1);
    
    if (firstStart){
      mellody();
      firstStart = false;
    }
    gameStatus = 1;
    gameStart = true;
    gamePause = false;
    tagID="";

  }

  //Game pauses
  if (tagID.equals(pauseTag) && gameStart || magSensor && gameStart){
    if (magSensor) gamePausedBy = 2;
    else if (tagID == pauseTag) gamePausedBy = 1;
    Serial.print("TagID: ");
    Serial.println(tagID);
    Serial.println("game pauses");
    gameStart = false;
    gamePause = true;
    tagID="";    

  }

  //Game resumes
  if (tagID.equals(startTag) && gamePause && gamePausedBy == 1 || !magSensor && gamePause && gamePausedBy == 2){
    gamePausedBy = 0;
    Serial.print("TagID: ");
    Serial.println(tagID);
    Serial.println("Game resumes");
    gameStart = true;
    gamePause = false;
    tagID="";    

  }

  if (gameStart){
    timer();
  }

  if (gamePause){
    
  }
  
}

void timer(){  
  
  if (secCounter < 10){
    TimeDisp[3] = secCounter;
    TimeDisp[2] = 0;
  }
  
  else{
    char cstrS[2];
    itoa(secCounter, cstrS, 10);
    TimeDisp[2] = cstrS[0] - '0';
    TimeDisp[3] = cstrS[1] - '0';
  }
  
  if (minCounter < 10){
    TimeDisp[1] = minCounter;
    TimeDisp[0] = 0;
  }
  else{
    char cstrM[2];
    itoa(minCounter, cstrM, 10);
    TimeDisp[0] = cstrM[0] - '0';
    TimeDisp[1] = cstrM[1] - '0';
  }

  if ((nowTime - oldTimeS) >= sec){
    secCounter--;
    Serial.print(minCounter);
    Serial.print(":");
    Serial.print(TimeDisp[2]);
    Serial.println(TimeDisp[3]);
    oldTimeS=nowTime;
    updateTimer();
  }
  
  if (secCounter < 0){
    minCounter--;
    secCounter = 59;
  }
  
  if (minCounter <0){                       // GAME OVER
    counter3++;
    toLogg += counter1 + counter2 + counter3;
    logg(toLogg); 
    minCounter=0;
    secCounter=0;    
    TimeDisp[1] = 0;
    TimeDisp[2] = 0;
    TimeDisp[3] = 0;
    TimeDisp[0] = 0;
	  gameStatus = 0;
    updateTimer();       
    gameOver();
  }

  //Warnings
  if (minCounter == 30 && secCounter == 00){
    tone(buzzerPin, 700);
    delay(500);
    noTone(buzzerPin);
  }

  else if (minCounter == 15 && secCounter == 00){
    tone(buzzerPin, 700);
    delay(500);
    noTone(buzzerPin);
  }
  
  else if (minCounter == 10 && secCounter == 00){
    tone(buzzerPin, 700);
    delay(500);
    noTone(buzzerPin);
  }

  else if (minCounter == 5 && secCounter == 00){
    tone(buzzerPin, 700);
    delay(500);
    noTone(buzzerPin);
  }

  else if (minCounter == 0 && secCounter <= 3){
    tone(buzzerPin, 500);
    delay(500);
    noTone(buzzerPin);
  }
}

void updateTimer(){
  lcd.point(POINT_OFF);
  lcd.display(0, TimeDisp[0]);
  lcd.point(POINT_ON);
  lcd.display(1, TimeDisp[1]);
  lcd.point(POINT_OFF);
  lcd.display(2, TimeDisp[2]);
  lcd.display(3, TimeDisp[3]);
}

void mellody(){
  const int c = 261;
  const int d = 294;
  const int e = 329;
  const int f = 349;
  const int g = 391;
  const int gS = 415;
  const int a = 440;
  const int aS = 455;
  const int b = 466;
  const int cH = 523;
  const int cSH = 554;
  const int dH = 587;
  const int dSH = 622;
  const int eH = 659;
  const int fH = 698;
  const int fSH = 740;
  const int gH = 784;
  const int gSH = 830;
  const int aH = 880;

  beep(a, 500);
  beep(a, 500);    
  beep(a, 500);
  beep(f, 350);
  beep(cH, 150);  
  beep(a, 500);
  beep(f, 350);
  beep(cH, 150);
  beep(a, 650);
 
  delay(300);
 
  beep(eH, 500);
  beep(eH, 500);
  beep(eH, 500);  
  beep(fH, 350);
  beep(cH, 150);
  beep(gS, 500);
  beep(f, 350);
  beep(cH, 150);
  beep(a, 650);
 
  delay(50);
}

void beep(int note, int duration)
{
  //Play tone on buzzerPin
  tone(buzzerPin, note);
  delay(duration);
  noTone(buzzerPin);
 
}

void recData(int howManyBytes){
  while(Wire.available()){
    char c = Wire.read();
    
    Serial.println("Incomming transmission!");
    Serial.print("Data: ");
    Serial.println(c);

    if (c== 'H'){
      minCounter=minCounter-5;    
    }

    if (c== 'W'){
      counter2++;
      toLogg += counter1 + counter2 + counter3;
      logg(toLogg); 
      while(true){
        //infinite loop  
      }
    }
  }
}


void gameOver(){
  
int NOTE_H =   0;
int NOTE_B0 =  31;
int NOTE_C1 = 33;
int NOTE_CS1 =35;
int NOTE_D1  =37;
int NOTE_DS1 =39;
int NOTE_E1  =41;
int NOTE_F1  =44;
int NOTE_FS1 =46;
int NOTE_G1  =49;
int NOTE_GS1 =52;
int NOTE_A1  =55;
int NOTE_AS1 =58;
int NOTE_B1  =62;
int NOTE_C2  =65;
int NOTE_CS2 =69;
int NOTE_D2  =73;
int NOTE_DS2 =78;
int NOTE_E2  =82;
int NOTE_F2  =87;
int NOTE_FS2 =93;
int NOTE_G2  =98;
int NOTE_GS2 =104;
int NOTE_A2  =110;
int NOTE_AS2 =117;
int NOTE_B2  =123;
int NOTE_C3  =131;
int NOTE_CS3 =139;
int NOTE_D3  =147;
int NOTE_DS3 =156;
int NOTE_E3  =165;
int NOTE_F3  =175;
int NOTE_FS3 =185;
int NOTE_G3  =196;
int NOTE_GS3 =208;
int NOTE_A3  =220;
int NOTE_AS3 =233;
int NOTE_B3  =247;
int NOTE_C4  =262;
int NOTE_CS4 =277;
int NOTE_D4  =294;
int NOTE_DS4 =311;
int NOTE_E4  =330;
int NOTE_F4  =349;
int NOTE_FS4 =370;
int NOTE_G4  =392;
int NOTE_GS4 =415;
int NOTE_A4  =440;
int NOTE_AS4 =466;
int NOTE_B4  =494;
int NOTE_C5  =523;
int NOTE_CS5 =554;
int NOTE_D5  =587;
int NOTE_DS5 =622;
int NOTE_E5  =659;
int NOTE_F5  =698;
int NOTE_FS5 =740;
int NOTE_G5  =784;
int NOTE_GS5 =831;
int NOTE_A5  =880;
int NOTE_AS5 =932;
int NOTE_B5  =988;
int NOTE_C6  =1047;
int NOTE_CS6 =1109;
int NOTE_D6  =1175;
int NOTE_DS6 =1245;
int NOTE_E6  =1319;
int NOTE_F6  =1397;
int NOTE_FS6 =1480;
int NOTE_G6  =1568;
int NOTE_GS6 =1661;
int NOTE_A6  =1760;
int NOTE_AS6 =1865;
int NOTE_B6  =1976;
int NOTE_C7  =2093;
int NOTE_CS7 =2217;
int NOTE_D7  =2349;
int NOTE_DS7 =2489;
int NOTE_E7  =2637;
int NOTE_F7  =2794;
int NOTE_FS7 =2960;
int NOTE_G7  =3136;
int NOTE_GS7 =3322;
int NOTE_A7  =3520;
int NOTE_AS7 =3729;
int NOTE_B7  =3951;
int NOTE_C8  =4186;
int NOTE_CS8 =4435;
int NOTE_D8  =4699;
int NOTE_DS8 =4978;

const int gameover[] = {15,                                               // Array for Game over song
  NOTE_C4, 8, NOTE_H, 8, NOTE_H, 8, NOTE_G3, 8, NOTE_H, 4, NOTE_E3, 4, NOTE_A3, 6, NOTE_B3, 6, NOTE_A3, 6, NOTE_GS3, 6, NOTE_AS3, 6, 
  NOTE_GS3, 6, NOTE_G3, 8, NOTE_F3, 8, NOTE_G3, 4};

  for (int thisNote = 1; thisNote < (gameover[0] * 2 + 1); thisNote = thisNote + 2) { // Run through the notes one at a time
      tone(buzzerPin, gameover[thisNote], (1000/gameover[thisNote + 1]));// Play the single note
      delay((1000/gameover[thisNote + 1]) * 1.30);                        // Delay for the specified time
      noTone(buzzerPin);                                                 // Silence the note that was playing
    }
  while(true){
    gameStatus = 0; 
    gameStart = false;
  }
}

void reqData(){
  Wire.write(gameStatus);
  Serial.print("Data: ");Serial.print(gameStatus);Serial.println(" send!");
}


//***************************SD Logg Methods********************************//


void logg(String dataString){
  //String dataString = String(rounds);
  dataFile = SD.open("data.txt", O_WRITE | O_CREAT | O_TRUNC);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  }

  else {
    Serial.println("Error opening data file...");
  }
  
}

String readFile(){
  //Serial.println("Reading file");
  dataFile = SD.open("data.txt", FILE_READ);
  
  if (dataFile) {

    //check number of bytes and declare size of a buffer
    int bufSize = dataFile.available();
    char buf[bufSize-2];

    //read data from file to buffer array
    for (int i = 0; i<bufSize; i++){
      dataFile.read(buf, bufSize);
    }
    
    dataFile.close();

    //create an output String from buffer array
    String return_value = "";
    for (int i = 0; i<sizeof(buf); i++){
        return_value += String(buf[i]);
    }

    return return_value;
  } else {
    Serial.println("error opening test.txt");
  }
}

void readValues(String fileData){
  
  String val1 = "0";
  String val2 = "0";
  String val3 = "0";
  
  if(fileData.indexOf(",") > 0){
    val1 = fileData.substring(0, fileData.indexOf(","));
      fileData = fileData.substring(fileData.indexOf(",")+1);
      if(fileData.indexOf(",") > 0){
        val3 = fileData.substring(fileData.indexOf(",")+1);
        fileData = fileData.substring(0,fileData.indexOf(","));
        val2 = fileData;
      }
      else{
        val2 = fileData;
      }
  }
  else{
    val1 = fileData;
  }
  counter1 = val1.toInt();
  counter2 = val2.toInt();
  counter3 = val3.toInt();
}

void showCounters(int number){
  int8_t dispVal[] = {0x00,0x00,0x00,0x00};

  if (number <10){
    char cstrM[1];
    itoa(number, cstrM, 10);
    dispVal[0] = 0;
    dispVal[1] = 0;
    dispVal[2] = 0;
    dispVal[3] = cstrM[0] - '0';
  }
  
  else if (number>9){
    char cstrM[2];
    itoa(number, cstrM, 10);
    dispVal[0] = 0;
    dispVal[1] = 0;
    dispVal[2] = cstrM[0] - '0';
    dispVal[3] = cstrM[1] - '0';
  }

  else if (number>99){
    char cstrM[3];
    itoa(number, cstrM, 10);
    dispVal[0] = 0;
    dispVal[1] = cstrM[0] - '0';
    dispVal[2] = cstrM[1] - '0';
    dispVal[3] = cstrM[2] - '0';
  }

  else if (number>999){
    char cstrM[4];
    itoa(number, cstrM, 10);
    dispVal[0] = cstrM[0] - '0';
    dispVal[1] = cstrM[1] - '0';
    dispVal[2] = cstrM[2] - '0';
    dispVal[3] = cstrM[3] - '0';
  }
    
  lcd.point(POINT_OFF);

  lcd.display(0, dispVal[0]);
  lcd.display(1, dispVal[1]);
  lcd.display(2, dispVal[2]);
  lcd.display(3, dispVal[3]);
  
}
