#!/bin/bash
# =============================================================================
# test_parallele.sh — Script de test du serveur concurrent multi-services
# =============================================================================
#
# Description :
#   Lance plusieurs clients de chaque service simultanément pour vérifier
#   que le serveur concurrent traite bien toutes les requêtes en parallèle.
#
#   Ce script vérifie deux choses :
#     1. Plusieurs clients du MÊME service sont traités en parallèle
#        (grâce au fork() dans server_concurrent.c)
#     2. Des clients de SERVICES DIFFÉRENTS tournent en parallèle
#        (grâce aux 3 processus écouteurs)
#
# Usage :
#   chmod +x test_parallele.sh
#   ./test_parallele.sh
#
# Prérequis :
#   - Les binaires client_service1, client_service2, client_service3
#     doivent être compilés.
#   - Le serveur concurrent doit être démarré :
#     ./server_concurrent
#
# =============================================================================

# ─── Couleurs pour la lisibilité ─────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ─── Configuration ────────────────────────────────────────────────────────────
NB_CLIENTS_PAR_SERVICE=3   # Nombre de clients lancés par service
LOG_DIR=""

# Optional environment flags for testing:
#  DETECT_ONLY=1     -> only detect binary locations and print them, then exit
#  SKIP_PORT_CHECK=1 -> skip server port/network availability checks

# Determine where the client binaries live. Priority order:
#  1) current working directory (./client_service1)
#  2) ./output (relative to cwd)
#  3) script directory
#  4) script_dir/output
detect_bin_dir() {
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    candidates=("$PWD" "$PWD/output" "$script_dir" "$script_dir/output")
    for d in "${candidates[@]}"; do
        if [ -x "$d/client_service1" ] && [ -x "$d/client_service2" ] && [ -x "$d/client_service3" ]; then
            BIN_DIR="$d"
            return 0
        fi
    done
    # Fallback: prefer ./output if it contains any binaries, or current dir
    if [ -e "./output/client_service1" ] || [ -e "./output/client_service2" ] || [ -e "./output/client_service3" ]; then
        BIN_DIR="./output"
    else
        BIN_DIR="."
    fi
}

detect_bin_dir
LOG_DIR="${LOG_DIR:-$BIN_DIR/logs_test}"

# ─── Vérifications préliminaires ──────────────────────────────────────────────
echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE}   TEST PARALLÈLE — Serveur Concurrent       ${NC}"
echo -e "${BLUE}=============================================${NC}\n"

# Vérifier que les binaires existent in the detected BIN_DIR
for bin in client_service1 client_service2 client_service3; do
    if [ ! -x "$BIN_DIR/$bin" ]; then
        echo -e "${RED}[ERREUR] Binaire '$BIN_DIR/$bin' introuvable ou non exécutable.${NC}"
        echo "         Lancez d'abord : make  (ou compilez manuellement)"
        exit 1
    fi
done

echo -e "${GREEN}[OK] Tous les binaires trouvés dans : $BIN_DIR ${NC}"

# If requested, only detect and report paths, then exit (useful for CI/dry-run)
if [ "${DETECT_ONLY}" = "1" ]; then
    echo "BIN_DIR=$BIN_DIR"
    echo "LOG_DIR=$LOG_DIR"
    exit 0
fi

# Vérifier que le serveur écoute (test rapide avec nc)
if [ -z "${SKIP_PORT_CHECK}" ]; then
    for port in 8080 8081 8082; do
        if ! nc -z 127.0.0.1 $port 2>/dev/null; then
            echo -e "${RED}[ERREUR] Rien ne répond sur le port $port.${NC}"
            echo "         Démarrez d'abord le serveur : ./server_concurrent"
            exit 1
        fi
    done
else
    echo -e "${YELLOW}[INFO] SKIP_PORT_CHECK set - skipping server port checks${NC}"
fi

echo -e "${GREEN}[OK] Serveur détecté sur les ports 8080, 8081, 8082.${NC}\n"

# Créer le répertoire de logs
mkdir -p "$LOG_DIR"

# ─── Test 1 : Multi-clients même service (Service 1 — heure) ─────────────────
echo -e "${YELLOW}═══ TEST 1 : $NB_CLIENTS_PAR_SERVICE clients simultanés sur Service 1 (port 8080) ═══${NC}"
echo "Lancement à $(date '+%H:%M:%S')..."
echo ""

