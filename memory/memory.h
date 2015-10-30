#ifndef __MEMORY_H_
#define __MEMORY_H_

#define MEMORY_SIZE 256
#define MEMORY_FILE "./memory.dat"

/* inicializa a memoria */
int memory_init();

int memory_write(unsigned int address, unsigned char *value, int len);

int memory_write_byte(unsigned int address, unsigned char *value);

int memory_write_halfword(unsigned int address, unsigned short *value);

int memory_write_word(unsigned int address, unsigned int *value);

int memory_read(unsigned int address, unsigned char *value, int len);

int memory_read_byte(unsigned int address, unsigned char *value);

int memory_read_halfword(unsigned int address, unsigned short *value);

int memory_read_word(unsigned int address, unsigned int *value);

#endif /* __MEMORY_H_ */