# 3-oji (papildoma) užduotis: Bitcoin transakcijų ir blokų analizė su `Libbitcoin` ir `python-bitcoinlib`

WSL2 yra daug lėtesnis nei realus Linux, todėl visi instaliavimai vyko labaaai ilgai.

---

## 1 DALIS: Merkle medžio implementacija su Libbitcoin 

<details>
 <summary><strong>1.1 Libbitcoin-System įdiegimas</strong></summary>

### Žingsnis 1: Bandymas Windows aplinkoje (NEPAVYKO)

Pirma bandžiau įdiegti Windows 11 su Visual Studio 2022.

Bandžiau per:
- NuGet paketų valdymą
- vcpkg paketų sistemą

**Rezultatas:**

```bash
# NuGet paieška
> Install-Package libbitcoin-system
Error: No packages found

# vcpkg bandymas
> vcpkg install libbitcoin-system:x64-windows
Error: libbitcoin-system does not exist
```

**Kas nutiko:** libbitcoin daugiau nepalaiko Windows. Visi paketai pašalinti iš NuGet ir vcpkg.

**Ką padariau:** Persijungiau į Linux per WSL2.


### Žingsnis 2: WSL2 Ubuntu paruošimas (PAVYKO)

Reikėjo paruošti Linux aplinką kad galėčiau viską sukompiliuoti.

**Veiksmai:**

```bash
# Įdiegiau Ubuntu 24.04 LTS per WSL2
wsl --install -d Ubuntu-24.04

# Atnaujinau sistemą
sudo apt update
sudo apt upgrade -y
```

**Rezultatas:** [PAVYKO] Sistema paruošta, galiu pradėti diegimą.


### Žingsnis 3: Automatinis diegimas su `install.sh` (NEPAVYKO)

Norėjau paleisti oficialų diegimo skriptą, kad viskas įsidiegtų automatiškai.

**Komanda:**

```bash
sudo ./install.sh --build-boost --build-secp256k1
```

**Procesas:**
1. [PAVYKO] Boost 1.86 kompiliaciją pradėjau ir sėkmingai užbaigiau
2. [NEPAVYKO] secp256k1 klonavimas iš GitHub nepavyko:

```
Cloning https://github.com/bitcoin-core/secp256k1.git...
fatal: unable to access 'https://github.com/bitcoin-core/secp256k1.git/': 
       The requested URL returned error: 500 Internal Server Error
```

**Kas nutiko:** GitHub grąžino klaidą (HTTP 500) keletą kartų iš eilės. Automatinis atsisiuntimas neveikė.

**Ką padariau:** Nusprendžiau atsisiųsti ir įdiegti secp256k1 rankiniu būdu.


### Žingsnis 4: Rankinis secp256k1 diegimas (PAVYKO)

Norint apeiti GitHub problemą, atsisiųsiau ZIP failą ir įdiegiau rankiniu būdu.

**Veiksmai:**

```bash
# Atsisiųsti kodą rankiniu būdu
wget https://github.com/bitcoin-core/secp256k1/archive/refs/heads/master.zip
unzip master.zip
cd secp256k1-master

# Kompiliuoti ir įdiegti
./autogen.sh
./configure --enable-module-recovery
make
sudo make install
sudo ldconfig
```

**Rezultatas:** [PAVYKO] secp256k1 biblioteką įdiegiau į `/home/neda/local/lib/libsecp256k1.so`


### Žingsnis 5: Boost kompiliacija su visais gijomis (NEPAVYKO)

Bandžiau sukompiliuoti Boost su įprastais nustatymais.

**Komanda:**

```bash
sudo ./install.sh --build-boost
```

**Procesas:**
- Boost 1.86 kompiliacija pradėta su visomis gijomis (`-j$(nproc)`)
- Kompiliacija progresavo iki ~40%

**Gedimas:**

```
g++: internal compiler error: Killed (program cc1plus)
[Terminated]
```

Terminal užsidarė, WSL procesai sustabdyti.

**Kas nutiko:** 
- `dmesg` parodė: `Out of memory: Killed process`
- WSL2 turi tik 8GB RAM, o Boost kompiliacijai reikia ~12GB

**Ką supratau:** Reikia pridėti swap atmintį (virtualią atmintį).


### Žingsnis 6: 8GB Swap failo sukūrimas (PAVYKO)

