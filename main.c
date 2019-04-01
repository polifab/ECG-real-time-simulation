#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<pthread.h>
#include 	<allegro.h>
#include 	<math.h>
#include 	<stdbool.h>
#include 	"task.h"


#define 	N 			2		//numero delle righe della matrice DATI
#define 	M 			360		//numero delle colonne della matrice DATI
#define 	K			240		
#define		L			120		//numero di campioni per battito
#define		B			10		//costanti utili nei calcoli
#define 	Q			5		
#define 	W			4000   		//dimensione della matrice in cui vengono salvate le anomalie
#define 	DIM_TEXT		48		//dimensione del testo 
#define		DIM_VALUE_X		180		//dimensione dei valori da visualizzare
#define 	SHIFT_NUMBER 		350		
#define		UPDATE_D1		35		
#define 	PICK_VALUE		0.96		//soglia per campionare i picchi
#define 	SECOND_FOR_MINUTE	60		//secondi in un minuto
#define 	SAMPLING_TIME		0.008		//inverso della frequenza di campionamento dei dati
#define 	SPACE			22		//spazio tra due segnalazioni di anomalie
#define 	CONST_FIBR		48		//costante utilizzata nella rilevazione della fibrillazione

// TASKS IDENTIFIER MACRO

#define		DRAW_TASK		0
#define		GENR_TASK		1
#define		USER_TASK		2
#define		INFO_TASK		3
#define		TACH_TASK		4
#define		ARRH_TASK		5	
#define		FIBR_TASK		6

// TASKS PERIOD MACRO

#define		DRAW_PERIOD		82
#define		GENR_PERIOD		82
#define		USER_PERIOD		50
#define		INFO_PERIOD		200
#define		TACH_PERIOD		300
#define		ARRH_PERIOD		300
#define		FIBR_PERIOD		300		

// LIMIT DETECTOR MACRO

#define 	BPM_INF			50		//limite bradicardia
#define 	BPM_SUP			100		//limite tachicardia
#define		FIBRIL_LIMIT		7		//limite per la segnalazione della fibrillazione
#define 	ARRHYT_LIMIT		3		//limite per la segnalazione dell'aritmia
#define 	BPM_LIMIT		280		//limite superiore dei BPM
#define		FIBRIL_JUMP		0.1		//costante per la verifica della fibrillazione

// CODE FOR SAVING ANOMALY

#define 	FIBRILLATION_CODE	65		//codici anomalie	
#define 	ARRHYTMIA_CODE 		75
#define 	TACHYCARDIA_CODE 	85
#define 	BRADYCARDIA_CODE 	95

// ************* GLOBAL VARIABLES *****************

bool		quit			=	false;		//  variabile di terminazione 
bool		stop_graphics		=	true;		//  variabile per lo stop dei thread produttori e consumatori

//	variabili per la simulazione delle anomalie

bool		fibrillation		=	false;		
bool		tachycardia		=	false;
bool		arrhythmia		=	false;

//	varibili per la segnalazione delle anomalie

bool		anomaly_fibril		=	false; 		
bool		anomaly_tachy		=	false;
bool		anomaly_brady		=	false;
bool		anomaly_arrhyt		=	false;

//	variabili per l'attivazione dei thread rivelatori di anomalie

bool		tachy_det_activation	= 	false;
bool		arr_det_activation	=	false;
bool		fibr_det_activation	=	false;

//	gruppo di variabili necessarie per la lettura/scrittura dei dati dai file
				
float 		DATI[N][M];
float 		vettore[M];
float		aux_draw[M];
float		samp[M];

//	gruppo di variabili per l'annotazione delle anomalie rilevate per la successiva scrittura su file

int		anomaly_note_tachy [N][W];
int		anomaly_note_brady [N][W];
int		anomaly_note_arrhyt[N][W];
int		anomaly_note_fibril[N][W];		

//	gruppo di variabili per l'esecuzione dei calcoli		
				
int 		bpm;
int 		read_count 	=	 0;

//	gruppo di variabili per la scrittura su file

float		moment;
int 		count_time 	= 	 0;
int 		tachy_c         = 	 0;
int 		brady_c 	= 	 0;
int 		arrhyt_c 	= 	 0; 		
int 		fibril_c 	= 	 0;

//	mutex

pthread_mutex_t 	DATI_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t 	activation_mutex = PTHREAD_MUTEX_INITIALIZER;

//	gruppo di variabili per la grafica

