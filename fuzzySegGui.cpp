#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <time.h>

GtkWidget *window, *image;
GtkWidget *vbox, *hbox;
GtkWidget *label1, *label2;
GtkWidget *eventBox;
char *nomeArquivo;

//widgets das opcoes
GtkWidget *widgetControleNivel;
GtkAdjustment *adjustmentControleNivel;

GtkWidget *widgetMisturarCanais;


typedef struct Imagem {
	unsigned char ***m;
	int w;
	int h;
	int numCanais;
} Imagem;


#include "fuzzyseg.h"

FuzzySegmentation *fs;
vector<int> seeds[3];

void printImagemInfo(Imagem img) {
	printf("Image size %d %d, ch %d\n", img.w, img.h, img.numCanais);
}

void carregarImagem(GtkWidget *widget, gpointer data) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		nomeArquivo = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );
		gtk_widget_destroy(dialog);
	}

	gtk_image_set_from_file(GTK_IMAGE(image), nomeArquivo);

	gtk_widget_queue_draw(image);

	gtk_label_set_text(GTK_LABEL(label1), "Image loaded");
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
	int nivel = (int) gtk_adjustment_get_value(adjustmentControleNivel);
	int ch1, ch2, ch3;

	fs = new FuzzySegmentation(&origem, origem.w, origem.h);
	for(int i = 0; i < seeds[0].size(); i++)
		fs->addSeed(seeds[0][i], seeds[1][i], seeds[2][i]);
	fs->fuzzySeg(1000);

	ch1 = 0;
	ch2 = 1;
	ch3 = 2;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgetMisturarCanais))) {
		ch1 = rand()%3;
		ch2 = (ch1+1+rand()%2)%3;
		ch3 = 3 - ch2 - ch1;
	}

	for(j = 0; j < destino.w; j++) {
		for(i = 0; i < destino.h; i++) {
			int r = 255*((fs->getClasse(j, i)&1) != 0);
			int g = 255*((fs->getClasse(j, i)&2) != 0);
			int b = 255*((fs->getClasse(j, i)&4) != 0);
			destino.m[i][j][0] = MIN(r*fs->getAffinity(j, i), 255);
			destino.m[i][j][1] = MIN(g*fs->getAffinity(j, i), 255);
			destino.m[i][j][2] = MIN(b*fs->getAffinity(j, i), 255);
		}
	}
	return destino;
}

void funcaoRestaurar(GtkWidget *widget, gpointer data) {

	gtk_image_set_from_file(GTK_IMAGE(image), nomeArquivo);
	gtk_widget_queue_draw(image);
	gtk_label_set_text(GTK_LABEL(label1), "Image restored");
}

void funcaoAplicar(GtkWidget *widget, gpointer data) {

	funcaoRestaurar(NULL, NULL);
	Imagem img = obterMatrizImagem();
	Imagem res = meuFiltro(img);
	desalocarImagem(img);
	atualizarGtkImage(res);
	desalocarImagem(res);
	gtk_label_set_text(GTK_LABEL(label1), "Filter applied");
}   

gboolean funcaoMouse(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	seeds[0].push_back((int)event->x);
	seeds[1].push_back((int)event->y);
	int nivel = (int) gtk_adjustment_get_value(adjustmentControleNivel);
	seeds[2].push_back(nivel);
	printf("seed at %f %f, class %d\n", event->x, event->y, nivel);
}

