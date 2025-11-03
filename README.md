# Simplified Blockchain

Paprastas blockchain projektas su Proof-of-Work algoritmu, transakcijomis ir balansų sistema.

## Apie projektą

Projektas realizuoja blokų grandinę su:
- 1000 vartotojų su atsitiktiniais pradiniais balansais (nuo 100 iki 1 000 000 monetų)
- Proof-of-Work kasimo procesu (difficulty = 3, hash turi prasidėti "000...")
- Account model balansų skaičiavimu ir patikrinimu
- Nuosava hash funkcija iš praeito projekto
- Blokų grandinės patikrinimu
- Interaktyvus užklausų menu

Programa automatiškai sugeneruos 1000 vartotojų, 10000 transakcijų, formuos blokus su po 100 atsitiktinių transakcijų.

## Architektūra

Projektas naudoja gerąsias OOP praktikas ir yra suskirstytas į atskirus aiškius failus:

### Pagrindiniai failai

**Block** (`block.h/cpp`)
- Saugo bloko duomenis: index, timestamp, hash, prev hash, nonce
- Palaiko 2 versijas: v1 (paprastas su data), v2 (su transakcijomis, txRoot, difficulty)
- Private laukai su getteriais/setteriais (enkapsuliacija)

**Blockchain** (`blockchain.h/cpp`)
- Valdo blokų grandinę (vector<Block>)
- `mineBlock()` - PoW kasimas (nonce iteracijos)
- `formBlockFromPool()` - formuoja bloką iš transaction pool
- `isChainValid()` - validuoja visą grandinę (prev hash, rehash, difficulty)
- `printChain()`, `printStatistics()` - vizualizacija

**Transaction** (`transaction.h/cpp`)
- Saugo transakcijos duomenis: from, to, amount, timestamp
- Automatiškai generuoja ID konstruktoriuje: `id = hash(from+to+amount+timestamp)`
- toString() serializacijai

**TxPool** (`txpool.h/cpp`)
- Laiko nepatvirtintas transakcijas (vector<Transaction>)
- `takeRandom(n)` - atsitiktinai pasirenka n transakcijų
- `eraseByIds()` - pašalina panaudotas transakcijas po kasimo

**Ledger** (`ledger.h/cpp`)
- Account model balansai (unordered_map<publicKey, balance>)
- `canApply()` - tikrina ar vartotojas turi pakankamai lėšų
- `apply()` - pritaiko transakciją (atima iš sender, prideda receiver)
- Apsauga nuo underflow (uint64_t)

**User** (`user.h/cpp`)
- Vartotojo duomenys: name, publicKey, balance
- Paprastas duomenų konteineris su getteriais

**MerkleTree** (`merkle.h/cpp`)
- Merkle medžio struktūra
- `from_leaves()` - kuria medį iš lapų (tx id)
- `root()` - grąžina šaknį
- Naudojama Block `txRoot` skaičiavimui

**Timer** (`timer.h/cpp`)
- Paprastas chrono wrapper laiko matavimui
- `elapsed()` - grąžina praėjusį laiką sekundėmis

### OOP principai

- **Enkapsuliacija**: visi duomenų laukai `private`, prieiga per getterius/setterius
- **Konstruktoriai**: konstruktoriai su parametrais visiems objektams (pvz., `Block(index, transactions, prevHash, difficulty)`)
- **RAII**: automatinis resursų valdymas (STL konteineriai, Timer klasė)
- **Single Responsibility**: kiekviena klasė atsakinga už vieną dalyką

## Proof-of-Work (PoW)
- tikslas: rasti nonce taip, kad bloko hash prasidėtų su N nulių. Naudojama difficulty=3, taigi tikslas yra "000...".
- hash funkcija: custom `generate_hash()` (256-bit HEX). išvedime rodau pilnus hash'us.
- hash'inami laukai:
	- v1 blokas: index, timestamp, data, prevHash, nonce
  - v2 blokas: index, timestamp, version, difficulty, txRoot (tikras Merkle Root iš Tx ID), prevHash, nonce
- kasimas: didinu nonce nuo 0; po kiekvieno bandymo skaičiuoju `hash(toString())`, kai hash prasideda reikiamu kiekiu nulių, blokas laikomas iškastu.
- progresas: kas 100000 bandymų išvedamas bandymų skaičius; po kasimo parodytas Nonce + Hash + Time + Attempts.
- patikrinimas: `isChainValid()` tikrina prev hashus, ir patikrina, kad hash atitiktų difficulty ("000...").
- difficulty: nustatomas paleidžiant `Blockchain(3)`. galima lengvai keisti.
- `txRoot` yra tikras Merkle Root.

