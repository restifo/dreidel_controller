#include <TimerOne.h>

/*

  Dreidel Controller v. 0.1a
  by Christian M. Restifo (restifo@gmail.com)
  11/30/10

  AC phase control courtesy of Ryan McLaughlin <ryanjmclaughlin@gmail.com>
  Much thanks to http://www.andrewkilpatrick.org/blog/?page_id=445
   and http://www.hoelscher-hi.de/hendrik/english/dimmer.htm

*/

#define TESTPIN 5 // Used to put the controller in test mode on start up
#define TESTMODE_LED 8

int dreidelPins[] = {9, 10, 11, 12};    // the 4 pins for the four symbols
volatile uint8_t dreidelFade[] = {120, 120, 120, 120}; // fade values for individual symbols
volatile uint8_t dreidelDim[] = {0, 0, 0, 0};        // individual dim value counter for comparison to dreidelFade, 0 = on, 120 = off
volatile uint8_t zero_crossArr[] = {0, 0, 0, 0};     // zero cross array for individual symbols. Not really needed but here in case we want to do funky things in the future.
// for example, you could artificialy change the array at different times to create a phase off set.
int numSymbols = 4;                     // total number of symbols in case you want to expand
int targetSymbol = 0;                   // used to index the dreidelPins array
int fade = 0;                           // flag to let us know if we should fade in/out
int startupDelay = 2000;                // delay value for startup flashing routine
int fadeDelay = 15;                     // delay for fading value changes
int roll = 1;                           // flag for button being pushed
uint8_t maxDim = 128;                       // max dim value for individual symbols (off). Lower this value if you get odd flickering. Max is 128 per freqStep below.
uint8_t minDim = 0;                         // min dim value for individual symbols (on)
int demoDelay = 5000;                   // delay counter to return to demo mode
int rollButton = 4;                     // input pin for roll button
int currentSymbol = 0;                  // the current symbol we're on when spinning dreidel
int decay;                              // random decaying value for delay between symbols when spinning
int decaytime = 700;                    // the max time from full spin to stop
int fastSpinDelay = 100;                // time symbol is one for fast part of spin
int phaseOffset = 4;                   // offset in dimming calculation to account for the zero cross over fall/rise time
//#include "TimerOne.h"                  // Avaiable from http://www.arduino.cc/playground/Code/Timer1


/* NOTE: For dreidelDim, I use about 120. I found that 128 caused some type of flickering, probably due to noise on the line. Adjust up or down as necessary to get a stable
         fading value. You will sacrifice some "dimness" as you go below 128, but I found it to be unnoticeable at 120 for my lights. */

volatile boolean zero_cross = 0; // Boolean to store a "switch" to tell us if we have crossed zero
int freqStep = 65;              // Set the delay for the frequency of power (65 for 60Hz, 78 for 50Hz) per step (using 128 steps)
// freqStep may need some adjustment depending on your power the formula
// you need to use is (500000/AC_freq)/NumSteps = freqStep. 500,000 is used because we're doing half cycle (2 cross overs per cycle)
// You could also write a seperate function to determine the freq

void setup() {

  turnOffSymbols();  // blank everything and turn it off.

  pinMode(rollButton, INPUT);
  pinMode(0, INPUT);

  attachInterrupt(2, zero_cross_detect, RISING);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check);                // Use the TimerOne Library to attach an interrupt
  // to the function we use to check to see if it is
  // the right time to fire the triac.  This function

  for (targetSymbol = 0; targetSymbol < numSymbols; targetSymbol++) {
    digitalWrite(dreidelPins[targetSymbol], HIGH);                     // flash each symbol in turn
    delay(startupDelay);
    digitalWrite(dreidelPins[targetSymbol], LOW);
  }

  pinMode(TESTPIN, INPUT);
  if (digitalRead(TESTPIN) == LOW) {
    pinMode(TESTMODE_LED, OUTPUT);
    digitalWrite(TESTMODE_LED, HIGH);
    testMode();
  }
  Serial.begin(115200);
} // End setup

void testMode() { // flashes lights so we can make sure things light up in order
  while (digitalRead(TESTPIN) == LOW) {
    int counter = 0;
    for (int i = 0; i < numSymbols; i++) {
      for (int j = 0; j <= i; j++) {
        digitalWrite(dreidelPins[i], HIGH);
        delay(1000);
        digitalWrite(dreidelPins[i], LOW);
        delay(1000);
      }
    }
  }
  digitalWrite(TESTMODE_LED, LOW);
}

void zero_cross_detect() {               // function to be fired at the zero crossing
  for (int j = 0; j < numSymbols; j++) { // we use this with a timer to determine when to fire a triac for dimming purpose
    zero_crossArr[j] = 1;               // set the boolean to true to tell our dimming function that a zero cross has occured
  }
} // End zero_cross_detect

