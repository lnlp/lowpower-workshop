#include <Arduino.h>
#include <U8x8lib.h>
#include <Wire.h>

// platformio.ini 
//   monitor_speed = 74880
//   libdeps = 
//       U8g2

// Only 4 scanned I2C addresses will fit on the screen
// because dedicated for the workshop setup (4 I2C devices).

#define SERIAL_SPEED 74880   // 74880 so we can see ESP8266 boot messages
const bool useSerial = true;

U8X8_SSD1306_128X64_NONAME_HW_I2C display(/*reset*/ U8X8_PIN_NONE, /*clock*/ SCL, /*data*/ SDA);

void setup() 
{
    if (useSerial) 
    {
        Serial.begin(SERIAL_SPEED);
        Serial.printf("\n\nI2C scan.\n\n");
    }

    // Wire.begin(); // Already done in display

    display.begin();
    display.setFont(u8x8_font_victoriamedium8_r);

    display.printf("I2C scan:\n\n");
   
    byte count = 0;
    for (byte i = 8; i < 120; i++)
    {
        Wire.beginTransmission (i);
        if (Wire.endTransmission () == 0)
        {
            display.printf("0x%02X\n", i);
            if (useSerial)
            {
                Serial.printf("Found address: %i (0x%02X)\n", i, i);
            }
            count++;
            delay (1);  // maybe unneeded?
        }
    }
    if (useSerial) Serial.printf("\nFound: %i device(s)\n", count);
    display.printf("\nFound %i devices", count);
}


void loop() 
{
}