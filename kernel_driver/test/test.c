#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE (512*342*3)

#include "bdc.xbm"

int main(int argc, char** argv) {
	char *fb;
	char *fbp;
	char n;
	int x;
	int p;
	int fd=open("/dev/fb0", O_RDWR);
	if (fd<0) perror("opening fb");
	fb=mmap(NULL, SIZE, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	if (fb==MAP_FAILED) {
		perror("Mmap failed");
		exit(0);
	}
	fbp=fb;
	for (x=0; x<((512*342)/8); x++) {
		for (p=0; p<8; p++) {
			if (bdc_bits[x]&(1<<p)) n=255; else n=0;
			*(fbp++)=n;
			*(fbp++)=n;
			*(fbp++)=n;
		}
	}
	munmap(fb, SIZE);
	close(fd);
	
}