#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<pthread.h>
#include 	<allegro.h>
#include 	<math.h>
#include 	<stdbool.h>
#include 	"task.h"


#define 	N 			2
#define 	M 			360
#define 	K			240
#define		L			120
#define		B 			10
#define 	Q			5
#define 	W			4000
#define 	DIM_TEXT		48
#define		DIM_VALUE_X		180
#define 	SHIFT_NUMBER 		350
#define		UPDATE_D1		35
#define 	PICK_VALUE		0.96
#define 	SECOND_FOR_MINUTE	60
#define 	SAMPLING_TIME		0.008
#define 	SPACE			22

// TASKS IDENTIFIER MACRO

#define		DRAW_TASK		0
#define		GENR_TASK		1
#define		USER_TASK		2
#define		INFO_TASK		3
#define		ANOM_TASK		4

// TASKS PERIOD MACRO

#define		DRAW_PERIOD		82
#define		GENR_PERIOD		82
#define		USER_PERIOD		100
#define		INFO_PERIOD		200
#define		ANOM_PERIOD		400

// LIMIT DETECTOR MACRO

#define		FIBRIL_LIMIT		7
#define 	ARRHYT_LIMIT		3
#define 	BPM_LIMIT		250
#define		FIBRIL_JUMP		0.25

// CODE FOR SAVING ANOMALY

#define 	FIBRILLATION_CODE	65
#define 	ARRHYTMIA_CODE 		75
#define 	TACHYCARDIA_CODE 	85
#define 	BRADYCARDIA_CODE 	95

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

//	gruppo di variabili necessarie per la lettura/scrittura dei dati dai file

char 		nome_file[M];				
FILE 		*fp;
FILE 		*fw;
float 		DATI[N][M];
float 		vettore[M];
float		aux_draw[M];
float		samp[M];
float		buff_arr[L];
float		arr[M];
int		anomaly_note_tachy [N][W];
int		anomaly_note_brady [N][W];
int		anomaly_note_arrhyt[N][W];
int		anomaly_note_fibril[N][W];		//variabili in cui vengono salvate le anomalie per la successiva scrittura su file

//	gruppo di variabili per l'esecuzione dei calcoli		

int 		picchi[B];				
int 		bpm;
int 		arrhyt_count 	= 	 0;
int 		read_count 	=	 0;
int 		bpm_counter 	= 	 0;
int		fibrill_vect[B];
int 		arrhyt_vect[Q];
int 		bpm_save[Q];

//	gruppo di variabili per la scrittura su file

float		moment;
int 		count_time 	= 	 0;
int 		tachy_c         = 	 0;
int 		brady_c 	= 	 0;
int 		arrhyt_c 	= 	 0; 		
int 		fibril_c 	= 	 0;

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
void		anomaly_save(float time_);	
bool		write_anomaly();		//funzione per la scrittura su file

// ******************************** MAIN FUNCTION *********************************

