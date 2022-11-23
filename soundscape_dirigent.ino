// PRINT macro definitie
// Als deze macro is gedefinieerd zal de Arduino allerlei debug info naar de 
// seriele poort sturen.
#define PRINT

// Het dataformaat voor het liedje
struct trackItem {
  unsigned int tick;
  unsigned int data;
};

// Include de data van het liedje
#include "WeWishYouAMerryChristmas_dirigent.h"

// Pin configuratie:
#define RESET_PIN 12
#define PIN_OFFSET 2
// Pin 13 wordt gebruikt omde ingebouwde LED aan te sturen
// Voor de tracks worden de pins tussen PIN_OFFSET en RESET_PIN gebruikt
// Dus bij een PIN_OFFSET van 2: 
//   track0 -> pin 2; 
//   track1 -> pin 3; 
//   etc.
// Maximaal 10 tracks dan dus

#define MICROSECONDS_PER_TICK (MICROSECONDS_PER_BEAT / TICKS_PER_BEAT)

// Pulsduur in microseconde
#define PULSE_TIME 2500
#define MAX_DELAY_MICROSECONDS 16383 // om delayMicroseconds te kunnen gebruiken

// Macro om de trackItem op een gegeven positie in de data array te lezen
// Dit is nodig omdat de data array in PROGMEM is gedeclareerd
#define readNextTrackItem(index) (memcpy_P(&currentItem, &track[index], sizeof(trackItem)))

// Globale variabelen
unsigned int dirigentIndex; // Houdt de array index van het huidige trackItem bij 
unsigned long tickOffset;   // Onthoudt de tick van het eerste trackItem
unsigned long startTime; 

trackItem currentItem;      // Het huidige trackItem

/*
 * De `setup()` functie wordt één keer uitgevoerd na opstarten of een reset
 */
void setup() {
  #ifdef PRINT
    Serial.begin(115200);
    Serial.println("Starting the conductor in debug mode");
  #endif

  // Initialiseer de pins
  pinMode(RESET_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  for (int n = 0; n < AMOUNT_OF_INSTRUMENTS; n++) {
    pinMode(n + PIN_OFFSET, OUTPUT);
  }

  // Lees het eerste item uit de dirigent array
  dirigentIndex = 0;
  readNextTrackItem(dirigentIndex++);

  // Initialiseer de overige variabelen
  tickOffset = currentItem.tick;
  #ifdef PRINT
    Serial.print("Microseconds per tick: ");
    Serial.println(MICROSECONDS_PER_TICK);
    Serial.print("Time offset: ");
    Serial.println(tickOffset);
  #endif

  // Stuur de reset puls naar de instrumenten
  digitalWrite(RESET_PIN, 1);
  delayMicroseconds(PULSE_TIME);
  digitalWrite(RESET_PIN, 0);
  // Wacht een seconde voor het liedje gaat afspelen
  delay(5000);  
  startTime = micros();
} // Einde van de `setup()` functie


/*
 * De `loop()` functie wordt iedere keer herhaald totdat de Arduino wordt 
 * uitgezet of gereset.
 */
void loop() {
  if (dirigentIndex <= DIRIGENTSIZE) {
    // Stuur de output pins aan
    #ifdef PRINT
      Serial.print(dirigentIndex);
      Serial.print("/");
      Serial.print(DIRIGENTSIZE);
      Serial.print(": ");
      print_binary(currentItem.data, AMOUNT_OF_INSTRUMENTS);
      Serial.print(" ");
    #endif
    for (int n = 0; n < AMOUNT_OF_INSTRUMENTS; n++) {
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(n + PIN_OFFSET, currentItem.data & (1 << n));
    }
    delayMicroseconds(PULSE_TIME);
    for (int n = 0; n < AMOUNT_OF_INSTRUMENTS; n++) {
      digitalWrite(n + PIN_OFFSET, LOW);
      digitalWrite(LED_BUILTIN, LOW); 
    }

    if (dirigentIndex < DIRIGENTSIZE) {
      // Lees het volgende item uit de dirigent array
      readNextTrackItem(dirigentIndex++);

      // Bepaal hoe lang er gewacht moet worden tot het volgende event
      unsigned long timeToNextEvent = (currentItem.tick - tickOffset) * MICROSECONDS_PER_TICK;
      unsigned long currentTime = micros() - startTime;
      unsigned long delayTime = 0;
      if (currentTime < timeToNextEvent) {
        delayTime = timeToNextEvent - currentTime;
      }

      // Wacht op het volgende event
      #ifdef PRINT
        unsigned long temp = micros();  
        Serial.print("Delaying (us): ");
        Serial.println(delayTime);
        unsigned long delta = micros() - temp;
        if (delta < delayTime) {
          delayTime -= delta;
        } else {
          delayTime = 0;
        }
      #endif
      if (delayTime <= MAX_DELAY_MICROSECONDS) {
        delayMicroseconds(delayTime);
      } else {
        delay(delayTime / 1000);
      }
    }      
  }
}// Einde van de `loop()` functie

#ifdef PRINT
void print_binary(int number, byte Length) {
  static int Bits;
  if (number) { //The remaining bits have a value greater than zero continue
    Bits++; // Count the number of bits so we know how many leading zeros to print first
    print_binary(number >> 1, Length); // Remove a bit and do a recursive call this function.
    if (Bits) for (byte x = (Length - Bits); x; x--)Serial.write('0'); // Add the leading zeros
    Bits = 0; // clear no need for this any more
    Serial.write((number & 1) ? '1' : '0'); // print the bits in reverse order as we depart the recursive function
  }
}
#endif
