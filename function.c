/*					FUNCTION.C				*/


/*******************************************************************************/
/************************ FUNCTIONS IMPLEMENTATION *****************************/
/*******************************************************************************/

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
int 	i	=	0;


	pthread_mutex_lock(&DATI_mutex);
	if (count > UPDATE_D1) {

		/** GESTIONE FIBRILLAZIONE **/

		for (i = 0; i < L; i++) {
			if (fibrillation) 
				casuale = rand()%20; 
			else casuale = 0;	
			
			DATI[1][i]				=	 vettore[i] + casuale/CONST_FIBR;
			DATI[1][i + L]				=	 vettore[i] + casuale/CONST_FIBR;
			DATI[1][i + K]				= 	 vettore[i] + casuale/CONST_FIBR;
		}
		
		/** GESTIONE GENERAZIONE ARITMIA **/

		if (arrhythmia) {
			gen_arr();

		}

		/** GESTIONE GENERAZIONE TACHICARDIA **/

		if (tachycardia) {
			sampler();
		}
		count = 0;
	}

	pthread_mutex_unlock(&DATI_mutex);
	return count;
}

//---------------------------------------------------------------------------
//                Generazione aritmia 
//---------------------------------------------------------------------------

void gen_arr()
{

int	index_arr,	i;
float	value;

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
//---------------------------------------------------------------------------
//                  Calcolo dei battiti per minuto
//---------------------------------------------------------------------------


int bpm_calculation(int counter)
{
	
int 	n_picchi 	= 	0;
int 	distanza_p, i, bpm_col, picchi[50];
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
		
		distanza_p = picchi[n_picchi - 1] - picchi[n_picchi - 2];	//calcolo la distanza di campioni tra gli ultimi due picchi, ogni campione corrisponde a 0,008 secondi
		distanza_t = distanza_p * SAMPLING_TIME;

		if (SECOND_FOR_MINUTE/distanza_t < BPM_LIMIT & SECOND_FOR_MINUTE/distanza_t > 0) {	
			bpm = floor(SECOND_FOR_MINUTE/distanza_t);

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
			fprintf(fw, "Bradicardia rilevata tra %d e %d secondi\n", anomaly_note_brady[1][i], anomaly_note_brady[1][j]); 
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

/******************************************* END OF FILE ****************************************************/
