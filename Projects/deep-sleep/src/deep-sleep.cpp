#include <Arduino.h>

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#include <U8x8lib.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "Led.h"

#include "../../../config/wifisettings.h"
#include "../../../config/mqttsettings.h"

// platformio.ini 
//   monitor_speed = 74880
//   libdeps = 
//       U8g2

size_t startedTime                   = millis();

const bool useSerial                 = false;
const bool disableDisplayDuringSleep = false;
const uint64_t sleepDurationUs       = 30 * 1000 * 1000;  // Microseconds
const uint16_t debounceIntervalMs    = 100;
const uint32_t serialSpeed           = 74880;    // 74880 so we can read the ESP8266 boot messages

const uint8_t  flashButtonPin        = 0;        // Hard-wired to GPIO0
volatile bool  flashButtonPressed    = false;    // Volatile because modified in interrupt handler
const uint8_t  buttonPin             = 12;       // GPIO12
volatile bool  buttonPressed         = false;    // Volatile because modified in interrupt handler

IPAddress staticIP = 0U;
IPAddress staticSubnet = 0U;
IPAddress staticGateway = 0U;
IPAddress staticDns = 0U;

U8X8_SSD1306_128X64_NONAME_HW_I2C display(/*reset*/ U8X8_PIN_NONE, /*clock*/ SCL, /*data*/ SDA);
Led led(13, Led::ActiveLevel::Low);             // External LED connected to GPIO13
Led builtinLed(2, Led::ActiveLevel::Low);       // LED on ESP-12F is connected to GPIO2


void disableWifi()
{
	WiFi.mode(WIFI_OFF);
	WiFi.forceSleepBegin();
	yield();    // Is yield() sufficient or does this require some delay()?	 	
}


void enableWifi()
{
	WiFi.forceSleepWake();
	yield();    // Is yield() sufficient or does this require some delay()?	 	
}


void configWifi()
{
    // Note: Wifi mode must be WIFI_STA or WIFI_AP_STA otherwise Wifi.config will fail

    // How to set fixed IP:
    // staticIP = 
    // staticGateway =
    // staticSubnet = 
    // staticDns =
    // WiFi.config(staticIP, staticGateway, staticSubnet, staticDns /*, staticDns2 */ );
}

void connectWifi()
{
    // Note: When Wifi is disabled then 
    // enable it here first before connecting 

	WiFi.mode(WIFI_STA);
    configWifi();
	WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);	
	yield();    // Is yield() sufficient or does this require some delay()?	 	
}


void disConnecWifi()
{
	WiFi.disconnect(true);
	yield();    // Is yield() sufficient or does this require some delay()?	 	
}


void deepSleep(uint64_t sleeptime_us, RFMode rfMode)
{
    // Note:      use rfMode WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
    //            Now and then RFMode RF_CAL should be used when RF-CAL has 
    //            not been performed / has been disabled for a while.
    //            Need for calibration appears to be also dependent on supply voltage.
    //
    // Important: RF_CAL should be enabled once after every N deep-sleeps 
    //            and possibly after each delta-V change of input power.
    // TODO:      (Empirically) determine proper values for N and delta-V.


    // If there is anything in the serial buffer that should be output
    // before entering sleep (otherwise it gets lost) flush serial here:
    // (Useful for testing but in most real life low-power situations
    // the serial port will/shall not be used for notifications or logging.)
    // Serial.flush();

    // saveStateToRtcRam();
    // saveStateToFram();

    if (useSerial)
    {
        Serial.printf("\nEntering sleep for %llu seconds\n", sleeptime_us / 1000000);
        Serial.flush();
    }
    display.printf("Entering sleep\n");
    if (disableDisplayDuringSleep)
    {
        display.setPowerSave(true);
    }
    ESP.deepSleep(sleeptime_us, rfMode);
}

// enum rst_reason {
//     REASON_DEFAULT_RST      = 0,    /* normal startup by power on */
//     REASON_WDT_RST          = 1,    /* hardware watch dog reset */
//     REASON_EXCEPTION_RST    = 2,    /* exception reset, GPIO status won’t change */
//     REASON_SOFT_WDT_RST     = 3,    /* software watch dog reset, GPIO status won’t change */
//     REASON_SOFT_RESTART     = 4,    /* software restart ,system_restart , GPIO status won’t change */
//     REASON_DEEP_SLEEP_AWAKE = 5,    /* wake up from deep-sleep */
//     REASON_EXT_SYS_RST      = 6     /* external system reset */
// };

// struct rst_info
// {
//     uint32 reason;
//     uint32 exccause;
//     uint32 epc1;
//     uint32 epc2;
//     uint32 epc3;
//     uint32 excvaddr;
//     uint32 depc;
// };


