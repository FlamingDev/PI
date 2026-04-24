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
void multiply(Pixel* p, void* factor){
	p->B = preventUnderflowAndOverflow((p->B - 128) * (*(float*)factor) + 128);
	p->G = preventUnderflowAndOverflow((p->G - 128)* (*(float*)factor) + 128);
	p->R = preventUnderflowAndOverflow((p->R - 128)* (*(float*)factor) + 128);
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
// quantizacao para niveis de cinza
void grayscale(Pixel* p, void* _unused){
	unsigned char v = 0.299*p->R + 0.587*p->G + 0.114*p->B;
	p->R = p->G = p->B = v;
}

void rgbQuantization(Pixel* p, void* k){
	int levels = *((int*)k);
	int step = 256/levels;
	p->B = (p->B/step) * step + step/2;
	p->G = (p->G/step) * step + step/2;
	p->R = (p->R/step) * step + step/2;
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
	r.B = (p->B + q->B) / 2;
	r.G = (p->G + q->G) / 2;
	r.R = (p->R + q->R) / 2;
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

BitmapImage* translation(BitmapImage* in, int dx, int dy){
	unsigned int h = in->altura;
	unsigned int w = in->largura;

	// inicializando imagem de saída
	BitmapImage* out = new BitmapImage();
	out->altura = h;
	out->largura = w;
	out->pixels = new Pixel[h*w];
	memcpy(out->header, in->header, HEADER_SIZE);

	// computacao da posicao dos pixels
	for (int i = 0; i < h; i++){
		for (int j = 0; j < w; j++){
			int orig_x = j-dx;
			int orig_y = i-dy;
			// se x ou y tao fora da imagem original
			if ((orig_x < 0 or orig_x >= w) or (orig_y < 0 or orig_y >= h)){				
				// preenche com branco
				out->pixels[i*w + j] = Pixel{255,255,255};
			} 
			else out->pixels[i*w + j] = in->pixels[orig_y*w + orig_x];
		}
	}
	return out;
}

BitmapImage* scale(BitmapImage* in, int s) {
    unsigned int h = in->altura * s;
    unsigned int w = in->largura * s;

    // Inicializando imagem de saída
    BitmapImage* out = new BitmapImage();
    out->altura = h;
    out->largura = w;
    out->pixels = new Pixel[h * w];
    memcpy(out->header, in->header, HEADER_SIZE);

    // atualizando metadados da imagem
    *(unsigned int*)&out->header[18] = w;
    *(unsigned int*)&out->header[22] = h;

    int bytesSemPadding = w * 3;
	int padding = (4 - (bytesSemPadding % 4)) % 4;
	int novoTamLinhaComPadding = bytesSemPadding + padding;
    unsigned int novoTamanhoPixels = novoTamLinhaComPadding * h;
    unsigned int novoTamanhoArquivo = HEADER_SIZE + novoTamanhoPixels;

    *(unsigned int*)&out->header[2] = novoTamanhoArquivo;

    *(unsigned int*)&out->header[34] = novoTamanhoPixels;

    // Computação da posição dos pixels (Nearest Neighbor)
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int orig_x = j / s;
            int orig_y = i / s;
			out->pixels[i*w + j] = in->pixels[orig_y*in->largura + orig_x];
        }
    }
    return out;
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

	BitmapImage* result = scale(img, 1);
	saveBitmap("new.bmp", result);	
	freeBitmap(img);
	freeBitmap(result);
	fclose(ptrFoto);
	return 0;
}
