// PRINT macro definitie
// Als deze macro is gedefinieerd zal de Arduino allerlei debug info naar de 
// seriele poort sturen.
#define PRINT
#ifdef PRINT
  #define SERIAL_BAUD_RATE 115200 // Baud rate voor de USB seriele poort
#endif

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

typedef struct MIDI_Command {
  byte channel;
  byte type;
  byte params[2];
};

// Timing settings
#define LOOP_TIME 1000 // vaste tijd in microsec. voor één loop iteratie
#define PULSE_TIME 50 // Aantal loop iteraties voor een puls 
#define MAX_DELAY_MICROSECONDS 16383 // om delayMicroseconds te kunnen gebruiken

// Pin configuratie:
#define MIN_PIN 2  // Eerste bruikbare pin
#define MAX_PIN 13 // Laatste bruikbare pin

// Macro dat bepaalt of een ontvangen byte een MIDI command is
#define isMidiCommand(recieved) (recieved > 127)
// Macro dat bepaalt of een MIDI command een 2e parameter heeft
#define hasSecondParam(type) (type != PATCH_CHANGE && type != CHANNEL_PRESSURE)
// Macro dat bepaalt welke command(s) en/of channel(s) voor een trigger zorgen 
#define isOutputTrigger(cmd) (cmd.channel == MIDI_CHANNEL && (cmd.type == NOTE_ON || cmd.type == NOTE_OFF))

// De mapping van notes naar pins. De array is 128 elementen, corresponderedend met
// het aantal mogelijke noten. De inhoud van elk element is een pin nummer. Als de noot
// Niet wordt gebruikt bevat deze de waarde 0.
// In de `setup()` functie moet de mapping worden ingesteld.
byte notes[128] = {0}; 

// De gewenste toestand van de output pins. De index van de array komt overeen met 
// een output pin nummer en de waarde een getal >=0. Als het getal >0 wordt die pin
// met een `HIGH`
unsigned int pulses[MAX_PIN] = {0}; // Item 0 en 1 zullen nooit worden gebruikt

/*
 * De `setup()` functie wordt één keer uitgevoerd na opstarten of een reset
 */
void setup() {
  #ifdef PRINT
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("Starting the conductor in debug mode");
  #endif
  Serial1.begin(MIDI_BAUD_RATE);
  // Koppel de gebruikte noten aan de output pins mbv de notes array  
  // syntax: notes[note] = pin;
  notes[36] = 2;
  notes[37] = 3;
  notes[38] = 4;
  notes[42] = 5;
  notes[44] = 6;
  notes[69] = 7;
  // ...
  // Initialiseer alle beschikbare pins maar als output pins
  for (int n = MIN_PIN; n <= MAX_PIN; n++) {
    pinMode(n, OUTPUT);
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
    // Lees alle databytes totdat een MIDI command is ontvangen
    byte recieved = Serial1.read();
    if (isMidiCommand(recieved)) {
      cmd.type = recieved & 0b11110000;
      cmd.channel = recieved & 0b00001111;
      cmd.params[0] = Serial1.read();
      if (hasSecondParam(cmd.type)) {
        cmd.params[1] = Serial1.read();
      }
      // Als er een afgesproken command op het afgesproken channel is ontvangen...
      if (isOutputTrigger(cmd)) {
        // Lees de aan te sturen pin nummer uit de notes array
        int index = notes[cmd.params[0]];
        // Zet die pin dan aan door de notes array te vullen
        if (index >= MIN_PIN) {
          pulses[index] = PULSE_TIME;
        } 
        #ifdef PRINT
        else {
          Serial.print("Channel ");
          Serial.print(cmd.channel);
          Serial.print("; Type ");
          Serial.print(cmd.type);
          Serial.print("; Note ");
          Serial.println(cmd.params[0]);
        }
        #endif
      } 
    }
  }

  // De outputs aansturen dmv de notes array
  for (int n = MIN_PIN; n < MAX_PIN; n++) {
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
