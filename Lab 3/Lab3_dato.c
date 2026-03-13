#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMAX 512
#define PMAX 256
#define LMAX 128

int main(int argc, char *argv[])
{
    FILE *f;

    // Controllo gli argomenti
    if(argc != 4)
    {
        printf("Numero di argomenti sbagliato!\n");
        exit(-1);
    }

    // Controllo il numero di processi
    if(atoi(argv[2]) <= 0)
    {
        printf("Numero di processi errato!\n");
        exit(-1);
    }

    // Controllo il file
    f = fopen(argv[1], "r");
    if(!f)
    {
        printf("Impossibile aprire file!\n");
        exit(-1);
    }

    char *cambio;
    char comando[CMAX];
    char comandi[LMAX][CMAX];
    char buf[CMAX];
    char riga[CMAX];
    int counter = 0;

    while(fgets(buf, sizeof(buf), f) != NULL)
    {
        // Rimuovo \n
        buf[strlen(buf)-1] = '\0';
        strcpy(comando, argv[3]);
        cambio = strstr(comando, "%");
        if (cambio) {
            strcpy(riga, cambio + 1);
            strcpy(cambio, buf);
            strcat(comando, riga);
        }

        if(counter >= LMAX)
        {
            printf("Numero massimo di comandi!\n");
            exit(-1);
        }
        strcpy(comandi[counter], comando);
        counter++;
    }
    fclose(f);

    int npipe[PMAX][2];

    int n = atoi(argv[2]);

    for(int i = 0; i < n; i++)
    {
        int* pipes = npipe[i];
        pipe(pipes);

        if(fork())
        {
            // Padre
            close(pipes[0]);
            for(int j = i; j < counter; j += n)
            {
                write(pipes[1], comandi[j], CMAX);
            }
            close(pipes[1]);
            continue;
        }
        else
        {
            // Figlio
            for(int j = 0; j <= i; j++)
            {
                close(npipe[j][1]);
            }

            char eseguibile[CMAX];

            while(read(pipes[0], eseguibile, CMAX) > 0)
            {
                system(eseguibile);
            }
            exit(0);
        }
    }

    while(wait(NULL) > 0) {}
    return 0;
}
