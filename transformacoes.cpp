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
