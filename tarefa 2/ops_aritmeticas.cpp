#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

BitmapImage* map(BitmapImage* a, BitmapImage* b, Pixel (*op)(Pixel* p, Pixel* q)){
	if (!(a->altura == b->altura and a->largura == b->largura)){
		perror("ERRO: Operacao aritmetica entre imagens de tamanhos diferentes");
		return NULL;
	}
	unsigned int h = a->altura;
	unsigned int w = b->largura;

	// imagem de saída
	BitmapImage* img = new BitmapImage();
	img->altura = h;
	img->largura = w;
	img->pixels = new Pixel[h*w];
	memcpy(img->header, a->header, HEADER_SIZE);
	// computacao dos valores dos pixels
	for (int i = 0; i < h; i++){
		for (int j = 0; j < w; j++){
			Pixel* p = &a->pixels[i*w + j];
			Pixel* q = &b->pixels[i*w + j];
			img->pixels[i*w + j] = op(p, q);
		}
	}
	return img;
}

Pixel add(Pixel* p, Pixel* q){
	Pixel r;
	r.B = preventUnderflowAndOverflow(p->B + q->B);
	r.G = preventUnderflowAndOverflow(p->G + q->G);
	r.R = preventUnderflowAndOverflow(p->R + q->R);
	return r;
}

Pixel subtract(Pixel* p, Pixel* q){
	Pixel r;
	r.B = p->B - q->B;
	r.G = p->G - q->G;
	r.R = p->R - q->R;
	return r;
}

Pixel multiply(Pixel* p, Pixel* q){
    Pixel r;
    r.B = (unsigned char)((p->B * q->B) / 255);
    r.G = (unsigned char)((p->G * q->G) / 255);
    r.R = (unsigned char)((p->R * q->R) / 255);
    return r;
}

Pixel divide(Pixel* p, Pixel* q){
    Pixel r;
    r.B = (q->B == 0) ? 255 : preventUnderflowAndOverflow((p->B * 255) / q->B);
    r.G = (q->G == 0) ? 255 : preventUnderflowAndOverflow((p->G * 255) / q->G);
    r.R = (q->R == 0) ? 255 : preventUnderflowAndOverflow((p->R * 255) / q->R);
    return r;
}

Pixel (*getOperation(char* op))(Pixel*, Pixel*){

	if (strcmp(op, "add") == 0)
		return add;

	if (strcmp(op, "subtract") == 0)
		return subtract;

	if (strcmp(op, "multiply") == 0)
		return multiply;

	if (strcmp(op, "divide") == 0)
		return divide;

	return NULL;
}

int main(int argc, char* argv[]){

	if (argc < 4){
		fprintf(
			stderr,
			"Uso: %s <imagem1.bmp> <imagem2.bmp> <operacao>\n",
			argv[0]
		);

		fprintf(
			stderr,
			"Operacoes: add, subtract, multiply, divide\n"
		);

		return 1;
	}

	char* foto1 = argv[1];
	char* foto2 = argv[2];
	char* operacao = argv[3];

	FILE* ptrFoto1 = fopen(foto1, "rb");
	FILE* ptrFoto2 = fopen(foto2, "rb");

	if (ptrFoto1 == NULL){
		perror("Erro ao abrir imagem 1");
		return 2;
	}

	if (ptrFoto2 == NULL){
		perror("Erro ao abrir imagem 2");
		fclose(ptrFoto1);
		return 3;
	}

	BitmapImage* img1 = parseBitmap(ptrFoto1);
	BitmapImage* img2 = parseBitmap(ptrFoto2);

	fclose(ptrFoto1);
	fclose(ptrFoto2);

	if (img1 == NULL || img2 == NULL){
		return 4;
	}

	Pixel (*op)(Pixel*, Pixel*) = getOperation(operacao);

	if (op == NULL){
		fprintf(stderr, "Operacao invalida.\n");

		freeBitmap(img1);
		freeBitmap(img2);

		return 5;
	}

	BitmapImage* result = map(img1, img2, op);

	if (result == NULL){
		freeBitmap(img1);
		freeBitmap(img2);
		return 6;
	}

	saveBitmap("subtract.bmp", result);

	printf("Imagem salva como resultado.bmp\n");

	freeBitmap(img1);
	freeBitmap(img2);
	freeBitmap(result);

	return 0;
}