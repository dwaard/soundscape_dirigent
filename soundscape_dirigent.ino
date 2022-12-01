// PRINT macro definitie
// Als deze macro is gedefinieerd zal de Arduino allerlei debug info naar de 
// seriele poort sturen.
#define PRINT
#ifdef PRINT
  #define SERIAL_BAUD_RATE 115200 // Baud rate voor de USB seriele poort
#endif

// Pin configuratie:
#define PIN_OFFSET 2 // Eerste bruikbare pin
#define MAX_PIN 13   // Laatste bruikbare pin
// Dus bij een PIN_OFFSET van 2: 
//   instrument  1 -> pin  2; 
//   instrument  2 -> pin  3; 
//   ...
//   instrument 12 -> pin 13;
// Maximaal zijn er dus 12 instrumenten beschikbaar

// MIDI settings
#define MIDI_BAUD_RATE 31250 // Standaard baud rate voor MIDI devices
#define MIDI_CHANNEL 9 // Het afgesproken MIDI channel om MIDI commands te ontvangen (0..15)

// MIDI commands
#define NOTE_OFF              0b10000000 // 128
#define NOTE_ON               0b10010000 // 144 Het afgesproken command om een instrument een puls te geven
#define AFTERTOUCH            0b10100000 // 160
#define CONTINUOUS_CONTROLLER 0b10110000 // 176
#define PATCH_CHANGE          0b11000000 // 192
#define CHANNEL_PRESSURE      0b11010000 // 208
#define PITCH_BEND            0b11100000 // 224
#define NON_MUSICAL           0b11110000 // 240

#define BASE_NOTE 36 // Noot om instrument 1 aan te sturen. Dus bv:
// instrument   1 : 60
// instrument   2 : 61
// ...
// instrument  12 : 71

// Timing settings
#define LOOP_TIME 1000 // vaste tijd in microsec. voor één loop iteratie
#define PULSE_TIME 25 // Aantal loop iteraties voor een puls 
#define MAX_DELAY_MICROSECONDS 16383 // om delayMicroseconds te kunnen gebruiken

typedef struct MIDI_Command {
  byte channel;
  byte type;
  byte params[2];
};

// Globale variabelen
unsigned int pulses[MAX_PIN]; // Item 0 en 1 zullen nooit worden gebruikt

/*
 * De `setup()` functie wordt één keer uitgevoerd na opstarten of een reset
 */
void setup() {
  #ifdef PRINT
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("Starting the conductor in debug mode");
  #endif
  Serial1.begin(MIDI_BAUD_RATE);
  // Initialiseer de pins
  pinMode(LED_BUILTIN, OUTPUT);
  for (int n = PIN_OFFSET; n < MAX_PIN; n++) {
    pinMode(n + PIN_OFFSET, OUTPUT);
  }
  for (int n = PIN_OFFSET; n < MAX_PIN; n++) {
    pulses[n] = 0;
  }
} // Einde van de `setup()` functie

/*
 * De `loop()` functie wordt iedere keer herhaald totdat de Arduino wordt 
 * uitgezet of gereset.
 */
void loop() {
  // Meet de process tijd
  unsigned long start = micros();
  // Ontvang en verwerk MIDI commands
  if (Serial1.available()) {
    MIDI_Command cmd;

    byte recieved = Serial1.read();
    if (recieved > 0b10000000) {
      cmd.type = recieved & 0b11110000;
      cmd.channel = recieved & 0b00001111;
      cmd.params[0] = Serial1.read();
      if (cmd.type != PATCH_CHANGE && cmd.type != CHANNEL_PRESSURE ) {
        cmd.params[1] = Serial1.read();
      }      
      #ifdef PRINT
        if (cmd.type != NON_MUSICAL) { // Filter dit command (komt vaak voor)
          Serial.print("Channel ");
          Serial.print(cmd.channel);
          Serial.print("; Type ");
          Serial.print(cmd.type);
          Serial.print("; Note ");
          Serial.println(cmd.params[0]);
        }
      #endif
      if (cmd.channel == (MIDI_CHANNEL) && cmd.type == NOTE_ON) {
        int index = PIN_OFFSET + cmd.params[0] - BASE_NOTE;
        #ifdef PRINT
        Serial.print("PULSE: ");
        Serial.println(index);
        #endif
        if (index < MAX_PIN) {
          pulses[index] = PULSE_TIME;
        }
      }
    }
  }

  // De outputs aansturen
  for (int n = PIN_OFFSET; n < MAX_PIN; n++) {
    if (pulses[n] > 0) {
      digitalWrite(n, HIGH);
      pulses[n]--;
    } else {
      digitalWrite(n, LOW);
    }
  }

  // Delay voor de volgende loop
  unsigned long processTime = micros() - start;
  if (processTime < LOOP_TIME) {
    unsigned long delayTime = LOOP_TIME - processTime;
    if (delayTime <= MAX_DELAY_MICROSECONDS) {
      delayMicroseconds(delayTime);
    } else {
      delay(delayTime / 1000);
    }
  }
  #ifdef PRINT
  else {
    Serial.print("OVER TIME LIMIT: ");
    Serial.println(processTime);
  }    
  #endif
}// Einde van de `loop()` functie
