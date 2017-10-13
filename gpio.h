#include <stdint.h>

#define GPIO_IN  0
#define GPIO_OUT 1

#define GPIO_LOW  0
#define GPIO_HIGH 1

int GPIOExport(int pin);

int GPIOUnexport(int pin);

int GPIODirection(int pin, int dir);

int GPIORead(int pin);

int GPIOWrite(int pin, int value);
