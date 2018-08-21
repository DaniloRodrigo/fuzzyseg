#ifndef FUZZY_SEG_H
#define FUZZY_SEG_H

#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <cv.h>

using namespace std;

//pelo artigo do Bruno, as afinidades são discretizadas
//nesse caso escolhi 1000 afinidades diferentes
//afinidade máxima 1 -> posição 999 do vetor
//...
//afinidade mínima 0 -> posição 0 do vetor
#define NUM_AFF 1000


//modifique para que corresponda ao tipo e à forma de acesso ao pixel de sua biblioteca
//por padrão usa-se aqui OpenCV
typedef cv::Mat TipoImagem;
#define GETPIXEL(img, x, y) img->at<uchar>(y, x)



//vetor de pixels
typedef struct AffinityStr {
	vector<int> buf;
	int & get(int i) {
		while((unsigned int) i >= buf.size())
			buf.resize(buf.size()*2);
		return buf[i];
	}
} AffinityStr;

//cada semente
typedef struct seed {
	float x, y; //posição na imagem
	int id;
} seed;

//cada afinidade possível i contém um conjunto de pixels que ainda vão ser visitados a partir dessa afinidade i
//esse conjunto é dado pela estrutura acima que é um vetor que vai ser redimensionando à medida que precisa de mais espaço
class FuzzySegmentation {
	AffinityStr affx[NUM_AFF]; 	//componente x do pixel
	AffinityStr affy[NUM_AFF]; 	//componente y do pixel
	int idx[NUM_AFF];			//ultimo indice de cada vetor
	int qtdPixelsSegmentados;

	vector<seed> seedSet;		//conjunto de todas as sementes
	int **visited;				//matriz de visitados
	int **dd;
	float **affinityMatrix;		//matriz de afinidades
	int w, h;					//tamanho da imagem

	TipoImagem *refImg;

	//cada vez que um pixel é descoberto com afinidade k, adiciona-se em affx[k] e affy[k]
	void addAff(int k, int x, int y, int id, int dist) {
		qtdPixelsSegmentados++;
		affx[k].get(idx[k]) = x;
		affy[k].get(idx[k]) = y;
		idx[k]++;
		visited[y][x] = id; //marca o pixel como visitado usando o identificador da semente no vetor de sementes
		dd[y][x] = dist+1; //vetor de distâncias para a semente mais próxima
	}

	int isValid(int x, int y) {
		return x < w && y < h && x >= 0 && y >= 0;
	}

	float affDistanceFactor(float dist) {
		const int affDF = 980;
		return 1*(1.0 - dist/affDF);
	}

	//retorna a afinidade entre os pixels (x1,y1) e (x2,y2)
	int getAffinity(int x1, int y1, int x2, int y2) {
		float disp;
		int p1 = GETPIXEL(refImg, x1, y1);
		int p2 = GETPIXEL(refImg, x2, y2);
		disp = (float)(p2 - p1);

		//essa afinidade leva em conta a distancia para alguma semente
		//float af = (NUM_AFF-1)*(affDistanceFactor(dd[y1][x1])*(1.0 - fabs(disp)/255.0));

		float af = (NUM_AFF-1)*((1.0 - fabs(disp)/255.0));
		return MAX(0, MIN(NUM_AFF - 1, (int) af)); //deixa no limite 0 e NUM_AFF-1
	}

	public:

		int getClasse(int x, int y) {
			return visited[y][x];
		}

		float getAffinity(int x, int y) {
			return affinityMatrix[y][x];
		}

		void freeMemory() {
			for(int i = 0; i < h; i++) {
				free(visited[i]);
				free(dd[i]);
			}
			free(visited);
			free(dd);
		}

		void reset(int w, int h) {
			this->w = w;
			this->h = h;
			qtdPixelsSegmentados = 0;
			for(int i = 0; i < NUM_AFF; i++) {
				affx[i].buf.clear();
				affy[i].buf.clear();
				affx[i].buf.resize(10);
				affy[i].buf.resize(10);
				idx[i] = 0;
			}
			visited = (int **) malloc(sizeof(int *)*h);
			dd = (int **) malloc(sizeof(int *)*h);
			affinityMatrix = (float **) malloc(sizeof(float *)*h);
			for(int i = 0; i < h; i++) {
				visited[i] = (int *) malloc(sizeof(int)*w);
				for(int j = 0; j < w; j++)
					visited[i][j] = 0;
				dd[i] = (int *) malloc(sizeof(int)*w);
				affinityMatrix[i] = (float *) malloc(sizeof(float)*w);
				for(int j = 0; j < w; j++)
					affinityMatrix[i][j] = 0;
			}
		}

		FuzzySegmentation(TipoImagem *img, int w, int h) {
			this->refImg = img;
			reset(w, h);
		}

		void addSeed(int x, int y, int classe) {
			seed newseed;
			newseed.id = seedSet.size()+1;
			newseed.x = x;
			newseed.y = y;
			visited[y][x] = classe;
			addAff(NUM_AFF-1, x, y, classe, 0);
			seedSet.push_back(newseed);
		}

		//o parâmetro maxDist é a afinidade mínima que deve ser considerada
		//por exemplo, estou usando NUM_AFF = 1000, se maxDist = 20
		//lembrando que a região cresce da afinidade máxima (1000) até a mínima (0)
		//então o algoritmo somente cresce até a afinidade NUM_AFF - maxDist
		//nesse exemplo, segmenta somente pixels com afinidade 980 até 1000 (0.98 até 1.0)
		void fuzzySeg(int maxDist = NUM_AFF-1) {
			qtdPixelsSegmentados = 0;
			int limit = MAX(0, NUM_AFF - maxDist);
			for(int k = NUM_AFF-1; k >= limit; k--) {
				for(int vidx = 0; vidx < idx[k]; vidx++) {
					int x = affx[k].get(vidx);
					int y = affy[k].get(vidx);
					int classe = visited[y][x];
					affinityMatrix[y][x] = ((float)k)/(NUM_AFF-1);
					//result.at<uchar>(y, x) = 255;//(int) ((255.0*k)/(NUM_AFF-1));
#define F1(dx, dy) if(isValid(x+dx, y+dy) && !visited[y+dy][x+dx]) {
#define F2(dx, dy) int afinidade = getAffinity(x, y, x+dx, y+dy);
#define F3(dx, dy) int a = MIN(k, afinidade);
#define F4(dx, dy) addAff(a, x+dx, y+dy, classe, dd[y][x]);}
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
};


#endif
