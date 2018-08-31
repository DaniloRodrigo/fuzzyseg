#ifndef FUZZY_SEG_H
#define FUZZY_SEG_H

#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


using namespace std;

//pelo artigo do Bruno, as afinidades são discretizadas
//nesse caso escolhi 1000 afinidades diferentes
//afinidade máxima 1 -> posição 999 do vetor
//...
//afinidade mínima 0 -> posição 0 do vetor
#define NUM_AFF 1000

//modifique para que corresponda ao tipo e à forma de acesso ao pixel de sua biblioteca
//por padrão usa-se aqui OpenCV
#ifdef FS_OPENCV
typedef cv::Mat TipoImagem;
#define GETPIXEL(img, x, y) img->at<uchar>((y), (x))
#endif

#ifdef FS_GTKIMG
typedef Imagem TipoImagem;
#define GETPIXEL(img, x, y) img->m[y][x][0]
#endif



//vetor de pixels
typedef struct FuzzyFragment {
	int x, y, classe;
} FuzzyFragment;

class FuzzyFragmentList {
	public:
		vector<FuzzyFragment> buf;			//buffer
		int idx;							//ultimo indice de cada vetor
		FuzzyFragmentList() {
			idx = 0;
		}
		FuzzyFragment & get(int i) {
			while((unsigned int) i >= buf.size())
				buf.resize(buf.size()*2);
			return buf[i];
		}
		void clear() {
			buf.clear();
			buf.resize(10);
			idx = 0;
		}
};

template <class tipo>
tipo ** alocarMatriz(int w, int h) {
	tipo **m;
	m = (tipo **) malloc(sizeof(tipo *)*h);
	for(int i = 0; i < h; i++) {
		m[i] = (tipo *) malloc(sizeof(tipo)*w);
		for(int j = 0; j < w; j++)
			m[i][j] = 0;
	}
	return m;
}

//cada semente
typedef struct seed {
	int x, y; //posição na imagem
	int classe;
} seed;

//cada afinidade possível i contém um conjunto de pixels que ainda vão ser visitados a partir dessa afinidade i
//esse conjunto é dado pela estrutura acima que é um vetor que vai ser redimensionando à medida que precisa de mais espaço
class FuzzySegmentation {
	FuzzyFragmentList fragmentSet[NUM_AFF];

	vector<seed> seedSet;		//conjunto de todas as sementes
	int **finalClass;				//matriz de visitados
	int **greatestk;		//matriz de afinidades
	int w, h;					//tamanho da imagem

	int currentK;				//afinidade [NUM_AFF-1 -> 0] a ser processada

	TipoImagem *refImg;
	int isValid(int x, int y) {
		return x < w && y < h && x >= 0 && y >= 0;
	}

	int media(int x, int y, int size) {
		int soma = 0;
		int qtd = 0;
		for(int i = -size; i <= size; i++) 
			for(int j = -size; j <= size; j++)
				if(isValid(x+j, y+i)) {
					soma += GETPIXEL(refImg, x+j, y+i);
					qtd++;
				}
		return soma/qtd;
	}

	//retorna a afinidade entre os pixels (x1,y1) e (x2,y2)
	int getAffinity(int x1, int y1, int x2, int y2) {
		float disp;
		int p1 = GETPIXEL(refImg, x1, y1);
		int p2 = GETPIXEL(refImg, x2, y2);
		p1 = media(x1, y1, 2);
		p2 = media(x2, y2, 2);
		disp = (float)(p2 - p1);

		float af = (NUM_AFF-1)*((1.0 - fabs(disp)/255.0));
		return MAX(0, MIN(NUM_AFF - 1, (int) af)); //deixa no limite 0 e NUM_AFF-1
	}

	//cada vez que um pixel é descoberto com afinidade k, adiciona-se em affx[k] e affy[k]
	void addFragment(int k, int x, int y, int id) {
		fragmentSet[k].get(fragmentSet[k].idx).x = x;
		fragmentSet[k].get(fragmentSet[k].idx).y = y;
		fragmentSet[k].get(fragmentSet[k].idx).classe = id;
		fragmentSet[k].idx++;
		greatestk[y][x] = k;
		//finalClass[y][x] = id; //marca o pixel como visitado usando o identificador da semente no vetor de sementes
	}

	public:

		void saveSeedSet(char *filename) {
			FILE *arq = fopen(filename, "w");
			for(int i = 0; i < seedSet.size(); i++) {
				fprintf(arq, "%d %d %d\n", seedSet[i].x, seedSet[i].y, seedSet[i].classe);
			}
			fclose(arq);
		}

		int getClasse(int x, int y) {
			return finalClass[y][x];
		}

		int getAffinity(int x, int y) {
			return greatestk[y][x];
		}

		float getNormalizedAffinity(int x, int y) {
			return ((float)greatestk[y][x])/(NUM_AFF-1);
		}

		void freeMemory() {
			for(int i = 0; i < h; i++) {
				free(finalClass[i]);
				free(greatestk[i]);
			}
			free(greatestk);
			free(finalClass);
		}

