/* Some basic tests for testing the FM24CL64B 64 kb (8 kB) Serial I2C FRAM module. 
 * Use with Electrodragon ESP-12E/F adapter board or NodeMCU board.
 * By Leonel Lopes Parente
 * 
 * Uses LED on ESP-12 module which is connected to GPIO2.
 * Uses on-board flash button connected to GPIO0 to initiate test-runs.
 */

#include <ESP8266WiFi.h>
#include <Wire.h>

#include "FramI2C.h"
#include "FramI2CTools.h"
#include "Hexdump.h"
#include "Led.h"

const uint16_t debounceIntervalMs = 100;
const uint8_t flashbuttonPin = 0; 	// Hard-wired
volatile bool flashbuttonPressed = false;
const uint8_t ledPin = 2; 			// Hard wired

FramI2C fram1;
FramI2C fram2;

Led led(ledPin, Led::OffLevel::High);

Stream& stream = Serial;


uint32_t flashbuttonLastPressedTime = 0;
void flashbuttonHandler()
{
	uint32_t now = millis();

	if (now - flashbuttonLastPressedTime > debounceIntervalMs)
	{
		flashbuttonPressed = true;
		flashbuttonLastPressedTime = now;
	}	
}


void hexdumpFramTest(FramI2C& fram)
{
	stream.println("\n--- hexdumpFramTest --------------------------\n");

	uint16_t address = 0;
	size_t pageSize = fram.pageSize();
	size_t byteCount = 32;
    uint8_t firstValue = 0;	
    uint8_t secondValue = 0xFF;	

	stream.printf("This test fills all FRAM bytes (of 1st page) with 0x%02X, then shows hexdumps of lower and upper parts.\n", firstValue);
	stream.printf("Then fills all bytes with 0x%02X and shows hexdumps again.\n\n", secondValue);

	// --------------------------------

	stream.printf("Filling all 0x%X (%u) bytes with 0x%X\n", pageSize , pageSize, firstValue);
	fram.fill(address, pageSize, firstValue);
	stream.println();

	hexdumpFram(stream, fram, address, byteCount, true, "Lower part");
	hexdumpFram(stream, fram, (uint16_t)(pageSize - byteCount), byteCount, true, "Upper part");

	// --------------------------------

	stream.printf("\nFilling all 0x%X (%u) bytes with 0x%X\n", pageSize , pageSize, secondValue);
	fram.fill(address, pageSize, secondValue);
	stream.println();

	hexdumpFram(stream, fram, address, byteCount, true, "Lower part");
	hexdumpFram(stream, fram, (uint16_t)(pageSize - byteCount), byteCount, true, "Upper part");
}


void readWriteTest(FramI2C& fram)
{
	stream.println("\n--- readWriteTest ----------------------------\n");

	stream.println("This test clears part of FRAM and shows a hexdump.");
	stream.println("It then writes different types of variables to FRAM and shows a hexdump.");
	stream.println("It then reads each of these variables from FRAM and shows their value.\n");

	uint16_t address = 0;
	uint32_t byteCount = 6 * 16;
	uint8_t emptyValue = 0xEE;

	stream.printf("\nFilling FRAM at 0x%04X, 0x%X (%u) bytes with 0x%02X.\n\n", address, byteCount, byteCount, emptyValue);
	fram.fill(address, byteCount, emptyValue);	

	hexdumpFram(stream, fram, address, byteCount, true);

	uint16_t uint16Value = 0x1234;
	stream.printf("write() uint16 0x%X to address 0x%X\n\n", uint16Value, address);
	fram.write(address, uint16Value); 

	int16_t int16Value = -1234;
	stream.printf("write() int16 %i to address 0x%X\n\n", int16Value, address + 16);
	fram.write(address + 16, int16Value); 	

	uint32_t uint32Value = 0x12345678;
	stream.printf("write() uint32 0x%X to address 0x%X\n\n", uint32Value, address + 32);
	fram.write(address + 32, uint32Value); 

	float floatValue = 55555.77777;
	stream.printf("write() float 0x%f to address 0x%X\n\n", floatValue, address + 48);
	fram.write(address + 48, floatValue); 		

	hexdumpFram(stream, fram, address, byteCount, true);

	uint16Value = 0;
	stream.printf("read() uint16 from address 0x%X.\n", address);	
	fram.read(address, uint16Value);
	stream.printf("uint16 value is: 0x%X\n\n", uint16Value);

	int16Value = 0;
	stream.printf("read() int16 from address 0x%X.\n", address + 16);	
	fram.read(address + 16, int16Value); 
	stream.printf("int16 value is: %i\n\n", int16Value);

	uint32Value = 0;
	stream.printf("read() uint32 from address 0x%X.\n", address + 32);	
	fram.read(address + 32, uint32Value); 
	stream.printf("uint32 value is: 0x%X\n\n", uint32Value);

	floatValue = 0.0;
	stream.printf("read() float from address 0x%X.\n", address + 48);	
	fram.read(address + 48, floatValue); 
	stream.printf("float value is: %f\n", floatValue);	
}


