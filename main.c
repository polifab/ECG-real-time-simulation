#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<pthread.h>
#include 	<allegro.h>
#include 	<math.h>
#include 	<stdbool.h>
#include 	"task.h"


#define 	N 			2
#define 	M 			360
#define		L			120
#define		n 			10
#define 	q			5
#define 	SHIFT_NUMBER 		350
#define		UPDATE_D1		35
#define 	pick_value		0.96
#define 	sampling_time		0.008

// TASKS IDENTIFIER MACRO

#define		DRAW_TASK		0
#define		GENR_TASK		1
#define		USER_TASK		2
#define		INFO_TASK		3
#define		ANOM_TASK		4

// LIMIT DETECTOR MACRO

#define		fibril_limit		7
#define 	arrhyt_limit		3

// ************* GLOBAL VARIABLES *****************

bool		quit		=	false;	//	variabile di terminazione 
bool		stop_graphics	=	true;	//	variabile per lo stop dei thread produttori e consumatori
bool		fibrillation	=	false;
bool		tachycardia	=	false;
bool		arrhythmia	=	false;
bool		anomaly_fibril	=	false; 	//  variabili per la rilevazione delle anomalie
bool		anomaly_tachy	=	false;
bool		anomaly_brady	=	false;
bool		anomaly_arrhyt	=	false;

//	gruppo di variabili necessarie per la lettura dei dati dai file

char 		nome_file[M];				
FILE 		*fp;
FILE 		*fw;
float 		DATI[N][M];
float 		vettore[M];
float		aux_draw[M];
float		samp[M];
float		buff_arr[L];
float		arr[M];

//	gruppo di variabili per l'esecuzione dei calcoli		

int 		picchi[n];				
int 		bpm;
int 		arrhyt_count 	= 	 0;
int 		read_count 	=	 0;
int 		bpm_counter 	= 	 0;
int		fibrill_vect[n];
int 		arrhyt_vect[q];
int 		bpm_save[q];
int 		count_time 	= 	 0;
float		moment;

//	mutex

pthread_mutex_t 	mutex = PTHREAD_MUTEX_INITIALIZER;

//	gruppo di variabili per la grafica

int		x = 1024, y = 600, col = 4;
int		rect_coord_x1 = 160; // x / 4
int		rect_coord_x2 = 640; // 3/4 * x 
int		rect_coord_y1 = 480;
int		rect_coord_y2 = 120;

// ************* TASK PROTOTYPES ****************************

void	*	draw_function();		//	task dedito ad aggiornare la grafica (consumatore)
void	*	user_command();			//	task dedito alla lettura dei comandi dell'utente
void 	* 	generatore();			//	task dedito alla generazione dei valori dell'ecg (produttore)
void	* 	info();				//	task dedito ai calcoli relativi l'analisi dell'ecg
void	* 	anomaly_detector(); 		// 	task dedito alla rilevazione di anomalie del ritmo sinusale
void 	*	write_report(); 		//	task dedito alla scrittura su file delle anomalie riscontrate

// ************* FUNCTION PROTOTYPES **********************

bool		init();				//	funzione di inizializzazione di dati, variabili e grafica
void		init_mutex();					
bool		carica_matrice();
int		draw_rect();			//	funzione per il disegno del rettangolo del grafico
void		read_command(char key);		//	interprete dei comandi inseriti dall'utente
void		sampler();
int		arrhythmia_sim();
void		shift();
int		update_D1(int count);
int 		bpm_calculation(int counter); 	//funzione per il calcolo dei bpm
void 		simulation_notice();		  
void 		warnings();
bool		arrhythmia_detector(int beat); 	//funzione per la rilevazione dell'aritmia		
bool		fibrillation_detector(); 	//funzione per la rilevazione della fibrillazione
bool		write_anomaly(float time_);	//funzione per la scrittura su file

// ******************************** MAIN FUNCTION *********************************

int main(void)
{

	if(init() != true)
		return 1;

	// tasks creation

	task_create(draw_function, DRAW_TASK, 82, 500, 20);
	task_create(generatore, GENR_TASK, 82, 500, 20);
	task_create(user_command, USER_TASK, 100, 80, 20);
	task_create(info, INFO_TASK, 200, 300, 20);
	task_create(anomaly_detector, ANOM_TASK, 400, 300, 20);

	// tasks joining

	pthread_join(tid[DRAW_TASK],NULL);
	printf("DRAW TASK\n");
	pthread_join(tid[GENR_TASK],NULL);
	printf("GENERATOR TASK\n");
	pthread_join(tid[USER_TASK],NULL);
	printf("USER TASK\n");
	pthread_join(tid[INFO_TASK],NULL);
	printf("INFO TASK\n");
	pthread_join(tid[ANOM_TASK],NULL);
	printf("ANOMALY DETECTOR TASK\n");

	
	return 0;
}

