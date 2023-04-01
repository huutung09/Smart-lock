#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>

/* Keypad configuration */
const byte ROWS = 4; // Bốn hàng
const byte COLS = 3; // Ba cột
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {7, 6, 5};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/* RFID configuration */
#define RST_PIN         9
#define SS_PIN          10
MFRC522 mfrc522(SS_PIN, RST_PIN);

/* Servo */
Servo myservo;
byte servoPin = 3;

/* LED */
byte redPin = 4;
byte greenPin = 8;
byte bluePin = 2;

/* LCD configuration */
LiquidCrystal_I2C lcd(0x27, 16, 2);  // default 0x27

/* Configure default password */
char pass[4] = {'1', '1', '1', '1'};
char str[4] = {' ', ' ', ' ', ' '};

byte secret[7] = {0x04, 0x15, 0x91, 0xCA, 0xCE, 0x06, 0x28};
byte code[2][7] = {
  {0xC1, 0x0C, 0x6C, 0x24, 0, 0, 0},
  {0xF0, 0xD1, 0xD9, 0x19, 0, 0, 0}
};

/* Configure status varialbe */
byte i, j, count = 0, warning = 0, pos = 0,   checked ;
volatile byte Status = 0, check = 0, countcard = 0, nochange = 1, done = 1, flag = 1, the1 = 0, the2 = 0, master = 0;

void setup() {

  /* LCD initialization */
  Serial.begin(115200);
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("-WELLCOME-");

  /* Configure led pin */
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  digitalWrite(greenPin, HIGH);

  /* Configure servo */
  myservo.attach(3);
  myservo.write(0);

  /* RFID initialization */
  SPI.begin();
  mfrc522.PCD_Init();

  /* Load Password */
  loadPassword();
}

void loop() {
  while (Serial.available())
  {
    if (Serial.read() == '1')
      opendoor();
    else if( Serial.read() == '0')
      closedoor();
  }
  password();
  scancard();
}

/* ======================= open door with RFID ==========================*/
void scancard() 
{
  int match = 0;
  int change = 0;

  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

  countcard ++;
  
  /* read UID card */
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] != code[0][i] && mfrc522.uid.uidByte[i] != code[1][i])
      match ++;
    if (mfrc522.uid.uidByte[i] != secret[i])
      change++;
  }

  if (change != 0 && done == 1) 
  {
    /* UID matches */
    if (match == 0 && Status == 0 ) 
    {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Chinh xac!");
      delay(100);
      opendoor();
    }
    else if (match == 0 && Status == 1) 
    {
      closedoor();
      delay(100);
    }
    else 
    {
      warning++;
      lcd.clear();
      lcd.print("Khong chinh xac!");
      delay(100);
      if (warning == 5) {
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Hay cho");
        lcd.setCursor(2, 1);
        lcd.print("roi thu lai...");
        warning = 0;
        Serial.print(2);
        delay(10000);
      }

      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Thu lai!");
      delay(1000);
      closedoor();
    }
    countcard = 0;
  }
  /* Add/remove card */
  if (change == 0) 
  {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Them/xoa the");
    check = 1;
    done = 0;
  }

  if (check == 1 && countcard == 2) 
  {
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
      if (mfrc522.uid.uidByte[i] != code[0][i] )
        the1 ++;
      if (mfrc522.uid.uidByte[i] != code[1][i] )
        the2 ++;
      if (mfrc522.uid.uidByte[i] != secret[i] )   
        master ++;
    }

    if (the1 != 0 && the2 != 0 && master != 0) 
    {
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
        code[flag][i] = mfrc522.uid.uidByte[i];
      }
      flag++;
      if (flag > 1)  flag = 0;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Da them the");
      delay(1000);
    }

    if (the1 == 0 ) 
    {
      flag = 0;
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
        code[flag][i] = 0;
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Da xoa the ");
      delay(1000);
    }

    if ( the2 == 0 ) 
    {
      flag = 1;
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
        code[flag][i] = 0;
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Da xoa the");
      delay(1000);
    }
    countcard = 0;
    check = 0;
    done = 1;
    the1 = 0;
    the2 = 0;
    master = 0;
  }



  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