void writeLargeTypeTest(FramI2C& fram)
{
	// Note: This test requires an FramI2C buffer size of at least 'dataSize'.
	 
	stream.println("\n--- writeLargeTypeTest -----------------------\n");

	stream.println("This test writes a large type to FRAM using the write() method.");
	stream.println("write() is normally used for integral types (e.g. int, float, uint8_t, double).");
	stream.println("write() can be used for larger types but that requires an FramI2C type buffer of equal size.\n");

	uint8_t data[] = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789";
	uint32_t dataSize = sizeof(data); //Note that this includes ending \0.

	if (dataSize > fram.typebufferSize())
	{
		stream.printf("This test requires a FramI2C buffer size of at least 0x%X (%u) bytes.\n", dataSize, dataSize);
		stream.printf("Current FramI2C buffer size is 0x%X (%u) bytes.\n", fram.typebufferSize(), fram.typebufferSize());
		return;
	}

	stream.printf("FramI2C type buffer size: 0x%X (%u) bytes\n\n", fram.typebufferSize(), fram.typebufferSize());

	stream.printf("The following data is used:\n\"%s\"\n", data);
	stream.printf("Data size is 0x%X (%u) bytes.\n\n", dataSize, dataSize);

	hexdump(stream, data, dataSize, true, "of data ('large type')", 2);	

	uint16_t address = 0x0000;
	uint32_t byteCount = 64;

	hexdumpFram(stream, fram, address, byteCount, true, "initial contents", 2);	

	uint8_t value = 0xEE;
	stream.printf("Filling FRAM at 0x%04X, 0x%X (%u) bytes with 0x%02X.\n\n", address, byteCount, byteCount, value);
	fram.fill(address, byteCount, value);		

	hexdumpFram(stream, fram, address, byteCount, true, "after clearing", 2);		

	stream.printf("write() data 0x%X (%u) bytes to FRAM.\n\n", dataSize, dataSize);
	fram.write(address, data);

	hexdumpFram(stream, fram, address, byteCount, true, "after writing data");	
}


void writeBytesTest(FramI2C& fram)
{
	stream.println("\n--- writeBytesTest ---------------------------\n");

	stream.println("This test writes a byte sequence to FRAM using the writeBytes() method.\n");	

	uint8_t data[] = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789";
	size_t dataSize = sizeof(data);
	uint16_t address = 0x0000;
	size_t byteCount = 64;	

	stream.printf("The following data is used:\n\"%s\"\n", data);
	stream.printf("Data size is 0x%X (%u) bytes.\n\n", dataSize, dataSize);

	hexdump(stream, data, dataSize, true, "of data", 2);

	hexdumpFram(stream, fram, address, byteCount, true, "initial contents", 2);	

	uint8_t emptyValue = 0xEE;
	stream.printf("fill() FRAM address 0x%04X, 0x%X (%u) bytes with value 0x%02X.\n\n", address, byteCount, byteCount, emptyValue);
	fram.fill(address, byteCount, emptyValue);

	hexdumpFram(stream, fram, address, byteCount, true, "after clearing", 2);		

	stream.printf("writeBytes() data 0x%X (%u) bytes to FRAM.\n", dataSize, dataSize);
	FramI2C::ResultCode resultcode = fram.writeBytes(address, dataSize, data);
	if (resultcode != FramI2C::ResultCode::Success)
	{
		printResultCodeDescription(stream, resultcode, 2);
		return;
	}	
	stream.println();

	hexdumpFram(stream, fram, address, byteCount, true, "after writing data");	
}


