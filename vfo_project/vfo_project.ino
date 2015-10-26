/*********************************************************************

vfo_project

an simple Arduino and DDS9850
based Voltage Controlled Oscillator

Uses DDS 9805 based VFO,  an Ardunio with SSD1306 OLED display
and a interupt driven based rotory encoder

Written by Brandon W. Hockle KB1WAH
for the Waltham Amateur Radio Association. WARA64

Adafruit SSD1306 driver written by Limor Fried/Ladyada
for Adafruit Industries.

9850 datasheet bitbang by Richard Visokey AD7C - www.ad7c.com

The Encoder changes the frequency, and a knob click changes the scale.

Check github http://github.com/bhockle

*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
// ADS9850 Pins
//
#define DDS_W_CLK 8   // DDS Clock Pin
#define DDS_FU_UD 9   // DDS Latch pin
#define DDS_DATA 10   // DDS Data Pin
#define DDS_RESET 11  // DDS Reset Pin

Adafruit_SSD1306 display(OLED_RESET);

#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

// Encoder Items
//
// these pins can not be changed 2/3 are the only interupt pins
int encoderPin1 = 2;
int encoderPin2 = 3;
int encoderSwitchPin = 4;                            // pin for push button switch for encoder scale
unsigned long encoderScale = 10000;                  // set the encoderScale to 10KHz
unsigned long lastEncoderScale = 0;
volatile unsigned long encoderValue = 29000000;      // setup the encoder value to a frequency in the 10m band 29Mhz
unsigned long lastEncoderValue = 0;
volatile int lastEncoded = 0;
int lastMSB = 0;
int lastLSB = 0;



void setup()   {
  Serial.begin(9600);
  Serial.println("Starting Setup.");

  pinMode(encoderPin1, INPUT);
  pinMode(encoderPin2, INPUT);

  pinMode(encoderSwitchPin, INPUT);

  pinMode(DDS_FU_UD, OUTPUT);
  pinMode(DDS_W_CLK, OUTPUT);
  pinMode(DDS_DATA, OUTPUT);
  pinMode(DDS_RESET, OUTPUT);


  pulseHigh(DDS_RESET);
  pulseHigh(DDS_W_CLK);
  pulseHigh(DDS_FU_UD);  // this pulse enables serial mode - Datasheet page 12 figure 10


  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  digitalWrite(encoderSwitchPin, HIGH); //turn pullup resistor on

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3)
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);

  // init done

  // initialize and display the splashscreen.
  display.display();

  // Clear the display buffer.
  display.clearDisplay();

  // Startup Message
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("DS9850 VFO");
  display.setTextSize(1);
  display.println("");
  display.print("An Arduino Project by");
  display.println(" Brandon W. Hockle");
  display.println("");
  display.setTextSize(2);
  display.print("  KB1WAH");
  display.display();
  delay(2000);
  Serial.println("Finished Setup.");
}

void loop() {
  // if button is pressed change the encoder scale
  if (digitalRead(encoderSwitchPin)) {
    //button is not being pushed
  } else {
    encoderScale = encoderScale * 10;
    if (encoderScale > 10000000) {
      encoderScale = 1;
    }
      delay(1000);
  }

  // Display and set frequency on changed valued.
  if (encoderValue != lastEncoderValue || encoderScale != lastEncoderScale ) { // if encoder value changes then set new frequency and update display
    if (encoderValue > 50000001 ) {       // If encoder is greater than maximum frequency of 50Mhz set it to zero.
     encoderValue = 0;
    }
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print( encoderValue );
    display.setTextSize(1);
    display.print( "Hz" );
    display.setCursor(0,32);
    display.println("Encoder Scale =");
    display.setTextSize(2);
    display.print( encoderScale );
    display.setTextSize(1);
    display.print("Hz");
    display.setTextSize(2);
    sendFrequency(encoderValue);
    lastEncoderValue = encoderValue;
    lastEncoderScale = encoderScale;
    display.display();
  }
}

void updateEncoder() {
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue = encoderValue - encoderScale;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue = encoderValue + encoderScale;

  lastEncoded = encoded; //store this value for next time
}

 // transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void tfr_byte(byte data)
{
  for (int i=0; i<8; i++, data>>=1) {
    digitalWrite(DDS_DATA, data & 0x01);
    pulseHigh(DDS_W_CLK);   //after each bit sent, CLK is pulsed high
  }
}
 
 // frequency calc from 9850 datasheet page 8 = <sys clock> * <frequency tuning word>/2^32
void sendFrequency(double frequency) {
  int32_t freq = frequency * 4294967295/125000000;  // note 125 MHz clock on 9850
  for (int b=0; b<4; b++, freq>>=8) {
    tfr_byte(freq & 0xFF);
  }
  tfr_byte(0x000);   // Final control byte, all 0 for 9850 chip
  pulseHigh(DDS_FU_UD);  // Done!  Should see output
}

 


