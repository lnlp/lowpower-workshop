/******************************************************************************
 * 
 * File:          sensors.ino
 * 
 * Description:   Reads SHT31 and BH1750 sensors and ADC.
 *                Displays temperature, humidity, light and battery voltage 
 *                on OLED display and serial port.
 *                Needs voltage divider for reading battery voltage.
 *                Uses MOSFET switch for voltage divider to save (battery) power.
 * 
 * Purpose:       Read and display sensor values.
 *                Read and correct ADC values.
 *                Demonstrate the use of a MOSFET switch for low-power applications.
 * 
 * Context:       Low-power workshop.
 * 
 * Requirements:  Hardware:
 *                - ESP8266 ESP12E/F module,
 *                - SHT31 I2C temperature & humidity sensor,
 *                - BH1750 I2C light sensor,
 *                - SSD1306 128x64 I2C OLED display,
 *                - LED with resistor connected to +3.3V and GPIO 13 
 *                  (or use builtin LED on GPIO2),
 *                - Voltage divider 1:5 (30k and 120k resistors 1%) for ADC to 
 *                  measure battery voltage (extends input range to 0-5V),
 *                - Optional: P-channel MOSFET switch (active low) on GPIO14. 
 *                Software: Arduino framework.
 * 
 * Dependencies:  Uses the following external libraries:
 *                U8g2, Adafruit SHT31 Library, Adafruit Unified Sensor, BH1750, Led.
 * 
 * Remarks:       ESP8266 max ADC input voltage is 1V. For measuring higher values
 *                a voltage divider (using two resistors) is required.
 *                The ESP8266 ADC is not the most accurate. Deviations of +/- 5% 
 *                possible. For best accuracy measure with multimeter and callibrate.
 *                For ESP8266, I2C pins are: SDA = GPIO4, SCL = GPIO5.
 *                The MOSFET switch is intended to prevent unnecessary power
 *                usage for low-power (deep sleep) applications (deep sleep
 *                is not further applied here). Switching off the voltage divider 
 *                will save 20uA (@3.0V) to 28uA (@4.2V).
 * 
 *                In PlatformIO IDE first open Serial Monitor and then upload the sketch.
 *                Any sketch output is then instantly displayed in Serial Monitor without
 *                risking missing the first serial output because not yet started
 *                (but it will be more difficult to notice if a sketch upload fails).
 * 
 * Author:        Leonel Lopes Parente.
 * 
 * License:       MIT (see LICENSE file in project or repository root).
 * 
 *****************************************************************************/

#include "Arduino.h"
#include "U8x8lib.h"
#include "Wire.h"
#include "ESP8266WiFi.h"
#include "Adafruit_SHT31.h"
#include "Adafruit_Sensor.h"
#include "BH1750.h"
#include "Led.h"


#define SERIAL_SPEED 74880                     // 74880 so ESP8266 boot messages can be read.

const uint32_t measureIntervalMs = 10 * 1000;  // Milliseconds.
const bool useSerial = true;                   // Enables/disables output to serial port.

const char title[] = "sensors";
const uint8_t sht31Address = 0x44;             // 0x44 or 0x45.
const uint8_t bh1750Address = 0x23;            // 0x23 or 0x5C.
const uint8_t batteryMeasureEnablePin = 14;    // GPIO14.

float temperature;
float humidity;
uint16_t light;
float batteryVoltage;
uint32_t lastMeasureTime = 0;
char floatbuf[11];

U8X8_SSD1306_128X64_NONAME_HW_I2C display(/*reset*/ U8X8_PIN_NONE, /*scl*/ SCL, /*sda*/ SDA);
Adafruit_SHT31 sht31;
BH1750 bh1750;
Led led(13, Led::ActiveLevel::Low);  // GPIO13.


void batteryMeasureEnable(bool enable = true)
{
    // Powers on/off the ADC voltage divider resistors (with MOSFET).
    // Power off when not measuring battery voltage to save (battery) power.
    if (enable)
    {
        digitalWrite(batteryMeasureEnablePin, LOW);
    }
    else
    {
        digitalWrite(batteryMeasureEnablePin, HIGH);
    }
}


