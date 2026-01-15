# Progetto Reti Informatiche - Kanban Board

Questo progetto implementa un sistema di **Kanban Board** distribuito basato su architettura client-server. Il sistema permette a più utenti di collaborare, creare task (card), assegnarseli e portarli a termine attraverso un meccanismo di peer review.

## Panoramica

L'applicazione è composta da due entità principali:
- **Server (`lavagna`)**: Gestisce lo stato globale della lavagna (ToDo, Doing, Done), memorizza le card e coordina l'assegnazione dei task agli utenti connessi.
- **Client (`utente`)**: Interfaccia utilizzata dagli utenti per connettersi al server, visualizzare la lavagna, creare nuove card e gestire il proprio lavoro.

Una caratteristica fondamentale è il sistema di **Peer Review**: prima che una card possa essere segnata come completata ("Done"), il lavoro deve essere revisionato e approvato da altri utenti connessi al sistema tramite una comunicazione P2P.

## Struttura del Progetto

Il progetto è organizzato nelle seguenti cartelle e file:

*   **`lavagna.c`**: Codice sorgente del server principale. Gestisce le connessioni TCP in entrata, lo scheduler delle card e la sincronizzazione dello stato.
*   **`utente.c`**: Codice sorgente del client. Gestisce l'interfaccia utente (CLI), la comunicazione con il server e la gestione delle richieste di review P2P.
*   **`include/`**: Contiene i file header con definizioni di strutture dati, costanti e prototipi di funzioni condivise.
    *   `lavagna.h`, `structure.h`, `generic.h`, `color.h`.
*   **`src/`**: Contiene l'implementazione delle funzioni di utilità condivise.
*   **`Makefile`**: Script per l'automazione della compilazione del progetto.

## Funzionamento

### Gestione delle Card
Le card attraversano tre stati principali:
1.  **ToDo**: La card è stata creata ma non ancora assegnata.
2.  **Doing**: La card è stata presa in carico da un utente. Ogni utente può avere in carico una sola card alla volta.
3.  **Done**: La card è stata completata e validata.

### Peer Review
Quando un utente completa il proprio task, deve richiedere una **review**. Il sistema seleziona casualmente altri utenti connessi che riceveranno una notifica di richiesta review. Solo dopo aver ricevuto un numero sufficiente di approvazioni, l'utente può spostare la card in "Done".

## Come Usarlo

### Prerequisiti
*   Sistema operativo Linux/Unix.
*   Compilatore `gcc`.
*   `make`.

### Compilazione
Per compilare il progetto, eseguire il comando `make` nella root del progetto:

```bash
make
```

Questo genererà due eseguibili: `lavagna` e `utente`.

### Esecuzione

#### 1. Avviare il Server
Aprire un terminale e avviare il server. Il server ascolterà sulla porta default `5678`.

```bash
./lavagna
```

#### 2. Avviare i Client
Aprire uno o più terminali separati per simulare diversi utenti. Ogni utente deve specificare una **porta locale univoca** (diversa da 5678 e da quelle degli altri utenti) per ricevere le comunicazioni P2P.

Sintassi:
```bash
./utente <porta_locale>
```

Esempio Utente 1:
```bash
./utente 6000
```

Esempio Utente 2:
```bash
./utente 6001
```

### Comandi Disponibili (Client)

Una volta avviato, il client presenta un menu interattivo con i seguenti comandi:

*   `CARD_CREATE`: Crea una nuova card da aggiungere alla colonna ToDo.
*   `SHOW_LAVAGNA`: Visualizza lo stato attuale di tutte le card sulla lavagna.
*   `SHOW_USR_LIST`: Mostra la lista degli utenti connessi.
*   `SHOW_CARD`: Mostra i dettagli della card attualmente assegnata.
*   `REVIEW_CARD`: Richiede una revisione per il lavoro svolto sulla propria card.
*   `OKAY_REVIEW <port>`: Approva la revisione richiesta dall'utente che ascolta sulla porta specificata.
*   `CARD_DONE`: Segnala al server che la card è completa (richiede review completata).
*   `CLEAR`: Pulisce la schermata del terminale.
*   `QUIT`: Chiude il client.
