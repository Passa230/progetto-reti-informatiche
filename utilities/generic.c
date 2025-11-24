enum colonna {TO_DO, DOING, DONE};

typedef enum colonna colonna_t;



// DEFINIZIONE DELLA STRUTTURA TIMESTAMP
//------------------------------------------------------------
struct {
    int anno;
    int mese;
    int giorno;
    int ora;
    int minuto;
    int secondo;
} timestamp;

typedef struct timestamp timestamp_t;

// FUNZIONI PER LA GESTIONE DEL TIMESTAMP
//------------------------------------------------------------
/*void aggiorna_timestamp(int anno, int mese, int giorno, int ora, int minuto, int secondo) {
    timestamp_t* ts = (timestamp_t*) malloc(sizeof(timestamp_t));
    ts->anno = anno;
    ts->mese = mese;
    ts->giorno = giorno;
    ts->ora = ora;
    ts->minuto = minuto;
    ts->secondo = secondo;
}*/