int		x = 1024, y = 640, col = 4;
int		rect_coord_x1 = 160; 		// 	x / 4
int		rect_coord_x2 = 640; 		// 	3/4 * x 
int		rect_coord_y1 = 480;
int		rect_coord_y2 = 120;

// ************* TASK PROTOTYPES ****************************

void		*draw_task();			//	task dedito ad aggiornare la grafica (consumatore)
void		*user_command();		//	task dedito alla lettura dei comandi dell'utente
void 		*generatore();			//	task dedito alla generazione dei valori dell'ecg (produttore)
void		*info();			//	task dedito ai calcoli relativi l'analisi dell'ecg
void		*tachycardia_detector(); 	// 	task dedito alla rilevazione di anomalie del ritmo sinusale
void		*arrhythmia_detector(); 	// 	task per la rilevazione dell'aritmia
void		*fibrillation_detector(); 	// 	task per la rilevazione della fibrillazione

// ************* FUNCTION PROTOTYPES **********************

bool		init();				//	funzione di inizializzazione di dati, variabili e grafica
void		init_mutex();					
bool		carica_matrice();
int		draw_rect();			//	funzione per il disegno del rettangolo del grafico
void		read_command(char key);		//	interprete dei comandi inseriti dall'utente
void 		display_command();		
void		sampler();
int		arrhythmia_computation();	// 	funzione che svolge i calcoli per il thread dell'aritmia
void		shift();
int		update_D1(int count);
int 		bpm_calculation(int counter); 	//	funzione per il calcolo dei bpm  
void 		warnings();
void		anomaly_save(float time_);	
bool		write_anomaly();		//	funzione per la scrittura su file

// ******************************** MAIN FUNCTION *********************************

int main(void)
{

	if (init() != true)
		return 1;

	// tasks creation

	task_create(draw_task,				DRAW_TASK,	DRAW_PERIOD,	1000,	19);
	task_create(generatore,				GENR_TASK,	GENR_PERIOD,	1000,	19);
	task_create(user_command,			USER_TASK,	USER_PERIOD,	500,	20);
	task_create(info,					INFO_TASK,	INFO_PERIOD,	500,	20);
	task_create(tachycardia_detector,	TACH_TASK,	TACH_PERIOD,	300,	20);
	task_create(arrhythmia_detector,	ARRH_TASK,	ARRH_PERIOD,	300,	20);
	task_create(fibrillation_detector,	FIBR_TASK,	FIBR_PERIOD,	300,	20);
	// tasks joining

	pthread_join(tid[DRAW_TASK],NULL);
	printf("DRAW TASK\n");
	pthread_join(tid[GENR_TASK],NULL);
	printf("GENERATOR TASK\n");
	pthread_join(tid[USER_TASK],NULL);
	printf("USER TASK\n");
	pthread_join(tid[INFO_TASK],NULL);
	printf("INFO TASK\n");
	pthread_join(tid[TACH_TASK],NULL);
	printf("TACHYCARDIA DETECTOR TASK\n");
	pthread_join(tid[ARRH_TASK],NULL);
	printf("ARRHYTMIA DETECTOR TASK\n");
	pthread_join(tid[FIBR_TASK],NULL);
	printf("FIBRILLATION DETECTOR TASK\n");

	write_anomaly();

	return 0;
}



//***************************** TASKS IMPLEMENTATION **********************************

//---------------------------------------------------------------------------
//            Task di lettura ed interpretazione comandi utente           
//---------------------------------------------------------------------------


void *user_command()
{

char	key;	//Salviamo qui il carattere inserito dall'utente

    set_activation(USER_TASK);

    while (quit == false) {
		if(keypressed()){
        	key = readkey() & 0xFF;
        	read_command(key);
		}

		
        if (deadline_miss(USER_TASK) == 1) printf("DEADLINE MISS USER COMMAND\n");     //soft real time
        wait_for_activation(USER_TASK);
    }
}

//---------------------------------------------------------------------------
//             Task di generazione del segnale simulato           
//---------------------------------------------------------------------------


