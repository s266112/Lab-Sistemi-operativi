#!/bin/bash

# Controllo che ci siano almeno 2 argomenti 
if [ "$#" -lt 2 ]; then
    echo "Uso: $0 conf-file comando"
    exit 1
fi

# Salvo il file di configurazione e il comando da eseguire 
CONFIG_FILE="$1"            # Arg 1: File di configurazione
COMMAND="${@:2}"            # Dal 2 in poi: Comando da eseguire nel container

# Creo una directory temporanea per simulare il filesystem del container 
WORKDIR=$(mktemp -d -t container-XXXXXX)
echo "[INFO] Directory container: $WORKDIR"

# Imposto IFS (Internal Field Separator) per leggere riga per riga
IFS=$'\n'

# Legge il file riga per riga usando while
while IFS= read -r line; do
    ORIGIN=$(echo "$line" | cut -d ' ' -f1)    # Primo campo: Percorso reale
    TARGET=$(echo "$line" | cut -d ' ' -f2)    # Secondo campo: Percorso nel container
    DEST="$WORKDIR$TARGET"                     # Percorso completo nel container

    mkdir -p "$(dirname "$DEST")"             # Crea eventuali cartelle intermedie

    if [ -d "$ORIGIN" ]; then
        mkdir -p "$DEST"                      # Crea la cartella di destinazione per bindfs
        # E' una directory: la monto nel container
        bindfs --no-allow-other "$ORIGIN" "$DEST"
    elif [ -f "$ORIGIN" ]; then
        # È un file: lo copio nel container
        cp "$ORIGIN" "$DEST"
    else
        echo "[WARNING] Origine non trovata: $ORIGIN"
    fi
done < "$CONFIG_FILE"

# Eseguo il comando nel container usando fakechroot
echo "[INFO] Avvio comando nel container: ${@:2}"
fakechroot chroot "$WORKDIR" "${@:2}"


# INFO: 
#   - IFS=$'\n': Serve a leggere ogni riga per intero, anche se ci sono spazi.
#   - cut -d' ': -f1 e -f2 prendono il primo e il secondo path.
#   - mkdir -p: Crea le cartelle necessarie.
#   - bindfs: Serve solo per le directory.
#   - cp: Serve per i file.

    
    