void dim_check() {                   // Function will fire the triac at the proper time
  for (int j = 0; j < numSymbols; j++) {//numSymbols
    if (zero_crossArr[j] == 1) { // check to see if zero cross has occurred for this light
      if (dreidelDim[j] >= dreidelFade[j] && fade == 1) {  // if the dim counter is greater than the fade value, turn on. Remember that 0 = full on, maxDim = full off.
        digitalWrite(dreidelPins[j], HIGH);        // Fire the Triac mid-phase
        delayMicroseconds(5);                     // Pause briefly to ensure the triac turned on
        digitalWrite(dreidelPins[j], LOW);         // Turn off the Triac gate (Triac will not turn off until next zero cross)
        dreidelDim[j] = minDim;                    // Reset the accumulator
        zero_crossArr[j] = 0;                      // Reset the zero_cross so it may be turned on again at the next zero_cross_detect
      } else {
        if (fade == 1) {
          dreidelDim[j]++;                         // increase the counter again until we get to the fade value to turn on.
        }
      }
    }
  }
} // end dim_check


void demoFlash() { // demo while game is not being played

  fade = 1;  // fade in and out
  while (roll == 1) {
    for (int i = 0; i < numSymbols; i++) {  // cycle through the symbols
      for (int j = minDim; j < maxDim; j++) {
        roll = digitalRead(rollButton);    // check for button push. if so, break out so we can "roll" dreidel
        if (roll == 0) {
          break;
        }
        dreidelFade[i] = min(j+phaseOffset, maxDim);                // set the fade value to j but don't set it so low that we fire during the zero cross over which causes flicker
        Serial.print(dreidelFade[0]); Serial.print(" - "); Serial.println(dreidelFade[1]);
        if (i == (numSymbols - 1)) {
          dreidelFade[0] = min(maxDim, maxDim - j + phaseOffset);     // set the first symbol to be 180 out of sync with last one. again avoid low value which causes flicker.
        }                                  // or
        else {
          dreidelFade[i + 1] = min(maxDim, maxDim - j + phaseOffset); // set the next symbol to 180 out of sync with the current one
        }
        delay(fadeDelay);                  // delay to control the speed of dimming
      }
    }
  }
} // end demoFlash

void turnOffSymbols() {
  for (targetSymbol = 0; targetSymbol < numSymbols; targetSymbol++) {
    pinMode(dreidelPins[targetSymbol], OUTPUT);                        // set dreidel symbol pins as outputs
    digitalWrite(dreidelPins[targetSymbol], LOW);                      // force everything off
    dreidelFade[targetSymbol] = maxDim;                                // set to off for dim_check function
    zero_crossArr[targetSymbol] = 0;
  }
}  // end of turnOffSymbols


void rollDreidel() {
  int numClicks;                         // number of times we'll switch symbols at the start
  int startSymbol;                       // symbol to start on
  turnOffSymbols();                      // set everything to off for the interrupt function that's constantly running
  fade = 0;                              // disable fading
  randomSeed(analogRead(0));             // random seed generator. used for random delay and random start, number of symbol switches
  numClicks = int(random(10, 20));       // random number of symbol changes
  startSymbol = int(random(0, 3));       // start on a random symbol

  for (int a = 0; a < numClicks; a++) {   // do a "spin" for a random number of symbols. this repeats some of the code below, but it's here so we can modify it if we want
    digitalWrite(dreidelPins[startSymbol], HIGH);  // turn it on
    delay(fastSpinDelay);
    digitalWrite(dreidelPins[startSymbol], LOW);  // turn it off
    startSymbol += 1;                             // move to next symbol
    if (startSymbol == numSymbols) {              // wrap back to first symbol [0] if at end
      startSymbol = 0;
    }
  }

  currentSymbol = startSymbol;

  decay = int(random(50, 100)); // set an initial random decay

  while (decay < decaytime) {
    digitalWrite(dreidelPins[currentSymbol], HIGH);  // I like to hold fuzzy little animals and give them huggie-wuggies
    delay(decay);
    digitalWrite(dreidelPins[currentSymbol], LOW);
    decay += int(random(50, 100));                  // add some random value to increase decay as we "slow down"
    currentSymbol += 1;
    if (currentSymbol == numSymbols) {
      currentSymbol = 0;
    }
  }

  for (int i = 0; i < 5; i++) {   // flash the winning pin
    digitalWrite(dreidelPins[currentSymbol], LOW);
    delay(500);
    digitalWrite(dreidelPins[currentSymbol], HIGH);
    delay(500);
  }


  int j = 0;
  while (j < demoDelay) {            // wait for a while for the person to push the button again. if not, go back to fading demo
    roll = digitalRead(rollButton);
    if (roll == 0) {
      fade = 0;
      break;
    }
    delay(1);
    j++;
  }
}  // end of rollDreidel


void loop() {                        // Main Loop

  turnOffSymbols();  // turn everything off so we don't have random values out there causing odd fading/behavior
  fade = 1;          // turn on fading (assuming we go to demo)

  while (roll == 1) {
    demoFlash();  // just do a nice fading demo when nobody is playing
  }

  rollDreidel();  // spin the drediel

}  // end of main loop
