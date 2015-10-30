#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "memory.h"

/* inicializa a memoria */
int memory_init()
{
	unsigned char *memory;
	FILE *fp;

	/* testa se o arquivo de memoria ja existe */
	if (access(MEMORY_FILE, R_OK | W_OK) == 0) {
		/* tudo ok: arquivo ja existe e pode ser escrito */
		return 0;
	}

	/* aloca um a regiao de memoria */
	memory = (unsigned char*)malloc(MEMORY_SIZE);
	if (memory == NULL) {
		perror("malloc");
		return -1;
	}

	/* preenche tudo com zeros */
	memset(memory, 0, MEMORY_SIZE);

	/* abre o arquivo de memoria */
	fp = fopen(MEMORY_FILE, "wb+");
	if (fp == NULL) {
		perror("fopen");
		return -1;
	}

	/* salva o conteudo */
 	fwrite(memory, MEMORY_SIZE, 1, fp);

 	/* fecha o arquivo */
 	fclose(fp);

	return 0;
}

int memory_write(unsigned int address, unsigned char *value, int len)
{
	FILE *fp;

	if (address < 0 || address > MEMORY_SIZE) {
		printf("Endereco invalido.\n");
		return -1;
	}

	fp = fopen(MEMORY_FILE, "rb+");
	if (fp == NULL) {
		perror("fopen");
		return -1;
	}

	rewind(fp);
	if (fseek(fp, address, SEEK_SET) < 0) {
		perror("fseek");
		fclose(fp);
		return -1;
	}

	fwrite(value, len, 1, fp);

	fclose(fp);

	return 0;
}

int memory_write_byte(unsigned int address, unsigned char *value)
{
	return memory_write(address, value, 1);
}

int memory_write_halfword(unsigned int address, unsigned short *value)
{
	return memory_write(address, (unsigned char*)value, 2);
}

int memory_write_word(unsigned int address, unsigned int *value)
{
	return memory_write(address, (unsigned char*)value, 4);
}

int memory_read(unsigned int address, unsigned char *value, int len)
{
	FILE *fp;

	if (address < 0 || address > MEMORY_SIZE) {
		printf("Endereco invalido.\n");
		return -1;
	}

	fp = fopen(MEMORY_FILE, "rb");
	if (fp == NULL) {
		perror("fopen");
		return -1;
	}

	rewind(fp);
	if (fseek(fp, address, SEEK_SET) < 0) {
		perror("fseek");
		fclose(fp);
		return -1;
	}

	fread(value, len, 1, fp);

	fclose(fp);

	return 0;
}

int memory_read_byte(unsigned int address, unsigned char *value)
{
	return memory_read(address, value, 1);
}

int memory_read_halfword(unsigned int address, unsigned short *value)
{
	return memory_read(address, (unsigned char*)value, 2);
}

int memory_read_word(unsigned int address, unsigned int *value)
{
	return memory_read(address, (unsigned char*)value, 4);
}