		void reset() {
			for(int i = 0; i < NUM_AFF; i++)
				fragmentSet[i].clear();
			finalClass = alocarMatriz<int>(w, h);
			greatestk = alocarMatriz<int>(w, h);
		}

		FuzzySegmentation(TipoImagem *img, int w, int h) {
			this->refImg = img;
			this->w = w;
			this->h = h;
			this->currentK = NUM_AFF - 1;
			reset();
		}

		void addFragmentsFromSeeds() {
			for(int i = 0; i < seedSet.size(); i++)
				addFragment(NUM_AFF-1, seedSet[i].x, seedSet[i].y, seedSet[i].classe);
		}

		void addSeed(int x, int y, int classe) {
			seed newseed;
			newseed.classe = classe;
			newseed.x = x;
			newseed.y = y;
			seedSet.push_back(newseed);
		}

		void fuzzySeg() {
			for(int i = 0; i < h; i++)
				for(int j = 0; j < w; j++) {
					finalClass[i][j] = 0;
					greatestk[i][j] = 0;
				}
			for(int i = 0; i < NUM_AFF; i++)
				fragmentSet[i].clear();
			addFragmentsFromSeeds();
			for(int k = NUM_AFF-1; k >= 0; k--) {
				for(int f = 0; f < fragmentSet[k].idx; f++) {
					int x = fragmentSet[k].get(f).x;
					int y = fragmentSet[k].get(f).y;
					if(!finalClass[y][x])
						finalClass[y][x] = fragmentSet[k].get(f).classe;
#define F1(dx, dy) if(isValid(x+dx, y+dy) && !finalClass[y+dy][x+dx]) {
#define F2(dx, dy) int afinidade = getAffinity(x, y, x+dx, y+dy);
#define F3(dx, dy) int a = MIN(k, afinidade); 
#define F4(dx, dy) if(a > greatestk[y+dy][x+dx]) addFragment(a, x+dx, y+dy, fragmentSet[k].get(f).classe);}
					F1(1, 0) F2(1,0) F3(1,0) F4(1,0)
						F1(-1, 0) F2(-1,0) F3(-1,0) F4(-1,0)
						F1(0,1) F2(0,1) F3(0,1) F4(0,1)
						F1(0,-1) F2(0,-1) F3(0,-1) F4(0,-1)
						F1(1,1) F2(1,1) F3(1,1) F4(1,1)
						F1(1,-1) F2(1,-1) F3(1,-1) F4(1,-1)
						F1(1, -1) F2(1,-1) F3(1,-1) F4(1,-1)
						F1(-1, -1) F2(-1,-1) F3(-1,-1) F4(-1,-1)
				}
			}
		}

		void step(int newK) {
			if(newK == currentK) return;
			if(newK < currentK) {
				printf("Advancing from %d to %d\n", currentK, newK);
				for(int k = currentK; k > newK; k--) {
					for(int f = 0; f < fragmentSet[k].idx; f++) {
						int x = fragmentSet[k].get(f).x;
						int y = fragmentSet[k].get(f).y;
						if(!finalClass[y][x])
							finalClass[y][x] = fragmentSet[k].get(f).classe;
						F1(1, 0) F2(1,0) F3(1,0) F4(1,0)
							F1(-1, 0) F2(-1,0) F3(-1,0) F4(-1,0)
							F1(0,1) F2(0,1) F3(0,1) F4(0,1)
							F1(0,-1) F2(0,-1) F3(0,-1) F4(0,-1)
							F1(1,1) F2(1,1) F3(1,1) F4(1,1)
							F1(1,-1) F2(1,-1) F3(1,-1) F4(1,-1)
							F1(1, -1) F2(1,-1) F3(1,-1) F4(1,-1)
							F1(-1, -1) F2(-1,-1) F3(-1,-1) F4(-1,-1)
					}
				}
				currentK = newK;
			} else {
				printf("Reseting and recalculating\n");
				this->currentK = NUM_AFF - 1;
				for(int i = 0; i < h; i++)
					for(int j = 0; j < w; j++) {
						finalClass[i][j] = 0;
						greatestk[i][j] = 0;
					}
				for(int i = 0; i < NUM_AFF; i++)
					fragmentSet[i].clear();
				addFragmentsFromSeeds();
				step(newK);
/*				printf("Erasing from %d to %d\n", newK-1, currentK+1);
				for(int k = newK; k >= 0; k--) {
					for(int f = 0; f < idx[k]; f++) {
						int x = affx[k].get(f);
						int y = affy[k].get(f);
						int origem = affclass[k].get(f);
						if(origem <= newK) {
							finalClass[y][x] = 0;
							greatestk[y][x] = 0;
						}
					}
					affx[k].buf.clear();
					affy[k].buf.clear();
					affx[k].buf.resize(10);
					affy[k].buf.resize(10);
					idx[k] = 0;
				}
				currentK = newK;*/
			}
		}
};


#endif
