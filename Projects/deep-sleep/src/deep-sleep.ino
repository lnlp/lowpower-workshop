/******************************************************************************
 * 
 * File:          deep-sleep.ino
 * 
 * Description:   This will put the ESP8266 into deep-sleep, automatically
 *                wake up after a sleep period, simulate doing some work
 *                by lighting a LED and pausing, enter deep-sleep again and
 *                repeat this endlessly.
 *                When (re)starting the ESP12E/F builtin LED will flash
 *                a certain number of times (reset reason dependent).
 * 
 * Purpose:       Demonstrate ESP8266 deep-sleep with wake up from
 *                automatic watchdog timer and wake up from external reset.
 *                Show relation between ESP8266 Reset Reason and deep-sleep.
 * 
 * Context:       Low-power workshop.
 * 
 * Requirements:  Hardware:
 *                - ESP8266 ESP12E/F module,
 *                - ESP8266 adapter board with on-board reset button 
 *                  (momentary pushbutton to ground),
 *                - For automatic wake by watchdog timer it is required that 
 *                  the Reset pin is connected to GPIO16.
 *                - External LED with resistor connected to +3.3V and GPIO 13.
 *                Software: Arduino framework.
 * 
 * Dependencies:  Uses the following external library: Led.
 * 
 * Remarks:       When the ESP8266 wakes up from deep-sleep it actually resets.
 *                Different from other MCU's like AVR ATmega328, the ESP8266
 *                will not maintain program state, RAM contents and GPIO status
 *                (only the RTC counter and RTC RAM contents are maintained).
 * 
 *                Instead of automatic wake up by a watchdog timer, the ESP8266
 *                can also be woken up by an external reset signal (on reset pin).
 *                An external reset can be triggered manually with a reset button 
 *                or (automatically) by some external device.
 *                An external reset while the ESP8266 is in deep-sleep will result
 *                in reset reason REASON_DEEP_SLEEP_AWAKE (even after a sketch was
 *                uploaded while it was in deep-sleep) and REASON_EXT_SYS_RST otherwise.
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

#include <Arduino.h>
extern "C" {
#include "user_interface.h"
}
#include "ESP8266WiFi.h"
#include "Led.h"

uint32_t startedTime = millis();

const uint64_t sleepDurationUs = 15 * 1000 * 1000;  // Microseconds
const uint32_t workDurationMs = 5 * 1000;           // Milliseconds
const uint32_t serialSpeed = 74880;                 // 74880 to read boot messages

Led builtinLed(2, Led::ActiveLevel::Low);           // ESP-12E/F builtin LED.
Led led(13, Led::ActiveLevel::Low);                 // External LED connected to GPIO13


void deepSleep(uint64_t sleeptimeUs, RFMode rfMode)
{
    Serial.printf("\nmillis(): %lu\n\n", millis());
    Serial.printf("Entering sleep for %llu microseconds\n", sleeptimeUs / 1000);
    Serial.flush();

    // ### If anything needs to be done before entering deep-sleep
    // e.g. store values in RTC RAM - do it here. ###

    // Not using WiFi so we can use deepSleepInstant() instead of deepSleep()
    // which sleeps instantly without waiting for WiFi to shutdown.
    ESP.deepSleepInstant(sleeptimeUs, rfMode); 
}


const char* const ResetReasons[] = 
{                                 // rst_reason enum values defined in user_interface.h:
    "Power-on Reset",             // REASON_DEFAULT_RST      = 0
    "Hardware Watchdog Timer",    // REASON_WDT_RST          = 1
    "Exception Reset",            // REASON_EXCEPTION_RST    = 2, GPIO status won’t change
    "Software Watdog Timer",      // REASON_SOFT_WDT_RST     = 3, GPIO status won’t change
    "Software Restart",           // REASON_SOFT_RESTART     = 4, GPIO status won’t change
    "Deep Sleep Awake",           // REASON_DEEP_SLEEP_AWAKE = 5
    "External Reset"              // REASON_EXT_SYS_RST      = 6
};


void setup() 
{
    // Disable Wifi, we are not going to use it.
    // This saves some 52mA unnecessary power usage.
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();

    // Get reset info.
    rst_info* resetInfo = system_get_rst_info();
    
    Serial.begin(serialSpeed);
    delay(2000); //WORKAROUND: wait to let PlatformIO Serial Monitor catch up.

    Serial.println("\n\nStarted.\n");
    Serial.printf("Millis(): %u\n\n", startedTime);
    Serial.printf("Reset reason: %s\n\n", ResetReasons[resetInfo->reason]);    

    switch (resetInfo->reason)
    {
        case rst_reason::REASON_DEEP_SLEEP_AWAKE:   // Awoken from deep-sleep
            builtinLed.flash(3);

            // ### If anything needs to be done to recover from deep-sleep
            // e.g. restore values from RTC RAM - do it here. ###

            break;

        case rst_reason::REASON_EXCEPTION_RST:  // Exception Reset
        case rst_reason::REASON_SOFT_WDT_RST:   // Software WDT
        case rst_reason::REASON_SOFT_RESTART:   // Software Restart
            // These three reset types have in common that GPIO status won't change.
            builtinLed.flash(2);
            break; 

        case rst_reason::REASON_DEFAULT_RST:   // Power-on Reset
        case rst_reason::REASON_EXT_SYS_RST:   // External Reset
            builtinLed.flash(1);
            break;
    }
}


void loop() 
{
    // Simulate doing some work.
    led.on();   // 'doing work' indicator  
    Serial.println("Doing work.");
    delay(workDurationMs);  // Fake that work takes time.

    // Enter deepsleep for sleepDurationUs microseconds with RF disabled at wakeup.
    deepSleep(sleepDurationUs, WAKE_RF_DISABLED);
}
