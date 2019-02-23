#include<stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <allegro.h>
#include<math.h>
#include<stdbool.h>
#include"task.h"


#define N 				2
#define M 				360
#define L				120
#define n 				10
#define SHIFT_NUMBER 	350
#define	UPDATE_D1		35

// ************* GLOBAL VARIABLES *****************

bool	quit			=	false;	//	variabile di terminazione 
bool	stop_graphics	=	true;	//	variabile per lo stop dei thread produttori e consumatori
bool	fibrillation	=	false;
bool	tachycardia		=	false;
bool	arrhythmia		=	false;

//	gruppo di variabile necessari per la lettura dei dati dai file

char 	nome_file[M];				
FILE 	*fp;
float 	DATI[N][M];
float 	vettore[M];
float	aux_draw[M];
float	samp[M];

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
void	sampler();
void	shift();
int		update_D1(int count);
// ******************************** MAIN FUNCTION *********************************

int main(void)
{

	if(init() != true)
		return 1;

	// tasks creation

	task_create(draw_function, 0, 120, 300, 20);
	task_create(generatore, 1, 120, 300, 20);
	task_create(user_command, 2, 100, 80, 20);
	//task_create(info, 3, 100, 300, 20);

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

int 		count = 0;
	
	set_activation(1);
	
	while(quit == false) {

		if(!stop_graphics){

			pthread_mutex_lock(&mutex);
			sampler();
			pthread_mutex_unlock(&mutex);

			// aggiornamento DATI[1]
			count = update_D1(count);

			//shift DATI[0]
			shift(count);

			count++;				
			
		}
		if (deadline_miss(1) == 1) printf("DEADLINE MISS GENERATORE\n");     //soft real time

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
				//printf("%.2f     %.2f\n", aux_draw[i], DATI[0][i]);
			}
			pthread_mutex_unlock(&mutex);
			//printf("[DRAW] MUTEX UNLOCKED\n");

			for(i = 0; i < L; i++){
				
				line(screen, rect_coord_x1 + 4*i, 320 - (int)(140*aux_draw[i]), rect_coord_x1 + 4*i + 4, 320 - (int)(140*aux_draw[i+1]),  makecol(0, 0, 0));

			}
			for(i = L; i < L; i++){
				
				line(screen, rect_coord_x1 + 4*i, 320 - (int)(140*aux_draw[i+120]), rect_coord_x1 + 4*i + 4, 320 - (int)(140*aux_draw[i+121]),  makecol(0, 0, 0));

			}
			for(i = L; i < L; i++){
				
				line(screen, rect_coord_x1 + 4*i, 320 - (int)(140*aux_draw[i+240]), rect_coord_x1 + 4*i + 4, 320 - (int)(140*aux_draw[i+241]),  makecol(0, 0, 0));

			}
		} 
	
		if (deadline_miss(0) == 1) printf("DEADLINE MISS DRAW\n");     //soft real time

		wait_for_activation(0);
	}
	
	return NULL;
}

//-------------------------------------------------------

