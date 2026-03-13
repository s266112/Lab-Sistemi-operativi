#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE_LUNG 512     // Lunghezza massima per ogni comando
#define MAX_COMMANDS 256     // Numero massimo di comandi
#define MAX_PROCESSES 128   // Numero massimo di parametri

int main(int argc, char *argv[])
{
    FILE *f;

    // Controllo gli argomenti passati al programma
    if(argc != 4)
    {
        printf("Numero di argomenti sbagliato!\n");
        exit(-1);
    }


    // Controllo se il numero di processi è valido
    if(atoi(argv[2]) <= 0)
    {
        printf("Numero di processi errato!\n");
        exit(-1);
    }


    // Apro il file
    f = fopen(argv[1], "r");
    if(!f)
    {
        printf("Impossibile aprire file!\n");
        exit(-1);
    }


    // Variabili per la sostituzione del simbolo '%' nei comandi
    char *cambio;
    char comando[MAX_LINE_LUNG];                    // Comando base
    char comandi[MAX_PROCESSES][MAX_LINE_LUNG];     // Array per memorizzare i comandi
    char buf[MAX_LINE_LUNG];                        // Buffer per leggere le righe dal file
    char riga[MAX_LINE_LUNG];                       // Buffer per la parte dopo '%'
    int counter = 0;                                // Conta il numero di comandi letti


    // Leggo ogni riga del file
    while(fgets(buf, sizeof(buf), f) != NULL)
    {
        // Rimuovo newline alla fine della riga
        buf[strlen(buf)-1] = '\0';

        // Assegno il comando base (es. "ls % -lh")
        strcpy(comando, argv[3]); 

        // Trovo il simbolo '%' nel comando base 
        cambio = strstr(comando, "%");

        if (cambio) 
        {
            strcpy(riga, cambio + 1);           // Copio la parte dopo '%'
            strcpy(cambio, buf);                // Sostituisco il '%' con il parametro
            strcat(comando, riga);              // Ricostruisco il comando completo
        }

        // Controllo se ho raggiunto il numero massimo di comandi
        if(counter >= MAX_PROCESSES)
        {
            printf("Numero massimo di comandi!\n");
            exit(-1);
        }

        // Memorizzo il comando completo nell'array
        strcpy(comandi[counter], comando);     
        counter++;
    }

    fclose(f);      // Chiudo il file dopo la lettura
    
    
    int npipe[MAX_COMMANDS][2];         // Array per le pipe
    int n = atoi(argv[2]);             // Ottengo il numero di processi da eseguire


    // Creo un processo figlio per ciascun comando
    for(int i = 0; i < n; i++)
    {
        int* pipes = npipe[i];
        pipe(pipes);                    // Creo una pipe per ciascun processo

        if(fork())                      // Se siamo nel processo padre
        {
            close(pipes[0]);           // Chiudo il lato di LETTURA della pipe nel padre

            // Scrivo i comandi nelle pipe
            for(int j = i; j < counter; j += n)
            {
                write(pipes[1], comandi[j], MAX_LINE_LUNG); // Scrivo i comandi nella pipe
            }
            close(pipes[1]);            // Chiudo il lato di SCRITTURA della pipe nel padre
            continue;
        }

        else                                    // Se siamo nel processo figlio

        {
            for(int j = 0; j <= i; j++)         // Chiudo i lati di scrittura delle pipe non necessari
            {
                close(npipe[j][1]);             // Chiudo i lati di scrittura non necessari
            }
            char eseguibile[MAX_LINE_LUNG];

            // Leggo i comandi dalla pipe e li eseguo
            while(read(pipes[0], eseguibile, MAX_LINE_LUNG) > 0)
            {
                system(eseguibile);         // Esegui il comando
            }
            exit(0);                       // Termina il processo figlio
        }
    }
    
    // Aspetto che tutti i figli terminino
    while(wait(NULL) > 0) {}  

    return 0;

}
