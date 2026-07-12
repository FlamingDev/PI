#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#define HEADER_SIZE 54

struct Pixel{ // 3 bytes
	unsigned char B, G, R;
};

struct Matrix3x3{
	float m[3][3];
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
// INVERSA DAS MATRIZES PARA TRANSFORMACOES USANDO BACKWARD MAPPING
Matrix3x3 inverseTranslation(float dx, float dy){
	return Matrix3x3 {{
		{1, 0, -dx},
		{0, 1, -dy},
		{0, 0, 1}
	}};
}

Matrix3x3 inverseScale(float sx, float sy){
	return Matrix3x3 {{
		{1.0f/sx, 0, 0},
		{0, 1.0f/sy, 0},
		{0, 0, 1}
	}};
}

Matrix3x3 inverseRotation(float theta){
	float c = cos(theta);
	float s = sin(theta);

	return Matrix3x3 {{
		{ c, s, 0},
		{-s, c, 0},
		{ 0, 0, 1}
	}};
}

Matrix3x3 inverseHorizontalShear(float kx){
	return Matrix3x3 {{
		{1, -kx, 0},
		{0,  1,  0},
		{0,  0,  1}
	}};
}

Matrix3x3 inverseVerticalShear(float ky){
	return Matrix3x3 {{
		{ 1, 0, 0},
		{-ky, 1, 0},
		{ 0, 0, 1}
	}};
}

void transformPoint(
	Matrix3x3* T,
	float x,
	float y,
	float* out_x,
	float* out_y
){
	*out_x = T->m[0][0]*x +
			 T->m[0][1]*y +
			 T->m[0][2];

	*out_y = T->m[1][0]*x +
			 T->m[1][1]*y +
			 T->m[1][2];
}

BitmapImage* applyTransform(
	BitmapImage* in,
	Matrix3x3* inverseTransform,
	unsigned int newW,
	unsigned int newH
){
	BitmapImage* out = new BitmapImage();

	out->largura = newW;
	out->altura = newH;
	out->pixels = new Pixel[newW * newH];

	memcpy(out->header, in->header, HEADER_SIZE);

	// atualizando metadados da imagem
    *(unsigned int*)&out->header[18] = newW;
    *(unsigned int*)&out->header[22] = newH;

    int bytesSemPadding = newW * 3;
	int padding = (4 - (bytesSemPadding % 4)) % 4;
	int novoTamLinhaComPadding = bytesSemPadding + padding;
    unsigned int novoTamanhoPixels = novoTamLinhaComPadding * newH;
    unsigned int novoTamanhoArquivo = HEADER_SIZE + novoTamanhoPixels;

    *(unsigned int*)&out->header[2] = novoTamanhoArquivo;

    *(unsigned int*)&out->header[34] = novoTamanhoPixels;

	for (int y = 0; y < newH; y++){
		for (int x = 0; x < newW; x++){
			// ponto correspondente na imagem original
			float src_x, src_y;

			transformPoint(
				inverseTransform,
				x,
				y,
				&src_x,
				&src_y
			);

			src_x = (int)src_x;
			src_y = (int)src_y;

			if (
				src_x < 0 || src_x >= in->largura ||
				src_y < 0 || src_y >= in->altura
			){
				out->pixels[y*newW + x] =
					Pixel{255,255,255};
			}
			else{
				out->pixels[y*newW + x] =
					in->pixels[(int)src_y*in->largura + (int)src_x];
			}
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
	
	Matrix3x3 inv = inverseHorizontalShear(-0.5); // cisalhamento horizontal

	BitmapImage* result = applyTransform(
		img,
		&inv,
		img->largura,
		img->altura
	);
	saveBitmap("horizontal_shear.bmp", result);	
	freeBitmap(img);
	freeBitmap(result);
	fclose(ptrFoto);
	return 0;
}
