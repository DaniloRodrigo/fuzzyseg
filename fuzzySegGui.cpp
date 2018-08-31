#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <time.h>

GtkWidget *window, *image;
GtkWidget *vbox, *hboxButton1, *hbox2, *hbox3, *hboxButton2;
GtkWidget *label1, *label2, *label3, *label4;
GtkWidget *eventBox;
char *nomeArquivoImagem;

//widgets das opcoes
GtkWidget *widgetControleClasse;
GtkAdjustment *adjustmentControleClasse;

GtkWidget *widgetControleSegmentacao;
GtkAdjustment *adjustmentControleSegmentacao;

GtkWidget *widgetMisturarCanais;


typedef struct Imagem {
	unsigned char ***m;
	int w;
	int h;
	int numCanais;
} Imagem;
Imagem original, resultado;

#include "fuzzyseg.h"

FuzzySegmentation *fs;
vector<int> seeds[3];

void printImagemInfo(Imagem img) {
	printf("Image size %d %d, ch %d\n", img.w, img.h, img.numCanais);
}

//retorna uma NOVA struct Imagem do GtkImage
Imagem obterMatrizImagem() {
	int i, j, ch, rowstride;
	guchar ***m, *pixels, *p;
	Imagem img;
	
	GdkPixbuf *buffer = gtk_image_get_pixbuf(GTK_IMAGE(image));
	img.h = gdk_pixbuf_get_height(buffer);
	img.w = gdk_pixbuf_get_width(buffer);
	img.numCanais = gdk_pixbuf_get_n_channels(buffer);
	rowstride = gdk_pixbuf_get_rowstride(buffer);
	pixels = gdk_pixbuf_get_pixels(buffer);

	img.m = (guchar ***) malloc(sizeof(guchar **)*img.h);
	for(i = 0; i < img.h; i++) {
		img.m[i] = (guchar **) malloc(sizeof(guchar *)*img.w);
		for(j = 0; j < img.w; j++)
			img.m[i][j] = (guchar *) malloc(sizeof(guchar)*img.numCanais);
	}

	for(i = 0; i < img.h; i++) {
		p = pixels + rowstride*i;
		for(j = 0; j < img.w; j++) {
			for(ch = 0; ch < img.numCanais; ch++) {
				img.m[i][j][ch] = *p;
				p++;
			}
		}
	}
	return img;
}


void carregarImagem(GtkWidget *widget, gpointer data) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		nomeArquivoImagem = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );
		gtk_widget_destroy(dialog);
	}

	gtk_image_set_from_file(GTK_IMAGE(image), nomeArquivoImagem);

	gtk_widget_queue_draw(image);

	gtk_label_set_text(GTK_LABEL(label1), "Image loaded");

	original = obterMatrizImagem();
	fs = new FuzzySegmentation(&original, original.w, original.h);
}

void carregarSeeds(GtkWidget *widget, gpointer data) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	char *nomeArquivo;
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		nomeArquivo = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );
		gtk_widget_destroy(dialog);
	}

	FILE *arq = fopen(nomeArquivo, "r");

	int x, y, classe;
	while(fscanf(arq, "%d %d %d", &x, &y, &classe) == 3) {
		printf("added seed: x = %d y = %d class = %d\n", x, y, classe);
		fs->addSeed(x, y, classe);
	}
	fclose(arq);
}

void salvarImagem(GtkWidget *widget, gpointer data) {

	GtkWidget *dialog = gtk_file_chooser_dialog_new("Nome arquivo", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	char *nomeDestino;

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		nomeDestino = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );
		gtk_widget_destroy(dialog);
		GdkPixbuf *buffer = gtk_image_get_pixbuf(GTK_IMAGE(image));
		gdk_pixbuf_save(buffer, nomeDestino, "jpeg", NULL, "quality", "100", NULL);
		gtk_label_set_text(GTK_LABEL(label1), "Image saved");
	}
	
}

void salvarSeeds(GtkWidget *widget, gpointer data) {

	GtkWidget *dialog = gtk_file_chooser_dialog_new("Nome arquivo", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	char *nomeDestino;

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		nomeDestino = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );
		gtk_widget_destroy(dialog);
		fs->saveSeedSet(nomeDestino);
		gtk_label_set_text(GTK_LABEL(label1), "Seeds saved");
	}
	
}


void atualizarGtkImage(Imagem img) {
	int i, j, ch;

	GdkPixbuf *buffer = gtk_image_get_pixbuf(GTK_IMAGE(image));
	GdkPixbuf *newbuffer = gdk_pixbuf_copy(buffer);
	int rowstride = gdk_pixbuf_get_rowstride(buffer);

	guchar *pixels = gdk_pixbuf_get_pixels(newbuffer);
	guchar *p;
	for(i = 0; i < img.h; i++) {
		p = pixels + rowstride*i;
		for(j = 0; j < img.w; j++)
			for(ch = 0; ch < img.numCanais; ch++) {
				*p = img.m[i][j][ch];
				p++;
			}
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(image), newbuffer);
}

