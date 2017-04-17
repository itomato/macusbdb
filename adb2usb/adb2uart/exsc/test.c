#include <stdio.h>

#include "scancodes.h"

int main() {
	int x;
	for (x=0; x<128; x++) {
		printf("\t\t{%i, %i},\n", scancodes[x].adb, scancodes[x].usb);
	}
}