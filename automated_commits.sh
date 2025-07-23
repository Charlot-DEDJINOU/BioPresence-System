#!/bin/bash

# Vérifie si un dépôt Git existe dans le répertoire courant
if [ ! -d ".git" ]; then
  echo "Ce répertoire n'est pas un dépôt Git."
  exit 1
fi

# Met à jour l'index
git add -A

# Récupère les fichiers modifiés, ajoutés et supprimés
files=$(git diff --cached --name-status)

if [ -z "$files" ]; then
  echo "Aucun changement à valider."
  exit 0
fi

# Traite chaque fichier modifié
echo "Traitement des fichiers modifiés..."
while IFS= read -r line; do
  status=$(echo "$line" | awk '{print $1}')
  file=$(echo "$line" | awk '{print $2}')
  
  case "$status" in
    A)
      git commit "$file" -m "Ajout de $file"
      ;;
    M)
      git commit "$file" -m "Modification de $file"
      ;;
    D)
      git commit "$file" -m "Suppression de $file"
      ;;
    *)
      echo "Statut inconnu pour $file : $status"
      ;;
  esac
done <<< "$files"

echo "Tous les fichiers modifiés ont été validés."