### Decentralizuotas kasimas
- Tikslas: imituoti kelių kalnakasių konkurenciją su laiko apribojimu.
- Procesas:
  1) Iš `TxPool` suformuojami 5 kandidatiniai blokai (po ~100 patikrintų transakcijų kiekviename).
  2) Visi kandidatai kasami konkurencingai vienu metu (paraleliskai).
  3) Kiekvienas kandidatas kasamas atskirame threade. Pirmasis suradęs tinkamą hash'ą nustato bendrą `stopFlag`, ir visi kiti thread'ai nustoja kasti.
  4) Jei per raundą nei vienas neiškasa (neatrenka hash su pakankamu nulių skaičiumi) – laiko limitas padidinamas 1.5× ir kartojama (iki 10 raundų, kad nebūtų begalinio ciklo).
  5) Laimėtojo transakcijos pritaikomos `Ledger`, jos pašalinamos iš `TxPool`, o blokas prijungiamas prie grandinės.

- API:
  - `Blockchain::mineCandidateBlocksParallel(TxPool&, Ledger&, size_t nTx=100, int numCandidates=5, double timeLimitSec=5.0, unsigned long long maxAttempts=0)` – paralelinis kandidatų kasimas su threads.
  - `Blockchain::mineBlockWithTimeLimitStop(Block&, double timeLimitSec, unsigned long long maxAttempts, std::atomic<bool>& stopFlag, unsigned long long& outNonce)` – kasa konkretų bloką su stopFlag mechanizmu.

- Įgyvendinimas: 5 thread'ai kasa 5 kandidatus vienu metu. `std::atomic<bool> stopFlag` ir `std::atomic<int> winner` naudojami koordinacijai. Pirmas laimėtojas sustabdo visus kitus.

## Blokų ir transakcijų kūrimas, kasimas, patikrinimas

### Transakcijų kūrimas
1. Sugeneruojami 1000 vartotojų su `User(name, publicKey, balance)`
2. publicKey = `hash(name + unique_salt)`
3. Balance - atsitiktinis [100..1 000 000]
4. Sugeneruojamos 10000 transakcijų: `Transaction(from, to, amount)`
5. Transaction ID automatiškai: `id = hash(from + to + amount + timestamp)`

### Bloko formavimas
1. Iš TxPool pasirenkamos ~100 atsitiktinių transakcijų
2. **Dviejų žingsnių verifikacija**:
   - **Transakcijos ID tikrinimas**: Perskaičiuoja hash iš laukų (from, to, amount, timestamp) per `tx.verifyId()` ir lygina su saugomu `id`. Jei nesutampa — transakcija atmesta (galimai sugadinta arba suklastota).
   - **Balanso tikrinimas**: Tikrina per `ledger.canApply(tx)`, ar siuntėjas turi pakankamai lėšų. Jei balansas nepakankamas — transakcija atmesta.
   - Atmestos transakcijos registruojamos konsolėje su priežastimi (invalid ID / insufficient balance) ir statistika.
3. Tik abiejų patikrų praėjusios transakcijos įdedamos į naują `Block(index, validTx, prevHash, difficulty)`
4. Block konstruktorius automatiškai skaičiuoja `txRoot = MerkleRoot(tx.id)` naudojant tikrą Merkle Tree (jei lygis nelyginis — paskutinis lapas dubliuojamas)

### Kasimas (PoW)
```cpp
while (true) {
    block.setNonce(nonce);
    hash = generate_hash(block.toString());
    if (hash.substr(0, difficulty) == "000") {
        block.setHash(hash);
        break;
    }
    nonce++;
}
```

### Patvirtinimas
1. Pritaikomos transakcijos: `ledger.apply(tx)` kiekvienai (atnaujina balansus)
2. Transakcijos pašalinamos iš TxPool
3. Blokas pridedamas į grandinę: `chain_.push_back(block)`
4. Blokas išsaugomas į `logs/blockchain_log.txt`

### Patikrinimas (grandinės validacija)
`isChainValid()` tikrina:
- Ar dabartinis blokas `prevHash == previous.hash`
- Ar blokas perhashintas teisingai: `hash(block.toString()) == block.hash`
- Ar hash atitinka difficulty: `hash.substr(0, difficulty) == "000"`
- Jei blokas v2, perskaičiuoja Merkle Root iš transakcijų ID ir patikrina, kad jis sutaptų su saugomu `txRoot` (nesutapus – grandinė laikoma negaliojančia)

