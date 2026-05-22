#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define HEADER_SIZE 54

struct Pixel{ // 3 bytes
	unsigned char B, G, R;
};

struct BitmapImage{
	unsigned int altura, largura;
	Pixel* pixels;
	unsigned char header[HEADER_SIZE];
};

BitmapImage* parseBitmap(FILE* imgp){
	BitmapImage* img = new BitmapImage();
	fread(img->header, HEADER_SIZE, 1, imgp);

	// Parsing do header
	unsigned char tipo_arquivo[3] = {img->header[0], img->header[1], 0};
	unsigned int offset  = *(unsigned int*)&img->header[10];
	unsigned int largura = *(unsigned int*)&img->header[18];
	unsigned int altura  = *(unsigned int*)&img->header[22];
	unsigned short bpp   = *(unsigned short*)&img->header[28];
	int tamLinha = largura * 3;

	printf("Tipo do arquivo: %s\n", tipo_arquivo);
	printf("offset dos pixels: %d\n", offset);
	printf("dimensoes da imagem: %dx%d\n", largura, altura);
	printf("bpp: %d\n", bpp);
	printf("tamLinha: %d\n", tamLinha);

	if (bpp != 24){
		perror("Erro de parsing: Imagem não é 24-bitmap");
		return NULL;
	}

	// indo pro inicio dos pixels
	fseek(imgp, offset, SEEK_SET);
	// criando a matriz
	
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

void freeBitmap(BitmapImage* bmp){
	delete[] bmp->pixels;
	delete bmp;
	bmp = NULL;
}

void saveBitmap(const char* filename, BitmapImage* img){
	FILE* imgp;
	if ((imgp = fopen(filename, "wb")) == NULL){
		perror("Erro ao abrir arquivo");
		return;
	}
	
	fwrite(img->header, HEADER_SIZE, 1, imgp);

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

unsigned char preventUnderflowAndOverflow(int v){
	if (v > 255) return 255;
	if (v < 0) return 0;
	return v;
}

void add(Pixel* p, void* value){
	p->B = preventUnderflowAndOverflow(p->B + *((int*)value));
	p->G = preventUnderflowAndOverflow(p->G + *((int*)value));
	p->R = preventUnderflowAndOverflow(p->R + *((int*)value));
}

void subtract(Pixel* p, void* value){
	p->B = preventUnderflowAndOverflow(p->B - *((int*)value));
	p->G = preventUnderflowAndOverflow(p->G - *((int*)value));
	p->R = preventUnderflowAndOverflow(p->R - *((int*)value));
}
// contraste
void contrastStretching(Pixel* p, void* factor){
	p->B = preventUnderflowAndOverflow((p->B - 128) * (*(float*)factor) + 128);
	p->G = preventUnderflowAndOverflow((p->G - 128) * (*(float*)factor) + 128);
	p->R = preventUnderflowAndOverflow((p->R - 128) * (*(float*)factor) + 128);
}

void correcaoGama(Pixel*p, void* expoente){
	p->B = preventUnderflowAndOverflow(pow(p->B, *(float*)expoente));
	p->G = preventUnderflowAndOverflow(pow(p->G, *(float*)expoente));
	p->R = preventUnderflowAndOverflow(pow(p->R, *(float*)expoente));
}

// Faz uma operação em cada pixel da imagem
void map(BitmapImage* img, void (*op)(Pixel* p, void* value), void* value){
	unsigned int altura = img->altura;
	unsigned int largura = img->largura;

	for (int i = 0; i < altura; i++){
		for (int j = 0; j < largura; j++){
			Pixel* p = &img->pixels[i*largura + j];
			op(p, value);
		}
	}
}

void negative(Pixel* p, void* _unused){
	p->B = (255 - p->B);
	p->G = (255 - p->G);
	p->R = (255 - p->R);
}

int main(int argc, char* argv[]){
	if (argc < 2){
		fprintf(stderr, "Erro: Voce esqueceu de passar o caminho da foto.\n");
        fprintf(stderr, "Uso: %s <nome_do_arquivo>\n", argv[0]);
		return 1;
	}
	char* foto = argv[1];
	FILE* ptrFoto;
	
	if (((ptrFoto = fopen(foto, "rb")) == NULL)){
		perror("Erro: nao foi possivel abrir a foto");
		return 2;
	}
	BitmapImage* img = parseBitmap(ptrFoto);

    float value = 0.9;
    map(img,correcaoGama, &value);
	BitmapImage* result = img;
	saveBitmap("realce_gama.bmp", result);	
	freeBitmap(img);
	freeBitmap(result);
	fclose(ptrFoto);
	return 0;
}