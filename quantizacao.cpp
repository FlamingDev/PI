#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	char* nome_foto = argv[1];
	FILE* fptr;
	if ((fptr = fopen(nome_foto, "rb")) == NULL){
		perror("nao foi possivel abrir a foto");
		return 1;
	}	
	unsigned char tipo_arquivo[2];
	fread(tipo_arquivo, 1, 2, fptr);
	
	printf("Tipo: %c%c\n", tipo_arquivo[0], tipo_arquivo[1]);
	
	return 0;
}
