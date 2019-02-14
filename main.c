#include<stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <allegro.h>
#include<math.h>
#include<stdbool.h>
#include"task.h"


#define N 2
#define M 114
#define n 10
#define NR_MAX 2 //definisco il numero massimo di lettori


// ---------- GLOBAL VARIABLES --------------

bool	quit			=	false;
bool	stop_graphics	=	false;			

char 	nome_file[M];
FILE 	*fp;
float 	DATI[N][M];
float 	vettore[M];
int 	picchi[n];
int 	bpm; 
int 	read_count = 0;

pthread_mutex_t 	mutex, rw_mutex = PTHREAD_MUTEX_INITIALIZER;

int	x = 1024, y = 600, col = 4;
int	rect_coord_x1 = 160; // x / 4
int	rect_coord_x2 = 640; // 3/4 * x 
int	rect_coord_y1 = 480;
int	rect_coord_y2 = 120;

// ---------- TASK PROTOTYPES ----------------

void	*	draw_function();
void	*	user_command();
void 	* 	generatore();
void	* 	info();


// --------- FUNCTION PROTOTYPES -------------

bool	init();
bool	carica_matrice();
int		draw_rect();
void	read_command(char key);


// --------- MAIN FUNCTION -------------------

int main(void)
{
	if(init() != true)
		return 1;
	task_create(draw_function, 0, 100, 80, 20);
	task_create(generatore, 1, 100, 80, 20);
	task_create(user_command, 2, 100, 80, 20);
	task_create(info, 3, 100, 80, 20);

	pthread_join(tid[0],NULL);
	printf("1\n");
	pthread_join(tid[1],NULL);
	printf("2\n");
	pthread_join(tid[2],NULL);
	printf("3\n");
	pthread_join(tid[3],NULL);
	printf("4\n");

	return 0;
}

// -------- TASK IMPLEMENTATION --------------

void * user_command()
{
struct timespec t;
char	key;	//Salviamo qui il carattere inserito dall'utente

    set_activation(2);
    while (quit == false) {
        key = readkey() & 0xFF;
        read_command(key);

		printf("hai digitato %c\n", key);  
		
        if (deadline_miss(2) == 1) printf("2!\n");     //soft real time
        wait_for_activation(2);
		printf("Ed ha la madre allegra\n");
    }
}



void * generatore()
{

	int 		i = 0;
	int 		j = 0;
	int 		count = 0;
	float 		casuale; 
	for(i = 0; i < M; i++){
		casuale = rand()/12;
		DATI[1][i] = vettore[i] + casuale;
	}
	count = 0;
	set_activation(1);
	while(quit == false) {
		
		if(!stop_graphics){

			pthread_mutex_lock(&mutex);

			if(count > 2){
			//set_activation(1);
				for(i = 0; i < M; i++){
					casuale = rand()%1;
					DATI[1][i] = vettore[i] + casuale;
					//printf("[GENERATOR] casuale = %f\n[GENERATOR] somma = %f\n", casuale, vettore[i] + casuale);
				
				}
				count = 0;
			}
				//shift
			for(i = 0; i < M; i++){
				if(i < 76){
					DATI[0][i] = DATI[0][i+38];
					//DATI[0][75] = DATI[0][114]
				}
				else{
					DATI[0][i] = DATI[1][i-76];
					//DATI[0][						
				}
			}

			count++;

		}
		for(i = 0; i < M; i++){
			printf("%.2f \n", vettore[i]);			
		}
		//	sleep(2);
		//	for(i = 0; i < M; i++){
			//printf("%.2f ", DATI[0][i]);	
				
		//}
			
		pthread_mutex_unlock(&mutex);
		wait_for_activation(1);
		}
}





void * draw_function()
{
	int i;

	set_activation(0);
	while(quit == false) {

		if(!stop_graphics){

		pthread_mutex_lock(&mutex);
	/*	read_count++;
		if (read_count == 1)
						pthread_mutex_lock(&rw_mutex);
		pthread_mutex_unlock(&mutex);
		*/
		for(i = 0; i < 114; i++){
			line(screen, rect_coord_x1 + 3.333*i, 320 - (int)(140*DATI[0][i]), rect_coord_x1 + 3.333*i + 3.333, 320 - (int)(140*DATI[0][i+1]),  12);
		}
		
	/*	pthread_mutex_lock(&mutex);
		read_count--;
		if(read_count == 0)
						pthread_mutex_unlock(&rw_mutex);
		*/
		pthread_mutex_unlock(&mutex);
		
		if (deadline_miss(0) == 1) printf("DEADLINE MISS\n");     //soft real time
		//printf("number of miss = %d\n", tp[0].dmiss);
		wait_for_activation(0);
		}
	}
	
	printf("Mi sono sbloccato, no come giorgio\n");
	return NULL;
}



void *info(){
	
	int n_picchi = 0;
	int distanza_p, i;
	float distanza_t; //distanza di punti e distanza temporale
	set_activation(3);
	while(quit == false) {

		if(stop_graphics){
			wait_for_activation(3);
			continue;
		}
	
/*		pthread_mutex_lock(&mutex);
		read_count++;
		if (read_count == 1)
						pthread_mutex_lock(&rw_mutex);
		pthread_mutex_unlock(&mutex);
*/
		for(i = 0; i < M; i++){
			if(vettore[i] > 0.8){
				picchi[n_picchi] = i; //salvo la posizione del picco
				n_picchi++;
			}
			//calcolo la distanza di campioni tra gli ultimi due picchi, ogni campione corrisponde a 0,0131 secondi
			distanza_p = picchi[n_picchi-1] - picchi[n_picchi-2];
			distanza_t = distanza_p * 0.0131;
			bpm = floor(60/distanza_t);
		}

/*			pthread_mutex_lock(&mutex);
			read_count--;
			if(read_count == 0)
				pthread_mutex_unlock(&rw_mutex); */
			pthread_mutex_unlock(&mutex);
				wait_for_activation(3);
	}
}


// ----------------- FUNCTION IMPLEMENTATION ----------------

bool init()
{

char text[24];
	
	srand(time(NULL));
	printf("RAND MAX = %d\n", RAND_MAX);
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
	
	return true;
}

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
			vettore[i] = DATI[0][i];
			//printf("%.2f %.2f\n", DATI[0][i], vettore[i]);			
		}
	}	
	fclose(fp);
	
	return true;
}


int draw_rect()
{
	rectfill(screen, rect_coord_x1, rect_coord_y1, rect_coord_x2, rect_coord_y2, 15);

	rect(screen, rect_coord_x1, rect_coord_y1, rect_coord_x2, rect_coord_y2, 14);

	char value_x[16];
	char value_y[16];
	for(int i = 0; i < 47; i++){
		line(screen, rect_coord_x1 + 10 + i*10, rect_coord_y1, rect_coord_x1 + 10 + i*10, rect_coord_y2, 14); // 11
		//sprintf(value_x, "%d", i+1);
		//textout_centre_ex(screen, font, value_x, rect_coord_x1 + 10 + i*10, rect_coord_y1 + 5, 1, -1);
		if(i > 34)
			continue;
		line(screen, rect_coord_x1, rect_coord_y1 - 10 - i*10, rect_coord_x2, rect_coord_y1 - 10 - i*10,  14);

	}
	return 0;
}



void read_command(char key)
{
	switch(key){

		case 'q':
			quit = true;
			printf("giorgio marleta, sei un bastardo\n");
			break;

		case 's':
			stop_graphics = true;
			break;
		
		case 'r':
			stop_graphics = false;
			break;

		default:
			break;

	}
	return;
}


