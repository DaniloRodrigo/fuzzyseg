#include <stdio.h>
#include <cv.h>
#include <opencv2/highgui/highgui.hpp>
#include "fuzzyseg.h"

int main(int argc, char **argv) {

	if(argc < 3) {
		fprintf(stderr, "Not enough parameters. Please provide these:\n(1) image file (2) seeds file\nExample: ./segmentar image.jpg seeds.in\nA seed file contain multiple lines with x y class, where class is in [1, 7]\n");
		return 1;
	}

	FILE *arq = fopen(argv[2], "r");

	cv::Mat img;
	cv::Mat res;

	img = cv::imread(argv[1], 0);
	res.create(img.rows, img.cols, CV_8UC3);

	FuzzySegmentation fs(&img, img.cols, img.rows);

	int x, y, classe;
	while(fscanf(arq, "%d %d %d", &x, &y, &classe) == 3) {
		fs.addSeed(x, y, classe);
	}

	fs.fuzzySeg(1000);

	printf("Size res = %d %d\n", res.rows, res.cols);
	for(int i = 0; i < res.rows; i++)
		for(int j = 0; j < res.cols; j++) {
			int r = 255*((fs.getClasse(j, i)&1) != 0);
			int g = 255*((fs.getClasse(j, i)&2) != 0);
			int b = 255*((fs.getClasse(j, i)&4) != 0);
			res.at<cv::Vec3b>(i, j).val[0] = MIN(r*fs.getAffinity(j, i), 255);
			res.at<cv::Vec3b>(i, j).val[1] = MIN(g*fs.getAffinity(j, i), 255);
			res.at<cv::Vec3b>(i, j).val[2] = MIN(b*fs.getAffinity(j, i), 255);
		}

	cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);
	cv::imshow("Display window", res);
	cv::waitKey(0); 

	fclose(arq);

	return 0;
}