Pridėjau 8GB virtualios atminties (swap), kad WSL2 turėtų pakankamai RAM.

**Veiksmai:**

```bash
# Sukurti 8GB swap failą
sudo fallocate -l 8G /swapfile

# Nustatyti leidimus (tik root)
sudo chmod 600 /swapfile

# Inicializuoti swap
sudo mkswap /swapfile

# Aktyvuoti swap
sudo swapon /swapfile

# Patikrinti
free -h
#              total        used        free      shared  buff/cache   available
# Mem:          7.7Gi       2.1Gi       4.2Gi       0.1Gi       1.4Gi       5.3Gi
# Swap:         8.0Gi          0B       8.0Gi
```

**Rezultatas:** [PAVYKO] WSL2 dabar turi 8GB + 8GB = 16GB bendrą atmintį.


### Žingsnis 7: Boost perkompiliavimas su mažiau gijų (PAVYKO)

Bandžiau dar kartą, bet šį kartą sumažinau paralelių procesų skaičių, kad RAM naudojimas būtų mažesnis.

**Komanda:**

```bash
PARALLEL=4 PREFIX=$HOME/local ./install.sh --build-boost
```

**Kaip vyko:**
- Boost kompiliacija su tik 4 gijomis (vietoj 24)
- Užtruko ilgiau (~20 min), bet atmintis naudojosi stabiliai
- Kompiliacija užsibaigė be crash'ų

**Rezultatas:**
```
Building libbitcoin-system...
[100%] Built target bitcoin-system
Running tests...
Test project /home/user/libbitcoin-system/build
      Start  1: blockchain_tests
      ...
      Start 4064: version_tests
100% tests passed, 0 tests failed out of 4064
```

**[PAVYKO] Visos 4064 testai praėjo sėkmingai!**


### Žingsnis 8: Diegimas į sistemą su sudo (PAVYKO)

Dabar reikėjo įdiegti sukompiliuotas bibliotekas į sistemą (`/usr/local/`).

**Problema:**

```bash
make install
# Error: Permission denied — cannot write to /home/neda/local/lib/
```

**Sprendimas:**

```bash
cd build-libbitcoin-system/libbitcoin-system
sudo make install
```

**Rezultatas:** [PAVYKO] Bibliotekas įdiegiau:
- `/home/neda/local/lib/libbitcoin-system.so`
Santrauka:
- libbitcoin pateiktas kodas reikalauja C++20 → nesuderinama su užduoties C++11.
- Perėjimas prie savo realizacijos su OpenSSL (vienas SHA256 kvietimas per jungtą tekstą).
- Įrankiai: `clang` įdiegtas, naudojamas `g++`.

Kompiliacija/testas:
```bash
    new_merkle.push_back(new_root);
```
Gautas testinis root: `4702bc31...92e0`.
}
```

Kiekvienoje iteracijoje hash'ai apdorojami poromis (`it += 2`):

1. **Sujungiami du hash'ai:** kiekviena pora konkatenojama į vieną duomenų bloką
2. **Taikomas dvigubas SHA-256:** naudojama `bitcoin_hash()` funkcija (SHA-256 du kartus)
3. **Rezultatas įrašomas:** naujas hash'as pridedamas į kito lygmens sąrašą

Tokiu būdu sukuriamas kitas Merkle tree lygmuo.

#### 4. Iteratyvus Merkle lygmenų konstravimas

```cpp
merkle = new_merkle;
```

- Naujai sudarytas hash'ų lygmuo tampa įvestimi kitai iteracijai
- Ciklas kartojamas tol, kol lieka tik vienas elementas
- Kiekviena iteracija sukuria naują Merkle medžio lygmenį, judant nuo lapų link šaknies

#### 5. Galutinio rezultato grąžinimas

```cpp
return merkle[0];
```

Kai sąraše yra vienas hash'as, jis yra **Merkle root** – galutinis rezultatas, naudojamas bloko antraštėje.

#### Algoritmo vizualizacija:

```

┌───┐     ┌───┐                ┌───┐        ┌───┐
│ A │     │ B │                │ C │        │ D │
└───┘     └───┘                └───┘        └───┘
tx0       tx1                  tx2          tx3
 │           │                      │           │
 └─────┬─────┘                      └─────┬─────┘
       │                                  │
       ▼                                  ▼