void readBytesTest(FramI2C& fram)
{
	stream.println("\n--- readBytesTest ----------------------------\n");

	stream.println("This test first writes a byte sequence to FRAM using the writeBytes() method");
	stream.println("and then reads it back from FRAM using the readBytes() method.\n");

	uint8_t data[] = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789";
	size_t dataSize = sizeof(data);
	uint16_t address = 0x0000;
	size_t byteCount = 64;	

	stream.printf("The following data is used:\n\"%s\"\n", data);
	stream.printf("Data size is 0x%X (%u) bytes.\n\n", dataSize, dataSize);

	hexdump(stream, data, dataSize, true, "of data", 2);

	uint8_t emptyValue = 0xEE;
	stream.printf("fill() FRAM address 0x%04X, 0x%X (%u) bytes with value 0x%02X.\n\n", address, byteCount, byteCount, emptyValue);
	fram.fill(address, byteCount, emptyValue);

	hexdumpFram(stream, fram, address, byteCount, true, "after clearing", 2);		

	stream.printf("writeBytes() data 0x%X (%u) bytes to FRAM.\n", dataSize, dataSize);
	FramI2C::ResultCode resultcode = fram.writeBytes(address, dataSize, data);
	if (resultcode != FramI2C::ResultCode::Success)
	{
		printResultCodeDescription(stream, resultcode, 2);
		return;
	}	
	stream.println();

	hexdumpFram(stream, fram, address, byteCount, true, "after writing data", 2);
	
	stream.printf("Filling all data bytes with 0x%02X.\n\n", emptyValue);
	for (size_t i = 0; i < dataSize; ++i)
	{
		data[i] = emptyValue;
	}

	hexdump(stream, data, dataSize, true, "of data after clearing", 2);	

	stream.printf("readBytes() 0x%X (%u) bytes from FRAM into data.\n", dataSize, dataSize);
	resultcode = fram.readBytes(address, dataSize, data);
	if (resultcode != FramI2C::ResultCode::Success)
	{
		printResultCodeDescription(stream, resultcode, 2);
		return;
	}	
	stream.println();

	hexdump(stream, data, dataSize, true, "of data after reading from FRAM");	
}


void pageTest(FramI2C& fram)
{
	stream.println("\n--- pageTest ---------------------------------\n");

	uint16_t address = 0;
	uint32_t pageSize = fram.pageSize();
	uint32_t dumpByteCount = 32;
    uint8_t emptyValue = 0xFF;

	stream.printf("This test first fills all bytes of each FRAM page with 0x%02X\n", emptyValue);
	stream.println("then shows hexdumps of lower and upper parts of each page.");
	stream.println("It then fills each page with a value mimicking its page number");
	stream.println("and then shows hexdumps again.\n");

	stream.printf("Number of FRAM pages: %u\n\n", fram.pageCount());

	// --------------------------------

	for (uint8_t page = 0; page < fram.pageCount(); ++page)
	{
		stream.printf("Filling all of page %u, 0x%X (%u) bytes with 0x%02X\n", page, pageSize, pageSize, emptyValue);
		fram.fill(page, address, pageSize, emptyValue);
	}
	stream.println("\n");

	for (uint8_t page = 0; page < fram.pageCount(); ++page)
	{		
		hexdumpFram(stream, fram, page, address, dumpByteCount, true);
		hexdumpFram(stream, fram, page, (uint16_t)(pageSize - dumpByteCount), dumpByteCount, true, "", 2);
	}

	// --------------------------------

	for (uint8_t page = 0; page < fram.pageCount(); ++page)
	{
		uint8_t value = page + (page << 4);
		stream.printf("Filling all of page %u, 0x%X (%u) bytes with 0x%02X\n", page, pageSize, pageSize, value);
		fram.fill(page, address, pageSize, value);
	}
	stream.println("\n");

	for (uint8_t page = 0; page < fram.pageCount(); ++page)
	{		
		hexdumpFram(stream, fram, page, address, dumpByteCount, true);
		hexdumpFram(stream, fram, page, (uint16_t)(pageSize - dumpByteCount), dumpByteCount, true, "", 2);
	}
}


