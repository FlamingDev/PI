#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	if (argc < 2){
		fprintf(stderr, "Erro: Voce esqueceu de passar o caminho da foto.\n");
        fprintf(stderr, "Uso: %s <nome_do_arquivo>\n", argv[0]);
		return 1;
	}
	char* nome_foto = argv[1];
	FILE* fptr;
	if ((fptr = fopen(nome_foto, "rb")) == NULL){
		perror("Erro: nao foi possivel abrir a foto");
		return 2;
	}
	// tipo de arquivo (BM)
	unsigned char tipo_arquivo[2];
	fread(tipo_arquivo, 1, 2, fptr);
	
	printf("Tipo do arquivo: %c%c\n", tipo_arquivo[0], tipo_arquivo[1]);
	
	// tamanho da imagem
	unsigned int tamanho_imagem;
	fread(&tamanho_imagem, 4, 1, fptr);

	printf("Tamanho da imagem: %d\n", tamanho_imagem);

	// ignorando proximos 4 bytes
	unsigned int lixo;
	fread(&lixo, 4, 1, fptr);

	// offset dos pixels
	unsigned int offset;
	fread(&offset, 4, 1, fptr);

	printf("offset dos pixels: %d\n", offset);

	// ignorando proximos 4 bytes
	fread(&lixo, 4, 1, fptr);

	unsigned int largura, altura;
	fread(&largura, 4, 1, fptr);
	fread(&altura, 4, 1, fptr);

	printf("dimensoes da imagem: %dx%d\n", largura, altura);

	// ignorando proximos 2 bytes
	fread(&lixo, 2, 1, fptr);

	unsigned short bpp;
	fread(&bpp, 2, 1, fptr);

	printf("bpp: %d\n", bpp);

	// indo pro inicio dos pixels
	fseek(fptr, offset, SEEK_SET);
	return 0;
}