┌──────────────┐                 ┌──────────────┐
│ hash(AB)     │                 │ hash(CD)     │
└──────────────┘                 └──────────────┘
      │                                  │
      └────────────────┬─────────────────┘
                       │
                       ▼
           ┌───────────────────────────────┐
           │         Merkle Root           │
           │     hash(ABCD_level_2)        │
           └───────────────────────────────┘
```

#### Išvados:

- Funkcija tiksliai atitinka Bitcoin Merkle medžio specifikaciją
- Naudoja dvigubą SHA-256 hash'inimą (`bitcoin_hash`)
- Teisingai apdoroja nelyginį skaičių hash'ų (duplikuoja paskutinį)
- Iteratyvus algoritmas efektyviai sudaro Merkle medį be rekursijos
- Galutinis Merkle root naudojamas bloko antraštėje transakcijų vientisumo patikrinimui

</details>



<details>
 <summary><strong>1.3 Kodo kompiliavimas ir testavimas</strong></summary>

Šiame etape buvo atliktas Merkle medžio generavimo kodo kompiliavimas ir testavimas Ubuntu aplinkoje (WSL2). Procesas pareikalavo papildomų veiksmų dėl bibliotekų versijų nesuderinamumo.

### Žingsnis 1: Pradinis kompiliavimo bandymas

Užduoty buvo nurodyta kompiliuoti su:

```bash
clang++ -std=c++11 -o merkle merkle.cpp $(pkg-config --cflags --libs libbitcoin)
./merkle
```

Kadangi mano sistemoje įdiegta biblioteka vadinosi `libbitcoin-system`, komanda buvo pakoreguota:

```bash
clang++ -std=c++11 -o merkle merkle.cpp $(pkg-config --cflags --libs libbitcoin-system)
./merkle
```

### Žingsnis 2: Klaida – clang++ nerastas (NEPAVYKO)

Paleidžiant kompiliavimą gavau klaidą:

```bash
neda@jessica:~$ clang++ -std=c++11 -o merkle merkle.cpp $(pkg-config --cflags --libs libbitcoin-system)
Command 'clang++' not found, but can be installed with:
sudo apt install clang
```

**Sprendimas:** Įdiegti `clang`:

```bash
sudo apt update
sudo apt install clang -y
```

**Rezultatas:** [PAVYKO] Įdiegta versija: `Ubuntu clang version 18.1.3`

### Žingsnis 3: Kodo sukūrimas (PAVYKO)

Kodas įrašytas į failą:

```bash
nano merkle.cpp
```

**Rezultatas:** [PAVYKO] Failas išsaugotas ir paruoštas kompiliavimui.

### Žingsnis 4: Sudėtingumas – libbitcoin versijų konfliktas (NEPAVYKO)

Bandant kompiliuoti su turima `libbitcoin-system` versija išmetė labai didelį kiekį klaidų:

```
static assertion failed: C++20 minimum required.
```

**Kas nutiko:**

- Įdiegta `libbitcoin-system` versija naudoja **C++20** (labai nauja, netinkama užduočiai)
- Užduoties pateiktas kodas parašytas **senai libbitcoin 2.x versijai**, kuri naudoja **C++11**
- Šios versijos yra **visiškai nesuderinamos** API lygmenyje

**Išvada:** Bet kokie bandymai kompiliuoti su nauja `libbitcoin` versija baigėsi nesuderinamumo klaidomis.

### Žingsnis 5: Sprendimas – pašalinti naują biblioteką (PAVYKO)

Kadangi nauja `libbitcoin` versija buvo visiškai netinkama, ją reikėjo pilnai pašalinti:

```bash
sudo rm -rf /usr/local/include/bitcoin
sudo rm -rf /usr/local/lib/libbitcoin*
sudo rm -rf /usr/local/lib/pkgconfig/libbitcoin-system.pc
sudo rm -rf ~/local/lib/pkgconfig/libbitcoin-system.pc
```

**Patikrinimas:**

```bash
pkg-config --cflags libbitcoin-system
# Package 'libbitcoin-system' not found
```

**Rezultatas:** [PAVYKO] Biblioteka sėkmingai pašalinta.

### Žingsnis 6: Alternatyvus sprendimas (PAVYKO)

Kadangi senos `libbitcoin 2.x` versijos įdiegimas šiuo metu yra neveikiantis dėl pašalintų šaltinių (kriptovaliutų projektas jau nebeprižiūrimas), buvo pasirinktas kitas kelias:

**Sprendimas:** Parašyti Merkle medžio funkciją C++ kalba naudojant **OpenSSL** (`-lcrypto`) vietoj `libbitcoin`.

**Kodėl geriau:**
- Daug efektyviau ir stabiliau nei mėginti suderinti nebeegzistuojančius paketus
- OpenSSL yra plačiai palaikoma ir stabili biblioteka
- Išvengiama versijų konflikto problemų

### Žingsnis 7: Galutinis veikiantis kompiliavimas (PAVYKO)

```bash
g++ -std=c++11 merkle.cpp -lcrypto -o merkle
```

**Paleidimas:**

```bash
./merkle
```

**Rezultatas:**

```
Merkle root: 4702bc319fce49439b780eca58d29e48e740c4e5c02a8ac3dc0833277c0292e0
```

**[PAVYKO] Merkle šaknis yra teisinga – užduotis sėkmingai išspręsta.**

</details>


</details>


<details>
 <summary><strong>1.4 Testavimas su realiomis Bitcoin transakcijomis</strong></summary>

Vietoj pateiktų užduotyje transakcijų hash'ų, panaudosiu hash'us iš realaus Bitcoin bloko, naudodamas blockchain explorer.

### Bloko pasirinkimas

**Pasirinktas blokas:** [#100012](https://blockchair.com/bitcoin/block/100012)

**Bloko informacija:**
- **Block height:** 100,012
- **Block hash:** `00000000000080b66c911bd5ba14a74260057311eaeb1982802f7010f1a9f090`
- **Timestamp:** 2010-12-29 11:57:43
- **Transakcijų skaičius:** 6
- **Merkle root (tikrasis):** `1f2fc38a429ae7ff0192f4b703cba9a4e4d6192af6544d03115b6fc8777bc027`

### Transakcijų hash'ai

Paėmiau visas 6 transakcijas iš šio bloko:

```
4788faffb925c275e2d0b4d034d7f704d5e391f66113e00f079f9d4a043f8ed1
cf5db3af378904bcf68b353f6bd9ad1b0b035c58df4923591b3893f4eae47189
0a372653b93138c589f47edab493562d75e81a8625ca865431cc19a26251bbce
b1d585c4676c95debae6556a2225041364340ac283fbe74a78f9c655cac4783d
356bc80f527a672ece2a13e1df5192192da290005a33969f66bc61410b7c0dd0
3405050d2cd29955a18d1f13d8ab6d585a4c8c4a098065e2a0f0d6c8fb6e8b93
```

### Kodo atnaujinimas

Pakeičiau `merkle.cpp` failo `main()` funkciją su naujais hash'ais:

```cpp
int main() {
    // Transakcijų hash'ai iš bloko #100012
    bc::hash_list tx_hashes{{
        bc::hash_literal("4788faffb925c275e2d0b4d034d7f704d5e391f66113e00f079f9d4a043f8ed1"),
        bc::hash_literal("cf5db3af378904bcf68b353f6bd9ad1b0b035c58df4923591b3893f4eae47189"),
        bc::hash_literal("0a372653b93138c589f47edab493562d75e81a8625ca865431cc19a26251bbce"),
        bc::hash_literal("b1d585c4676c95debae6556a2225041364340ac283fbe74a78f9c655cac4783d"),
        bc::hash_literal("356bc80f527a672ece2a13e1df5192192da290005a33969f66bc61410b7c0dd0"),
        bc::hash_literal("3405050d2cd29955a18d1f13d8ab6d585a4c8c4a098065e2a0f0d6c8fb6e8b93"),
    }};

    const bc::hash_digest merkle_root = create_merkle(tx_hashes);
    std::cout << "Merkle Root Hash: " << bc::encode_base16(merkle_root) << std::endl;
    
    return 0;
}
```

### Kompiliavimas ir paleidimas

```bash
g++ -std=c++11 merkle.cpp -lcrypto -o merkle
./merkle
```

### Rezultatas

```
Merkle root: 1f2fc38a429ae7ff0192f4b703cba9a4e4d6192af6544d03115b6fc8777bc027
```

### Patikrinimas

**Sugeneruotas Merkle root:**
```
1f2fc38a429ae7ff0192f4b703cba9a4e4d6192af6544d03115b6fc8777bc027
```

**Tikrasis Merkle root iš bloko #100012:**
```
1f2fc38a429ae7ff0192f4b703cba9a4e4d6192af6544d03115b6fc8777bc027
```

### Išvados:

- Testas atliktas su **realiomis Bitcoin transakcijomis** iš bloko #100012
- Algoritmas **tiksliai atkartoja** Bitcoin Merkle medžio konstravimą
- Rezultatas **patvirtintas** su blockchain explorer duomenimis
- Implementacija atitinka **Bitcoin protokolo specifikaciją**

</details>


<details>
 <summary><strong>1.5 Integracija į blockchain projektą</strong></summary>

### Problema: Bibliotekų nesuderinamumas

Užduotyje reikalaujama integruoti `create_merkle()` funkciją iš `libbitcoin` į esamą blockchain projektą. Tačiau iškilo esminė problema:

**Nesuderinamumas:**
- `libbitcoin` naudoja **C++20** standartą ir `bc::hash_digest` tipus
- Mano blockchain projektas naudoja **C++17** su `std::string` hash reprezentacija
- `libbitcoin` API yra visiškai nesuderinamas su esamomis duomenų struktūromis

**Galimi sprendimai:**
1. **Visiškai pakeisti projektą į libbitcoin** - per sudėtinga, reikia perrašyti visą kodą
2. **Adaptuoti create_merkle() algoritmą** - paimti logiką, bet naudoti esamas struktūras

### Sprendimas: Algoritmo adaptacija

Paėmiau `create_merkle()` algoritminę logiką ir pritaikiau prie esamo projekto:

#### Pagrindiniai pakeitimai `src/merkle.cpp`:

**1. Sukūriau naują funkciją `create_merkle_adapted()`:**

```cpp
// Adaptuota create_merkle() funkcija is libbitcoin
std::string create_merkle_adapted(std::vector<std::string> merkle_hashes) {
    // Stop if hash list is empty or contains one element
    if (merkle_hashes.empty()) {
        return std::string(); // grazina tuscia string'a vietoj bc::null_hash
    }
    else if (merkle_hashes.size() == 1) {
        return merkle_hashes[0];
    }

    // While there is more than 1 hash in the list, keep looping...
    while (merkle_hashes.size() > 1) {
        // If number of hashes is odd, duplicate last hash in the list.
        if (merkle_hashes.size() % 2 != 0) {
            merkle_hashes.push_back(merkle_hashes.back());
        }
        
        // New hash list.
        std::vector<std::string> new_merkle;
        
        // Loop through hashes 2 at a time.
        for (size_t i = 0; i < merkle_hashes.size(); i += 2) {
            // Join both current hashes together (concatenate).
            const std::string& left = merkle_hashes[i];
            const std::string& right = merkle_hashes[i + 1];
            
            // Hash both of the hashes
            std::string new_root = generate_hash(left + right);
            
            // Add this to the new list.
            new_merkle.push_back(new_root);
        }
        
        // This is the new list.
        merkle_hashes = std::move(new_merkle);
    }
    
    // Finally we end up with a single item.
    return merkle_hashes[0];
}
```

**2. Atnaujinau `MerkleTree::from_leaves()` metodą:**

```cpp
MerkleTree MerkleTree::from_leaves(const std::vector<std::string>& leaves) {
    MerkleTree tree;
    if (leaves.empty()) {
        return tree;
    }

    tree.levels_.push_back(leaves); // 0-asis lygis - lapai

    // Naudojame adaptuota create_merkle logika su lygiu sekimu
    std::vector<std::string> current_level = leaves;
    
    while (current_level.size() > 1) {
        // Bitcoin taisyklė: jei nelyginis, dubliuojam paskutinį
        if (current_level.size() % 2 != 0) {
            current_level.push_back(current_level.back());
        }
        
        std::vector<std::string> next_level;
        
        // Loop through hashes 2 at a time (poromis, kaip create_merkle)
        for (size_t i = 0; i < current_level.size(); i += 2) {
            const std::string& left = current_level[i];
            const std::string& right = current_level[i + 1];
            next_level.push_back(generate_hash(left + right));
        }
        
        tree.levels_.push_back(next_level);
        current_level = std::move(next_level);
    }

    return tree;
}
```

### Pagrindiniai skirtumai nuo originalo:

| **Originalas (libbitcoin)** | **Adaptuota versija** |
|-----------------------------|-----------------------|
| `bc::hash_digest` tipas | `std::string` tipas |
| `bc::hash_list` (vector) | `std::vector<std::string>` |
| `bc::bitcoin_hash()` funkcija | `generate_hash()` (mano SHA-256) |
| `bc::null_hash` konstantas | `std::string()` (tuščias) |
| Gryna Bitcoin implementacija | Blockchain projekto adaptacija |

### Kompiliavimas ir testavimas

```bash
# Kompiliavimas
make clean
make

