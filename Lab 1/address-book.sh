#!/bin/bash

DB_FILE="address-book-database.csv"     # File del database CSV

# Controlla se il file esiste, altrimenti lo crea con l'intestazione
if [ ! -f "$DB_FILE" ]; then
    echo "name,surname,phone,mail,city,address" > "$DB_FILE"
fi

# Funzione per VISUALIZZARE la rubrica (punto 1)
function view()
{
    # Mostra intestazione e dati ordinati per mail, tutto allineato
    (head -n 1 "$DB_FILE" && tail -n +2 "$DB_FILE" | sort -t ',' -k4) | column -t -s ','
}

# Funzione di RICERCA (punto 2)
function search()
{
    # Verifica se è stato passato un termine di ricerca
    if [ -z "$1" ]; then
        echo "Usage: $0 search <string>"
        exit 1
    fi

    local search_term="$1"  
    local found=0           # Flag per tracciare i risultati (0 = non trovato; 1= trovato)

    # Imposta IFS per processare righe interne nel for
    local old_IFS="$IFS"    # Salva il valore originale IFS per salvarlo dopo
    IFS=$'\n'               # Imposta la variabile d'ambiente IFS a newline

    # Cerca il termine in TUTTI i campi
    for line in $(tail -n +2 "$DB_FILE"); do
        if echo "$line" | grep -iq "$search_term"; then
            found=1
            # Ripristina IFS per split sui campi CSV
            IFS=',' read -r name surname phone mail city address <<< "$line"

            # Stampa degli output della ricerca
            echo "Name: $name"
            echo "Surname: $surname"
            echo "Phone: $phone"
            echo "Mail: $mail"
            echo "City: $city"
            echo "Address: $address"
            echo ""  # Linea vuota tra i record
        fi
    done

    # Ripristina IFS originale
    IFS="$old_IFS"

    # Caso non trovato
    [ "$found" -eq 0 ] && echo "Not found"
}

# Funzione di INSERIMENTO (Punto 3)
function insert()
{
    # Richiesta dati all'utente
    read -p "Name: " name
    read -p "Surname: " surname
    read -p "Phone: " phone
    read -p "Mail: " mail
    read -p "City: " city
    read -p "Address: " address

    # Controllo se la Mail esiste già nel file CSV
    if grep -q "^[^,]*,[^,]*,[^,]*,$mail," "$DB_FILE"; then
        echo "Errore: mail già presente nel database"
        exit 1
    fi

    # Aggiungo al file CSV
    echo "$name,$surname,$phone,$mail,$city,$address" >> "$DB_FILE"
    echo "Added"
}

# Funzione di CANCELLAZIONE (Punto 4)
function delete()
{
    # Verifica se è stata fornita una mail come argomento
    [ -z "$1" ] && { echo "Usage: $0 delete <mail>"; exit 1; }

    local mail="$1"            # Salva la mail passata come argomento in una variabile locale
    local temp_file=$(mktemp)  # Crea un file temporaneo
    local found=0

    # Processa il file riga per riga
    while IFS= read -r line; do
        if [[ "$line" == *,"$mail",* ]] || [[ "$line" == *,"$mail" ]]; then
            found=1
        else
            echo "$line" >> "$temp_file"
        fi
    done < "$DB_FILE"

    # Verifica se è stata trovata e cancellata la mail
    if [ $found -eq 1 ]; then
        mv "$temp_file" "$DB_FILE"
        echo "Deleted"
    else
        rm "$temp_file"
        echo "Cannot find any record"
        exit 1
    fi
}


# Gestione dei comandi
if [ "$1" = "view" ]; then
    view                     # Chiama la funzione VIEW

elif  [ "$1" = "search" ]; then
    if [ $# -lt 2 ]; then
        echo "Usage: $0 search <string>"
        exit 1
    fi
    search "$2"              # Chiama la funzione SEARCH

elif [ "$1" = "insert" ]; then
    insert                   # chiama la funzione INSERT

elif  [ "$1" = "delete" ]; then
    if [ $# -lt 2 ]; then
        echo "Usage: $0 delete <mail>"
        exit 1
    fi
    delete "$2"              # Chiama la funzione DELETE

else
    echo "Usage: $0 {view|search|insert|delete} [args...]" 
    exit 1   
fi



