#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <DDS.h>
#include "Rotary.h"

#define oled_i2c_Address 0x3c

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO

// encoder
#define ENCODER_A    2    // Encoder pin A  D2 (interrupt pin)
#define ENCODER_B    3    // Encoder pin B  D3 (interrupt pin)
#define ENCODER_BTN  A3   // Encoder push buttonh

// freq constants
#define INIT_FREQ    1000
#define INIT_INCR    1000

// ad9850 <--> nano pin assignments
#define W_CLK 8           // D8 - connect to AD9850 module word load clock pin (CLK)
#define FQ_UD 9           // D9 - connect to freq update pin (FQ)
#define DATA 10           // D10 - connect to serial data load pin (DATA)
#define RESET 11          // D11 - connect to reset pin (RST) 


// instantiation
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Rotary r = Rotary(ENCODER_A,ENCODER_B);  // sets the pins the rotary encoder.  Must be interrupt pins.
DDS dds(W_CLK, FQ_UD, DATA, RESET);      // Instantiate the DDS

// encoder vars
volatile int encoder_count = 0; // count of encoder clicks +1 for CW, -1 for CCW  (volatile since used in ISR)
int prev_encoder_count = 0;     // used to measure changes over time
int encoder_delta = 0;          // difference between successive checks of encoder count
                                // when multiplied by tuning increment tells what the frequency change on 
                                // on the active VFO will be
int EncButtonState = 0;
int lastEncButtonState = 0;

// freq vars
int_fast32_t freq = INIT_FREQ;      //  Startup frequency
uint32_t increment = INIT_INCR;     //  Tuning startup VFO tuning increment in HZ.

ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_CW) {
    encoder_count++;
  } else if (result == DIR_CCW) {
    encoder_count--;
  }
}

void displayFreq(int_fast32_t freq) {
    byte millions, hundredthousands, tenthousansds, thousands, hundreds, tens, ones;

    millions = freq/1000000;
    hundredthousands = (freq/100000)%10;
    tenthousansds = (freq/10000)%10;
    thousands = (freq/1000)%10;
    hundreds = (freq/100)%10;
    tens = (freq/10)%10;
    ones = (freq)%10;

    display.clearDisplay();
    display.setCursor(0, 10);

    // Display static text
    display.println("Frequency");

    if (increment == 1000000) {
        display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
        display.print(millions);
        display.setTextColor(SH110X_WHITE); // 'inverted' text
    } else {
        display.print(millions);
    }
    display.print(".");
    if (increment == 100000) {
        display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
        display.print(hundredthousands);
        display.setTextColor(SH110X_WHITE); // 'inverted' text
    } else {
        display.print(hundredthousands);
    }

    if (increment == 10000) {
        display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
        display.print(tenthousansds);
        display.setTextColor(SH110X_WHITE); // 'inverted' text
    } else {
        display.print(tenthousansds);
    }

    if (increment == 1000) {
        display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
        display.print(thousands);
        display.setTextColor(SH110X_WHITE); // 'inverted' text
    } else {
        display.print(thousands);
    }

    display.print(".");

    if (increment == 100) {
        display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
        display.print(hundreds);
        display.setTextColor(SH110X_WHITE); // 'inverted' text
    } else {
        display.print(hundreds);
    }

    if (increment == 10) {
        display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
        display.print(tens);
        display.setTextColor(SH110X_WHITE); // 'inverted' text
    } else {
        display.print(tens);
    }

    if (increment == 1) {
        display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
        display.print(ones);
        display.setTextColor(SH110X_WHITE); // 'inverted' text
    } else {
        display.print(ones);
    }

    // display.println(freq);
    //display.println("MHz");
    display.display();
    delay(1);
}

void CheckEncoder(void) {
  int current_count = encoder_count;  // grab the current encoder_count
  long encoder_delta = 0;

  if (current_count != prev_encoder_count) {  // if there is any change in the encoder coount
    
    #ifdef DEBUG
      sprintf(debugmsg, "Freq: %ld", freq);
      Serial.println(debugmsg);
    #endif

    //
    //  Calculate the delta (how many click positive or negaitve)
    //
    encoder_delta = current_count - prev_encoder_count;
    
    //
    //  Calculate and display the new frequency
    //
    freq = freq + (encoder_delta * increment);
    if (freq >= 30000000L) {
        freq = 30000000L;
    }
    if (freq <= 0) {
        freq = 0;
    }
    //dds.setFrequency(freq);
    //displayFrequency(freq);
    
    #ifdef DEBUG
      sprintf(debugmsg, "current_count: %d, New Freq: %ld", current_count, freq);
      Serial.println(debugmsg);
    #endif

    prev_encoder_count = current_count;  // save the current_count for next time around
  }
}

void CheckIncrement (){
  EncButtonState = digitalRead(ENCODER_BTN);
  
  if(EncButtonState != lastEncButtonState){ 
    #ifdef DEBUG
      sprintf(debugmsg, "Encoder button state: %d", EncButtonState);
      Serial.println(debugmsg);
    #endif
    if(EncButtonState == LOW) {
        if (increment == 10) {
            increment = 100;
        } else if (increment == 100) {
            increment = 1000;
        } else if (increment == 1000) {
            increment = 10000;
        } else if (increment == 10000) {
            increment = 100000;
        } else if (increment == 100000) {
            increment = 1000000;
        } else {
            increment = 10; 
        }
    }
    lastEncButtonState = EncButtonState;
    delay(50); // debounce
    EncButtonState = digitalRead(ENCODER_BTN);
  }
}

void setupDDS() {
  dds.init();
  // dds.trim(125000000);  // Optional oscilattor trim
}

void setup()
{
  delay(250); // wait for the OLED to power up
  display.begin(oled_i2c_Address, true); // Address 0x3C default
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);

  // setup ISR for encoder pins
  PCICR |= (1 << PCIE2);                    //sets interupt pins D2 D3
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);         
  sei();

  // setup pins
  pinMode(ENCODER_BTN,INPUT); digitalWrite(ENCODER_BTN,HIGH); // Encoder switch - STEP

  // setup DDS
  setupDDS();
}

void loop()
{
  displayFreq(freq);
  dds.setFrequency(freq);
  CheckEncoder();
  CheckIncrement();
}
