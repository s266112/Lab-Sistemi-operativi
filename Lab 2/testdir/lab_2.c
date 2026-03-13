#include <stdio.h>
#include <stdlib.h>     // Per funzioni di sistema (exit)
#include <dirent.h>     // Per gestire directory (opendir, readdir)
#include <sys/stat.h>   // Per informazioni sui file
#include <string.h>     // Per manipolare stringhe (strcmp, strcat)
#include <pwd.h>        // Per informazioni utente (getpwuid)
#include <grp.h>        // Per informazioni gruppo (getgrgid)

void stampaDettagli(const char *percorso, struct stat *stat_buf)
{
    struct passwd *pw = getpwuid(stat_buf->st_uid);     // Info del proprietario 
    struct group *gr = getgrgid(stat_buf->st_gid);      // Info del gruppo 

    printf("Node: %s\n", percorso);
    printf("    Inode: %lu\n", stat_buf->st_ino);

    // Determina il tipo di file
     printf("    Type: ");
     if (S_ISDIR(stat_buf->st_mode))
     {
         printf("directory");
     }
     else if (S_ISREG(stat_buf->st_mode))
     {
         printf("file");
     }
     else if (S_ISLNK(stat_buf->st_mode))
     {
         printf("symbolic link");
     }
     else if (S_ISFIFO(stat_buf->st_mode))
     {
         printf("FIFO");
     }
     else
     {
         printf("other");         // Default per tipi non richiesti
     }
     printf("\n");

    // Stampa dimensione e proprietari
    printf("    Size: %ld\n", stat_buf->st_size);
    printf("    Owner: %d %s\n", stat_buf->st_uid, pw ? pw->pw_name : "unknown");
    printf("    Group: %d %s\n", stat_buf->st_gid, gr ? gr->gr_name : "unknown");
}


void esploraDirectory(const char *percorso, int *prima_volta)
{
    DIR *dir;
    struct dirent *entry;
    struct stat stat_buf;
    
    // Apro la directory
    if (!(dir = opendir(percorso)))
    {
        perror("Errore opendir");
        return;
    }

    // Stampo la dirctory corrente solo la prima volta
    if (*prima_volta == 0)
    {
        if (stat(percorso, &stat_buf) == 0)
        {
            stampaDettagli(percorso, &stat_buf);
            (*prima_volta)++;
        }
    }

    // Leggo ogni elemento della directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Salta "." e ".." per evitare loop infiniti
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Costruisco il percorso completo
        char path_completo[1024];
        snprintf(path_completo, sizeof(path_completo), "%s/%s", percorso, entry->d_name);

        if (stat(path_completo, &stat_buf) == -1) 
        {
            perror("Errore stat");
            continue;
        }

        // Stampa info elemento
        stampaDettagli(path_completo, &stat_buf);

        // Se è directory, chiamata ricorsiva
        if (S_ISDIR(stat_buf.st_mode))
        {
            esploraDirectory(path_completo, prima_volta);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[])
{ 
    // Controllo degli argomenti
    if (argc != 2)
    {
        fprintf(stderr, "Utilizzo: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int prima_volta = 0;
    esploraDirectory(argv [1], &prima_volta);
    return EXIT_SUCCESS;
}