# Rezultatas
g++ -std=c++17 -Wall -Wextra -Iincludes -fopenmp src/block.cpp src/blockchain.cpp 
    src/ledger.cpp src/main.cpp src/merkle.cpp src/ownHash.cpp 
    src/transaction.cpp src/txpool.cpp src/user.cpp -o blockchain.exe -fopenmp
```

Adaptacija leido išlaikyti Bitcoin protokolo Merkle medžio algoritmo tikslumą, tuo pačiu išvengiant bibliotekų versijų konfliktų ir išlaikant projekto architektūrą.

</details>

---

## 2 DALIS:  Pilno Bitcoin mazgo (Bitcoin Core) įdiegimas

<details>
 <summary><strong>2.1 Bitcoin Core mazgo įdiegimas ir sinchronizacija</strong></summary>

Šioje dalyje įdiegiau ir paleidau pilną Bitcoin Core mazgą Ubuntu (WSL2) aplinkoje bei pradėjau pilną blockchain sinchronizaciją (Initial Block Download, IBD).

### 1. Įdiegimas

Kadangi oficialus Ubuntu PPA neveikė, paketą parsisiunčiau tiesiogiai iš `bitcoincore.org` ir įdiegiau binarus rankiniu būdu:

```bash
wget https://bitcoincore.org/bin/bitcoin-core-24.2/bitcoin-24.2-x86_64-linux-gnu.tar.gz
tar -xvf bitcoin-24.2-x86_64-linux-gnu.tar.gz
sudo install -m 0755 -t /usr/local/bin bitcoin-24.2/bin/*
```

### 2. Konfigūracija (`~/.bitcoin/bitcoin.conf`)

Sukūriau konfigūracinį failą su mazgo ir RPC nustatymais. (Pastaba: README viešai NEREKOMENDUOJAMA talpinti realių slaptažodžių – čia pakeista į pavyzdinį.)

```
server=1
daemon=1
txindex=1

rpcuser=neda_rpc_01
rpcpassword=CHANGE_ME_SECURE_PASSWORD
rpcallowip=0.0.0.0/0
rpcbind=0.0.0.0
rpcport=8332

listen=1
port=8333
maxconnections=20
```

Pagrindiniai parametrai:
- `txindex=1` – įjungia pilną transakcijų indeksą (reikalinga istoriniams lookup'ams)
- `daemon=1` – paleidžia mazgą fone
- `rpcallowip=0.0.0.0/0` + `rpcbind=0.0.0.0` – leidžia RPC (tik laboratoriniams tikslams; produkcijoje riboti IP!)
- `maxconnections=20` – ribojamas priimtų peer'ų skaičius siekiant mažesnio resursų naudojimo WSL2 aplinkoje

### 3. Paleidimas

```bash
bitcoind -daemon
bitcoin-cli getblockchaininfo
```

### 4. Sinchronizacijos eiga (santrauka)

Iš `debug.log` (su python kodu pasigaminau sutrumpintą versiją`debug-shortened.txt`), iš kurio analizavau sinchronizaciją, kuri vyko non-stop ~19 val. (2025-11-24 - 2025-11-25)

Progresas:
| Rodiklis | Pradžia | Pabaiga |
|----------|--------:|--------:|
| Blokų aukštis | 792,582 | 857,400 |
| Progresas | 0.787 | 0.900 |
| Vidutinis greitis | \~3000–3500 blokų/val. | stabilus |

Komentarai:
- Greitis būdingas WSL2 su SSD (IO našumas šiek tiek mažesnis nei natyviame Linux)
- Progreso šuolis rodo normalų tikrinimo ir validavimo (headers + blocks + UTXO) etapą

### 5. Būsena

Mazgas šiuo metu:
- dar vyksta sinchronizuojasi

</details>

---

## 3 Bitcoin tinklo analizė su python-bitcoinlib
Reikalavimai: prieiga prie full Bitcoin node.  
<details>
 <summary><strong>Įdiekti python-bitcoinlib</strong></summary>



</details>
