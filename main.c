#include<stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <allegro.h>
#include<math.h>
#include<stdbool.h>
#include"task.h"


#define D 4046
#define M 188


// ---------- GLOBAL VARIABLES --------------

bool	quit			=	false;
bool	stop_graphics	=	false;			

char nome_file[M];
FILE *fp;
float DATI[D][M];

pthread_mutex_t mutex_data = PTHREAD_MUTEX_INITIALIZER;

int	x = 1024, y = 600, col = 4;
int	rect_coord_x1 = 160; // x / 4
int	rect_coord_x2 = 640; // 3/4 * x 
int	rect_coord_y1 = 480;
int	rect_coord_y2 = 120;

// ---------- TASK PROTOTYPES ----------------

void	*	draw_function();
void	*	user_command();


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
	task_create(user_command,  2, 100, 80, 20);

	pthread_join(tid[0],NULL);
	pthread_join(tid[2],NULL);

	return 0;
}

// -------- TASK IMPLEMENTATION --------------

void * user_command()
{

char	key;	//Salviamo qui il carattere inserito dall'utente

    set_activation(2);
    while (quit == false) {
        key = readkey() & 0xFF;
        read_command(key);

        if (deadline_miss(2) == 1) printf("2!\n");     //soft real time
        wait_for_activation(2);
    }
}




void * draw_function()
{

int i;
	
	set_activation(0);
	while(quit == false) {

		if(stop_graphics){
			wait_for_activation(0);
			continue;
		}

		pthread_mutex_lock(&mutex_data);
		for(i = 0; i < 144; i++){
			line(screen, rect_coord_x1 + 3.333*i, 320 - (int)(140*DATI[0][i]), rect_coord_x1 + 3.333*i + 3.333, 320 - (int)(140*DATI[0][i+1]),  12);
		}
		pthread_mutex_unlock(&mutex_data);
		
		if (deadline_miss(0) == 1) printf("DEADLINE MISS\n");     //soft real time
		//printf("number of miss = %d\n", tp[0].dmiss);
		wait_for_activation(0);
	}

	return NULL;
}

// ----------------- FUNCTION IMPLEMENTATION ----------------

bool init()
{

char text[24];

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
			for(j = 0; j < D; j++){
				fscanf(fp, "%e,", &DATI[i][j]);
				//printf("%.2f ", DATI[i][j]);
			}				
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


