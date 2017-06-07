/*******************************************************************************

  Bare Conductive Pi Cap
  ----------------------

  tts.cpp - touch triggered text-to-speech synthesis

  Written for Raspberry Pi.

  Bare Conductive code written by Szymon Kaliski.

  This work is licensed under a MIT license https://opensource.org/licenses/MIT

  Copyright (c) 2016, Bare Conductive

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

 *******************************************************************************/

#include <MPR121.h>
#include <signal.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <vector>

#define NUM_ELECTRODES 12

#define RED_LED_PIN 22
#define GREEN_LED_PIN 21
#define BLUE_LED_PIN 25

using namespace std;

bool volatile keepRunning = true;

void lightRGB(int r, int g, int b) {
  // we are inverting the values, because the LED is active LOW
  // LOW - on
  // HIGH - off
  digitalWrite(RED_LED_PIN, !r);
  digitalWrite(GREEN_LED_PIN, !g);
  digitalWrite(BLUE_LED_PIN, !b);
}

// this allows us to exit the program via Ctrl+C while still exiting elegantly
void intHandler(int dummy) {
  keepRunning = false;
  lightRGB(0, 0, 0);
  exit(0);
}

void speak(string text) {
  cout << "speaking: " << text << endl;

  stringstream textStream;
  textStream << "espeak \"" << text << "\" --stdout | aplay > /dev/null 2>&1";
  system(textStream.str().c_str());
}

int main(void) {
  // register our interrupt handler for the Ctrl+C signal
  signal(SIGINT, intHandler);

  // default MPR121 address on the Pi Cap
  if (!MPR121.begin(0x5C)) {
    cout << "error setting up MPR121: ";

    switch (MPR121.getError()) {
      case NO_ERROR:
        cout << "no error" << endl;
        break;
      case ADDRESS_UNKNOWN:
        cout << "incorrect address" << endl;
        break;
      case READBACK_FAIL:
        cout << "readback failure" << endl;
        break;
      case OVERCURRENT_FLAG:
        cout << "overcurrent on REXT pin" << endl;
        break;
      case OUT_OF_RANGE:
        cout << "electrode out of range" << endl;
        break;
      case NOT_INITED:
        cout << "not initialised" << endl;
        break;
      default:
        cout << "unknown error" << endl;
        break;
    }

    exit(1);
  }

  // this is the touch threshold - setting it low makes it more like a proximity trigger
  // default value is 40 for touch
  int touchThreshold = 40;

  // this is the release threshold - must ALWAYS be smaller than the touch threshold
  // default value is 20 for touch
  int releaseThreshold = 20;

  MPR121.setTouchThreshold(touchThreshold);
  MPR121.setReleaseThreshold(releaseThreshold);   

  // set up LED
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  lightRGB(0, 0, 0);

  // create vector of strings to hold the texts
  std::vector<string> texts;

  // initialize tracks
  for (int i = 0; i < NUM_ELECTRODES; ++i) {
    char path[50];
    sprintf(path, "texts/TEXT%03d.txt", i);

    // some helpful info
    cout << "loading file: " << path << endl;

    ifstream file;
    file.open(path);

    stringstream stream;
    stream << file.rdbuf();
    string text = stream.str();

    // add to vector
    texts.push_back(text);
  }

  while (keepRunning) {
    if (MPR121.touchStatusChanged()) {
      MPR121.updateTouchData();

      bool isAnyTouchRegistered = false;

      for (int i = 0; i < NUM_ELECTRODES; i++) {
        // check if touch is registered to set the led status
        if (MPR121.getTouchData(i)) {
          isAnyTouchRegistered = true;
        }

        // play sample only on new touch
        if (MPR121.isNewTouch(i)) {
          speak(texts[i]);
        }
      }

      if (isAnyTouchRegistered) {
        lightRGB(1, 0, 0);
      }
      else {
        lightRGB(0, 0, 0);
      }
    }

    // a little delay so that we don't just sit chewing CPU cycles
    // could implement this with proper interrupts for greater efficiency
    delay(10);
  }

  // make sure we return gracefully
  return(0);
}
