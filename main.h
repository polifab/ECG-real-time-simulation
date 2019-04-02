/*					MAIN.H								*/

#define 	N 			2		//numero delle righe della matrice DATI
#define 	M 			360		//numero delle colonne della matrice DATI
#define 	K			240		
#define		L			120		//numero di campioni per battito
#define		B			10		//costanti utili nei calcoli
#define 	Q			5		
#define 	W			10000  		//dimensione della matrice in cui vengono salvate le anomalie
#define 	DIM_TEXT		48		//dimensione del testo 
#define		DIM_VALUE_X		180		//dimensione dei valori da visualizzare
#define 	SHIFT_NUMBER 		350		
#define		UPDATE_D1		35		
#define 	PICK_VALUE		0.96		//soglia per campionare i picchi
#define 	N_PICCHI 		50		//numero di picchi che può rilevare ad ogni istanza
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

pthread_mutex_t 	DATI_mutex 		= 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t 	activation_mutex 	= 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t 	tachy_mutex 		= 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t 	brady_mutex 		= 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t 	arrhyt_mutex 		= 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t 	fibril_mutex 		= 	PTHREAD_MUTEX_INITIALIZER;

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