void  *generatore()
{

int 	count = 0;
bool	activation;
	
	set_activation(GENR_TASK);
	
	while (quit == false) {

		pthread_mutex_lock(&activation_mutex);
		activation = stop_graphics;
		pthread_mutex_unlock(&activation_mutex);

		if (!activation) {
			
			pthread_mutex_lock(&DATI_mutex);

			sampler();
			pthread_mutex_unlock(&DATI_mutex);

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

//---------------------------------------------------------------------------
//         Task per l'aggiornamento grafico del plot del segnale          
//---------------------------------------------------------------------------


void *draw_task()
{

int 	i;
char 	text[DIM_TEXT];
bool	activation;

	set_activation(DRAW_TASK);

	while (quit == false) {
		pthread_mutex_lock(&activation_mutex);
		activation = stop_graphics;
		pthread_mutex_unlock(&activation_mutex);

		if (!activation) { //se l'utente setta a true stop_graphics blocca
		
			draw_rect(); // cancello il grafico precedente

			pthread_mutex_lock(&DATI_mutex); // sezione critica, lettura variabile condivisa DATI
			for(i = 0; i < M; i++){
				aux_draw[i] = DATI[0][i];
			}
			
			rectfill(screen, rect_coord_x2 + 220, rect_coord_y2 + 430, rect_coord_x2 + 320, rect_coord_y2 + 455, 0);
			sprintf(text, "TIME: %.2f", moment);
			textout_centre_ex(screen, font, text, rect_coord_x2 + 270, rect_coord_y2 + 440, 7, -1);
			
			display_command();			

			pthread_mutex_unlock(&DATI_mutex);

			for(i = 0; i < M; i++) {
				line(screen, rect_coord_x1 + 1.5*i, 340 - (int)(140*aux_draw[i]), rect_coord_x1 + 1.5*i + 1.5, 340 - (int)(140*aux_draw[i+1]),  makecol(0, 0, 0));
			}

		} 
	
		if (deadline_miss(DRAW_TASK) == 1) printf("DEADLINE MISS DRAW\n");     //soft real time

		wait_for_activation(DRAW_TASK);
	}
	
	return NULL;
}

//---------------------------------------------------------------------------
//             Task per la visualizzazione di informazioni
//---------------------------------------------------------------------------


void *info()
{
	
int 	counter	= 0;
bool	activation;

	set_activation(INFO_TASK);

	while (quit == false) {

		pthread_mutex_lock(&activation_mutex);
		activation = stop_graphics;
		pthread_mutex_unlock(&activation_mutex);

		if (!activation) {
			
			counter++;
			bpm_calculation(counter);
			
			//ogni campione vale 0.008 s
			moment = count_time * SAMPLING_TIME * B;
			//conservo la anomalia riscontrata al tempo corrente
			anomaly_save(moment);
			
			warnings();
		} 
		if (deadline_miss(INFO_TASK) == 1) printf("DEADLINE MISS INFO\n");     //soft real time
		wait_for_activation(INFO_TASK);
	}
}

//---------------------------------------------------------------------------
//            Task per la rilevazione della tachicardia
//---------------------------------------------------------------------------


void *tachycardia_detector()
{

bool	activation;
bool	activation_tachy;

	set_activation(TACH_TASK);

	while (quit == false) {

		pthread_mutex_lock(&activation_mutex);
		activation = !stop_graphics;
		activation_tachy = tachy_det_activation;
		pthread_mutex_unlock(&activation_mutex);

	if(activation){
		if (activation_tachy == false) {
			if (anomaly_tachy == true)
				anomaly_tachy = false;
			wait_for_activation(TACH_TASK);

			continue;
		}
			
		if (bpm >= BPM_SUP) {
			anomaly_tachy = true;		//se supera il valore di soglia setta la variabile a 'true'
		} else {
			anomaly_tachy = false;
		} 
			

		if (bpm <= BPM_INF) {
			anomaly_brady = true;
		} else {
			anomaly_brady = false;
		}

	}
		if (deadline_miss(TACH_TASK) == 1) printf("DEADLINE MISS TACHYCARDIA\n");     //soft real time

		wait_for_activation(TACH_TASK);
	}	
	
	
}


//---------------------------------------------------------------------------
//               Task per la rilevazione dell'aritmia
//---------------------------------------------------------------------------


void *arrhythmia_detector()
{

int 	bpm_save[Q], arrhyt_vect[Q];
int 	i	 		=	 0;
int 	arrhyt_sum 		=	 0;
int 	bpm_counter 		=	 0;
int 	arrhyt_count 		= 	 0;
bool	activation;
bool	activation_arr;

	set_activation(ARRH_TASK);

	while (quit == false) {

		pthread_mutex_lock(&activation_mutex);
		activation = !stop_graphics;
		activation_arr = arr_det_activation;
		pthread_mutex_unlock(&activation_mutex);

		if(activation){

			if (activation_arr == false) {
				if (anomaly_arrhyt == true)
					anomaly_arrhyt = false;
				wait_for_activation(ARRH_TASK);
				continue;
			}
			if (bpm_counter < Q) {
				pthread_mutex_lock(&DATI_mutex);

				if (bpm > bpm_save[bpm_counter - 1] + B/N || bpm < bpm_save[bpm_counter - 1] - B/N) {
					if (bpm < BPM_LIMIT && bpm > 0) {
						bpm_save[bpm_counter] = bpm;		//per variazioni troppo grandi nei BPM salvo i valori
						bpm_counter++;					
						arrhyt_count = 0;
					}
				}
				else arrhyt_count++;

				if (arrhyt_count >= Q) {
					for (i = 0; i < Q; i++) 
						arrhyt_vect[i] = 0;			
				
				anomaly_arrhyt = false;	
				arrhyt_count = 0;
				}
				pthread_mutex_unlock(&DATI_mutex);		
			} 	
			else {
				i = 0;
				while (bpm_save[i] > 0) {
					if (bpm_save[i + 1] > bpm_save[i] + B*N || bpm_save[i + 1] < bpm_save[i] - B*N) 
						arrhyt_vect[i + 1] = 1;								//conto il numero di variazioni 
					else 
						arrhyt_vect[i] = 0;
				i++;
				}

				for (i = 0; i < Q; i++) 
					arrhyt_sum += arrhyt_vect[i]; 						//se le variazioni sono tante si suppone che ci sia aritmia
			
				if (arrhyt_sum >= ARRHYT_LIMIT)  anomaly_arrhyt = true;
				else 				 anomaly_arrhyt = false;
				arrhyt_sum = 0;	
				bpm_counter = 0;
			}
		}
		if (deadline_miss(ARRH_TASK) == 1) printf("DEADLINE MISS ARRHYTMIA\n");     //soft real time
		wait_for_activation(ARRH_TASK);
	}
}


//---------------------------------------------------------------------------
//             Task per la rilevazione della fibrillazione
//---------------------------------------------------------------------------


void *fibrillation_detector()
{

int 	i 	=    0;
int 	sum	=    0;
int	fibrill_vect[B];
bool	activation_fibr;
bool	activation;

	set_activation(FIBR_TASK);

	while (quit == false){


		pthread_mutex_lock(&activation_mutex);
		activation = !stop_graphics;
		activation_fibr = fibr_det_activation;
		pthread_mutex_unlock(&activation_mutex);

		if(activation){
			if (activation_fibr == false){
				if (anomaly_fibril == true)
					anomaly_fibril = false;
				wait_for_activation(FIBR_TASK);

				continue;
			}

			for (i = M - B; i < M ; i++) {
				if (DATI[0][i] > (DATI[0][i-1] + FIBRIL_JUMP) || DATI[0][i] < (DATI[0][i-1] - FIBRIL_JUMP)) {		//guardo le variazioni tra un campione ed il precedente
					fibrill_vect[i - M + B] = 1;									//se superano la soglia scrivo 1 nel vettore	
				}													//altrimenti scrivo 0
				else {
					fibrill_vect[i - M + B] = 0;
				}
			}
			
			for (i = 0; i < B; i++) 								//calcolo il numero di volte che la variazione ha superato la soglia
				sum += fibrill_vect[i];
			
			if (anomaly_fibril == false){
				if (sum >= FIBRIL_LIMIT)  anomaly_fibril = true;
				else anomaly_fibril = false;
			}

			else {
				if (sum == 0)  anomaly_fibril = false;
			}
			sum = 0;
		}
		if (deadline_miss(FIBR_TASK) == 1) printf("DEADLINE MISS FIBRILLATION\n");     //soft real time

		wait_for_activation(FIBR_TASK);
	}
}



// ********************** FUNCTIONS IMPLEMENTATION *****************************


//--------------------------------------------------------------------------
//             Inizializzazione Allegro e lettura da file
//--------------------------------------------------------------------------


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
	
	sprintf(value_x, "Press 's' to start/stop the graphics");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+220, rect_coord_y1 + 15, 15, -1);
	sprintf(text, "ECG");
	textout_centre_ex(screen, font, text, 530, 25, 10, -1);
	sprintf(text, "Press 'q' to exit");
	textout_centre_ex(screen, font, text, 380, 100, 15, -1);
	sprintf(text, "BPM:");
	textout_centre_ex(screen, font, text, rect_coord_x1-50, rect_coord_y2-20, 10, -1);
	sprintf(value_x, "Warnings");
	textout_centre_ex(screen, font, value_x, rect_coord_x2+180, rect_coord_y2 -15 , 4, -1);

	rectfill(screen, rect_coord_x2 + 60, rect_coord_y1, rect_coord_x2 + 300, rect_coord_y2, 15);
	rect(screen, rect_coord_x2 + 60, rect_coord_y1, rect_coord_x2 + 300, rect_coord_y2, 4);

	return true;
}


//---------------------------------------------------------------------------
//                 Caricamento da file dei valori ecg 
//---------------------------------------------------------------------------


bool carica_matrice()
{

FILE 		*fp;
	
	fp = fopen("ptbdb_normal.csv", "r");
	if (fp == NULL) 
		return false;
	
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
		
	}	

	fclose(fp);
	
	return true;
}


