#include <Arduino.h>
#include <U8x8lib.h>
#include <Wire.h>

// platformio.ini 
//   monitor_speed = 74880
//   libdeps = 
//       U8g2


#define SERIAL_SPEED 74880   // 74880 so we can see ESP8266 boot messages
const bool useSerial = true;

U8X8_SSD1306_128X64_NONAME_HW_I2C display(/*reset*/ U8X8_PIN_NONE, /*clock*/ SCL, /*data*/ SDA);

#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];
U8X8LOG console;


void setup() 
{
    // Initiallize UART
    if (useSerial) 
    {
        Serial.begin(SERIAL_SPEED);
        Serial.printf("\n\nTesting the OLED display.\n");
    }

    // Initialize display
    display.begin();
    display.setFont(u8x8_font_victoriamedium8_r);

    console.begin(display, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
    console.setRedrawMode(0);   // Update screen with, 0: with newline, 1: for every char      
}


void loop() 
{
    console.print("\f"); // \f = form feed: clears the screen

    // Note: due to a bug in U8x8 there must be a char before newline
    console.print(" \n \n \n \n \nThe display\n\n");
    console.print("is working\n"); 
    delay(1000) ;

    for (int i = 1; i <= 5; i++)
    {
        delay(250);
        console.print(" \n");
    }

    delay(1000);
}