int main(void)
{

	if(init() != true)
		return 1;

	// tasks creation

	task_create(draw_function, DRAW_TASK, DRAW_PERIOD, 500, 20);
	task_create(generatore, GENR_TASK, GENR_PERIOD, 500, 20);
	task_create(user_command, USER_TASK, USER_PERIOD, 80, 20);
	task_create(info, INFO_TASK, INFO_PERIOD, 300, 20);
	task_create(anomaly_detector, ANOM_TASK, ANOM_PERIOD, 300, 20);

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

	write_anomaly();

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

int 	i;
char 	text[DIM_TEXT];

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
			
			rectfill(screen, rect_coord_x2 + 220, rect_coord_y2 + 430, rect_coord_x2 + 300, rect_coord_y2 + 455, 0);
			sprintf(text, "TIME: %f", moment);
			textout_centre_ex(screen, font, text, rect_coord_x2 + 270, rect_coord_y2 + 440, 7, -1);
			rectfill(screen, rect_coord_x2 + 293, rect_coord_y2 + 430, rect_coord_x2 + 330, rect_coord_y2 + 455, 0);
			
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

			for(i = 0; i < M; i++) {
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
			
			//ogni campione vale 0.008 s
			moment = count_time * SAMPLING_TIME * B;
			//conservo la anomalia riscontrata al tempo corrente
			anomaly_save(moment);
			
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

char 	text[DIM_TEXT];
char 	value_x[DIM_VALUE_X];

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
			if (i < B) {
				vettore[i]     = 0.1;
				vettore[i + L] = 0.1;
				vettore[i + K] = 0.1;
			}
			else {
				vettore[i]     = DATI[0][i];
				vettore[i + L] = DATI[0][i];
				vettore[i + K] = DATI[0][i];
			}
			DATI[1][i]     = vettore[i];
			DATI[1][i + L] = vettore[i];
			DATI[1][i + K] = vettore[i];
			DATI[0][i + L] = DATI[1][i];
			DATI[0][i + K] = DATI[1][i];
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
	
	for (int i = 0; i < DIM_TEXT; i++) {
		if(i % 4 == 0)
			line(screen, rect_coord_x1 + i*B, rect_coord_y1, rect_coord_x1 + i*B, rect_coord_y2, 12); // 11
		else
			line(screen, rect_coord_x1 + i*B, rect_coord_y1, rect_coord_x1 + i*B, rect_coord_y2, makecol(255,150,150));
		if (i > 35)
			continue;
		if (i % 4 == 0)
			line(screen, rect_coord_x1, rect_coord_y1 - i*B, rect_coord_x2, rect_coord_y1 - i*B, 12);
		else
			line(screen, rect_coord_x1, rect_coord_y1 - i*B, rect_coord_x2, rect_coord_y1 - i*B, makecol(255,150,150));			
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

	for (i = 0; i < M/N; i++) {
		samp[i]		= DATI[1][N * i];
		samp[i + 59]	= DATI[1][N * i];
		samp[i + 119]	= DATI[1][N * i];
		samp[i + 179]	= DATI[1][N * i];
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
			DATI[0][i] = DATI[0][i + B];
		}
		else {
			if (tachycardia) { // FIXME
				DATI[0][i] = samp[i - SHIFT_NUMBER + (count * B)];
			} else {
				DATI[0][i] = DATI[1][i - SHIFT_NUMBER + (count * B)];
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
				casuale = rand()%B; 
			else
				casuale = 0;

			DATI[1][i]				=	 vettore[i] + (casuale/(rand()%100 + B));
			DATI[1][i + L]				=	 vettore[i] + (casuale/(rand()%150 + B));
			DATI[1][i + K]			= 	 vettore[i] + (casuale/(rand()%50 + B));

		}
		if (arrhythmia) {

			for (i = 25; i < 46; i++) {
					value 				= 	0.1 + (rand()%B)/100.0;
					DATI[1][i] 	 		= 	value; 
					DATI[1][i + L]	 		= 	value; 
					DATI[1][i + K]	 		= 	value; 			
			}
			index_arr	=	(rand()*11)%70;
			value		=	0.9;
			DATI[1][index_arr + 30]			=	DATI[1][index_arr + 49] + 0.15;
			DATI[1][index_arr + 31]			=	DATI[1][index_arr + 50] + 0.2;
			DATI[1][index_arr + 32]			=	DATI[1][index_arr + 51] + 0.15;

			DATI[1][index_arr + 15]			=	DATI[1][index_arr + 60] + value;

			index_arr	=	(rand()*34)%70;

			DATI[1][index_arr + L + 30]		=	DATI[1][index_arr + L + 49] + 0.15;
			DATI[1][index_arr + L + 31]		=	DATI[1][index_arr + L + 50] + 0.2;
			DATI[1][index_arr + L + 32]		=	DATI[1][index_arr + L + 51] + 0.15;

			DATI[1][index_arr + L + 15]		=	DATI[1][index_arr + L + 60] + value;

			index_arr	=	(rand()*27)%70;

			DATI[1][index_arr + K + 30]		=	DATI[1][index_arr + K + 49] + 0.15;
			DATI[1][index_arr + K + 31]		=	DATI[1][index_arr + K + 49] + 0.2;
			DATI[1][index_arr + K + 32]		=	DATI[1][index_arr + K + 49] + 0.15;

			
			DATI[1][index_arr + K + 15]		=	DATI[1][index_arr + K] + value;	

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
char 	text[Q];
	

	if (counter > 1) {
		pthread_mutex_lock(&mutex);
		for (i = 0; i < M; i++) {
				if (DATI[0][i] > PICK_VALUE) {
					picchi[n_picchi] = i;
					//printf("%d\n", i);
					n_picchi++;
				}
			}
		pthread_mutex_unlock(&mutex);
		//calcolo la distanza di campioni tra gli ultimi due picchi, ogni campione corrisponde a 0,0141 secondi
		distanza_p = picchi[n_picchi - 1] - picchi[n_picchi - N];
		distanza_t = distanza_p * SAMPLING_TIME;
		//printf("DISTANZA T = %f\n", distanza_t);
		bpm = floor(SECOND_FOR_MINUTE/distanza_t);

		//printf("BPM: %d\n", bpm);
		
		if (bpm < BPM_LIMIT && bpm > 0) {

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


	if (bpm_counter < Q) {
		pthread_mutex_lock(&mutex);

		if (bpm > bpm_save[bpm_counter - 1] + B/N || bpm < bpm_save[bpm_counter - 1] - B/N) {
			if (bpm < BPM_LIMIT && bpm > 0) {
				bpm_save[bpm_counter] = beat;
				bpm_counter++;
				arrhyt_count = 0;
			}
		}
		

		if (arrhyt_count >= Q) {
			for (i = 0; i < Q; i++) {
				arrhyt_vect[i] = 0;
			}
		}
		pthread_mutex_unlock(&mutex);	
	} 
	
		
	else {
		i = 0;
		while (bpm_save[i] > 0) {
			if (bpm_save[i + 1] > bpm_save[i] + B*N || bpm_save[i + 1] < bpm_save[i] - B*N) {
				arrhyt_vect[i] = 1;
			}
			else {
				arrhyt_vect[i] = 0;
			}
			i++;
		}
	
		bpm_counter = 0;
	}

	
	for (i = 0; i < Q; i++) {
		arrhyt_sum += arrhyt_vect[i];
	}
	
	
	//printf("arrhyt_sum: %d\n", arrhyt_sum); 
	if (arrhyt_sum >= ARRHYT_LIMIT) {
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

	for (i = M - B; i < M ; i++) {
		if (DATI[0][i] > (DATI[0][i-1] + FIBRIL_JUMP) || DATI[0][i] < (DATI[0][i-1] - FIBRIL_JUMP)) {
			fibrill_vect[i - M + B] = 1;
		}
		else {
			fibrill_vect[i - M + B] = 0;
		}
	}
	
	for (i = 0; i < B; i++) {
		sum += fibrill_vect[i];
	}

	//printf("SUM: %d\n", sum);
	if (sum > FIBRIL_LIMIT) {
		return true;
	} 
	
	return false;
}

//--------------------------------------

void warnings()
{
	
int 	i = 0;
char 	text[DIM_TEXT];
	
	rectfill(screen, rect_coord_x2 + 65, rect_coord_y2 + 3, rect_coord_x2 + 295, rect_coord_y2 + 355, 15);
	
	if (anomaly_tachy) {
		sprintf(text, "High heartbeat, tachycardia");
		textout_centre_ex(screen, font, text, rect_coord_x2 + 182, rect_coord_y2 + 12 + (i * SPACE), 1, -1);
		i++;
	}
	else if (anomaly_brady) {
		sprintf(text, "Slow heartbeat, bradycardia");
		textout_centre_ex(screen, font, text, rect_coord_x2 + 182, rect_coord_y2 + 12 + (i * SPACE), 1, -1);
		i++;	
	}
	
	if (anomaly_arrhyt) {
		sprintf(text, "Irregular beat, arrhythmia");
		textout_centre_ex(screen, font, text, rect_coord_x2 + 182, rect_coord_y2 + 12 + (i * SPACE), 1, -1);
		i++;
	}
	

	if (anomaly_fibril) {
		sprintf(text, "Risk of fibrillation");
		textout_centre_ex(screen, font, text, rect_coord_x2 + 182, rect_coord_y2 + 12 + (i * SPACE), 1, -1);
		i++;
	}

}

//--------------------------------------

void simulation_notice()
{

int 	i = 0;
char 	text[DIM_TEXT];
	
	
	rectfill(screen, rect_coord_x1, rect_coord_y2 - 100, rect_coord_x1 + 500, rect_coord_y2 - 30, 0);
	
	if (tachycardia) {
		sprintf(text, "Tachycardia simulation in progress");
		textout_centre_ex(screen, font, text, rect_coord_x1 + 320, rect_coord_y2 - 100 + (i * SPACE), 2, -1);
		i++;
	}	
	
	if (arrhythmia) {
		sprintf(text, "Arrhythmia simulation in progress");
		textout_centre_ex(screen, font, text, rect_coord_x1 + 320, rect_coord_y2 - 100 + (i * SPACE), 2, -1);
		i++;
	}
	
	if (fibrillation) {
		sprintf(text, "Fibrillation simulation in progress");
		textout_centre_ex(screen, font, text, rect_coord_x1 + 320, rect_coord_y2 - 100 + (i * SPACE), 2, -1);
		i++;
	}
	
}

//--------------------------------------

void anomaly_save(float moment)
{
	
	if (anomaly_tachy) {
		anomaly_note_tachy[0][tachy_c] = TACHYCARDIA_CODE;
		anomaly_note_tachy[1][tachy_c] = floor(moment);
		tachy_c++;
	}
	
	if (anomaly_brady) {
		anomaly_note_brady[0][brady_c] = BRADYCARDIA_CODE;
		anomaly_note_brady[1][brady_c] = floor(moment);
		brady_c++;
	}

	if (anomaly_arrhyt) {
		anomaly_note_arrhyt[0][arrhyt_c] = ARRHYTMIA_CODE;
		anomaly_note_arrhyt[1][arrhyt_c] = floor(moment);
		arrhyt_c++;
	}

	if (anomaly_fibril) {
		anomaly_note_fibril[0][fibril_c] = FIBRILLATION_CODE;
		anomaly_note_fibril[1][fibril_c] = floor(moment);
		fibril_c++;
	}

}

//--------------------------------------

bool write_anomaly()
{

int 	i 	=     0;
int 	j;


	fw = fopen("ECG_report.txt", "w");
	if (fw == NULL) {
		return false;
	}



	i = 0;
	while (anomaly_note_tachy[0][i] > 0) {

		j = i;
		while (anomaly_note_tachy[1][j] == anomaly_note_tachy[1][j+1] || anomaly_note_tachy[1][j] + 1 == anomaly_note_tachy[1][j+1]) {
			j++;	
		}

		if(i != j){	
			fprintf(fw, "Tachicardia rilevata tra %d e %d secondi\n\n", anomaly_note_tachy[1][i], anomaly_note_tachy[1][j]); 
		}

		i = j + 1;
	}



	i = 0;
	while (anomaly_note_brady[0][i] > 0) {

		j = i;
		while (anomaly_note_brady[1][j] == anomaly_note_brady[1][j+1] || anomaly_note_brady[1][j] + 1 == anomaly_note_brady[1][j+1]) {
			j++;
		}

		if (i != j) {	
			fprintf(fw, "Braducardia rilevata tra %d e %d secondi\n\n", anomaly_note_brady[1][i], anomaly_note_brady[1][j]); 
		}

		i = j + 1;
	}


	i = 0;
	while (anomaly_note_arrhyt[0][i] > 0) {

		j = i;
		while (anomaly_note_arrhyt[1][j] == anomaly_note_arrhyt[1][j+1] || anomaly_note_arrhyt[1][j] + 1 == anomaly_note_arrhyt[1][j+1]) {
			j++;
		}

		if (i != j) {	
			fprintf(fw, "Aritmia rilevata tra %d e %d secondi\n", anomaly_note_arrhyt[1][i], anomaly_note_arrhyt[1][j]); 
		}

		i = j + 1;
	}


	i = 0;
	while (anomaly_note_fibril[0][i] > 0) {

		j = i;
		while (anomaly_note_fibril[1][j] == anomaly_note_fibril[1][j+1] || anomaly_note_fibril[1][j] + 1 == anomaly_note_fibril[1][j+1]) {
			j++;
		}

		if (i != j) {	
			fprintf(fw, "Fibrillazione rilevata tra %d e %d secondi\n", anomaly_note_fibril[1][i], anomaly_note_fibril[1][j]); 
		}

		i = j + 1;
	}


	fclose(fw);

}






