const char* const RST_REASONS[] = 
{
  "Power-on Reset",
  "Hardware Watchdog Timer",
  "Exception Reset",
  "Software Watdog Timer",
  "Software Restart",
  "Deep Sleep Awake",
  "External Reset"
};

const char* const FLASH_SIZE_MAP_NAMES[] = {
  "FLASH_SIZE_4M_MAP_256_256",
  "FLASH_SIZE_2M",
  "FLASH_SIZE_8M_MAP_512_512",
  "FLASH_SIZE_16M_MAP_512_512",
  "FLASH_SIZE_32M_MAP_512_512",
  "FLASH_SIZE_16M_MAP_1024_1024",
  "FLASH_SIZE_32M_MAP_1024_1024"
};

const char* const WL_STATUS[] =
{
    "Idle",
    "No SSID available",
    "Scan completed",
    "Connected",
    "Connect failed",
    "Connection lost",
    "Disconnected"
};

void print_system_info(Stream& stream) 
{
    // rst_info* resetInfo = system_get_rst_info();

    stream.println();
    // stream.print(F("system_get_rst_info() reset reason: "));
    // stream.println(RST_REASONS[resetInfo->reason]);

    stream.print(F("system_get_free_heap_size(): "));
    stream.println(system_get_free_heap_size());

    // stream.print(F("system_get_os_print(): "));
    // stream.println(system_get_os_print());

    // system_set_os_print(1);

    // stream.print(F("system_get_os_print(): "));
    // stream.println(system_get_os_print());

    // system_print_meminfo();   // ### Eh, to what?!

    stream.print(F("system_get_chip_id(): 0x"));
    stream.println(system_get_chip_id(), HEX);

    // stream.print(F("system_get_sdk_version(): "));
    // stream.println(system_get_sdk_version());

    // stream.print(F("system_get_boot_version(): "));
    // stream.println(system_get_boot_version());

    // stream.print(F("system_get_userbin_addr(): 0x"));
    // stream.println(system_get_userbin_addr(), HEX);

    // stream.print(F("system_get_boot_mode(): "));
    // stream.println(system_get_boot_mode() == 0 ? F("SYS_BOOT_ENHANCE_MODE") : F("SYS_BOOT_NORMAL_MODE"));

    stream.print(F("system_get_cpu_freq(): "));
    stream.println(system_get_cpu_freq());

    // stream.print(F("system_get_flash_size_map(): "));
    // stream.println(FLASH_SIZE_MAP_NAMES[system_get_flash_size_map()]);
}


uint32_t lastFlashButtonPressTime = 0;
void flashButtonHandler()
{
	uint32_t now = millis();
	if (now - lastFlashButtonPressTime > debounceIntervalMs)
	{
		flashButtonPressed = true;
		lastFlashButtonPressTime = now;
	}	
}


uint32_t lastButtonPressTime = 0;
void buttonHandler()
{
	uint32_t now = millis();
	if (now - lastButtonPressTime > debounceIntervalMs)
	{
		buttonPressed = true;
		lastButtonPressTime = now;  
	}	
}


void setup() 
{
    // Determine reset cause
    rst_info* resetInfo = system_get_rst_info();
    
    if (useSerial)
    {
        Serial.begin(serialSpeed);

        delay(2000); //WORKAROUND: 'wait' for PlatformIO Serial Monitor to be displayed

        Serial.println("\n\nStarted.\n");
    }

    // Initialize display
    display.begin();
    display.setFont(u8x8_font_victoriamedium8_r);  

    display.printf("Started\n");
    display.printf(" \n");

    switch (resetInfo->reason)
    {
        case rst_reason::REASON_DEEP_SLEEP_AWAKE:   // Awoken from deep-sleep
            builtinLed.flash(2);
            break;
        case rst_reason::REASON_WDT_RST:            // Hardware WDT reset
        case rst_reason::REASON_SOFT_WDT_RST:       // Software WDT reset
            builtinLed.flash(4);
            break;
        default:
            builtinLed.flash(3);
    }

    if (useSerial) Serial.printf("Reset reason: %s\n\n", RST_REASONS[resetInfo->reason]);    
    display.printf("Reset reason:\n%s\n \n", RST_REASONS[resetInfo->reason]);

    wl_status_t wifiStatus = WiFi.status();
    if (useSerial) Serial.printf("Wifi status: %s\n\n", WL_STATUS[wifiStatus]);  

    // Disable Wifi persistence to prevent damaging flash EEPROM wear issues.
    // Check WiFi class documentation for more information.
    WiFi.persistent(false);  

    // if (/* TODO: check if Wifi is currently enabled (don't rely on assumptions about the boatloader) */ true)
    // {
    disableWifi();
    // }

    // TODO: Check RTCRAM content validity/integrity
    //       Read RTCRAM values for any needed state    

    // TODO: Check if FRAM is available
    //       Check RTCRAM content validity/integrity
    //       Read RTCRAM values for any needed state    

    // TODO: Check if the last Wifi connection was successful.
    //       if not, was there a clear reason for that?
    //       Is it worth retrying or does it need fix 'by user' e.g. incorrect Wifi passphrase
    //       Possibly check if the configured SSID Wifi network is available or not.
    //       Are there any actions that have yet to be performed because they failed the last time(s)?     

    pinMode(flashButtonPin, INPUT); // Has external pull-up on adapter board
    attachInterrupt(digitalPinToInterrupt(flashButtonPin), flashButtonHandler, FALLING);

    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonHandler, FALLING);
}


