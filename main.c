#include<stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <allegro.h>
#include<math.h>
#include<stdbool.h>
#include"task.h"


#define N 2
#define M 120
#define n 10
//#define NR_MAX 2 //definisco il numero massimo di lettori


// ************* GLOBAL VARIABLES *****************

bool	quit			=	false;	//	variabile di terminazione 
bool	stop_graphics	=	true;	//	variabile per lo stop dei thread produttori e consumatori
bool	fibrillation	=	false;


//	gruppo di variabile necessari per la lettura dei dati dai file

char 	nome_file[M];				
FILE 	*fp;
float 	DATI[N][M];
float 	vettore[M];
float	aux_draw[M];

//	gruppo per l'esecuzione dei calcoli		

int 	picchi[n];				
int 	bpm; 
int 	read_count = 0;

//	mutex

pthread_mutex_t 	mutex = PTHREAD_MUTEX_INITIALIZER;

//	gruppo variabili per la grafica

int	x = 1024, y = 600, col = 4;
int	rect_coord_x1 = 160; // x / 4
int	rect_coord_x2 = 640; // 3/4 * x 
int	rect_coord_y1 = 480;
int	rect_coord_y2 = 120;

// ************* TASK PROTOTYPES ****************************

void	*	draw_function();	//	task dedito ad aggiornare la grafica (consumatore)
void	*	user_command();		//	task dedito alla lettura dei comandi dell'utente
void 	* 	generatore();		//	task dedito alla generazione dei valori dell'ecg (produttore)
void	* 	info();				//	task dedito ai calcoli relativi l'analisi dell'ecg

// ************* FUNCTION PROTOTYPES **********************

bool	init();					//	funzione di inizializzazione di dati, variabili e grafica
void	init_mutex();					
bool	carica_matrice();
int		draw_rect();			//	funzione per il disegno del rettangolo del grafico
void	read_command(char key);	//	interprete dei comandi inseriti dall'utente

// ******************************** MAIN FUNCTION *********************************

int main(void)
{

	if(init() != true)
		return 1;

	// tasks creation

	task_create(draw_function, 0, 150, 80, 20);
	task_create(generatore, 1, 150, 80, 20);
	task_create(user_command, 2, 100, 80, 20);
	//task_create(info, 3, 100, 80, 20);

	// tasks joining

	pthread_join(tid[0],NULL);
	printf("1\n");
	pthread_join(tid[1],NULL);
	printf("2\n");
	pthread_join(tid[2],NULL);
	printf("3\n");
	//pthread_join(tid[3],NULL);
	//printf("4\n");

	return 0;
}

//***************************** TASK IMPLEMENTATION **********************************

void * user_command()
{

char	key;	//Salviamo qui il carattere inserito dall'utente

    set_activation(2);

    while (quit == false) {
        key = readkey() & 0xFF;
        read_command(key);

		printf("hai digitato %c\n", key);  
		
        if (deadline_miss(2) == 1) printf("2!\n");     //soft real time
        wait_for_activation(2);
    }
}

//-------------------------------------------------------------

void * generatore()
{

int 		i = 0;
int 		j = 0;
int 		count = 0;
float 		casuale; 
	
	set_activation(1);
	
	while(quit == false) {

		if(!stop_graphics){

			pthread_mutex_lock(&mutex);

			if(count > 11){
				//printf("[COUNT %d]\n", count);
				for(i = 0; i < M; i++){
					if(fibrillation) 
						casuale = rand()%10;
					else
						casuale = 0;
					DATI[1][i] = vettore[i] + (casuale/100);
				
				}
				count = 0;
			}
			//printf("[GENERATOR] WAITING FOR MUTEX\n");
			
			//shift

			for(i = 0; i < M; i++){
				if(i < 110){
					DATI[0][i] = DATI[0][i + 10];
				}
				else{
					DATI[0][i] = DATI[1][i - 110 + (count * 10)];
				}
			}

			count++;
/*
			for(i = 0; i < M; i++){
				DATI[1][i] = DATI[1][i+10];
			}
*/					
			pthread_mutex_unlock(&mutex);
		}
		
		//printf("[GENERATOR] MUTEX UNLOCKED\n");
		wait_for_activation(1);
		}
}

//------------------------------------------------------------

