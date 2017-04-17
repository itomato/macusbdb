//Macros to quickly modify gpio values

#define GPIOMOD(port, mask, bits) (* (pREG32 (0x50000000+(0x10000*port)+(mask<<2))))=bits
#define GPIOSET(port, bits) GPIOMOD(port, (1<<bits), (1<<bits));
#define GPIOCLEAR(port, bits) GPIOMOD(port, (1<<bits), 0);