//---------------------------------------------------------------------------
//             Funzione per il ripristino della grafica
//---------------------------------------------------------------------------


int draw_rect()
{

	rectfill(screen, rect_coord_x1, rect_coord_y1, rect_coord_x2, rect_coord_y2, 15);
	rect(screen, rect_coord_x1, rect_coord_y1, rect_coord_x2, rect_coord_y2, 4);
	
	for (int i = 0; i < DIM_TEXT; i++) {
		if (  i % 4 == 0)
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


//---------------------------------------------------------------------------
//          Funzione per la lettura dei comandi da tastiera
//---------------------------------------------------------------------------


void read_command(char key)
{
	pthread_mutex_lock(&activation_mutex);
	switch (key) {

		case 'q':
			quit = true;
			break;

		case 's':
			stop_graphics		=	!stop_graphics;
			break;

		case 'f':
			fibrillation		=	!fibrillation;
			break;

		case 'a':
			arrhythmia		=	!arrhythmia;
			break;

		case 't':
			tachycardia		=	!tachycardia;
			break;

		case 'd':
			arr_det_activation	=	!arr_det_activation;
			break;

		case 'i':
			fibr_det_activation	=	!fibr_det_activation;
			break;

		case 'k':
			tachy_det_activation	=	!tachy_det_activation;
			break;

		default:
			break;

	}
	pthread_mutex_unlock(&activation_mutex);
	return;
}


//---------------------------------------------------------------------------
//               Visualizzazione a video dei comandi
//---------------------------------------------------------------------------


void display_command()
{

char 	text[DIM_TEXT];
char 	value_x[DIM_VALUE_X];
int	tachy_color_sim,  arrhyt_color_sim,  fibril_color_sim;
int 	tachy_color_dect, arrhyt_color_dect, fibril_color_dect;


	if (tachycardia)	 tachy_color_sim = 10;		//i tasti attivati diventano verdi (10)
	else 			 tachy_color_sim = 15; 		//altrimenti rimangono bianchi (15)
								
	if (arrhythmia)		 arrhyt_color_sim = 10;
	else 			 arrhyt_color_sim = 15;
 
	if (fibrillation)	 fibril_color_sim = 10;
	else 			 fibril_color_sim = 15; 	

	
	sprintf(value_x, "Tachycardia simulation  ('t')");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+70, rect_coord_y1 + 55, tachy_color_sim, -1);
	sprintf(value_x, "Arrhythmia simulation   ('a')");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+70, rect_coord_y1 + 75, arrhyt_color_sim, -1);
	sprintf(value_x, "Fibrillation simulation ('f')");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+70, rect_coord_y1 + 95, fibril_color_sim, -1);


	if (tachy_det_activation) tachy_color_dect = 10;
	else 			  tachy_color_dect = 15; 

	if (arr_det_activation)	 arrhyt_color_dect = 10;
	else 			 arrhyt_color_dect = 15;
 
	if (fibr_det_activation) fibril_color_dect = 10;
	else 			 fibril_color_dect = 15; 	


	sprintf(value_x, "Tachycardia detector  ('k')");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+390, rect_coord_y1 + 55, tachy_color_dect, -1);
	sprintf(value_x, "Arrhythmia detector   ('d')");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+390, rect_coord_y1 + 75, arrhyt_color_dect, -1);
	sprintf(value_x, "Fibrillation detector ('i')");
	textout_centre_ex(screen, font, value_x, rect_coord_x1+390, rect_coord_y1 + 95, fibril_color_dect, -1);

}


