#include "Arduino.h"

// Example: Storing struct data in RTC user rtcDataory
//
// Struct data with the maximum size of 512 bytes can be stored
// in the RTC user rtcDataory using the ESP-specifc APIs.
// The stored data can be retained between deep sleep cycles.
// However, the data might be lost after power cycling the ESP8266.
//
// This example uses deep sleep mode, so connect GPIO16 and RST
// pins before running it.
//
// Based on an example by Macro Yau.


// Structure which will be stored in RTC memory.
// First field is CRC32, which is calculated based on the
// rest of structure contents.
// Any fields can go after crc32.
// Byte array is used as example.

const size_t userRtcRamSize = 512;
struct RtcData 
{
  uint32_t crc32;
  uint8_t data[userRtcRamSize - 4];
};

Stream& stream = Serial;
RtcData rtcData;


uint32_t calculateCrc32(const uint8_t* const data, const size_t size)
{
  const uint8_t* elementPtr = data;
  size_t restSize = size;
  uint32_t crc = 0xffffffff;

  while (restSize--) 
  {
    uint8_t element = *elementPtr++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) 
    {
      bool bit = crc & 0x80000000;
      if (element & i) 
      {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) 
      {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}


// Prints all rtcData, including the leading crc32
void printRtcData(Stream& stream) 
{
  char buf[3];
  uint8_t *ptr = (uint8_t *)&rtcData;
  for (size_t i = 0; i < sizeof(rtcData); i++) 
  {
    sprintf(buf, "%02X", ptr[i]);
    stream.print(buf);
    if ((i + 1) % 32 == 0) 
    {
      stream.println();
    } 
    else 
    {
      stream.print(" ");
    }
  }
  stream.println();
}


void fillWithRandomBytes(uint8_t* const data, const size_t size)
{
  for (size_t i = 0; i < size; i++)
  {
    data[i] = random(0, 128);
  }
}


void setup() 
{
  Serial.begin(74880);
  delay(5000); 

  stream.println("\n\nStarted\n");

  stream.println("Reading data from RTC ram.");
  if (!ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) 
  {
    // TODO: ABORT
  }
  stream.println("Calculating CRC32 from RTC data.");
  uint32_t crc32 = calculateCrc32((uint8_t*) &rtcData.data[0], sizeof(rtcData.data)); //TODO: Check if sizeof works correctly here
//TODO: can this be used instead?: crc32 = calculateCrc32(rtcData.data, sizeof(rtcData.data));

  stream.print("Calculated CRC32: ");
  stream.print(crc32, HEX);
  stream.print(", CRC32 in RTC data: ");
  stream.println(rtcData.crc32, HEX);

  if (crc32 != rtcData.crc32) 
  {
    stream.println("Calculated CRC32 does not match CRC32 in RTC data, data is Invalid.");
  } 
  else 
  {
    stream.println("Calculated CRC32 matches CRC32 in RTC data, data assumed Valid.");
  }

  stream.println("Data read from RTC ram:");
  printRtcData(stream);

  // Generate new random data
  stream.println("Filling data with random values.");
  fillWithRandomBytes(rtcData.data, sizeof(rtcData.data));

  stream.println("Calculating CRC32 and storing it into data.");
  rtcData.crc32 = calculateCrc32((uint8_t*) &rtcData.data[0], sizeof(rtcData.data));
//TODO: can this be used instead?: rtcData.crc32 = calculateCrc32(rtcData.data, sizeof(rtcData.data));




  stream.println("New data:");
  printRtcData(stream);

  // Write data to RTC ram
  stream.println("Writing new data to RTC ram.");  
  if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) 
  {
    stream.println("Write Succeeded.");
  }
  else
  {
    stream.println("Write Failed.");
  }

  stream.println("\nEntering deep sleep for 5 seconds");
  ESP.deepSleep(5 * 1000000);
}

void loop() {
}


