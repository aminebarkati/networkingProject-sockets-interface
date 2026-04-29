# Guide des tests — TP Réseaux

Avant de commencer : ouvre **4 terminaux** et laisse-les ouverts pendant toute la session.

---

## Test 1 — HTTP (Phase 2)

### 1a. Lancer un serveur HTTP local

**Terminal 1 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface
python3 -m http.server 8000
```
Laisse tourner — les requêtes entrantes s'affichent ici.

### 1b. Lancer la capture réseau

**Terminal 2 :**
```bash
sudo tcpdump -i lo -A tcp port 8000
```

### 1c. Requête HEAD avec telnet

**Terminal 3 :**
```bash
telnet 127.0.0.1 8000
```
Une fois connecté, tape exactement ceci puis appuie deux fois sur Entrée :
```
HEAD / HTTP/1.1
Host: localhost

```
> **À noter :** toute la sortie du terminal 3 (depuis "Trying..." jusqu'à "Connection closed")

### 1d. Requête GET

Reconnecte-toi :
```bash
telnet 127.0.0.1 8000
```
```
GET / HTTP/1.1
Host: localhost

```
> **À noter :** toute la sortie du terminal 3 — observer la différence avec HEAD

### 1e. Récupérer la capture

Arrête tcpdump (**Ctrl+C** dans terminal 2).
> **À noter :** les 30-40 premières lignes de la sortie tcpdump

---

## Test 2 — TCP avec sleep (Phase 3a)

### 2a. Capture

**Terminal 2 :**
```bash
sudo tcpdump -i lo -A tcp port 8080
```

### 2b. Lancer le serveur TCP

**Terminal 1 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/server
./server_tcp
```

### 2c. Lancer le client TCP

**Terminal 3 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/Client
./client_tcp
```
L'heure s'affiche toutes les secondes pendant 60 secondes.

> **À noter :**
> - Les 5 premières lignes d'heure reçues
> - La ligne "Au revoir"
> - La ligne finale `recv() appelé X fois`
> - 20 lignes de la sortie tcpdump

Arrête tcpdump et le serveur (Ctrl+C dans leurs terminaux).

---

## Test 3 — TCP sans sleep (Phase 3b)

On retire le `sleep` pour observer la différence de comportement TCP.

**Terminal 1 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/server
sed -i 's/        sleep(1);/        \/\/ sleep(1);/' server_tcp.c
make server_tcp
```

**Terminal 2 :**
```bash
sudo tcpdump -i lo -A tcp port 8080
```

**Terminal 1 :**
```bash
./server_tcp
```

**Terminal 3 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/Client
./client_tcp
```
Ça se termine en quelques secondes.

> **À noter :**
> - La sortie complète du client (le `recv() appelé X fois` est la donnée clé)
> - 20 lignes de tcpdump

Arrête tout. **Remets le sleep :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/server
sed -i 's/        \/\/ sleep(1);/        sleep(1);/' server_tcp.c
make server_tcp
```

---

## Test 4 — UDP (Phase 4)

### 4a. Capture UDP

**Terminal 2 :**
```bash
sudo tcpdump -i lo -A udp port 8080
```

### 4b. Serveur + client UDP

**Terminal 1 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/server
./server_udp
```

**Terminal 3 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/Client
./client_udp
```

> **À noter :**
> - Les 5 premières lignes reçues
> - La ligne `Datagrammes reçus : X`
> - 20 lignes de tcpdump UDP

Arrête tout (Ctrl+C).

---

## Test 5 — Serveur concurrent (Phase 5)

### 5a. Lancer le serveur concurrent

**Terminal 1 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/server
./server_concurrent
```

### 5b. Deux clients simultanés sur le service 1

**Terminal 3 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/Client
./client_service1
```
**Immédiatement** (sans attendre la fin), dans **Terminal 4 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/Client
./client_service2
```

> **À noter :** la sortie complète des deux terminaux avec les timestamps `[HH:MM:SS]`
> → Si les timestamps de service2 apparaissent **pendant** que service1 tourne, la concurrence est prouvée.

### 5c. Script de test parallèle

**Terminal 3 :**
```bash
cd ~/Documents/projects/networkingProject-sockets-interface/Client
./test_parallel.sh
```

> **À noter :** toute la sortie du script
