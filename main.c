/***************************************************************************/
/* MAIN.C ECG RTS PROJECT, WRITTEN BY FABIO POLISANO AND ANTONIO MANUELE   */
/***************************************************************************/

#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<pthread.h>
#include 	<allegro.h>
#include 	<math.h>
#include 	<stdbool.h>
#include 	"task.h"
#include	"main.h"
#include	"function.c"

/******************************************************************************/
/******************************* MAIN FUNCTION ********************************/
/******************************************************************************/

int main(void)
{

	if (init() != true)
		return 1;

	// tasks creation

	task_create(draw_task,			DRAW_TASK,	DRAW_PERIOD,	1000,	19);
	task_create(generatore,			GENR_TASK,	GENR_PERIOD,	1000,	19);
	task_create(user_command,		USER_TASK,	USER_PERIOD,	500,	20);
	task_create(info,			INFO_TASK,	INFO_PERIOD,	500,	20);
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
	//pthread_join(tid[INFO_TASK],NULL);
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



/******************************************************************************/
/************************* TASK IMPLEMENTATION ********************************/
/******************************************************************************/

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
									
			moment = count_time * SAMPLING_TIME * B;	//ogni campione vale 0.008 s
			anomaly_save(moment);				//conservo la anomalia riscontrata al tempo corrente
			
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
		
		pthread_mutex_lock(&tachy_mutex);	
		if (bpm >= BPM_SUP) {
			anomaly_tachy = true;		//se supera il valore di soglia setta la variabile a 'true'
		} else {
			anomaly_tachy = false;
		} 
		pthread_mutex_unlock(&tachy_mutex);
			
		pthread_mutex_lock(&brady_mutex);
		if (bpm <= BPM_INF) {
			anomaly_brady = true;
		} else {
			anomaly_brady = false;
		}
		pthread_mutex_unlock(&brady_mutex);

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

int 	i   =   0;
bool	activation, activation_arr;

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
			
			pthread_mutex_lock(&arrhyt_mutex);
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
				arrhythmia_computation();
			}
			pthread_mutex_unlock(&arrhyt_mutex);
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

int 	i   =   0;
bool	activation_fibr, activation;

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
			
			fibrillation_computation(); 	//funzione che svolge i calcoli

		}
		if (deadline_miss(FIBR_TASK) == 1) printf("DEADLINE MISS FIBRILLATION\n");     //soft real time

		wait_for_activation(FIBR_TASK);
	}
}

/******************************************* END OF FILE ****************************************************/