Imagem alocarImagem(Imagem referencia) {
	Imagem img;
	int i, j;
	img.h = referencia.h;
	img.w = referencia.w;
	img.numCanais = referencia.numCanais;
	img.m = (guchar ***) malloc(sizeof(guchar **)*img.h);
	for(i = 0; i < img.h; i++) {
		img.m[i] = (guchar **) malloc(sizeof(guchar *)*img.w);
		for(j = 0; j < img.w; j++)
			img.m[i][j] = (guchar *) malloc(sizeof(guchar)*img.numCanais);
	}
	return img;
}

void desalocarImagem(Imagem referencia) {
	for(int i = 0; i < referencia.h; i++) {
		for(int j = 0; j < referencia.w; j++) {
			free(referencia.m[i][j]);
		}
		free(referencia.m[i]);
	}
	free(referencia.m);
}

Imagem meuFiltro(Imagem origem) {
	int i, j;
	Imagem destino = alocarImagem(origem);
	int nivel = (int) gtk_adjustment_get_value(adjustmentControleClasse);

	//fs = new FuzzySegmentation(&origem, origem.w, origem.h);
	//for(int i = 0; i < seeds[0].size(); i++)
	//	fs->addSeed(seeds[0][i], seeds[1][i], seeds[2][i]);
	fs->fuzzySeg();

	for(j = 0; j < destino.w; j++) {
		for(i = 0; i < destino.h; i++) {
			int r = 255*((fs->getClasse(j, i)&1) != 0);
			int g = 255*((fs->getClasse(j, i)&2) != 0);
			int b = 255*((fs->getClasse(j, i)&4) != 0);
			float aff = pow(fs->getNormalizedAffinity(j, i), 5);
			destino.m[i][j][0] = MIN(r*aff, 255);
			destino.m[i][j][1] = MIN(g*aff, 255);
			destino.m[i][j][2] = MIN(b*aff, 255);
		}
	}
	return destino;
}

Imagem segmentationStep(Imagem origem, int step) {
	int i, j;
	Imagem destino = alocarImagem(origem);

/*	fs = new FuzzySegmentation(&origem, origem.w, origem.h);
	for(int i = 0; i < seeds[0].size(); i++)
		fs->addSeed(seeds[0][i], seeds[1][i], seeds[2][i]);*/
	fs->step(NUM_AFF - step - 1);

	for(j = 0; j < destino.w; j++) {
		for(i = 0; i < destino.h; i++) {
			int r = 255*((fs->getClasse(j, i)&1) != 0);
			int g = 255*((fs->getClasse(j, i)&2) != 0);
			int b = 255*((fs->getClasse(j, i)&4) != 0);
			float aff = pow(fs->getNormalizedAffinity(j, i), 5);
			destino.m[i][j][0] = MIN(r*aff, 255);
			destino.m[i][j][1] = MIN(g*aff, 255);
			destino.m[i][j][2] = MIN(b*aff, 255);
		}
	}
	return destino;
}

void funcaoRestaurar(GtkWidget *widget, gpointer data) {

	gtk_image_set_from_file(GTK_IMAGE(image), nomeArquivoImagem);
	gtk_widget_queue_draw(image);
	gtk_label_set_text(GTK_LABEL(label1), "Image restored");
}

void funcaoAplicar(GtkWidget *widget, gpointer data) {

	funcaoRestaurar(NULL, NULL);
	//Imagem img = obterMatrizImagem();
	Imagem res = meuFiltro(original);
	//desalocarImagem(img);
	atualizarGtkImage(res);
	desalocarImagem(res);
	gtk_label_set_text(GTK_LABEL(label1), "Filter applied");
}   

void funcaoAplicarPasso(GtkWidget *widget, gpointer data) {

	funcaoRestaurar(NULL, NULL);
	//Imagem img = obterMatrizImagem();
	int nivel = (int) gtk_adjustment_get_value(adjustmentControleSegmentacao);
	Imagem res = segmentationStep(original, nivel);
	//desalocarImagem(img);
	atualizarGtkImage(res);
	desalocarImagem(res);
	gtk_label_set_text(GTK_LABEL(label1), "Filter applied");
}   

gboolean funcaoMouse(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	int nivel = (int) gtk_adjustment_get_value(adjustmentControleClasse);
	printf("seed at %f %f, class %d\n", event->x, event->y, nivel);
	fs->addSeed((int)event->x, (int)event->y, nivel);
}