void * draw_function()
{

int i;

	set_activation(0);

	while(quit == false) {

		if(!stop_graphics){ //se l'utente setta a true stop_graphics blocca
		
			draw_rect(); // cancello il grafico precedente

			//printf("[DRAW] WAITING FOR MUTEX\n");
			pthread_mutex_lock(&mutex); // sezione critica, lettura variabile condivisa DATI
			for(i = 0; i < M; i++){
				aux_draw[i] = DATI[0][i];
			}
			pthread_mutex_unlock(&mutex);
			//printf("[DRAW] MUTEX UNLOCKED\n");

			for(i = 0; i < M; i++){
				
				line(screen, rect_coord_x1 + 4*i, 320 - (int)(140*aux_draw[i]), rect_coord_x1 + 4*i + 4, 320 - (int)(140*aux_draw[i+1]),  12);

			}

		} 
	
		if (deadline_miss(0) == 1) printf("DEADLINE MISS\n");     //soft real time

		wait_for_activation(0);
	}
	
	return NULL;
}

//-------------------------------------------------------

void *info()
{
	
int n_picchi = 0;
int distanza_p, i;
float distanza_t; //distanza di punti e distanza temporale

	set_activation(3);

	while(quit == false) {

		if(stop_graphics){
			wait_for_activation(3);
			continue;
		}
	
		for(i = 0; i < M; i++){
			if(vettore[i] > 0.9){
				picchi[n_picchi] = i; //salvo la posizione del picco
				n_picchi++;
			}
			//calcolo la distanza di campioni tra gli ultimi due picchi, ogni campione corrisponde a 0,0131 secondi
			distanza_p = picchi[n_picchi-1] - picchi[n_picchi-2];
			distanza_t = distanza_p * 0.0131;
			bpm = floor(60/distanza_t);
		}

			pthread_mutex_unlock(&mutex);
				wait_for_activation(3);
	}
}


// ********************** FUNCTIONS IMPLEMENTATION *****************************


void init_mutex()
{
	if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("\n mutex init failed\n");
    } else {
		printf("\n mutex init ok\n");
	}
}

bool init()
{

char text[24];
char value_x[180];

	//init_mutex();
	srand(time(NULL));

	carica_matrice();
    if(allegro_init() != 0)
		return false;

    install_keyboard();
	install_mouse();
    set_color_depth(8);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, x, y, 0, 0); 
    enable_hardware_cursor();
    show_mouse(screen);
	clear_to_color(screen, makecol(0, 0, 0));

	draw_rect();

	sprintf(text, "Press 'q' to exit");
	textout_centre_ex(screen, font, text, 320, 100, 15, -1);
	sprintf(value_x, "Press 's' to stop the graphics and press 'r' to resume");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+220, rect_coord_y1 + 15, 15, -1);
	sprintf(value_x, "Press 'f' to start/stop fibrillation");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+220, rect_coord_y1 + 40, 15, -1);
	sprintf(value_x, "Warnings");
	textout_centre_ex(screen, font, value_x, rect_coord_x2+180, rect_coord_y2 -15 , 4, -1);

	rectfill(screen, rect_coord_x2 + 60, rect_coord_y1, rect_coord_x2 + 300, rect_coord_y2, 15);
	rect(screen, rect_coord_x2 + 60, rect_coord_y1, rect_coord_x2 + 300, rect_coord_y2, 4);

	return true;
}

//--------------------------

bool carica_matrice()
{
	
	fp = fopen("ptbdb_normal.csv", "r");
	if(fp == NULL){
		return false;
	}
	else {
		int i, j = 0;
		for(i = 0; i < M; i++){
			fscanf(fp, "%e,", &DATI[0][i]);
			if(i < 20)
				vettore[i] = 0.1;
			else
				vettore[i] = DATI[0][i];				
			DATI[1][i] = vettore[i];		
		}
	}	

	fclose(fp);
	
	return true;
}

//-------------------------

int draw_rect()
{

	rectfill(screen, rect_coord_x1, rect_coord_y1, rect_coord_x2, rect_coord_y2, 15);
	rect(screen, rect_coord_x1, rect_coord_y1, rect_coord_x2, rect_coord_y2, 4);
	
	for(int i = 0; i < 47; i++){
		line(screen, rect_coord_x1 + 10 + i*10, rect_coord_y1, rect_coord_x1 + 10 + i*10, rect_coord_y2, 14); // 11
		if(i > 34)
			continue;
		line(screen, rect_coord_x1, rect_coord_y1 - 10 - i*10, rect_coord_x2, rect_coord_y1 - 10 - i*10,  14);

	}
	return 0;
}

//-------------------------

void read_command(char key)
{
	switch(key){

		case 'q':
			quit = true;
			break;

		case 's':
			stop_graphics	=	true;
			break;
		
		case 'r':
			stop_graphics	=	false;
			break;

		case 'f':
			fibrillation	=	!fibrillation;
			break;

		default:
			break;

	}
	return;
}