void *info()
{
	
int n_picchi = 0;
int distanza_p, i; 
float distanza_t; //distanza temporale
char text[24];
	
	set_activation(3);

	while(quit == false) {

		if(!stop_graphics){
			wait_for_activation(3);
		
		for(i = 0; i < M; i++){
			if(DATI[0][i] > 0.96){
				picchi[n_picchi] = i;
				printf("%d\n", i);
				n_picchi++;
			}
		}
			
			
			//calcolo la distanza di campioni tra gli ultimi due picchi, ogni campione corrisponde a 0,0141 secondi
			distanza_p = picchi[n_picchi - 1] - picchi[n_picchi - 2];
			distanza_t = distanza_p * 0.008;
			bpm = floor(60/distanza_t);
			
			printf("BPM: %d\n", bpm);
				
			rectfill(screen, rect_coord_x1-90, rect_coord_y2-4, rect_coord_x1, rect_coord_y2+8, 0);
				
			sprintf(text, "%d", bpm);
			textout_centre_ex(screen, font, text, rect_coord_x1-50, rect_coord_y2, 11, -1);
		
			for(i = 0; i < n_picchi + 1; i++) 
								picchi[i] = 0;
			
			
			pthread_mutex_unlock(&mutex);
			wait_for_activation(3);
			
		}	
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
	sprintf(text, "BPM:");
	textout_centre_ex(screen, font, text, rect_coord_x1-50, rect_coord_y2-15, 10, -1);
	sprintf(value_x, "Press 's' to stop the graphics and press 'r' to resume");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+220, rect_coord_y1 + 15, 15, -1);
	sprintf(value_x, "Press 'f' to start/stop fibrillation");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+220, rect_coord_y1 + 40, 15, -1);
	sprintf(value_x, "Press 't' to start/stop tachycardia");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+220, rect_coord_y1 + 60, 15, -1);
	sprintf(value_x, "Press 'a' to start/stop arrhythmia");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+220, rect_coord_y1 + 80, 15, -1);
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
		for(i = 0; i < L; i++){
			fscanf(fp, "%e,", &DATI[0][i]);
			
			if(i < 10){
				vettore[i] = 0.1;
				vettore[i+120] = 0.1;
				vettore[i+240] = 0.1;
			}
			else{
				vettore[i] = DATI[0][i];
				vettore[i+120] = DATI[0][i];
				vettore[i+240] = DATI[0][i];
			}
			DATI[1][i] = vettore[i];
			DATI[1][i+120] = vettore[i];
			DATI[1][i+240] = vettore[i];
			DATI[0][i+120] = DATI[1][i];
			DATI[0][i+240] = DATI[1][i];
		}
		
		
		
		//for(i=L;i<M;i++) printf("%.2f ", DATI[0][i]);
	}	

	fclose(fp);
	
	return true;
}

//-------------------------

int draw_rect()
{

	rectfill(screen, rect_coord_x1, rect_coord_y1, rect_coord_x2, rect_coord_y2, 15);
	rect(screen, rect_coord_x1, rect_coord_y1, rect_coord_x2, rect_coord_y2, 4);
	
	for(int i = 0; i < 48; i++){
		if(i % 4 == 0)
			line(screen, rect_coord_x1 + i*10, rect_coord_y1, rect_coord_x1 + i*10, rect_coord_y2, 12); // 11
		else
			line(screen, rect_coord_x1 + i*10, rect_coord_y1, rect_coord_x1 + i*10, rect_coord_y2, makecol(255,150,150));
		if(i > 35)
			continue;
		if(i % 4 == 0)
			line(screen, rect_coord_x1, rect_coord_y1 - i*10, rect_coord_x2, rect_coord_y1 - i*10, 12);
		else
			line(screen, rect_coord_x1, rect_coord_y1 - i*10, rect_coord_x2, rect_coord_y1 - i*10, makecol(255,150,150));			
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

		case 'a':
			arrhythmia		=	!arrhythmia;
			break;

		case 't':
			tachycardia		=	!tachycardia;

		default:
			break;

	}
	return;
}

void sampler()
{
int i = 0, j = 0;

	for(i = 0; i < M/2; i++){
		samp[i]		= DATI[1][2 * i];
		samp[i + 59]	= DATI[1][2 * i];
		samp[i + 119]	= DATI[1][2 * i];
		samp[i + 179]	= DATI[1][2 * i];
		//printf("aux(%d) = %f\n", j, aux[j]);
	}
}

void shift(int count)
{
	pthread_mutex_lock(&mutex);
	for(int i = 0; i < M; i++){
		if(i < SHIFT_NUMBER){
			//printf("%.2f\n", DATI[0][i+10]);
			DATI[0][i] = DATI[0][i + 10];
		}
		else{
			if(tachycardia){ // FIXME
				DATI[0][i] = samp[i - 350 + (count * 10)];
			} else{
				DATI[0][i] = DATI[1][i - 350 + (count * 10)];
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}


int update_D1(int count)
{

float 		casuale; 

	pthread_mutex_lock(&mutex);
	if(count > UPDATE_D1){
		//printf("[COUNT %d]\n", count);
		for(int i = 0; i < L; i++){
			if(fibrillation) 
				casuale = rand()%10;
			else
				casuale = 0;

			DATI[1][i] = vettore[i] + (casuale/100);
			DATI[1][i+120] = vettore[i] + (casuale/100);
			DATI[1][i+240] = vettore[i] + (casuale/100);

			if(tachycardia){
				sampler();
			}
		}
		
		count = 0;
	}
	pthread_mutex_unlock(&mutex);
	return count;
}




