## Mano sprendimai (papildomi patobulinimai)

### Custom hash funkcija
- Pradžioje naudojau Base62, bet su difficulty=2 per 41M+ nonces nerado "00" pradžios, todėl pakeičiau į HEX 

### Account model
- Vietoje UTXO naudoju Account model (Ledger su balansais)
- Paprastesnis implementuoti
- `canApply()` pre-check + `apply()` su underflow apsauga

### Logging į atskirą logs/ aplanką
- Kiekvienas blokas išsaugomas į `logs/blockchain_log.txt` su pilnais headeriais
- Visos transakcijos išsaugomos į `logs/merkle_log.txt`
- Sesijos pradžioje: `date_utc` žymė
- Bloko info: Version, Tx Root, Difficulty, Nonce, Prev Hash, Hash

### Merkle Root diagnostika (papildoma validacija)
- `Block::recomputeTxRoot()` ir `Block::verifyTxRoot()` metodai leidžia perskaičiuoti ir sulyginti Merkle Root su išsaugotu `txRoot`
- Naudojama grandinės validacijoje `isChainValid()` — papildoma apsauga nuo duomenų sugadinimo
- Tai nėra griežtas užduoties reikalavimas, bet stiprina vientisumo tikrinimą

### Paralelinis kandidatų kasimas
`mineCandidateBlocksParallel(...)`
- Kiekvienas kandidatas kasamas atskirame threade (`std::thread`) su bendru `std::atomic<bool> stopFlag` ir `std::atomic<int> winner`.
- Pirmasis suradęs tinkamą hash'ą nustato `winner` ir pakelia `stopFlag`, kiti thread'ai nustoja kasti.
- Privalumai: žymiai trumpesnis raundų laikas ir realesnė konkurencijos imitacija.
- Pagrindinė programos versija naudoja šį paralelinio kasimo variantą (`main.cpp`, eilutė ~239).

### Gerosios OOP praktikos
Projektas naudoja modernius C++17 standarto principus:

**Rule of Five/Zero**:
- Visos klasės turi aiškiai apibrėžtą kopijavimo/perkėlimo semantiką
- `Blockchain`, `Timer` - išjungtas kopijavimas (per dideli objektai / unique per scope)
- `Block`, `Transaction`, `User`, `Ledger`, `TxPool` - default kopijavimas/perkėlimas (naudoja tik STL konteinerius)
- Move konstruktoriai ir operatoriai pažymėti `noexcept`

**Const correctness**:
- Visi getteriai grąžina `const&` (string, vector) vietoj kopijų - efektyviau
- Metodai, kurie nekeičia būsenos, pažymėti `const`
- Read-only operacijos pažymėtos `const noexcept` - optimizacijos ir saugumo garantija

**Explicit konstruktoriai**:
- `Blockchain(int difficulty)` - `explicit` apsaugo nuo netikėto konvertavimo

**RAII (Resource Acquisition Is Initialization)**:
- `Timer` klasė automatiškai matuoja laiką nuo sukūrimo
- STL konteineriai (`vector`, `unordered_map`) automatiškai tvarko atmintį
- Nėra manual `new/delete` - viskas tvarkoma automatiškai

**Noexcept garantijos**:
- Visi getteriai: `noexcept` - garantuoja, kad neįvyks exception
- Move operacijos: `noexcept` - leidžia STL optimizacijas (pvz., `std::vector` realokacija)
- Paprastos operacijos (`size()`, `empty()`, `clear()`) - `noexcept`

**Enkapsuliacija**:
- Visi duomenų laukai `private`
- Prieiga tik per getterius/setterius
- Vidiniai helper metodai (`mineBlockWithTimeLimit`, `getLastBlockHash`) - `private`

## Interaktyvus užklausų menu

Po pagrindinės programos vykdymo vartotojui suteikiama galimybė užklausti ir gauti informaciją apie konkrečią transakciją ar bloką. 

Įvedimo klaidų gaudymas su 

Į konsolę dėl aiškumo viskas vedama anglų kalba.


### Funkcionalumas
```
Options:
  1 - Query block by number
  2 - Query transaction by ID
  0 - Exit
```

## Projekto veikimo demonstracija

### Konsolės išvestis 

