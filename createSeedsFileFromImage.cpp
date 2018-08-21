#include <stdio.h>
#include <cv.h>
#include <opencv2/highgui/highgui.hpp>

int main(int argc, char **argv) {

	if(argc < 3) {
		fprintf(stderr, "Not enough parameters. Please provide these:\n(1) image file (2) seeds file\nExample: ./createSeedsFileFromImage image.jpg seeds.in\nimage file should contain a differente saturated color to specify a class (blue, green, red, yellow, cian, etc)\n");
		return 1;
	}

	FILE *arq = fopen(argv[2], "w");

	cv::Mat img;

	img = cv::imread(argv[1], 1);

	printf("Image Size, width = %d, height = %d\n", img.cols, img.rows);
	for(int i = 0; i < img.rows; i++)
		for(int j = 0; j < img.cols; j++) {
			int r = img.at<cv::Vec3b>(i, j).val[0] > 200 ? 1 : 0;
			int g = img.at<cv::Vec3b>(i, j).val[1] > 200 ? 1 : 0;
			int b = img.at<cv::Vec3b>(i, j).val[2] > 200 ? 1 : 0;
			int classe = (r << 2) + (g << 1) + b;
			if(classe != 0)
				fprintf(arq, "%d %d %d\n", j, i, classe);
		}

	fclose(arq);

	return 0;
}

