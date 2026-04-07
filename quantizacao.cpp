#include <stdio.h>
#include <stdlib.h>

struct Pixel{ // 3 bytes
	unsigned char B, G, R;
};

struct ImageMatrix{
	unsigned int altura, largura;
	Pixel* img;
};
 
ImageMatrix* parseBitmap(FILE* imgp){
	printf("%lu\n",sizeof(Pixel));
	// tipo de arquivo (BM)
	unsigned char tipo_arquivo[2];
	fread(tipo_arquivo, 1, 2, imgp);
	
	printf("Tipo do arquivo: %c%c\n", tipo_arquivo[0], tipo_arquivo[1]);
	
	// tamanho da imagem
	unsigned int tamanho_imagem;
	fread(&tamanho_imagem, 4, 1, imgp);

	printf("Tamanho da imagem: %d\n", tamanho_imagem);

	// ignorando proximos 4 bytes
	unsigned int lixo;
	fread(&lixo, 4, 1, imgp);

	// offset dos pixels
	unsigned int offset;
	fread(&offset, 4, 1, imgp);

	printf("offset dos pixels: %d\n", offset);

	// ignorando proximos 4 bytes
	fread(&lixo, 4, 1, imgp);

	unsigned int largura, altura;
	fread(&largura, 4, 1, imgp);
	fread(&altura, 4, 1, imgp);

	printf("dimensoes da imagem: %dx%d\n", largura, altura);

	// ignorando proximos 2 bytes
	fread(&lixo, 2, 1, imgp);

	unsigned short bpp;
	fread(&bpp, 2, 1, imgp);

	printf("bpp: %d\n", bpp);

	unsigned short Bpp = bpp/8;
	// indo pro inicio dos pixels
	fseek(imgp, offset, SEEK_SET);

	// Parsing dos pixels
	int tamLinha = largura * 3;
	printf("tamLinha: %d\n", tamLinha);
	
	ImageMatrix* pixels = new ImageMatrix();
	pixels->img = new Pixel[largura*altura];
	pixels->largura = largura;
	pixels->altura = altura;

	// criando a matriz
	int padding = (4 - (tamLinha % 4)) % 4;

	for (int i = 0; i < altura; i++){
		fread(&pixels->img[i*largura], sizeof(Pixel), largura, imgp);
		// pulando padding
		fseek(imgp, padding, SEEK_CUR);
	}
	return pixels;
}

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
	ImageMatrix* pixels = parseBitmap(fptr);
	printf("%d %d %d\n", pixels->img[0].B, pixels->img[0].G, pixels->img[0].R);
	return 0;
}
