#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

int main(int argc, char *argv[])
{
	unsigned int address = 0;
	unsigned char byte = 0;
	unsigned int word = 0;

	if (argc < 3 || argc > 4) {
		printf("%s <type> <address> <value>\n", argv[0]);
		exit(1);
	}

	if (memory_init() < 0) {
		printf("Erro ao inicializar a memoria.\n");
		exit(1);
	}

	address = atoi(argv[2]);

	if (strcmp(argv[1], "byte") == 0) {
		if (argc == 4) {
			byte = (unsigned char) atoi(argv[3]);
			if (memory_write_byte(address, &byte) < 0) {
				printf("Erro ao escrever byte na memoria.\n");
				exit(1);
			}
		} else {
			if (memory_read_byte(address, &byte) < 0) {
				printf("Erro ao ler byte da memoria.\n");
				exit(1);
			}		
		}
		printf("byte %d %d\n", address, byte);
	} else if (strcmp(argv[1], "word") == 0) {
		if (argc == 4) {
			word = (unsigned int) atol(argv[3]);
			if (memory_write_word(address, &word) < 0) {
				printf("Erro ao escrever word na memoria.\n");
				exit(1);
			}
		} else {
			if (memory_read_word(address, &word) < 0) {
				printf("Erro ao ler word da memoria.\n");
				exit(1);
			}		
		}
		printf("word %d %d\n", address, word);
	} else {
		printf("Tipo invalido.\n");
	}

	return 0;
}