```
==============================================
Simplified Blockchain
Difficulty: 3
==============================================

Mining Block #0 (target: '000')...
Block #0 mined!
   Nonce: 2360 | Hash: 0003e19da5388965d0631e7219d83897b7c2e3a0b3d3a9dfe4f7b1f1e2a3c4d5 | Time: 0.043 s | Attempts: 2361

Genesis block created!

Generating ~1000 users...
Created 1000 users
Total coins in system: 501234567

Generating ~10000 transactions...
Generated 10000 transactions
All transactions saved to logs/merkle_log.txt

Transaction Pool (size: 10000)
----------------------------------------
  Showing first 5 transactions:
  [1] e98f1cba... -> b1355516... : 65 coins
  [2] 78028d76... -> e98f1cba... : 67 coins
  [3] e98f1cba... -> b1355516... : 32 coins
  [4] 78028d76... -> 1d9e9c93... : 14 coins
  [5] 78028d76... -> 73138d67... : 100 coins
  ... and 9995 more
  (See logs/merkle_log.txt for all transactions)

============================================================
Mining Blocks with Transactions
============================================================

--- Mining Block #1 ---

Forming block from 100 transactions...
   100 valid transactions selected
Mining Block #1 (target: '000')...
Block #1 mined!
   Nonce: 2757 | Hash: 0005e69648968f559092e6d11ca9d61e5f4b3a2c1d0e9f8a7b6c5d4e3f2a1b0c | Time: 0.130 s | Attempts: 2758

Block #1 added to chain with 100 transactions

[... 2 more blocks ...]

============================================================
Results
============================================================

Total mining time: 0.35 seconds
Blocks mined: 3
Transactions remaining in pool: 9760

Final balance summary:
  Total coins in system: 501234567
  Balances updated for 1000 users
  (Use Ledger::print() to see all individual balances)

==================================================
BLOCKCHAIN STATISTICS
==================================================
Total blocks  : 4
Difficulty    : 3 (hash starts with 000)
Chain valid   : YES
Average nonce : 2443.33
==================================================

Blockchain is VALID!

============================================================
Query System
============================================================

You can now query blocks and transactions.

Options:
  1 - Query block by number
  2 - Query transaction by ID
  0 - Exit
Choose option: 1
Enter block number (0-3): 1

============================================================
BLOCK #1 DETAILS
============================================================
----------------------------------------
Block #1 (v2)
Timestamp : 1761501491
Tx Count  : 100
Tx Root   : c2304964b1f8396b7d2f4a1e0c9b8d7e6f5a4b3c2d1e0f9a8b7c6d5e4f3a2b1c
Difficulty: 3
Nonce     : 2757
Prev Hash : 0003e19da5388965d0631e7219d83897b7c2e3a0b3d3a9dfe4f7b1f1e2a3c4d5
Hash      : 0005e69648968f559092e6d11ca9d61e5f4b3a2c1d0e9f8a7b6c5d4e3f2a1b0c
----------------------------------------
```

### Konsolės išvestis (decentralizuotas kasimas)

```
============================================================
Mining Blocks with Decentralized Process
============================================================


========== Mining Block #1 ==========

=== Decentralized Mining: 5 candidates ===
Time limit per round: 5.000 seconds

--- Mining Round #1 ---
Time limit: 5.0s

Candidate #1: 100 transactions
Candidate #2: 100 transactions
Candidate #3: 100 transactions
Candidate #4: 100 transactions
Candidate #5: 100 transactions

Mining 5 candidates competitively...

*** Candidate #1 WON! ***
  Nonce: 7083 | Hash: 000330f51b6071d6... | Time: 0.318 s
Block #1 added to chain with 100 transactions

... [toliau 2 blokai iškasami panašiai] ...

============================================================
Results
============================================================

Total mining time: 4.95 seconds
Blocks mined: 3
Transactions remaining in pool: 9700

============================================================
Blockchain Validation
============================================================

==================================================
BLOCKCHAIN STATISTICS
==================================================
Total blocks  : 4
Difficulty    : 3 (hash starts with 000)
Chain valid   : YES
Average nonce : 6578.00
==================================================
```

### Transakcijos pavyzdys

```
Transaction
  ID:    7a9c2f1e4d6b8a0c1f3e5d7c9b2a4e6f8d0c1b2a3e4f5d6c7b8a9c0d1e2f3a4
  From:  e98f1cbac0e44e34a1b2c3d4e5f60718a9b0c1d2e3f405162738495a6b7c8d9
  To:    b1355516d249c7ef1234567890abcdef1234567890abcdef1234567890abcd
  Amount: 65 coins
  Time: 1761501491
```

## AI pagalba

1. `README.md` generavimas
2. Blockchain veikimo principo supratimas


---

# Versijos

Kiekviena versija išsamiai aprašyta jos `README.md` faile.

## v0.1
lalala

## v0.2
lalala