//***************************** TASKS IMPLEMENTATION **********************************

void * user_command()
{

char	key;	//Salviamo qui il carattere inserito dall'utente

    set_activation(USER_TASK);

    while (quit == false) {
        key = readkey() & 0xFF;
        read_command(key);

		printf("hai digitato %c\n", key);  
		
        if (deadline_miss(USER_TASK) == 1) printf("2!\n");     //soft real time
        wait_for_activation(USER_TASK);
    }
}

//-------------------------------------------------------------

void * generatore()
{

int 	count = 0;
	
	set_activation(GENR_TASK);
	
	while (quit == false) {

		if (!stop_graphics) {
			
			pthread_mutex_lock(&mutex);

			sampler();
			pthread_mutex_unlock(&mutex);

			// aggiornamento DATI[1]
			count = update_D1(count);

			//shift DATI[0]
			shift(count);
			
			count++;				
			
		}
		if (deadline_miss(GENR_TASK) == 1) printf("DEADLINE MISS GENERATORE\n");     //soft real time

		wait_for_activation(GENR_TASK);
	}
}

//------------------------------------------------------------

void * draw_function()
{

int i;

	set_activation(DRAW_TASK);

	while (quit == false) {

		if (!stop_graphics) { //se l'utente setta a true stop_graphics blocca
		
			draw_rect(); // cancello il grafico precedente

			//printf("[DRAW] WAITING FOR MUTEX\n");
			pthread_mutex_lock(&mutex); // sezione critica, lettura variabile condivisa DATI
			for(i = 0; i < M; i++){
				aux_draw[i] = DATI[0][i];
				//printf("%.2f     %.2f\n", aux_draw[i], DATI[0][i]);
			}
			pthread_mutex_unlock(&mutex);
			//printf("[DRAW] MUTEX UNLOCKED\n");

			/*for(i = 0; i < L; i++){
				
				line(screen, rect_coord_x1 + 4*i, 320 - (int)(140*aux_draw[i]), rect_coord_x1 + 4*i + 4, 320 - (int)(140*aux_draw[i+1]),  makecol(0, 0, 0));

			}
			for(i = L; i < L; i++){
				
				line(screen, rect_coord_x1 + 4*i, 320 - (int)(140*aux_draw[i+120]), rect_coord_x1 + 4*i + 4, 320 - (int)(140*aux_draw[i+121]),  makecol(0, 0, 0));

			}
			for(i = L; i < L; i++){
				
				line(screen, rect_coord_x1 + 4*i, 320 - (int)(140*aux_draw[i+240]), rect_coord_x1 + 4*i + 4, 320 - (int)(140*aux_draw[i+241]),  makecol(0, 0, 0));

			}*/

			for(i = 0; i < M; i++){
				line(screen, rect_coord_x1 + 1.5*i, 340 - (int)(140*aux_draw[i]), rect_coord_x1 + 1.5*i + 1.5, 340 - (int)(140*aux_draw[i+1]),  makecol(0, 0, 0));
			}

		} 
	
		if (deadline_miss(DRAW_TASK) == 1) printf("DEADLINE MISS DRAW\n");     //soft real time

		wait_for_activation(DRAW_TASK);
	}
	
	return NULL;
}

//-------------------------------------------------------

void *info()
{
	
int 	counter	= 0;
	
	set_activation(INFO_TASK);

	while (quit == false) {

		if (!stop_graphics) {
			//wait_for_activation(INFO_TASK);
			
			counter++;
			bpm_calculation(counter);
			simulation_notice();
			
			//scrivo le informazione su file
			//ogni campione vale 0.008 s
			moment = count_time * sampling_time * n;
			write_anomaly(moment);
			
			//pthread_mutex_unlock(&mutex);
			wait_for_activation(INFO_TASK);
			
		} 
	}
}

//-------------------------------------------------------

