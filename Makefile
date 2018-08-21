all:
	g++ segmentar.cpp `pkg-config --libs --cflags opencv` -g -Wall -o segmentar
	g++ createSeedsFileFromImage.cpp `pkg-config --libs --cflags opencv` -g -Wall -o createSeedsFileFromImage