PIDS_S1=()
for i in $(seq 1 $NB_CLIENTS_PAR_SERVICE); do
    LOG_FILE="$LOG_DIR/client_s1_instance${i}.log"
    "$BIN_DIR/client_service1" > "$LOG_FILE" 2>&1 &
    PIDS_S1+=($!)
    echo -e "  ${GREEN}[LANCÉ]${NC} client_service1 instance $i — PID=${!} — log: $LOG_FILE"
done

echo ""
echo "[...] Attente de la fin des clients Service 1..."
for pid in "${PIDS_S1[@]}"; do
    wait $pid
    STATUS=$?
    if [ $STATUS -eq 0 ]; then
        echo -e "  ${GREEN}[OK]${NC} PID $pid terminé avec succès (code $STATUS)"
    else
        echo -e "  ${RED}[KO]${NC} PID $pid terminé avec erreur (code $STATUS)"
    fi
done

echo ""
echo -e "${YELLOW}─── Vérification des résultats Test 1 ───${NC}"
# Vérifier que chaque client a bien reçu des messages
for i in $(seq 1 $NB_CLIENTS_PAR_SERVICE); do
    LOG_FILE="$LOG_DIR/client_s1_instance${i}.log"
    NB_MSG=$(grep -c "Il est" "$LOG_FILE" 2>/dev/null || echo 0)
    echo "  Instance $i : $NB_MSG messages d'heure reçus (attendus : ~60)"
done
echo ""

# ─── Test 2 : Services différents en parallèle ───────────────────────────────
echo -e "${YELLOW}═══ TEST 2 : 1 client par service, tous en parallèle ═══${NC}"
echo "Lancement à $(date '+%H:%M:%S')..."
echo ""

"$BIN_DIR/client_service1" > "$LOG_DIR/test2_s1.log" 2>&1 &
PID_T2_S1=$!
echo -e "  ${GREEN}[LANCÉ]${NC} client_service1 — PID=$PID_T2_S1"

"$BIN_DIR/client_service2" > "$LOG_DIR/test2_s2.log" 2>&1 &
PID_T2_S2=$!
echo -e "  ${GREEN}[LANCÉ]${NC} client_service2 — PID=$PID_T2_S2"

"$BIN_DIR/client_service3" > "$LOG_DIR/test2_s3.log" 2>&1 &
PID_T2_S3=$!
echo -e "  ${GREEN}[LANCÉ]${NC} client_service3 — PID=$PID_T2_S3"

echo ""
echo "[...] Attente de la fin de tous les clients..."

T_DEBUT=$SECONDS

wait $PID_T2_S1 && echo -e "  ${GREEN}[OK]${NC} Service 1 terminé" || echo -e "  ${RED}[KO]${NC} Service 1 erreur"
wait $PID_T2_S2 && echo -e "  ${GREEN}[OK]${NC} Service 2 terminé" || echo -e "  ${RED}[KO]${NC} Service 2 erreur"
wait $PID_T2_S3 && echo -e "  ${GREEN}[OK]${NC} Service 3 terminé" || echo -e "  ${RED}[KO]${NC} Service 3 erreur"

T_FIN=$SECONDS
DUREE=$((T_FIN - T_DEBUT))

echo ""
echo -e "${YELLOW}─── Vérification des résultats Test 2 ───${NC}"
echo "  Durée totale : ${DUREE} secondes"
echo "  (Si < 65s : les services ont bien tourné EN PARALLÈLE)"
echo "  (Si ≥ 180s : les services ont tourné en SÉQUENCE — problème !)"
echo ""

# Afficher le contenu du log Service 2 (résultat court et lisible)
echo "  Résultat Service 2 (nb processus) :"
cat "$LOG_DIR/test2_s2.log" | grep "processus"

# Vérifier que le fichier Service 3 a été créé
FICHIER_RECU=$(ls "$BIN_DIR"/fichier_recu_*.txt 2>/dev/null | head -1)
if [ -n "$FICHIER_RECU" ]; then
    TAILLE=$(wc -c < "$FICHIER_RECU")
    echo "  Fichier Service 3 reçu : $FICHIER_RECU ($TAILLE octets)"
else
    echo -e "  ${RED}[ATTENTION]${NC} Aucun fichier reçu pour Service 3."
fi

# ─── Résumé final ─────────────────────────────────────────────────────────────
echo ""
echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE}               RÉSUMÉ FINAL                  ${NC}"
echo -e "${BLUE}=============================================${NC}"
echo ""
echo "  Logs disponibles dans : $LOG_DIR/"
ls "$LOG_DIR/"
echo ""
echo "  Pour analyser un log :"
echo "    cat $LOG_DIR/client_s1_instance1.log"
echo ""
echo -e "${GREEN}Test terminé à $(date '+%H:%M:%S').${NC}"
