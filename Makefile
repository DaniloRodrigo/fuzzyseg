all:
	g++ segmentar.cpp `pkg-config --libs --cflags opencv` -g -Wall -o segmentar -DFS_OPENCV
	g++ createSeedsFileFromImage.cpp `pkg-config --libs --cflags opencv` -g -Wall -o createSeedsFileFromImage
	g++ fuzzySegGui.cpp `pkg-config --libs --cflags gtk+-3.0` -g -Wall -o gui -DFS_GTKIMG -O3