/* ===================== open door with password ======================== */
void password() {
  char key = keypad.getKey();
  if (key)
  {  
    if (nochange == 1)
    {
      lcd.setCursor(1, 0);
      lcd.print("Nhap mat khau");
    } 
    if (i == 0) {
      str[0] = key;
      lcd.setCursor(6, 1);
      lcd.print(str[0]);
      delay(200); // show key input
      lcd.setCursor(6, 1);
      lcd.print("*"); // hide key input
    }
    if (i == 1) {
      str[1] = key;
      lcd.setCursor(7, 1);
      lcd.print(str[1]);
      delay(200);
      lcd.setCursor(7, 1);
      lcd.print("*");
    }
    if (i == 2) {
      str[2] = key;
      lcd.setCursor(8, 1);
      lcd.print(str[2]);
      delay(200);
      lcd.setCursor(8, 1);
      lcd.print("*");
    }
    if (i == 3) {
      str[3] = key;
      lcd.setCursor(9, 1);
      lcd.print(str[3]);
      delay(200);
      lcd.setCursor(9, 1);
      lcd.print("*");
      count = 1;
    }
    i++;
  }

  if (count == 1  &&  nochange == 1)
  {
    /* Correct password */
    if (str[0] == pass[0] && str[1] == pass[1] && str[2] == pass[2] &&
        str[3] == pass[3] && Status == 0)
    {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Chinh xac!");
      delay(2000);
      opendoor();      
    }
    /* incorrect password */
    else
    {
      warning++;
      lcd.clear();
      lcd.print("Khong chinh xac!");
      delay(1000);
      // wrong 5 times
      if (warning == 5)
      {
        Serial.print(2);
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Hay cho ");
        lcd.setCursor(2, 1);
        lcd.print("roi thu lai...");
        delay(10000);
        warning = 0;
      }
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Thu lai!");
      delay(1000);
      closedoor();
    }
  }

  /* Press '#' to close door, '*' to change password */
  switch (key)
  {
    case '#':
      closedoor();
      delay(1000);
      break;
    case '*':
      nochange = 0;
      count = 0;
      i = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mat khau cu");
      break;
  }
  
  /* Confirm old password */
  if (str[0] == pass[0] && str[1] == pass[1] && str[2] == pass[2] && str[3] == pass[3] && count == 1)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mat khau moi");
    i = 0;
    count = 0;
    checked = 1; //if true then enable checked flag
  }
  else if (checked == 0 && count == 1 )
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sai mat khau");
    delay(2000);
    i = 0;
    count = 0;
    nochange = 1;
  }
  
  if (count == 1 && nochange == 0 && checked == 1) 
  {
    for (j = 0; j < 4; j++)
      pass[j] = str[j];

    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Doi mat khau");
    lcd.setCursor(3, 1);
    lcd.print("Thanh cong");
    savePassword();
    delay(2000);
  }
}

void loadPassword()
{
  flash();
  if (EEPROM.read(0) == 1)
  {
    for (int j = 0; j < 4; j++)
    {
      pass[j] = EEPROM.read(j + 1);
    }
  }
  flash();
}

void savePassword()
{
  /* restore the flags and variables */
  nochange = 1;
  checked = 0;
  i = 0;
  count = 0;
  
  flash();
  for (int j = 0; j < 4; j++)
  {
    EEPROM.write(j + 1, pass[j]);
  }
  EEPROM.write(0, 1);
  flash();
  flash();
}

void flash()
{
  digitalWrite(redPin, HIGH);
  digitalWrite(greenPin, HIGH);
  digitalWrite(bluePin, HIGH);
  delay(500);
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(bluePin, LOW);
}

void closedoor() {
  /* restore the flags and variables */
  i = 0;
  count = 0;
  Status = 0;
  nochange = 1;
  
  digitalWrite(redPin, HIGH);
  digitalWrite(greenPin, LOW);
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Dong cua!");
  myservo.write(5);
  
  delay(500);
  lcd.setCursor(3, 0);
  lcd.print("-WELLCOME-");
  
  Serial.print(0);
}

void opendoor() 
{
  /* restore the flags and variables */
  i = 0;
  count = 0;
  Status = 1;
  nochange = 1;
  warning = 0;
  
  digitalWrite(greenPin, HIGH);
  digitalWrite(redPin, LOW);
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Mo cua!");
  myservo.write(15);
  
  Serial.print(1);
}
