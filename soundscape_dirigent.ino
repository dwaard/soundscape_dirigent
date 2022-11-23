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

#define MIDI_BAUD_RATE 31250 // Standaard baud rate voor MIDI devices
#define MIDI_CHANNEL 16 // Het afgesproken MIDI channel om MIDI commands te ontvangen
#define NOTE_ON 144 // Het afgesproken command om een instrument een puls te geven
#define BASE_NOTE 60 // Noot om instrument 1 aan te sturen. Dus:
// instrument   1 : 60
// instrument   2 : 61
// ...
// instrument  12 : 71


// Pulsduur in microseconde
#define LOOP_TIME 1000 // vaste tijd in microsec. voor één loop iteratie
#define PULSE_TIME 5 // Aantal loop iteraties 
#define MAX_DELAY_MICROSECONDS 16383 // om delayMicroseconds te kunnen gebruiken

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
  // Verwerk de MIDI commands
  if (Serial1.available() > 2) { // Als er minstens 3 bytes beschikbaar zijn
    byte command = Serial1.read();
    byte note = Serial1.read();
    byte velocity = Serial1.read();
    byte channel = command && 0b00001111;
    if (channel == (MIDI_CHANNEL - 1)) {
      command = command && 0b11110000;
      if (command == NOTE_ON) {
        pulses[PIN_OFFSET + note - BASE_NOTE] = PULSE_TIME;
      }
      #ifdef PRINT
        Serial.print("Instrument ");
        Serial.println(note - BASE_NOTE + 1);
      #endif
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
  unsigned long delayTime = LOOP_TIME - (micros() - start);
  if (delayTime <= MAX_DELAY_MICROSECONDS) {
    delayMicroseconds(delayTime);
  } else {
    delay(delayTime / 1000);
  }
}// Einde van de `loop()` functie
