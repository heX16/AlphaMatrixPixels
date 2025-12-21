#ifndef CONFIG_HPP_
#define CONFIG_HPP_

//WARN: only one definition: BUILD_ARDUINO_IDE or BUILD_VSCODE_PLATFORMIO_IDE
//#define BUILD_ARDUINO_IDE
#define BUILD_VSCODE_PLATFORMIO_IDE

// Library free version - library as copy in folder "LED_EFEN"
//todo: !!! #define BUILD_LED_EFEN_AS_FOLDER

//#define MODE_ARDUINO

// Library free version - library as copy in some folder
#define LIB_SMALL_TIMER_PATH "small_timer.hpp"

#ifdef DEBUG_SERIAL
#define DEBUGLN(x) Serial.println(x)
#define DEBUG(x) Serial.print(x)
#else
#define DEBUGLN(x)
#define DEBUG(x)
#endif

#endif // CONFIG_HPP_
