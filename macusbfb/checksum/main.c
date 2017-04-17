#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char** argv) {
	int fd;
	unsigned char* file;
	int i;
	unsigned long ivt_CRC;

	if (argc!=2) {
		printf("Usage: %s file.bin\n");
		exit(0);
	}

	fd=open(argv[1], O_RDWR);
	if (fd<0) {
		perror("opening file");
		exit(1);
	}
	file=(char*)mmap(NULL, 1024, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	if (file==(unsigned char*)-1) {
		perror("mmap");
		exit(1);
	}

	ivt_CRC = 0;
	/* Calculate a native checksum of the little endian vector table: */
	for(i = 0; i < (7*4);) {
		ivt_CRC += file[i++];
		ivt_CRC += file[i++] << 8;
		ivt_CRC += file[i++] << 16;
		ivt_CRC += file[i++] << 24;
	}
	/* Negate the result and place in the vector at 0x14 as little endian
	 * again. The resulting vector table should checksum to 0. */
	ivt_CRC = (unsigned long)-ivt_CRC;
	for(i = 0; i < 4; i++) {
		file[i + (7*4)] = (unsigned char)(ivt_CRC >> (8 * i));
	}


	/* Check checksum: */
	ivt_CRC = 0;
	for(i = 0; i < (0x20);) {
		ivt_CRC += file[i++];
		ivt_CRC += file[i++] << 8;
		ivt_CRC += file[i++] << 16;
		ivt_CRC += file[i++] << 24;
	}
	printf("Checksum over vector table now is %lu.\n",ivt_CRC);


	munmap(file, 1024);
	close(fd);
}