int main(int argc, char **argv) {


	//inicializa a semente de acordo com time
	srand(time(NULL));

	//inicializa o gtk
	gtk_init(&argc, &argv);

	//o argumento pode ser GTK_WINDOW_TOPLEVEL ou GTK_WINDOW_POPUP
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	//altera o titulo da janela
	gtk_window_set_title(GTK_WINDOW(window), "Fuzzy Segmentation");
	
	//altera a posicao da janela para estar centralizada
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	//altera o tamanho da janela
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

	//a janela pode ser redimensionada
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

	//espessura da borda da janela (espaco sem widgets)
	gtk_container_set_border_width(GTK_CONTAINER(window), 20);

	//GtkVBox eh um container vertical para widgets
	//o primeiro argumento significa se os widgets sao igualmente distribuidos
	//o segundo argumento significa o espacamento entre os widgets em pixels
	vbox = gtk_vbox_new(FALSE, 5);
	GtkWidget *vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 20);

	//GtkHBox eh um container horizontal para widgets
	hbox = gtk_hbox_new(TRUE, 3);

	//cria um botao com titulo carregar imagem
	GtkWidget *botaoCarregar = gtk_button_new_with_label("Load Image");

	//cria um botao com titulo aplicar filtro
	GtkWidget *botaoAplicar = gtk_button_new_with_label("Apply Filter");

	//cria um botao para restaurar imagem
	GtkWidget *botaoRestaurar = gtk_button_new_with_label("Restore Image");
	
	//cria um botao para salvar imagem
	GtkWidget *botaoSalvar = gtk_button_new_with_label("Save Image");

	//cria um frame para colocar as opcoes do filtro
	GtkWidget *frameFiltro = gtk_frame_new("Filter Options");

	//cria um scrolled window para inserir a imagem
	GtkWidget *scrolledWindow = gtk_scrolled_window_new(gtk_adjustment_new(0, 0, 100, 1, 1, 1), gtk_adjustment_new(0, 0, 100, 1, 1, 1));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	//widgets das opcoes de filtro
	adjustmentControleNivel =  gtk_adjustment_new(0, 1, 30, 1, 1, 1);
	widgetControleNivel = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjustmentControleNivel);
	widgetMisturarCanais = gtk_check_button_new_with_label("Mix Channels");


	//altera o tamanho do botao carregar
	gtk_widget_set_size_request(botaoCarregar, 70, 30);
	//altera o tamanho do botao aplicar
	gtk_widget_set_size_request(botaoAplicar, 70, 30);

	//adiciona os botoes no container horizontal (hbox)
	gtk_container_add(GTK_CONTAINER(hbox), botaoCarregar);
	gtk_container_add(GTK_CONTAINER(hbox), botaoAplicar);
	gtk_container_add(GTK_CONTAINER(hbox), botaoRestaurar);
	gtk_container_add(GTK_CONTAINER(hbox), botaoSalvar);

	//cria labels
	label1 = gtk_label_new("Load an image");
	label2 = gtk_label_new("Author: Rafael Beserra Gomes");

	eventBox = gtk_event_box_new();
	//adiciona um widget imagem vazio
	image = gtk_image_new();

	//adiciona os demais widgets no container vertical (vbox)
	//a funcao gtk_box_pack_start eh similar a gtk_container_add
	//mas com mais parametros
	gtk_box_pack_start(GTK_BOX(vbox), scrolledWindow, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(eventBox), image);
	gtk_container_add(GTK_CONTAINER(scrolledWindow), eventBox);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), frameFiltro, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(frameFiltro), vbox2);
	gtk_container_add(GTK_CONTAINER(vbox2), widgetControleNivel);
	gtk_container_add(GTK_CONTAINER(vbox2), widgetMisturarCanais);

	//adiciona o vbox na janela (window)
	gtk_container_add(GTK_CONTAINER(window), vbox);

	//trata o clique do mouse no eventBox
	g_signal_connect(G_OBJECT(eventBox), "button_press_event", G_CALLBACK(funcaoMouse), NULL);

	//conecta o evento clicked do botaoCarregar a funcao carregarImagem
	g_signal_connect(G_OBJECT(botaoCarregar), "clicked", G_CALLBACK(carregarImagem), NULL);

	//conecta o evento clicked do botaoAplicar a funcaoTeste (argv[0] eh o argumento)
	g_signal_connect(G_OBJECT(botaoAplicar), "clicked", G_CALLBACK(funcaoAplicar), NULL);
	
	//conecta o evento clicked do botaoAplicar a funcaoTeste (argv[0] eh o argumento)
	//g_signal_connect(G_OBJECT(widgetControleNivel), "value-changed", G_CALLBACK(funcaoAplicar), NULL);
	
	//conecta o evento clicked do botaoRestaurar a funcaoRestaurar
	g_signal_connect(G_OBJECT(botaoRestaurar), "clicked", G_CALLBACK(funcaoRestaurar), NULL);

	//conecta o evento clicked do botaoSalvar a funcao salvarImagem
	g_signal_connect(G_OBJECT(botaoSalvar), "clicked", G_CALLBACK(salvarImagem), NULL);

	//conecta o evento destroy da janela a
	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	//exibe a janela window e todos os widgets e assim sucessivamente
	gtk_widget_show_all(window);

	//main loop do gtk
	gtk_main();

	//so escreve quando finalizar a interface grafica
	printf("fim do programa\n");

	return 0;
}