void * anomaly_detector()
{

	set_activation(ANOM_TASK);

	while (quit == false) {

		if (!stop_graphics) {

			
			if (bpm >= 100) {
				anomaly_tachy = true;
			} else {
				anomaly_tachy = false;
			} 
			

			if (bpm <= 50) {
				anomaly_brady = true;
			} else {
				anomaly_brady = false;
			}
			

			if (arrhythmia_detector(bpm)) {
				anomaly_arrhyt = true;
			} else {
				anomaly_arrhyt = false;
			}

			if (fibrillation_detector()) {
				anomaly_fibril = true;
			} else {
				anomaly_fibril = false;
			}

			warnings();

			wait_for_activation(ANOM_TASK);
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

char 	text[24];
char 	value_x[180];

	//init_mutex();
	srand(time(NULL));

	carica_matrice();
   	if (allegro_init() != 0)
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
	textout_centre_ex(screen, font, text, 380, 100, 15, -1);
	sprintf(text, "BPM:");
	textout_centre_ex(screen, font, text, rect_coord_x1-50, rect_coord_y2-20, 10, -1);
	sprintf(value_x, "Press 's' to start/stop the graphics");
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
	if (fp == NULL) {
		return false;
	}
	else {
		int i, j = 0;
		for(i = 0; i < L; i++){
			fscanf(fp, "%e,", &DATI[0][i]);
			//printf("%e\n", DATI[0][i]);
			if (i < 10) {
				vettore[i] = 0.1;
				vettore[i+120] = 0.1;
				vettore[i+240] = 0.1;
			}
			else {
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
		
		for (i = 0; i < L; i++) {
			buff_arr[i] = vettore[i];
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
	
	for (int i = 0; i < 48; i++) {
		if(i % 4 == 0)
			line(screen, rect_coord_x1 + i*10, rect_coord_y1, rect_coord_x1 + i*10, rect_coord_y2, 12); // 11
		else
			line(screen, rect_coord_x1 + i*10, rect_coord_y1, rect_coord_x1 + i*10, rect_coord_y2, makecol(255,150,150));
		if (i > 35)
			continue;
		if (i % 4 == 0)
			line(screen, rect_coord_x1, rect_coord_y1 - i*10, rect_coord_x2, rect_coord_y1 - i*10, 12);
		else
			line(screen, rect_coord_x1, rect_coord_y1 - i*10, rect_coord_x2, rect_coord_y1 - i*10, makecol(255,150,150));			
	}

	return 0;
}

//-------------------------

void read_command(char key)
{

	switch (key) {

		case 'q':
			quit = true;
			break;

		case 's':
			stop_graphics	=	!stop_graphics;
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

//--------------------------------------------

void sampler()
{

	if (tachycardia == false)
		return;

int i = 0, j = 0;

	for (i = 0; i < M/2; i++) {
		samp[i]		= DATI[1][2 * i];
		samp[i + 59]	= DATI[1][2 * i];
		samp[i + 119]	= DATI[1][2 * i];
		samp[i + 179]	= DATI[1][2 * i];
		//printf("aux(%d) = %f\n", j, aux[j]);
	}
}

//----------------------------------------------

void shift(int count)
{
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < M; i++) {
		if (i < SHIFT_NUMBER ){
			//printf("%.2f\n", DATI[0][i+10]);
			DATI[0][i] = DATI[0][i + 10];
		}
		else {
			if (tachycardia) { // FIXME
				DATI[0][i] = samp[i - 350 + (count * 10)];
			} else {
				DATI[0][i] = DATI[1][i - 350 + (count * 10)];
			}
		}
	}

	count_time++;
	pthread_mutex_unlock(&mutex);
}

//-------------------------------------------------------

int update_D1(int count)
{

float 	casuale; 
float	value;
int	index_arr;
int 	i	=	0;


	pthread_mutex_lock(&mutex);
	if (count > UPDATE_D1) {
		//printf("[COUNT %d]\n", count);
		for (i = 0; i < L; i++) {
			if (fibrillation) 
				casuale = rand()%10; 
			else
				casuale = 0;

			DATI[1][i]				=	 vettore[i] + (casuale/(rand()%100 + 10));
			DATI[1][i + 120]			=	 vettore[i] + (casuale/(rand()%150 + 10));
			DATI[1][i + 240]			= 	 vettore[i] + (casuale/(rand()%50 + 10));

		}
		if (arrhythmia) {

			for (i = 25; i < 46; i++) {
					value 				= 	0.1 + (rand()%10)/100.0;
					DATI[1][i] 	 		= 	value; 
					DATI[1][i + 120] 		= 	value; 
					DATI[1][i + 240] 		= 	value; 			
			}
			index_arr	=	(rand()*11)%70;
			value		=	0.9;
			DATI[1][index_arr + 30]			=	DATI[1][index_arr + 49] + 0.15;
			DATI[1][index_arr + 31]			=	DATI[1][index_arr + 50] + 0.2;
			DATI[1][index_arr + 32]			=	DATI[1][index_arr + 51] + 0.15;

			DATI[1][index_arr + 15]			=	DATI[1][index_arr + 60] + value;

			index_arr	=	(rand()*34)%70;

			DATI[1][index_arr + 120 + 30]		=	DATI[1][index_arr + 120 + 49] + 0.15;
			DATI[1][index_arr + 120 + 31]		=	DATI[1][index_arr + 120 + 50] + 0.2;
			DATI[1][index_arr + 120 + 32]		=	DATI[1][index_arr + 120 + 51] + 0.15;

			DATI[1][index_arr + 120 + 15]		=	DATI[1][index_arr + 120 + 60] + value;

			index_arr	=	(rand()*27)%70;

			DATI[1][index_arr + 240 + 30]		=	DATI[1][index_arr + 240 + 49] + 0.15;
			DATI[1][index_arr + 240 + 31]		=	DATI[1][index_arr + 240 + 49] + 0.2;
			DATI[1][index_arr + 240 + 32]		=	DATI[1][index_arr + 240 + 49] + 0.15;

			
			DATI[1][index_arr + 240 + 15]		=	DATI[1][index_arr + 240] + value;	

		}
		if (tachycardia) {
			sampler();
		}
		count = 0;
	}

	pthread_mutex_unlock(&mutex);
	return count;
}

//--------------------------------------------

int bpm_calculation(int counter)
{
	
int 	n_picchi 	= 	0;
int 	distanza_p, i;
int 	bpm_col;
float 	distanza_t; //distanza temporale
char 	text[q];
	

	if (counter > 1) {
		pthread_mutex_lock(&mutex);
		for (i = 0; i < M; i++) {
				if (DATI[0][i] > pick_value) {
					picchi[n_picchi] = i;
					//printf("%d\n", i);
					n_picchi++;
				}
			}
		pthread_mutex_unlock(&mutex);
		//calcolo la distanza di campioni tra gli ultimi due picchi, ogni campione corrisponde a 0,0141 secondi
		distanza_p = picchi[n_picchi - 1] - picchi[n_picchi - 2];
		distanza_t = distanza_p * sampling_time;
		//printf("DISTANZA T = %f\n", distanza_t);
		bpm = floor(60/distanza_t);

		//printf("BPM: %d\n", bpm);
		
		if (bpm < 250 && bpm > 0) {

			rectfill(screen, rect_coord_x1-90, rect_coord_y2-4, rect_coord_x1, rect_coord_y2+8, 0); //cancello il precedente valore BPM	

			if (bpm >= 100) {
				bpm_col = 4;
			}
			else if (bpm < 50) {
				bpm_col = 15;
			}
			else {
				bpm_col = 11; 
			}
			
			
			sprintf(text, "%d", bpm);
			textout_centre_ex(screen, font, text, rect_coord_x1-50, rect_coord_y2, bpm_col, -1);
		
		}		

		for (i = 0; i < n_picchi + 1; i++) 
			picchi[i] = 0;
		counter = 0;
	}
	
	return counter;
}

//-----------------------------------------

int arrhythmia_sim()
{
	if(arrhythmia == false)
		return 1;

int random_sampler;
int	i	=	0;
int	j	=	0;
int counter = 1;
int pivot_points = 0;
float previous = 0;


}

//-----------------------------------------

bool arrhythmia_detector(int beat)
{

int 	i 		=	 0;
int 	arrhyt_sum 	=	 0;


	if (bpm_counter < q) {
		pthread_mutex_lock(&mutex);

		if (bpm > bpm_save[bpm_counter - 1] + n/2 || bpm < bpm_save[bpm_counter - 1] - n/2) {
			if (bpm < 250 && bpm > 0) {
				bpm_save[bpm_counter] = beat;
				bpm_counter++;
				arrhyt_count = 0;
			}
		}
		

		if (arrhyt_count >= q) {
			for (i = 0; i < q; i++) {
				arrhyt_vect[i] = 0;
			}
		}
		pthread_mutex_unlock(&mutex);	
	} 
	
		
	else {
		i = 0;
		while (bpm_save[i] > 0) {
			if (bpm_save[i + 1] > bpm_save[i] + n*2 || bpm_save[i + 1] < bpm_save[i] - n*2) {
				arrhyt_vect[i] = 1;
			}
			else {
				arrhyt_vect[i] = 0;
			}
			i++;
		}
	
		bpm_counter = 0;
	}

	
	for (i = 0; i < q; i++) {
		arrhyt_sum += arrhyt_vect[i];
	}
	
	
	//printf("arrhyt_sum: %d\n", arrhyt_sum); 
	if (arrhyt_sum >= arrhyt_limit) {
		arrhyt_sum = 0;
		return true;
	}

	
	else {	
		arrhyt_sum = 0;
		return false;
	}
}

//-----------------------------------------

bool fibrillation_detector()
{

int 	i 	=    0;
int 	sum	=    0;

	for (i = M - n; i < M ; i++) {
		if (DATI[0][i] > (DATI[0][i-1] + 0.25) || DATI[0][i] < (DATI[0][i-1] - 0.25)) {
			fibrill_vect[i - M + n] = 1;
		}
		else {
			fibrill_vect[i - M + n] = 0;
		}
	}
	
	for (i = 0; i < n; i++) {
		sum += fibrill_vect[i];
	}

	//printf("SUM: %d\n", sum);
	if (sum > fibril_limit) {
		return true;
	} 
	
	return false;
}

//--------------------------------------

void warnings()
{
	
int 	i = 0;
char 	text[48];
	
	rectfill(screen, rect_coord_x2 + 65, rect_coord_y2 + 3, rect_coord_x2 + 295, rect_coord_y2 + 355, 15);
	
	if (anomaly_tachy) {
		sprintf(text, "High heartbeat, tachycardia");
		textout_centre_ex(screen, font, text, rect_coord_x2 + 182, rect_coord_y2 + 12 + (i * 22), 1, -1);
		i++;
	}
	else if (anomaly_brady) {
		sprintf(text, "Slow heartbeat, bradycardia");
		textout_centre_ex(screen, font, text, rect_coord_x2 + 182, rect_coord_y2 + 12 + (i * 22), 1, -1);
		i++;	
	}
	
	if (anomaly_arrhyt) {
		sprintf(text, "Irregular beat, arrhythmia");
		textout_centre_ex(screen, font, text, rect_coord_x2 + 182, rect_coord_y2 + 12 + (i * 22), 1, -1);
		i++;
	}
	

	if (anomaly_fibril) {
		sprintf(text, "Risk of fibrillation");
		textout_centre_ex(screen, font, text, rect_coord_x2 + 182, rect_coord_y2 + 12 + (i * 22), 1, -1);
		i++;
	}

}

//--------------------------------------

void simulation_notice()
{

int 	i = 0;
char 	text[48];
	
	
	rectfill(screen, rect_coord_x1, rect_coord_y2 - 100, rect_coord_x1 + 500, rect_coord_y2 - 30, 0);
	
	if (tachycardia) {
		sprintf(text, "Tachycardia simulation in progress");
		textout_centre_ex(screen, font, text, rect_coord_x1 + 320, rect_coord_y2 - 100 + (i * 22), 2, -1);
		i++;
	}	
	
	if (arrhythmia) {
		sprintf(text, "Arrhythmia simulation in progress");
		textout_centre_ex(screen, font, text, rect_coord_x1 + 320, rect_coord_y2 - 100 + (i * 22), 2, -1);
		i++;
	}
	
	if (fibrillation) {
		sprintf(text, "Fibrillation simulation in progress");
		textout_centre_ex(screen, font, text, rect_coord_x1 + 320, rect_coord_y2 - 100 + (i * 22), 2, -1);
		i++;
	}
	
}

//--------------------------------------

bool write_anomaly(float time_)
{

	printf("MOMENT: %f\n", time_);

	fw = fopen("ECG_report.txt", "w");
	if (fp == NULL) {
		return false;
	}
	
	if (anomaly_tachy) {
		fprintf(fw, "Tachicardia rilevata al tempo %f\n\n", time_);
	}

	if (anomaly_brady) {
		fprintf(fw, "Bradicardia rilevata al tempo %f\n\n", time_);
	}
	
	if (anomaly_arrhyt) {
		fprintf(fw, "Aritmia rilevata al tempo %f\n\n", time_);
	}
	
	if (anomaly_fibril) {
		fprintf(fw, "Fibrillazione rilevata al tempo %f\n\n", time_);
	}
		
	fclose(fw);

	return true;

}






