void loop() 
{
    // Awake, do your thing:

    led.on();   // 'doing work' indicator  
    if (useSerial) Serial.println("Doing work.");
    display.printf("Doing work\n \n");
    startedTime = millis();
    do
    {
        if (flashButtonPressed)
        {
            flashButtonPressed = false;     // Reset flag
            builtinLed.toggle();
            if (useSerial) Serial.println("Flash button pressed.");
            display.printf("flash button\n");
        }

        if (buttonPressed)
        {
            buttonPressed = false;  // Reset flag
            // led.toggle();
            if (useSerial) Serial.println("\nExternal button pressed.");
            display.printf("external button\n");


            // if (useSerial) print_system_info(Serial);

            // // Generate Software WDT for testing
            // if (useSerial) 
            // {
            //     Serial.println("\nCausing a software WDT\n");
            //     Serial.flush();
            // }            
            // int dummy = 0;
            // while (true)
            // {
            //     ++dummy;
            //     --dummy;
            // }


            // // Generate Hardware WDT for testing
            // if (useSerial) 
            // {
            //     Serial.println("\nCausing a hardware WDT\n");
            //     Serial.flush();
            // }            
            // ESP.wdtDisable();   // Disable soft WDT
            // int dummy = 0;
            // while (true)
            // {
            //     ++dummy;
            //     --dummy;
            // }


            // Do a restart for testing
            if (useSerial) 
            {
                Serial.println("\nDoing a Restart\n");
                Serial.flush();
            }
            ESP.restart();

            // // Do a reset for testing
            // if (useSerial) 
            // {
            //     Serial.println("\nDoing a Reset\n");
            //     Serial.flush();
            // }
            // ESP.reset();

        }    
        delay(50);

    } while ((millis() - startedTime) < 5000);    // Simulate work for 5 seconds


    // Enter deepsleep for sleepDurationUs microseconds with RF disabled at wakeup.
    // ### TODO: Check if RF is disabled only if reset reason is deepsleep wakeup or for any reset reason.
    deepSleep(sleepDurationUs, WAKE_RF_DISABLED);
}


/*
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
        yield();


        Just before you choose to connect:

        WiFi.forceSleepWake();
        yield();

        // Bring up the WiFi connection
        WiFi.mode(WIFI_STA);
        WiFi.begin(WLAN_SSID, WLAN_PASSWD);
        And, when you go to sleep, tell the system to keep the radio off by default:

        WiFi.disconnect(true);
        delay( 1 );

        // WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
        ESP.deepSleep( SLEEPTIME, WAKE_RF_DISABLED );
        Part 3 of this blog also notes that if you will continually be connecting to 
		the same WiFi network, you can tell the ESP8266 to not spend the time loading the 
		previous connection's details from flash memory, but requiring you to provide them in the connection call:

        WiFi.forceSleepWake();
        yield(1);

        // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
        WiFi.persistent( false );

        WiFi.mode( WIFI_STA );
        WiFi.begin( WLAN_SSID, WLAN_PASSWD );



        #include <ESP8266WiFi.h>

        const char *ssid =      "YourSSID";      
        const char *pass =      "YourPassword";

        // Update these with values suitable for your network.
        IPAddress ip(192,168,0,128);  //Node static IP
        IPAddress gateway(192,168,0,1);
        IPAddress subnet(255,255,255,0);

        void setup()
        {
        WiFi.begin(ssid, pass);
        WiFi.config(ip, gateway, subnet);

        //Wifi connection
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("");
        Serial.print("WiFi connected, using IP address: ");
        Serial.println(WiFi.localIP());`
        }        
*/