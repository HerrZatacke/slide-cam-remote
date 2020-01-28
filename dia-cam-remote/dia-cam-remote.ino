#include <EEPROM.h>

// Arduino Pro / Pro Mini

#define LED 13
#define BUTTON 2
#define PROJECTOR 6
#define SHUTTER 5

#define SHUTTER_ON_TIME 50
#define PROJECTOR_ON_TIME 500

#define ADDRESS_DELAY_SHUTTER 0
#define ADDRESS_COUNT_SHUTTER 2
#define ADDRESS_DELAY_PROJECTOR 4
#define ADDRESS_REPEAT_SEQUENCE 6

unsigned int delayShutter;
unsigned int countShutter;
unsigned int delayProjector;
unsigned int repeatSequence;

bool buttonState;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(SHUTTER, OUTPUT);
  digitalWrite(SHUTTER, LOW);
  pinMode(PROJECTOR, OUTPUT);
  digitalWrite(PROJECTOR, LOW);

  pinMode(BUTTON, INPUT_PULLUP);
  buttonState = HIGH;

  Serial.begin(9600);

  stats();
}

void loop() {
  if (Serial.available()) {
    char ch = Serial.read();

    switch(ch) {
      case 'p':
        delayProjector = intFromSerial();
        writeIntEEPROM(ADDRESS_DELAY_PROJECTOR, max(PROJECTOR_ON_TIME, delayProjector));
        stats();
        break;
      case 's':
        delayShutter = intFromSerial();
        writeIntEEPROM(ADDRESS_DELAY_SHUTTER, max(SHUTTER_ON_TIME, delayShutter));
        stats();
        break;
      case 'c':
        countShutter = intFromSerial();
        writeIntEEPROM(ADDRESS_COUNT_SHUTTER, max(1, countShutter));
        stats();
        break;
      case 'r':
        repeatSequence = intFromSerial();
        writeIntEEPROM(ADDRESS_REPEAT_SEQUENCE, max(1, repeatSequence));
        stats();
        break;        
      case 'h':
      case '\n':
        clearSerial();
        stats();
        break;
    }
  }
  
  if (buttonChangeToOn()) {
    runSequence();
  }
}

void runSequence() {
  for (int sequence = 0; sequence < repeatSequence; sequence++) {
    Serial.print("\nSequence ");
    Serial.print(sequence + 1);
    Serial.print("/");
    Serial.println(repeatSequence);

    Serial.print("  Triggering Projector ");
    projector();
    if (buttonChangeToOn()) {
      Serial.println("\nCancelled.");
      buttonWaitToOff();
      return;
    }
    Serial.println(" done.");
    
    for(int i = 0; i < countShutter; i++) {
      Serial.print("  Triggering Shutter ");
      Serial.print(i + 1);
      Serial.print("/");
      Serial.print(countShutter);
      Serial.print(" ");
      shutter();
      if (buttonChangeToOn()) {
        Serial.println("\nCancelled.");
        buttonWaitToOff();
        return;
      }      
      Serial.println(" done.");
    }
  }
  Serial.print("\nSequence done.\n");
}

void projector() {
  Serial.print(".");
  digitalWrite(PROJECTOR, HIGH);
  delay(PROJECTOR_ON_TIME);
  Serial.print(".");
  digitalWrite(PROJECTOR, LOW);
  delay(delayProjector - PROJECTOR_ON_TIME);
  Serial.print(".");
}

void shutter() {
  Serial.print(".");
  digitalWrite(LED, HIGH);
  digitalWrite(SHUTTER, HIGH);
  delay(SHUTTER_ON_TIME);
  Serial.print(".");
  digitalWrite(LED, LOW);
  digitalWrite(SHUTTER, LOW);
  delay(delayShutter - SHUTTER_ON_TIME);
  Serial.print(".");
}

unsigned int readIntEEPROM(byte address) {
  return EEPROM.read(address) + (EEPROM.read(address + 1) << 8);
}

void writeIntEEPROM(byte address, unsigned int value) {
  EEPROM.write(address, value & 255);
  delay(5);
  EEPROM.write(address + 1, (value >> 8) & 255);  
  delay(5);
}

unsigned int intFromSerial() {
  String inString = "";
  
  // Loop until non digit received
  while (true) {
    if (Serial.available()) {
      int inChar = Serial.read();

      if (isDigit(inChar) && inString.length() < 5) {
        inString += (char)inChar;
      } else {
        clearSerial();
        return inString.toInt();
      }
    }

    if (inString.length() > 4) {
      return inString.toInt();
    }
  }
}

void stats() {
  loadValues();
  Serial.println("\n\n--- CURRENT SETTINGS ---");
  Serial.print("(p)rojector delay: ");
  Serial.println(delayProjector);
  Serial.print("(s)hutter delay: ");
  Serial.println(delayShutter);
  Serial.print("shutter (c)ount: ");
  Serial.println(countShutter);
  Serial.print("(r)epeat sequence: ");
  Serial.println(repeatSequence);
}

void loadValues() {
  delayShutter = max(SHUTTER_ON_TIME, readIntEEPROM(ADDRESS_DELAY_SHUTTER));
  delayProjector = max(PROJECTOR_ON_TIME, readIntEEPROM(ADDRESS_DELAY_PROJECTOR));
  countShutter = max(1, readIntEEPROM(ADDRESS_COUNT_SHUTTER));
  repeatSequence = min(80, max(1, readIntEEPROM(ADDRESS_REPEAT_SEQUENCE)));

  // Set values to default
  if (delayShutter == 65535) {
    delayShutter = 650;
    writeIntEEPROM(ADDRESS_DELAY_SHUTTER, delayShutter);
  }

  if (countShutter == 65535) {
    countShutter = 1;
    writeIntEEPROM(ADDRESS_COUNT_SHUTTER, countShutter);
  }

  if (delayProjector == 65535) {
    delayProjector = 1500;
    writeIntEEPROM(ADDRESS_DELAY_PROJECTOR, delayProjector);
  }

  if (repeatSequence == 65535) {
    repeatSequence = 80;
    writeIntEEPROM(ADDRESS_REPEAT_SEQUENCE, repeatSequence);
  }  
}

void clearSerial() {
  delay(10);
  while(Serial.available()) {
    Serial.read();
  }
}

bool buttonChangeToOn() {
  // LOW == pushed
  if (digitalRead(BUTTON) == LOW) {
    if (buttonState == HIGH) {
      delay(150); // simplest debounce
      return true;
    }
    buttonState = LOW;
  } else {
    buttonState = HIGH;
  }
  return false;
}

void buttonWaitToOff() {
  while (digitalRead(BUTTON) == LOW) {}
  // more debounce
  delay(50);
}
