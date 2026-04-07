#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define HEADER_SIZE 54

struct Pixel{ // 3 bytes
	unsigned char B, G, R;
};

struct ImageMatrix{
	unsigned int altura, largura;
	Pixel* pixels;
};

unsigned char header[HEADER_SIZE];

ImageMatrix* parseBitmap(FILE* imgp){
	fread(header, 54, 1, imgp);

	// Parsing do header
	unsigned char tipo_arquivo[3] = {header[0], header[1], 0};
	unsigned int offset  = *(unsigned int*)&header[10];
	unsigned int largura = *(unsigned int*)&header[18];
	unsigned int altura  = *(unsigned int*)&header[22];
	unsigned short bpp   = *(unsigned short*)&header[28];
	int tamLinha = largura * 3;

	printf("Tipo do arquivo: %s\n", tipo_arquivo);
	printf("offset dos pixels: %d\n", offset);
	printf("dimensoes da imagem: %dx%d\n", largura, altura);
	printf("bpp: %d\n", bpp);
	printf("tamLinha: %d\n", tamLinha);

	// indo pro inicio dos pixels
	fseek(imgp, offset, SEEK_SET);
	// criando a matriz
	ImageMatrix* img = new ImageMatrix();
	img->pixels = new Pixel[largura*altura];
	img->largura = largura;
	img->altura = altura;

	// parsing dos pixels
	int padding = (4 - (tamLinha % 4)) % 4;

	for (int i = 0; i < altura; i++){
		fread(&img->pixels[i*largura], sizeof(Pixel), largura, imgp);
		// pulando padding
		fseek(imgp, padding, SEEK_CUR);
	}
	return img;
}

void freeBitmap(ImageMatrix* bmp){
	free(bmp->pixels);
	free(bmp);
	bmp = NULL;
}

void saveBitmap(const char* filename, ImageMatrix* img){
	FILE* imgp;
	if ((imgp = fopen(filename, "wb")) == NULL){
		perror("Erro ao abrir arquivo");
		return;
	}
	
	// Assume que header nao esta mudado
	fwrite(header, HEADER_SIZE, 1, imgp);

	unsigned int altura = img->altura;
	unsigned int largura = img->largura;

	int padding = (4 - (largura * 3) % 4) % 4;
	unsigned char pad[3] = {0,0,0};

	for (int i = 0; i < altura; i++){
		fwrite(&img->pixels[i*largura], sizeof(Pixel), largura, imgp);
		fwrite(pad, 1, padding, imgp);
	}
	fclose(imgp);
}

int preventOverflow(int value){
	return value > 255? 255 : value;
}

int preventUnderflow(int value){
	return value < 0? 0 : value;
}

void add(ImageMatrix* img, int n){
	unsigned int altura = img->altura;
	unsigned int largura = img->largura;

	for (int i = 0; i < altura; i++){
		for (int j = 0; j < largura; j++){
			Pixel* p = &img->pixels[i*largura + j];
			p->B = preventOverflow(p->B + n);
			p->G = preventOverflow(p->G + n);
			p->R = preventOverflow(p->R + n);
		}
	}
}

void subtract(ImageMatrix* img, int n){
	unsigned int altura = img->altura;
	unsigned int largura = img->largura;

	for (int i = 0; i < altura; i++){
		for (int j = 0; j < largura; j++){
			Pixel* p = &img->pixels[i*largura + j];
			p->B = preventUnderflow(p->B - n);
			p->G = preventUnderflow(p->G - n);
			p->R = preventUnderflow(p->R - n);
		}
	}
}

int main(int argc, char* argv[]){
	if (argc < 2){
		fprintf(stderr, "Erro: Voce esqueceu de passar o caminho da foto.\n");
        fprintf(stderr, "Uso: %s <nome_do_arquivo>\n", argv[0]);
		return 1;
	}
	char* nome_foto = argv[1];
	FILE* imgp;
	if ((imgp = fopen(nome_foto, "rb")) == NULL){
		perror("Erro: nao foi possivel abrir a foto");
		return 2;
	}
	ImageMatrix* img = parseBitmap(imgp);

	subtract(img, 150);
	saveBitmap("new.bmp", img);	
	freeBitmap(img);
	fclose(imgp);
	return 0;
}