void fillPerformanceTest(FramI2C& fram)
{
	stream.println("\n--- fillPerformanceTest ----------------------\n");

	uint16_t address = 0;
	uint32_t pageSize = fram.pageSize();
    uint8_t fillValue = 0x77;	

	stream.println("This test measures the performance of fill().\n");

	stream.printf("Filling all of page 0, 0x%X (%u) bytes with 0x%02X\n", pageSize, pageSize, fillValue);
	size_t started = millis();
	fram.fill(address, pageSize, fillValue);
	size_t duration = millis() - started;

	double durationPerByte = (double)duration / pageSize;
	size_t bytesPerSecond = 1000.0 / durationPerByte;
	stream.printf("\nFilled 0x%X (%u) bytes, duration %u ms, %u bytes/s, %f ms/byte\n", 
	              pageSize, pageSize, duration, bytesPerSecond, durationPerByte);	
}


void singleBytePerformanceTest(FramI2C& fram)
{
	stream.println("\n--- singleBytePerformanceTest ----------------\n");

	stream.println("This test measures the single byte read/write performance of read(), readBytes(), write() and writeBytes().\n");

	uint8_t page = 0;
	uint16_t address = 0;
	size_t pageSize = fram.pageSize();
    uint8_t value = 0;	
	size_t started = 0;
	size_t duration = 0;
	double durationPerByte = 0;
	size_t bytesPerSecond = 0;
	FramI2C::ResultCode resultcode;

// --------------------------------

	stream.printf("read() all 0x%X (%u) bytes of page %u\n", pageSize , pageSize, page);

	started = millis();
    address = 0;
    for (size_t i = 0; i < pageSize; ++i)
    {
        resultcode = fram.read(page, address, value);
        if (resultcode != FramI2C::ResultCode::Success)
        {
			printResultCodeDescription(stream, resultcode);
            return;
        }
        ++address;
#if defined(ESP8266)        
        // If ESP8266 MCU yield() every 32 iterations to prevent WDT reset. 
        if ((address % 32) == 0)
        {
            yield();
        }
#endif           
    }	
	duration = millis() - started;
	durationPerByte = (double)duration / pageSize;
	bytesPerSecond = 1000.0 / durationPerByte;
	stream.printf("0x%X (%u) bytes, duration %u ms, %u bytes/s, %f ms/byte\n", 
	              pageSize, pageSize, duration, bytesPerSecond, durationPerByte);	

// --------------------------------

    value = 0x08;
	stream.printf("\nreadBytes() all 0x%X (%u) bytes of page %u\n", pageSize , pageSize, page);

	started = millis();
    address = 0;
    for (size_t i = 0; i < pageSize; ++i)
    {
        resultcode = fram.readBytes(page, address, 1, &value);
        if (resultcode != FramI2C::ResultCode::Success)
        {
			printResultCodeDescription(stream, resultcode);
            return;
        }
        ++address;
#if defined(ESP8266)        
        // If ESP8266 MCU yield() every 32 iterations to prevent WDT reset. 
        if ((address % 32) == 0)
        {
            yield();
        }
#endif           
    }	
	duration = millis() - started;
	durationPerByte = (double)duration / pageSize;
	bytesPerSecond = 1000.0 / durationPerByte;
	stream.printf("0x%X (%u) bytes, total duration %u ms, %u bytes/s, %f ms/byte\n", 
	              pageSize, pageSize, duration, bytesPerSecond, durationPerByte);	

// --------------------------------

    value = 0x02;
	stream.printf("\nwrite() all 0x%X (%u) bytes of page %u with 0x%X\n", pageSize , pageSize, page, value);

	started = millis();
    address = 0;
    for (size_t i = 0; i < pageSize; ++i)
    {
        resultcode = fram.write(page, address, value);
        if (resultcode != FramI2C::ResultCode::Success)
        {
			printResultCodeDescription(stream, resultcode);
            return;
        }
        ++address;
#if defined(ESP8266)        
        // If ESP8266 MCU yield() every 32 iterations to prevent WDT reset. 
        if ((address % 32) == 0)
        {
            yield();
        }
#endif           
    }	
	duration = millis() - started;
	durationPerByte = (double)duration / pageSize;
	bytesPerSecond = 1000.0 / durationPerByte;
	stream.printf("0x%X (%u) bytes, duration %u ms, %u bytes/s, %f ms/byte\n", 
	              pageSize, pageSize, duration, bytesPerSecond, durationPerByte);	

// --------------------------------

    value = 0x08;
	stream.printf("\nwriteBytes() all 0x%X (%u) bytes of page %u with 0x%X\n", pageSize , pageSize, page, value);

	started = millis();
    address = 0;
    for (size_t i = 0; i < pageSize; ++i)
    {
        resultcode = fram.writeBytes(page, address, 1, &value);
        if (resultcode != FramI2C::ResultCode::Success)
        {
			printResultCodeDescription(stream, resultcode);
            return;
        }
        ++address;
#if defined(ESP8266)        
        // If ESP8266 MCU yield() every 32 iterations to prevent WDT reset. 
        if ((address % 32) == 0)
        {
            yield();
        }
#endif           
    }	
	duration = millis() - started;
	durationPerByte = (double)duration / pageSize;
	bytesPerSecond = 1000.0 / durationPerByte;
	stream.printf("0x%X (%u) bytes, duration %u ms, %u bytes/s, %f ms/byte\n", 
	              pageSize, pageSize, duration, bytesPerSecond, durationPerByte);					  
}



