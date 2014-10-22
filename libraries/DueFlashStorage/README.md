# DueFlashStorage
DueFlashStorage saves non-volatile data for Arduino Due. The library is made to be similar to the EEPROM library. Uses flash block 1 per default.

### Features
- Non-volatile data storage. Resetting or loss of power to the Arduino will not affect the data.
- Similar to the standard EEPROM library
- Write and read byte by byte
- Write and read byte arrays to store arbitrary variable types (strings, structs, integers, floats)

Note: The flash storage is reset every time you upload a new sketch to your Arduino.

Inspiration and some code from Pansenti at https://github.com/Pansenti/DueFlash

## Install
Create a new folder in your Arduino sketch folder named DueFlashStorage. 
Download and put all files from this repository into the folder. 

## Use
### Basic use
```cpp
// write the value 123 to address 0
dueFlashStorage.write(0,123);

// read byte at address 0
byte b = dueFlashStorage.read(0);
```

### Advanced use to store configuration parameters
```cpp
// say you want to store a struct with parameters:
struct Configuration {
  uint8_t a;
  uint8_t b;
  int32_t bigInteger;
  char* message;
  char c;
};
Configuration configuration;

// then write it to flash like this:
byte b2[sizeof(Configuration)]; // create byte array to store the struct
memcpy(b2, &configuration, sizeof(Configuration)); // copy the struct to the byte array
dueFlashStorage.write(4, b2, sizeof(Configuration)); // write byte array to flash at address 4

// and read from flash like this:
byte* b = dueFlashStorage.readAddress(4); // byte array which is read from flash at adress 4
Configuration configurationFromFlash; // create a temporary struct
memcpy(&configurationFromFlash, b, sizeof(Configuration)); // copy byte array to temporary struct

/* see example code for a working example */
```

## Examples
### DueFlashStorageExample.cpp
This example will write 3 bytes to 3 different addresses and print them to the serial monitor.
Try resetting the Arduino Due or unplug the power to it. The values will stay stored.
   
### DueFlashStorageStructExample.cpp
This example will write a struct to memory which is a very convinient way of storing configuration parameters.
Try resetting the Arduino Due or unplug the power to it. The values will stay stored.
