/* v0.1
 
 Uses Freescale MPXV7025DP: https://goo.gl/Sb4YQy 
 Implements Freescale Best Practices: https://goo.gl/YfErPb
 
VOUT = VS x (0.018 x P + 0.92) ± (Pressure Error x Temp Multi x 0.018 x VS)
In other words:
       -25kPa = .1v   (-25,0.1)  or 0  
         0kPa = 2.3v  (0, 2.3)   or 1023/2
        25kPA = 4.6v  (0, 4.6)  or 1023
 
 Assuming 512 for 0kPa, we have a slope of y=mx+b of y=18.52x+512
 We have kPa=(analogread() - 512)/18.52
 PSI=kPa*.1450377 
 Bar = PSI*0.0689476
 
 Note: bitmaps on Arduino are Little Endian, Horizontally aligned using LCDAssistant

To do:
1. Reduce global variables from Adafruit code
*/

//#include <SPI.h>
//#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Fonts/FreeSerif9pt7b.h>

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define ZEROKPA    512

// Use i2c protocol
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);



const unsigned char PROGMEM dog_paw [] = {
0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x78, 0x00, 0x01, 0xE0, 0xFC, 0x00, 0x03, 0xE1, 0xFC, 0x00,
0x03, 0xE1, 0xFC, 0x00, 0x03, 0xE1, 0xFC, 0x00, 0x03, 0xE1, 0xFC, 0x00, 0x03, 0xF0, 0xFC, 0x00,
0x03, 0xE0, 0xFC, 0x40, 0x01, 0xE0, 0x78, 0xF0, 0x01, 0xE0, 0x79, 0xF0, 0x00, 0xE0, 0x03, 0xF8,
0x38, 0x00, 0x03, 0xF0, 0x3C, 0x00, 0x07, 0xF0, 0x7E, 0x00, 0x07, 0xF0, 0x7E, 0x00, 0x07, 0xF0,
0x7E, 0x07, 0x83, 0xE0, 0x7E, 0x0F, 0x83, 0xC0, 0x7E, 0x1F, 0xC1, 0x80, 0x7C, 0x1F, 0xE0, 0x00,
0x3C, 0x3F, 0xF0, 0x00, 0x00, 0x7F, 0xF8, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0xFF, 0xFF, 0x80,
0x01, 0xFF, 0xFF, 0x80, 0x01, 0xFF, 0xFF, 0x80, 0x03, 0xFF, 0xFF, 0x00, 0x01, 0xF8, 0x00, 0x00,
0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const int analogPin = A0;    // select the input pin for the MPXV7025DP
const int sensorValue = 0;   // variable to store the value coming from the sensor


int8_t pos = 127;
float kPa = 0;               // Pressure 

// Sampling Array - Freescale's 10 samples with filtered sensors looks really good.  
const unsigned int numReadings = 16;
int readings[numReadings];      // the readings from the analog input
int delta = 0;
int index = 0;                  // the index of the current reading
long deltaTotal = 0;            // Keep a running total of deltas - can be negative
int deltaAverage = 0;           // init averages
int zeroKPa = 512;              // calibrate in setup() to get real value

void setup()   {                

  long count = 0;
  long sum = 0;
  
  // initialize all array readings to 0: 
  for (int index = 0; index < numReadings; index++) {
          readings[index] = 0;    // start at center
  }
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  
  // Calibrate for 5 seconds - averaging ~8,860 samples per second during calibration.
  display.clearDisplay();
  display.setFont();
  display.setCursor(0,10);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Calibrating...");
  display.drawBitmap(90, 20,  dog_paw, 32, 32, 1);
  display.setCursor(93,50); display.println("v0.1");
  display.display();
  while (millis() < 10000) {
    sum=sum + analogRead(analogPin);
    count++;
  }
  zeroKPa=sum/count;
  if (zeroKPa < ZEROKPA*0.9 || zeroKPa > ZEROKPA*1.1) {  // if "0 kPA" is 10% off, either the MPX sensor is shot or a TB is pulling on it
  display.setCursor(0,10);
  display.println("Calibrating... Fail"); 
  display.println(); 
  display.println("Drawing vacuum?"); 
  display.println(); 
  display.println(":-O"); 
  display.display();
  while (1);  //Ugh - spin until power is pulled.
  }
   else {    
  display.setCursor(0,10);
  display.println("Calibrating... Done.");
  display.print("Count: "); display.println(count);
  display.print("ZKPa:  "); display.println(zeroKPa);
  display.display();
  delay(10000);
  } 
}
void loop() {

    // remove the last reading from the total to make way for new value
    deltaTotal = deltaTotal - readings[index];
  
    // read from the sensor and calculate Moving Sample Average (MSA)
    delta = analogRead(analogPin); 
    readings[index] = delta;
    deltaTotal = deltaTotal + readings[index];    // add the readings to the totals:
    deltaAverage = deltaTotal/numReadings;        // average the running totals
    
    kPa = (deltaAverage - zeroKPa)/18.52;         // Calculate kPa
    
    display.clearDisplay(); 
    // display the sampled results
    frame();
    bar(deltaAverage);
    text(kPa);
    display.display();
    
    if ( (index + 1) >= numReadings) { // we're at the end of the array... wrap 
         index = 0;
    }    
    else index = index + 1;    
}

void frame(void) {
  display.drawRect(0, 0, display.width(), display.height()/4, WHITE);
  display.drawFastVLine(display.width()/2,0,4,WHITE);                      //Top center mark
  display.drawFastVLine(display.width()/2,display.height()/4-4,4,WHITE);   //Bottom center mark
}

void bar(int pressure) {
  int pos = 0;
  pos = map(pressure, 0, 2*zeroKPa, 0, display.width());
  display.drawFastVLine(display.width()-pos,0,16,WHITE);
}

void text(float kPa) {
   char cylinder = 'L';
// print results in text
    if (kPa < 0) cylinder = 'R'; else cylinder = 'L';
    kPa = abs(kPa); // remove negative values
    display.drawBitmap(90, 20,  dog_paw, 32, 32, 1);
    display.setCursor(0,20);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("Hg:  ");display.print(cylinder);display.print(": ");display.println(kPa*0.2953);
    display.print("kPa: ");display.print(cylinder);display.print(": ");display.println(kPa);
    display.print("PSI: ");display.print(cylinder);display.print(": ");display.println(kPa*0.1450377);
    display.print("Bar: ");display.print(cylinder);display.print(": ");display.println(kPa*0.01000000132452);
    display.setCursor(93,50); display.println("v0.1");
}



