#include <Arduino.h>
#include <U8x8lib.h>
#include <Wire.h>
#include "Led.h"

// platformio.ini 
//   monitor_speed = 74880
//   libdeps = 
//       U8g2
//       https://github.com/lnlp/Led


#define SERIAL_SPEED 74880   // 74880 so we can see ESP8266 boot messages
const bool useSerial = true;

U8X8_SSD1306_128X64_NONAME_HW_I2C display(/*reset*/ U8X8_PIN_NONE, /*clock*/ SCL, /*data*/ SDA);

#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];
U8X8LOG console;

Led builtinLed(2, Led::ActiveLevel::Low);	// LED on ESP-12F connected to GPIO2
Led led(13, Led::ActiveLevel::Low);

const uint16_t debounceIntervalMs       = 40;
const uint8_t  buttonPin                = 12;     // GPIO12
const uint8_t  flashButtonPin           = 0;      // Hard-wired to GPIO0
volatile bool  buttonPressed            = false;  
volatile bool  flashButtonPressed       = false;


uint32_t timeLastFlashButtonDown = 0;
void flashButtonHandler()
{
	const uint16_t debounceInterval = 40;
	uint32_t now = millis();

	if (now - timeLastFlashButtonDown > debounceInterval)
	{
		flashButtonPressed = true;
		timeLastFlashButtonDown = now;
	}	
}


uint32_t timeLastButtonDown = 0;
void buttonHandler()
{
	const uint16_t debounceInterval = 40;
	uint32_t now = millis();

	if (now - timeLastButtonDown > debounceInterval)
	{
		buttonPressed = true;
		timeLastButtonDown = now;  
	}	
}


void setup() 
{
    // Initiallize UART
    if (useSerial) 
    {
        Serial.begin(SERIAL_SPEED);

		delay(2000);	// Give PlatformIO Serial Monitor some time to pick up

        Serial.printf("\n\nLeds and buttons test.\n");
        Serial.printf("\nPress the on-board flash button or external button to toggle their corresponding LED.\n\n");        
    }

    // Initialize display
    display.begin();
    display.setFont(u8x8_font_victoriamedium8_r);    

    console.begin(display, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
    console.setRedrawMode(0);   // Update screen with, 0: with newline, 1: for every char      
    console.print("\f"); // \f = form feed: clears the screen   

    console.printf("Leds & buttons\n \n"); // Due to U8x8 bug there must be a char before \n    
    console.printf("Press external\n");
    console.printf("button or flash\n");
    console.printf("button to\n");
    console.printf("toggle LEDs\n \n");

    pinMode(flashButtonPin, INPUT); // Has external pull-up on adapter board
    attachInterrupt(digitalPinToInterrupt(flashButtonPin), flashButtonHandler, FALLING);

    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonHandler, FALLING);     
}


void loop() 
{
    if (flashButtonPressed)
    {
        flashButtonPressed = false; // Reset flag
        builtinLed.toggle();
        if (useSerial) Serial.println("Flash button pressed.");
        console.printf("flash button\n");
    }

    if (buttonPressed)
    {
        buttonPressed = false; // Reset flag
        led.toggle();
        if (useSerial) Serial.println("External button pressed.");
        console.printf("external button\n");
    }    
}