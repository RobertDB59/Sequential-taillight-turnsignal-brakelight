/*          Sequential taillight turn signal and brake light and DRL

   This project is to get a sequential indicator light in combination with a brake light.
   I want the circuit to be small so I opted for the ATtiny84.
   The programm however is build with an Arduino Nano.

   The indicator lights are sequential from all on to all off.
   When the brake is hit, all led's turn on.
   When the brake is hit and one of the indictor switches is triggered, one side stay's on
   and the other is a sequential running turn light.

   Work to do;
   one of the led's on each side should blink in a in a preset morse code message when no 
   indicator is used and no brake is applied -> use of an interrupt or no delay morse code?


   LICENCE
   Copyright November 2022 Robert de Beer

   Licensed under the Apache License, Version 2.0 ( the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   For an AtTiny84:
      program uses 2598 ( 31%) bytes program storage space
      variables use  52 ( 10%) bytes of dynamic memory
*/

#define dbounceMs 10                                    // debounce time 5+ ms is usual enough
#define numBtns sizeof( btnPin)
#define numLeds sizeof( ledPin)

byte brakeFlag = 0;
byte leftFlag = 0;
byte rightFlag = 0;
byte lastBrakeState = 1;                                // assume switch open because of input_pullup
byte lastLeftState = 1;                                 // assume switch open because of input_pullup
byte lastRightState = 1;                                // assume switch open because of input_pullup
byte btnPin[] = {5, 7, 8};                              // array that holds the button port numbers [NANO]
byte ledPin[] = {4, 3, 2, 9, 10, 11};                   // array that holds the led port numbers    [NANO]
//byte btnPin[] = {0, 1, 2};                              // array that holds the button port numbers [ATtiny84]
//byte ledPin[] = {5, 3, 6, 7, 4, 8};                     // array that holds the led port numbers    [ATtiny84]
byte pinNum;
byte nextStep;
byte turnInd;

static byte stepInterval = 120;

unsigned long currMs;
unsigned long nextMs;
unsigned long waitMs;
unsigned long brakePressMs;
unsigned long rightPressMs;
unsigned long leftPressMs;


void setup() {
//  Serial.begin( 115200);
  for( byte i = 0; i < numBtns; i++){
    pinMode( btnPin[i], INPUT_PULLUP);
  }

  for( byte i = 0; i < numLeds; i++) {
    pinMode( ledPin[i], OUTPUT);
    digitalWrite( ledPin[i], 0);
  }

  for( byte j = 0; j < 3; j++){                         // repeat 3 times
    for( byte i = 0; i < numLeds / 2; i++) {            // all leds on from center to edge
      digitalWrite( ledPin[i], 1);
      digitalWrite( ledPin[i + 3], 1);
      delay( stepInterval / 2);
    }

    delay( stepInterval / 2);

    for( byte i = 0; i < numLeds / 2; i++) {            // all leds off from center to edge
      digitalWrite( ledPin[i], 0);
      digitalWrite( ledPin[i + 3], 0);
      delay( stepInterval / 2);
    }

    delay( stepInterval / 2);
  }
}


void loop() {
  currMs = millis();

  byte brakeState = digitalRead( btnPin[0]);
  byte rightState = digitalRead( btnPin[1]);
  byte leftState  = digitalRead( btnPin[2]);

  if( brakeState != lastBrakeState){                    // has it changed?
    if( currMs - brakePressMs >= dbounceMs){            // debounce
      brakePressMs = currMs;                            // time switch is closed
      lastBrakeState = brakeState;                      // remember for next time
      if( brakeState == 0){
        brakeFlag = 1;                                  // end if brakeState is LOW
      } else {
        brakeFlag = 0;                                  // end if brakeState is HIGH
        if( rightFlag == 1){
          ledsLeft();
        } else if( leftFlag == 1){
          ledsRight();
        } else {
          allLeds(0);
        }
      }
    }
  }

  if( rightState != lastRightState){
    if( currMs - rightPressMs >= dbounceMs){
      rightPressMs = currMs;
      lastRightState = rightState;
      if( rightState == 0){
        digitalWrite( ledPin[0], 0);
        digitalWrite( ledPin[1], 0);
        digitalWrite( ledPin[2], 0);
        turnInd = 0;
        nextStep = 0;
        rightFlag = 1;
      } else {
        for( byte i = 0; i < numLeds / 2; i++){
          digitalWrite( ledPin[i], 0);
        }
        turnInd = 0;
        nextStep = 0;
        rightFlag = 0;
      }
    }
  }

  if( leftState != lastLeftState){
    if( currMs - leftPressMs >= dbounceMs){
      leftPressMs = currMs;
      lastLeftState = leftState;
      if( leftState == 0){
        digitalWrite( ledPin[3], 0);
        digitalWrite( ledPin[4], 0);
        digitalWrite( ledPin[5], 0);
        turnInd = 3;
        nextStep = 0;
        leftFlag = 1;
      } else {
        for( byte i = 0; i < numLeds / 2; i++){
          digitalWrite( ledPin[i + numLeds / 2], 0);
        }
        turnInd = 3;
        nextStep = 0;
        leftFlag = 0;
      }
    }
  }

  if( brakeFlag == 0 && rightFlag == 0 && leftFlag == 0){
    drlLight( ledPin[1], ledPin[4]);
    analogWrite( ledPin[3], 20);
    analogWrite( ledPin[5], 20);
  } else if ( brakeFlag == 1 && rightFlag == 0 && leftFlag == 0){     // only brake
    allLeds(1);
  } else if ( brakeFlag == 1 && leftFlag == 1){                       // left indicator and brake
    ledSequence( 1, 0);                                               // brake and left turn signal
  } else if ( brakeFlag == 1 && rightFlag == 1){                      // right indicator and brake
    ledSequence( 1, 3);                                               // brake and right turn signal
  } else if ( brakeFlag == 0 && leftFlag == 1){                       // left indicator no brake
    ledSequence( 0, 0);                                               // no brake and left turn signal
  } else if ( brakeFlag == 0 && rightFlag == 1){                      // right indicator no brake
    ledSequence( 0, 3);                                               // no brake and right turn signal
  }
}


void ledSequence( int ledState, int brakeInd){
  digitalWrite( ledPin[brakeInd + 0], ledState);
  digitalWrite( ledPin[brakeInd + 1], ledState);
  digitalWrite( ledPin[brakeInd + 2], ledState);

  if( currMs >= nextMs){
    digitalWrite( ledPin[turnInd + nextStep], !digitalRead( ledPin[turnInd + nextStep]));
    nextStep += 1;
    if( nextStep > 2) nextStep = 0;
    nextMs += stepInterval;
  }
}


void ledsLeft(){
  for( pinNum = 3; pinNum < 6; pinNum++){
    digitalWrite( ledPin[pinNum], 0);
  }
}


void ledsRight(){
  for( pinNum = 0; pinNum < 3; pinNum++){
    digitalWrite( ledPin[pinNum], 0);
  }
}


void allLeds( byte state){
  for( pinNum = 0; pinNum < 6; pinNum++){
    digitalWrite( ledPin[pinNum], state);
  }
}


void drlLight( byte drl1, byte drl2) {
  if( currMs > nextMs) {
    digitalWrite(drl1, !digitalRead( drl1));
    digitalWrite(drl2, !digitalRead( drl2));
    waitMs = random( 40, 240);
    nextMs = currMs + waitMs;
  }
}
