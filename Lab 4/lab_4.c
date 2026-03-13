#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>        // read, write, close
#include <semaphore.h>     // sem_open, sem_wait, sem_post...
#include <fcntl.h>         // O_CREAT, O_EXCL, ecc.
#include <sys/stat.h>      // Per i permessi (mode_t)
#include <errno.h>         // Gestione errori

#define MAX_NAME_LEN 64



// --- Struttura per rappresentare una vettura ---
typedef struct
{
    char nome[MAX_NAME_LEN];    // es: "corsa-1"
    char stato[8];              // "free" oppure  "busy"
}
vettura;



// --- Funzione per caricare le auto dal catalogo ---
vettura* carica_catalogo(const char* nome_file, int* num_vetture)
{
    FILE* file = fopen(nome_file, "r");
    if (!file)
    {
        perror("Errore nell'apertura del catalogo");
        exit(1);
    }

    vettura* elenco = malloc(sizeof(vettura) * 100);  // supponiamo max 100 auto
    *num_vetture = 0;

    while (fgets(elenco[*num_vetture].nome, MAX_NAME_LEN, file))
    {
        elenco[*num_vetture].nome[strcspn(elenco[*num_vetture].nome, "\n")] = '\0';
        strcpy(elenco[*num_vetture].stato, "free");
        (*num_vetture)++;
    }

    fclose(file);
    return elenco;
}



// --- Funzione per inizializzare i semafori per ogni macchina ---
void inizializza_semafori(vettura* elenco, int num_vetture)
{
    for (int i = 0; i < num_vetture; i++)
    {
        char nome_semaforo[128];
        snprintf(nome_semaforo, sizeof(nome_semaforo), "/sem_%s", elenco[i].nome);

        sem_t* sem = sem_open(nome_semaforo, O_CREAT, 0666, 1);  
        if (sem == SEM_FAILED)
        {
            perror("Errore nella creazione del semaforo");
            exit(1);
        }

        sem_close(sem);
    }
}

/* INFO:
- snprintf(...): Crea un nome tipo: /sem_corsa-1
- sem_open(..., O_CREAT, 0666, 1): Crea un semaforo con permessi 0666 (lettura/scrittura per tutti) e valore 1 (risorsa libera)
- sem_close(sem): Chiude il descrittore locale, ma non rimuove il semaforo */



// --- Funzione che legge il file stato.txt----
void aggiorna_stati_da_file(vettura* elenco, int num_vetture, const char* nome_file_stato)
{
    FILE* file = fopen(nome_file_stato, "r");
    if (!file)
    {
        perror("Errore nell'apertura del file stato");
        return;    // Non interrompe il programma, magari è la prima esecuzione
    }

    char nome[MAX_NAME_LEN];
    char stato[8];

    while (fscanf(file, "%s %s", nome, stato) == 2)
    {
        for (int i = 0; i < num_vetture; i++)
        {
            if (strcmp(elenco[i].nome, nome) == 0)
            {
                // Copia lo stato
                strncpy(elenco[i].stato, stato, sizeof(elenco[i].stato));
                
                // Rimuove eventuale newline o \r
                elenco[i].stato[strcspn(elenco[i].stato, "\r\n")] = '\0';
            }
        }
    }

    fclose(file);
}

/* INFO:
- fscanf(file, "%s %s", ...): Legge una riga alla volta con nome e stato;
- strcmp(...): Confronta il nome letto con ogni vettura;
- strncpy(...): Aggiorna il campo stato (con protezione sulla lunghezza);
Se il file non esiste (es. prima esecuzione), stampa un errore ma non termina il programma.*/



// --- Funzione che salva sul file stato.txt ---
void salva_stati_su_file(vettura* elenco, int num_vetture, const char* nome_file_stato)
{
    FILE* file = fopen(nome_file_stato, "w");
    if (!file)
    {
        perror("Errore nell'apertura in scrittura del file stato");
        return;
    }

    for (int i = 0; i < num_vetture; i++)
    {
        fprintf(file, "%s %s\n", elenco[i].nome, elenco[i]. stato);
    }

    fclose(file);
}

/* INFO:
- fopen(..., "w"): Apre (o crea) il file e cancella il contenuto precedente;
- fprintf(...): Scrive una riga per ogni vettura, in formato <nome> <stato>;
- fclose(...): Chiude correttamente il file.*/



// COMANDI DELL'UTENTE: Funzione comando_view
void comando_view(vettura* elenco, int num_vetture)
{
    for (int i = 0; i < num_vetture; i++)
    {
        printf("Car: %s, status: %s\n", elenco[i].nome, elenco[i].stato);
    }
}