void doTests(FramI2C& fram, const char* const instanceName = "FramI2C")
{
	stream.println("\n\n=== Tests ====================================\n");
	stream.printf("doTests() - Running tests for instance %s\n\n", instanceName);

	printFramInfo(stream, fram, instanceName);

	hexdumpFramTest(fram);
	// readWriteTest(fram);
	// writeLargeTypeTest(fram);
	// writeBytesTest(fram);
	// readBytesTest(fram);
	// pageTest(fram);
	// fillPerformanceTest(fram);
	// singleBytePerformanceTest(fram);
}


void waitForever()
{
	while (true)
	{
		delay(1000);
	}   
}


void setup() 
{
	WiFi.mode(WIFI_OFF);	// Disable Wifi because not needed
	WiFi.forceSleepBegin();	
	yield();

	Serial.begin(74880);	// 74880 so we can read the ESP8266 boot messages
	while (!Serial);		// Wait until Serial ready

	delay(1000); // WORKAROUND: Give PlatformIO Serial monitor some time to pickup the first output

	stream.println("\n\n==============================================\n");

	stream.println("FramI2C Tests\n");

    pinMode(flashbuttonPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(flashbuttonPin), flashbuttonHandler, FALLING);

	Wire.begin();
	// Wire.setClock(400000L);	//Sets I2C clock to 400 kHz (default is 100 kHz).

	FramI2C::ResultCode resultcode;

	// resultcode = fram.begin(16);
	// resultcode = fram.begin(64);
	// resultcode = fram.begin(1024);
	// resultcode = fram.begin(1024, 0x50, 0x4000);
	// resultcode = fram.begin(1024, 0x50, 0x10001);  //This will fail at least because > pageSize

	resultcode = fram1.begin(64);
	if (resultcode != FramI2C::ResultCode::Success)
	{
		stream.println("fram1.begin() failed.");
		printResultCodeDescription(stream, resultcode);
		waitForever();   
	}
	printFramInfo(stream, fram1, "fram1");	

	resultcode = fram2.begin(1024, 0x52);
	if (resultcode != FramI2C::ResultCode::Success)
	{
		stream.println("fram2.begin() failed.");
		printResultCodeDescription(stream, resultcode);
		waitForever();
	}	
	printFramInfo(stream, fram2, "fram2");

	stream.println("Press Flash button to run the tests\n");	
}


void loop() 
{
	if (flashbuttonPressed)
	{
		flashbuttonPressed = false;  //We now handle it so clear flag
		stream.println("Flashbutton pressed");
		doTests(fram1, "fram1");
		doTests(fram2, "fram2");
		stream.println("\n--- Test(s) completed ---\n\nPress Flash button to repeat the tests\n");
		// stream.flush();
	}
}