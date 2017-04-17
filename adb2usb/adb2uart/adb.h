#define C_TALK  0xC
#define C_LISTN 0x8
#define C_RESET 0x0 //?
#define C_FLUSH 0x1 //?

#define REG_0 0
#define REG_1 1
#define REG_2 2
#define REG_3 3

//PB6
#define ADBPIN 6


int adbCommand(unsigned char cmd, unsigned char *data, int size);
void adbReset();