float getCalibratedAdcValue(void)
{
    // Reads ADC value, converts it to float (0-1.0)
    // and applies calibration correction.
    // Not yet calibrated therefore currently applying
    // a linear calibration factor 1.0.

    float analogValue = analogRead(A0) / 1023.0;
    float calibrationFactor = 1.0; 
    float calibratedValue = analogValue * calibrationFactor;

    // Example of possible calibration (will be different per ESP8266):
    // float calibrationFactor = 1.05; 
    // float calibratedValue = analogValue / calibrationFactor;    

    return calibratedValue;
}


void getSensorData(void)
{
    led.on();   // LED is used as 'reading sensors' indicator.

    if (useSerial)
    {
        Serial.println("Reading sensors.\n");
    }
    temperature = sht31.readTemperature();
    humidity = sht31.readHumidity();
    light = bh1750.readLightLevel();  

    // Using 1:5 voltage divider so for true value multiply by 5.
    // Note: For accurate ADC measurement Wifi should not be active.
    batteryMeasureEnable(true);
    batteryVoltage = getCalibratedAdcValue() * 5;
    batteryMeasureEnable(false);

    led.off();
}


void displaySensorData(void)
{
    // Convert values to strings and add units.
    char temperatureStr[13];
    char humidityStr[13];
    char lightStr[13];
    char batteryVoltageStr[13];    

    // Note: Arduino lacks float support in (s)printf.
    //       Using dtostrf() as workaround.
    dtostrf(temperature, 4, 2, floatbuf);   
    sprintf(temperatureStr, "%s C", floatbuf);

    dtostrf(humidity, 4, 2, floatbuf);
    sprintf(humidityStr, "%s %%", floatbuf);

    sprintf(lightStr, "%d lx", light);

    dtostrf(batteryVoltage, 4, 2, floatbuf);
    sprintf(batteryVoltageStr, "%s V", floatbuf);    

    // Write to display.
    uint8_t  lineNr = 2;
    display.clearLine(lineNr);
    display.setCursor(0, lineNr++);
    display.printf("temp: %s", temperatureStr);

    display.clearLine(lineNr);
    display.setCursor(0, lineNr++);
    display.printf("hum:  %s", humidityStr);

    display.clearLine(lineNr);
    display.setCursor(0, lineNr++);
    display.printf("lght: %s", lightStr);

    display.clearLine(lineNr);
    display.setCursor(0, lineNr++);
    display.printf("batt: %s", batteryVoltageStr);      

    // Write to serial port.
    if (useSerial) 
    {
        Serial.printf("Temperature: %s\n", temperatureStr);
        Serial.printf("Humidity:    %s\n", humidityStr);
        Serial.printf("Light:       %s\n", lightStr);
        Serial.printf("Battery:     %s\n\n", batteryVoltageStr);    
    }   
}


void displayTitle(const char* title)
{
    display.clearLine(0);
    display.inverse();
    display.drawString(0, 0, title);
    display.noInverse();

    if (useSerial) 
    {
        Serial.printf("\n\n%s started.\n\n", title);
    }          
}


void setup() 
{
    // Disable Wifi, we are not going to use it.
    // This saves some 52mA unnecessary power usage.
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();

    // Initialize the voltage divider on/off switch.
    pinMode(batteryMeasureEnablePin, OUTPUT_OPEN_DRAIN);
    batteryMeasureEnable(false);

    if (useSerial) 
    {
        // Initialize Serial port.
        Serial.begin(SERIAL_SPEED);
        delay(2000); // WORKAROUND: wait to let PlatformIO Serial Monitor catch up.        
    }

    // Initialize display.
    display.begin();
    display.setFont(u8x8_font_victoriamedium8_r);

    // Initialize temperature humidity sensor.
    sht31.begin(sht31Address);

    // Initialize light sensor.
    bh1750.begin();

    displayTitle(title);  
}


void loop() 
{
    uint32_t now = millis();

    if (now - lastMeasureTime >= measureIntervalMs || lastMeasureTime == 0)
    {
        lastMeasureTime = now;
        getSensorData(); 
        displaySensorData();
    }
}