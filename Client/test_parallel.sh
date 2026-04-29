#!/bin/bash
# =============================================================================
# test_parallel.sh — Test de concurrence du serveur multi-services
# =============================================================================
#
# Lance les 3 clients de service simultanément en arrière-plan (&).
# Les timestamps affichés par chaque client prouvent qu'ils s'exécutent
# en parallèle : si le serveur était séquentiel, client_service1 bloquerait
# les deux autres pendant 60 secondes.
#
# Usage : chmod +x test_parallel.sh && ./test_parallel.sh
# =============================================================================

echo "=== Lancement des 3 clients en parallèle ==="
echo ""

./client_service1 > /tmp/out_service1.txt 2>&1 &
PID1=$!
echo "[PID=$PID1] client_service1 lancé"

./client_service2 > /tmp/out_service2.txt 2>&1 &
PID2=$!
echo "[PID=$PID2] client_service2 lancé"

./client_service3 > /tmp/out_service3.txt 2>&1 &
PID3=$!
echo "[PID=$PID3] client_service3 lancé"

echo ""
echo "Attente de la fin de tous les clients..."
wait $PID1 $PID2 $PID3

echo ""
echo "=== Résultats ==="
echo ""
echo "--- Service 1 (heure) ---"
cat /tmp/out_service1.txt

echo ""
echo "--- Service 2 (processus) ---"
cat /tmp/out_service2.txt

echo ""
echo "--- Service 3 (fichier) ---"
cat /tmp/out_service3.txt

echo ""
echo "=== Test terminé ==="
echo "Vérifiez que les timestamps de Service1 et Service2/3 se chevauchent."