int main(int argc, char **argv) {

	srand(time(NULL));
	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Fuzzy Segmentation");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(window), 20);

	//GtkVBox eh um container vertical para widgets
	//o primeiro argumento significa se os widgets sao igualmente distribuidos
	//o segundo argumento significa o espacamento entre os widgets em pixels
	vbox = gtk_vbox_new(FALSE, 5);
	GtkWidget *vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 20);

	//GtkHBox eh um container horizontal para widgets
	hboxButton1 = gtk_hbox_new(TRUE, 3);
	hbox2 = gtk_hbox_new(TRUE, 3);
	hbox3 = gtk_hbox_new(TRUE, 3);
	hboxButton2 = gtk_hbox_new(TRUE, 3);

	GtkWidget *botaoCarregar = gtk_button_new_with_label("Load Image");
	GtkWidget *botaoAplicar = gtk_button_new_with_label("Run Segmentation");
	GtkWidget *botaoRestaurar = gtk_button_new_with_label("Restore Image");
	GtkWidget *botaoSalvar = gtk_button_new_with_label("Save Image");
	GtkWidget *botaoCarregarSeeds = gtk_button_new_with_label("Load Seeds");
	GtkWidget *botaoSalvarSeeds = gtk_button_new_with_label("Save Seeds");
	GtkWidget *botaoRunStepByStep = gtk_button_new_with_label("Step By Step");

	//cria um frame para colocar as opcoes do filtro
	GtkWidget *frameFiltro = gtk_frame_new("Filter Options");

	//cria um scrolled window para inserir a imagem
	GtkWidget *scrolledWindow = gtk_scrolled_window_new(gtk_adjustment_new(0, 0, 100, 1, 1, 1), gtk_adjustment_new(0, 0, 100, 1, 1, 1));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	adjustmentControleClasse =  gtk_adjustment_new(0, 1, 30, 1, 1, 1);
	widgetControleClasse = gtk_spin_button_new(adjustmentControleClasse, 1, 0);
	adjustmentControleSegmentacao =  gtk_adjustment_new(0, 0, NUM_AFF-1, 1.0, 100.0, 0);
	widgetControleSegmentacao = gtk_spin_button_new(adjustmentControleSegmentacao, 1, 0);
	widgetMisturarCanais = gtk_check_button_new_with_label("Mix Channels");

	gtk_widget_set_size_request(botaoCarregar, 70, 30);
	gtk_widget_set_size_request(botaoAplicar, 70, 30);

	gtk_container_add(GTK_CONTAINER(hboxButton1), botaoCarregar);
	gtk_container_add(GTK_CONTAINER(hboxButton1), botaoAplicar);
	gtk_container_add(GTK_CONTAINER(hboxButton1), botaoRestaurar);
	gtk_container_add(GTK_CONTAINER(hboxButton1), botaoSalvar);
	gtk_container_add(GTK_CONTAINER(hboxButton2), botaoCarregarSeeds);
	gtk_container_add(GTK_CONTAINER(hboxButton2), botaoSalvarSeeds);
	gtk_container_add(GTK_CONTAINER(hboxButton2), botaoRunStepByStep);

	label1 = gtk_label_new("Load an image");
	label2 = gtk_label_new("Author: Rafael Beserra Gomes");

	eventBox = gtk_event_box_new();
	image = gtk_image_new(); //adiciona um widget imagem vazio

	gtk_box_pack_start(GTK_BOX(vbox), scrolledWindow, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(eventBox), image);
	gtk_container_add(GTK_CONTAINER(scrolledWindow), eventBox);

	gtk_box_pack_start(GTK_BOX(vbox), hboxButton1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hboxButton2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), frameFiltro, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(frameFiltro), vbox2);
	gtk_box_pack_start(GTK_BOX(hbox2), gtk_label_new("Class"), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(hbox2), widgetControleClasse);
	gtk_box_pack_start(GTK_BOX(hbox3), gtk_label_new("Segmentation Step"), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(hbox3), widgetControleSegmentacao);

	gtk_container_add(GTK_CONTAINER(vbox2), hbox2);
	gtk_container_add(GTK_CONTAINER(vbox2), hbox3);
	gtk_container_add(GTK_CONTAINER(vbox2), widgetMisturarCanais);

	//adiciona o vbox na janela (window)
	gtk_container_add(GTK_CONTAINER(window), vbox);

	g_signal_connect(G_OBJECT(eventBox), "button_press_event", G_CALLBACK(funcaoMouse), NULL);
	g_signal_connect(G_OBJECT(botaoCarregar), "clicked", G_CALLBACK(carregarImagem), NULL);
	g_signal_connect(G_OBJECT(botaoAplicar), "clicked", G_CALLBACK(funcaoAplicar), NULL);
	g_signal_connect(G_OBJECT(widgetControleSegmentacao), "value-changed", G_CALLBACK(funcaoAplicarPasso), NULL);
	g_signal_connect(G_OBJECT(botaoRestaurar), "clicked", G_CALLBACK(funcaoRestaurar), NULL);
	g_signal_connect(G_OBJECT(botaoSalvar), "clicked", G_CALLBACK(salvarImagem), NULL);
	g_signal_connect(G_OBJECT(botaoCarregarSeeds), "clicked", G_CALLBACK(carregarSeeds), NULL);
	g_signal_connect(G_OBJECT(botaoSalvarSeeds), "clicked", G_CALLBACK(salvarSeeds), NULL);
	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}