// COMANDI DELL'UTENTE: Funzione comando_lock
void comando_lock(vettura* elenco, int num_vetture, const char* nome_vettura, const char* nome_file_stato)
{
    int trovata = 0;

    for (int i = 0; i < num_vetture; i++)
    {
        if (strcmp(elenco[i].nome, nome_vettura) == 0)
        {
            trovata = 1;

            // Verifico se la vettura è gia occupata
            if (strcmp(elenco[i].stato, "busy") == 0)
            {
                printf("Error. Car %s already locked\n", nome_vettura);

                return;
            }

            // Nome del semaforo
            char nome_semaforo[128];
            snprintf(nome_semaforo, sizeof(nome_semaforo), "/sem_%s", nome_vettura);

            // Tenta di acquisire il semaforo
            sem_t* sem = sem_open(nome_semaforo, 0);
            if (sem == SEM_FAILED)
            {
                perror("Errore nell'apertura del semaforo");
                return;
            }

            sem_wait(sem);    // LOCK

            // Modifico lo stato
            strcpy(elenco[i].stato, "busy");

            // Salva il nuovo stato sul file
            salva_stati_su_file(elenco, num_vetture, nome_file_stato);

            printf("Car: %s is now locked\n", nome_vettura);;

            sem_close(sem);     // Chiude il semaforo
            return;
        }
    }

    if (!trovata)
        printf("Cannot find car %s\n", nome_vettura);
}

// COMANDI DELL'UTENTE: Funzione comando_release
void comando_release(vettura* elenco, int num_vetture, const char* nome_vettura, const char* nome_file_stato)
{
    int trovata = 0;

    for (int i = 0; i < num_vetture; i++)
    {
        if (strcmp(elenco[i].nome, nome_vettura) == 0)
        {
            trovata = 1;

            // Verifico se la vettura è gia libera
            if (strcmp(elenco[i].stato, "free") == 0)
            {
                printf("Error. Car %s already free\n", nome_vettura);
                return;
            }

            // Nome del semaforo
            char nome_semaforo[128];
            snprintf(nome_semaforo, sizeof(nome_semaforo), "/sem_%s", nome_vettura);

            // Rimuovo eventualmente un semaforo bloccato
            sem_unlink(nome_semaforo);  // Elimina il vecchio semaforo se esiste

            // Apro (o creo) il semaforo nuovo, con valore iniziale 1 (disponibile)
            sem_t* sem = sem_open(nome_semaforo, O_CREAT, 0666, 1);
            if (sem == SEM_FAILED)
            {
                perror("Errore nella creazione del semaforo");
                return;
            }
            
            sem_wait(sem);    // LOCK
           
            // Modifico lo stato a "free"
            strcpy(elenco[i].stato, "free");

            // Salva il nuovo stato sul file
            salva_stati_su_file(elenco, num_vetture, nome_file_stato);

            printf("Car: %s is now free\n", nome_vettura);

            sem_close(sem);
            return;
        }
    }

    if (!trovata)
        printf("Cannot find car %s\n", nome_vettura);
}

/* INFO:
- sem_unlink(nome): elimina un semaforo esistente (se presente)
- sem_open(..., O_CREAT, 0666, 1): crea un nuovo semaforo disponibile
- sem_wait: entra nella sezione critica
- sem_close: chiude il semaforo
*/

int main()
{
    int num_vetture;
    vettura* elenco = carica_catalogo("catalog.txt", &num_vetture);

    inizializza_semafori(elenco, num_vetture);
    aggiorna_stati_da_file(elenco, num_vetture, "stato.txt");

    char comando[128];
    char nome_vettura[MAX_NAME_LEN];

    while (1)
    {
        printf("Command: ");
        fflush(stdout);

        // Legge una riga da input (es: "lock corsa")
        if (!fgets(comando, sizeof(comando), stdin))
            break;

        // Rimuove newline
        comando[strcspn(comando, "\n")] = '\0';

        // Comando: quit
        if (strcmp(comando, "quit") == 0)
        {
            break;
        }
        // Comando: view
        else if (strcmp(comando, "view") == 0)
        {
            comando_view(elenco, num_vetture);
        }
        // Comando: lock <nome>
        else if (sscanf(comando, "lock %s", nome_vettura) == 1)
        {
            comando_lock(elenco, num_vetture, nome_vettura, "stato.txt");
        }
        // Comando: release <nome>
        else if (sscanf(comando, "release %s", nome_vettura) == 1)
        {
            comando_release(elenco, num_vetture, nome_vettura, "stato.txt");
        }
        else
        {
            printf("Unknown Command\n");
        }
    }

    free(elenco);
    return 0;
}