//---------------------------------------------------------------------------
//                        Campionamento valori
//---------------------------------------------------------------------------


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


//---------------------------------------------------------------------------
//                         Shift dei valori 
//---------------------------------------------------------------------------


void shift(int count)
{
	pthread_mutex_lock(&DATI_mutex);
	for (int i = 0; i < M; i++) {
		if (i < SHIFT_NUMBER ){
			//printf("%.2f\n", DATI[0][i+10]);
			DATI[0][i] = DATI[0][i + B];
		}
		else {
			if (tachycardia) { 
				DATI[0][i] = samp[i - SHIFT_NUMBER + (count * B)];
			} else {
				DATI[0][i] = DATI[1][i - SHIFT_NUMBER + (count * B)];
			}
		}
	}

	count_time++;
	pthread_mutex_unlock(&DATI_mutex);
}


//---------------------------------------------------------------------------
//                Aggiornamento seconda riga DATI
//---------------------------------------------------------------------------


int update_D1(int count)
{

float 	casuale; 
float	value;
int	index_arr;
int 	i	=	0;


	pthread_mutex_lock(&DATI_mutex);
	if (count > UPDATE_D1) {

		for (i = 0; i < L; i++) {
			if (fibrillation) 
				casuale = rand()%20; 
			else casuale = 0;	
			
			DATI[1][i]					=	 vettore[i] + casuale/CONST_FIBR;
			DATI[1][i + L]				=	 vettore[i] + casuale/CONST_FIBR;
			DATI[1][i + K]				= 	 vettore[i] + casuale/CONST_FIBR;
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

	pthread_mutex_unlock(&DATI_mutex);
	return count;
}


//---------------------------------------------------------------------------
//                  Calcolo dei battiti per minuto
//---------------------------------------------------------------------------


int bpm_calculation(int counter)
{
	
int 	n_picchi 	= 	0;
int 	distanza_p, i, bpm_col, picchi[B];
float 	distanza_t; //distanza temporale
char 	text[Q];
	

	if (counter > 1) {
		pthread_mutex_lock(&DATI_mutex);

		for (i = 0; i < M; i++) {
				if (DATI[0][i] > PICK_VALUE) {
					picchi[n_picchi] = i;
					n_picchi++;
				}
			}

		pthread_mutex_unlock(&DATI_mutex);
		
		distanza_p = picchi[n_picchi - 1] - picchi[n_picchi - N];	//calcolo la distanza di campioni tra gli ultimi due picchi, ogni campione corrisponde a 0,008 secondi
		distanza_t = distanza_p * SAMPLING_TIME;				
		bpm = floor(SECOND_FOR_MINUTE/distanza_t);
		
		if (bpm < BPM_LIMIT && bpm > 0) {

			rectfill(screen, rect_coord_x1-90, rect_coord_y2-4, rect_coord_x1, rect_coord_y2+8, 0);   //cancello il precedente valore BPM	

			if (bpm >= BPM_SUP) {		//cambia colore in base al valore
				bpm_col = 4;		//rosso
			}
			else if (bpm < BPM_INF) {		
				bpm_col = 15;		//bianco
			}
			else {
				bpm_col = 11; 		//azzurro
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


//---------------------------------------------------------------------------
//                  Visualizzazione a video le anomalie
//---------------------------------------------------------------------------


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


//---------------------------------------------------------------------------
//     Salvataggio delle anomalie per la successiva scrittura su file
//---------------------------------------------------------------------------


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


//---------------------------------------------------------------------------
//                    Scrittura delle anomalie su file
//---------------------------------------------------------------------------


bool write_anomaly()
{

FILE 	*fw;
int 	j, i	=    0;

	fw = fopen("ECG_report.txt", "w");
	if (fw == NULL) 
		return false;

	i = 0;	
	while (anomaly_note_tachy[0][i] > 0) {

		j = i;
		while (anomaly_note_tachy[1][j] == anomaly_note_tachy[1][j+1] || anomaly_note_tachy[1][j] + 1 == anomaly_note_tachy[1][j+1]) {
			j++;	
		}
		if (i != j){	
			fprintf(fw, "Tachicardia rilevata tra %d e %d secondi\n", anomaly_note_tachy[1][i], anomaly_note_tachy[1][j]); 
			fprintf(fw, "------------------------------------------\n");
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
			fprintf(fw, "Braducardia rilevata tra %d e %d secondi\n", anomaly_note_brady[1][i], anomaly_note_brady[1][j]); 
			fprintf(fw, "------------------------------------------\n");
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
			fprintf(fw, "------------------------------------------\n");
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
			fprintf(fw, "------------------------------------------\n");
		}
		i = j + 1;
	}

	fclose(fw);
}






















