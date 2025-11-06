# Simplified Blockchain

Blockchain projektas su Proof-of-Work algoritmu, transakcijomis ir **UTXO modeliu**.

## Apie projektą

Projektas realizuoja blokų grandinę su:
- 1000 vartotojų su atsitiktiniais pradiniais UTXO (nuo 100 iki 1 000 000 monetų)
- Proof-of-Work kasimo procesu (difficulty = 3, hash turi prasidėti "000...")
- **UTXO model** balansų skaičiavimu ir patikrinimu
- Nuosava hash funkcija iš praeito projekto
- Blokų grandinės patikrinimu
- Fiksuotu transakcijų mokesčiu (implicit per UTXO input-output skirtumą)
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
- **v0.2**: Saugo inputs (TxInput: prevTxId, outputIndex) ir outputs (TxOutput: receiver, amount)
- Automatiškai generuoja ID konstruktoriuje: `id = hash(inputs+outputs)`
- Compatibility getters: getFrom(), getTo(), getAmount() senam kodui

**TxPool** (`txpool.h/cpp`)
- Laiko nepatvirtintas transakcijas (vector<Transaction>)
- `takeRandom(n)` - atsitiktinai pasirenka n transakcijų
- `eraseByIds()` - pašalina panaudotas transakcijas po kasimo

**Ledger** (`ledger.h/cpp`)
- **v0.2**: UTXO model (unordered_map<"txid:index", TxOutput>)
- Saugo neišleistus UTXO: `"txid:index" → TxOutput(receiver, amount)`
- `getBalance(address)` - susumuoja visus neišleistus UTXO, priklausančius adresui
- `canApply()` - tikrina ar visi transakcijos input UTXO egzistuoja ir jų bendra suma pakanka padengti output'us
- `apply()` - pašalina panaudotus input UTXO (išleidžia) ir sukuria naujus output UTXO gavėjams
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
  - blokas: index, timestamp, version, difficulty, txRoot (tikras Merkle Root iš Tx ID), prevHash, nonce
- kasimas: didinu nonce nuo 0; po kiekvieno bandymo skaičiuoju `hash(toString())`, kai hash prasideda reikiamu kiekiu nulių, blokas laikomas iškastu.
- progresas: kas 100000 bandymų išvedamas bandymų skaičius; po kasimo parodytas Nonce + Hash + Time + Attempts.
- patikrinimas: `isChainValid()` tikrina prev hashus, ir patikrina, kad hash atitiktų difficulty ("000...").
- difficulty: nustatomas paleidžiant `Blockchain(3)`. galima lengvai keisti.
- `txRoot` yra tikras Merkle Root.

### Decentralizuotas kasimas
- Tikslas: imituoti 5 kandidatinių blokų kasimą su laiko apribojimu.
- Procesas:
1. Iš `TxPool` suformuojami 5 kandidatiniai blokai (po 100 patikrintų transakcijų kiekviename).
2. Visi kandidatai kasami konkurencingai vienu metu (paraleliskai).
3. Kiekvienas kandidatas kasamas atskirame threade. Pirmasis suradęs tinkamą hash'ą nustato bendrą `stopFlag`, ir visi kiti thread'ai nustoja kasti.
4. Jei per raundą nei vienas neiškasa (neatrenka hash su pakankamu nulių skaičiumi) – laiko limitas padidinamas 1.5× ir kartojama (iki 10 raundų, kad nebūtų begalinio ciklo).
5. Laimėtojo transakcijos pritaikomos `Ledger`, jos pašalinamos iš `TxPool`, o blokas prijungiamas prie grandinės.

- API:
  - `Blockchain::mineCandidateBlocksParallel(TxPool&, Ledger&, size_t nTx=100, int numCandidates=5, double timeLimitSec=5.0, unsigned long long maxAttempts=0)` – paralelinis kandidatų kasimas su threads.
  - `Blockchain::mineBlockWithTimeLimitStop(Block&, double timeLimitSec, unsigned long long maxAttempts, std::atomic<bool>& stopFlag, unsigned long long& outNonce)` – kasa konkretų bloką su stopFlag mechanizmu.
  ```      


- Įgyvendinimas: 5 thread'ai kasa 5 kandidatus vienu metu. `std::atomic<bool> stopFlag` ir `std::atomic<int> winner` naudojami koordinacijai. Pirmas laimėtojas sustabdo visus kitus.

## PSEUDO-KODAI

Žemiau – sutrumpintos, svarbiausios vietos kaip pseudo-kodas, be implementacijos detalių.

### Ledger (UTXO) – tikrinimas ir pritaikymas

```pseudo
// Raktas UTXO žemėlapiui
key(txid, index) -> txid + ":" + to_string(index)

canApply(tx):
  sumInputs  := 0
  sumOutputs := 0
  for inp in tx.inputs:
    utxo := UTXO[key(inp.prevTxId, inp.outputIndex)]
    if utxo not exists: return false  // UTXO neegzistuoja arba jau išleistas
    sumInputs += utxo.amount
  for out in tx.outputs:
    sumOutputs += out.amount
  return sumInputs >= sumOutputs      // input UTXO suma turi padengti output'us (implicit fee = inputs - outputs)

apply(tx):
  assert canApply(tx)
  // Pašalinti (išleisti) input UTXO
  for inp in tx.inputs:
    erase UTXO[key(inp.prevTxId, inp.outputIndex)]
  // Sukurti naujus output UTXO gavėjams
  for i, out in enumerate(tx.outputs):
    UTXO[key(tx.id, i)] = out
```

### Transaction ID ir coinbase atpažinimas

```pseudo
// ID = hash(visi įėjimai + visi išėjimai)
computeTxId(tx):
  buf := []
  for inp in tx.inputs:
    buf.append(inp.prevTxId)
    buf.append(to_bytes(inp.outputIndex))
  for out in tx.outputs:
    buf.append(out.receiver)
    buf.append(to_bytes(out.amount))
  return HASH(buf)

isCoinbase(tx):
  return len(tx.inputs) == 1 and tx.inputs[0].prevTxId == "coinbase"
```

### Coinbase (unikalus) kūrimas kandidato bloke

```pseudo
newIndex      := chain.length()        // bloko indeksas
coinbaseInput := TxInput("coinbase", newIndex)
reward        := blockReward + sum(fees of selectedTx)
coinbaseOut   := TxOutput(minerAddress, reward)
coinbaseTx    := Transaction([coinbaseInput], [coinbaseOut])

// Įdėti į pradžią, kad fee priklausytų šiam blokui
txSet := [coinbaseTx] + selectedTx
```

### Paralelinis decentralizuotas kasimas (5 kandidatai)

```pseudo
winner   := -1
stopFlag := false
candidates := [ buildCandidate(nTx) for i in 1..numCandidates ]

for round in 1..maxRounds:
  launch threads:
    for i in 1..numCandidates:
      thread i:
        ok, nonce, hash := mineWithTimeLimit(candidates[i], timeLimit, stopFlag)
        if ok and not stopFlag:
          winner   := i
          stopFlag := true
  join all threads
  if winner != -1: break
  timeLimit := timeLimit * 1.5

if winner != -1:
  ids := [ tx.id for tx in candidates[winner].txs ]
  for tx in candidates[winner].txs: ledger.apply(tx)
  pool.eraseByIds(ids)
  pool.removeInvalid(ledger, TX_FEE)
  chain.push(candidates[winner])
```

### Merkle Root skaičiavimas iš Tx ID

```pseudo
merkleRoot(leaves):               // leaves = [tx.id]
  if leaves.empty(): return ZERO
  cur := leaves
  while len(cur) > 1:
    nxt := []
    for j in range(0, len(cur), 2):
      L := cur[j]
      R := cur[j+1] if j+1 < len(cur) else L   // dubliavimas jei nelyginis
      nxt.push( HASH(L + R) )
    cur := nxt
  return cur[0]
```

### TxPool – nebegaliojančių šalinimas

```pseudo
removeInvalid(ledger, fee):
  toRemove   := set()
  usedInputs := set()
  for tx in pool:
    if not ledger.canApply(tx):
      toRemove.add(tx.id)
      continue
    for inp in tx.inputs:
      k := key(inp.prevTxId, inp.outputIndex)
      if k in usedInputs:         // konfliktuoja dėl to paties UTXO
        toRemove.add(tx.id)
      else:
        usedInputs.add(k)
  eraseByIds(toRemove)
  return size(toRemove)
```

### Užklausa pagal transakcijos ID prefiksą

```pseudo
findTxByPrefix(prefix):
  for block in chain:
    for tx in block.txs:
      if tx.id.startsWith(prefix):
        return (tx, block.index)
  return null
```



## Blokų ir transakcijų kūrimas, kasimas, patikrinimas

### Transakcijų kūrimas
1. Sugeneruojami 1000 vartotojų su `User(name, publicKey, balance)`
2. publicKey = `hash(name + unique_salt)`
3. Kiekvienam vartotojui sukuriamas pradinis UTXO ledger'yje su atsitiktiniu balansu [100..1 000 000]
4. Sugeneruojamos 10000 transakcijų: kiekviena transakcija referuoja vartotojo UTXO kaip input ir sukuria output'us gavėjui (+ change output siuntėjui)
5. Transaction ID automatiškai: `id = hash(inputs + outputs)`

### Bloko formavimas
1. Iš TxPool pasirenkamos 100 atsitiktinių (patvirtintų) transakcijų
2. **Dviejų žingsnių verifikacija**:
   - **Transakcijos ID tikrinimas**: Perskaičiuoja hash iš laukų (inputs + outputs) per `tx.verifyId()` ir lygina su saugomu `id`. Jei nesutampa — transakcija atmesta (galimai sugadinta arba suklastota).
   - **UTXO validacija**: `ledger.canApplyWithFee(tx, TX_FEE)` tikrina, ar visi transakcijos input UTXO egzistuoja ledger'yje ir ar jų bendra suma pakanka padengti output'us + mokestį. Jei input UTXO suma nepakankama arba UTXO neegzistuoja — transakcija atmesta.
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
1. Pritaikomos transakcijos: `ledger.apply(tx)` kiekvienai – pašalina panaudotus input UTXO ir sukuria naujus output UTXO
2. Transakcijos pašalinamos iš TxPool
3. Blokas pridedamas į grandinę: `chain_.push_back(block)`
4. Blokas išsaugomas į `logs/...` katalogą.

### Patikrinimas (grandinės validacija)
`isChainValid()` tikrina:
- Ar dabartinis blokas `prevHash == previous.hash`
- Ar blokas perhashintas teisingai: `hash(block.toString()) == block.hash`
- Ar hash atitinka difficulty: `hash.substr(0, difficulty) == "000"`
- Perskaičiuoja Merkle Root iš transakcijų ID ir patikrina, kad jis sutaptų su saugomu `txRoot` (nesutapus – grandinė laikoma negaliojančia)

## Mano sprendimai (papildomi patobulinimai)

### Custom hash funkcija
- Pradžioje naudojau Base62, bet su difficulty=2 per 41M+ nonces nerado "00" pradžios, todėl pakeičiau į HEX 


### Logging / eksportas į logs/
- JSON eksportas į `logs/block_#.json` įjungiamas `EXPORT_LOGS=1`.
- Saugojamos kiekvieno bloko transakcijos ir Merkle Tree.
<div style="display:flex; gap:10px; align-items:flex-start;">
  <img src="https://github.com/user-attachments/assets/1e16cc57-56b8-4def-a67c-d3a930e74989" style="width:26%;"/>
  <img src="https://github.com/user-attachments/assets/87049a90-df47-47e5-bba1-1f5dcbb2cfae" style="width:11%;"/>
  <img src="https://github.com/user-attachments/assets/64dcff85-d3b3-4943-993e-4ce8f744282d" style="width:55%;"/>
</div>

### Merkle Root diagnostika (papildoma validacija)
- Užklausiant konkretų bloką galima atspausdinti Merkle medžio struktūrą (pasirenkama interaktyviai).

### Paralelinis kandidatų kasimas
`mineCandidateBlocksParallel(...)`
- Kiekvienas kandidatas kasamas atskirame threade (`std::thread`) su bendru `std::atomic<bool> stopFlag` ir `std::atomic<int> winner`.
- Pirmasis suradęs tinkamą hash'ą nustato `winner` ir pakelia `stopFlag`, kiti thread'ai nustoja kasti.
- Privalumai: žymiai trumpesnis raundų laikas ir realesnė konkurencijos imitacija.
- Pagrindinė programos versija naudoja šį paralelinio kasimo variantą (`main.cpp`).


## Interaktyvus užklausų menu

Po pagrindinės programos vykdymo vartotojui suteikiama galimybė užklausti ir gauti informaciją apie konkrečią transakciją ar bloką. 
- Interaktyvus meniu palaiko paiešką pagal ID arba prefiksą (case-insensitive).
- Ieškoma tiek pool’e (nepatvirtintos), tiek blokuose (patvirtintos); radus kelis – rodoma pasirinkimų lentelė.
- Į konsolę dėl aiškumo viskas vedama anglų kalba.


### Funkcionalumas
```
Options:
  1 - Query block by number
  2 - Query transaction by ID/prefix
  0 - Exit
```

### Transakcijų mokestis (fees)
- Fiksuotas mokestis: `TX_FEE = 1` moneta už kiekvieną transakciją.
- Tikrinimas: `Ledger::canApplyWithFee(tx, fee)` – tikrina, kad input UTXO suma padengtų output'ų sumą + mokestį.
- Pritaikymas: Mokestis surenkamas per coinbase transakciją – bloko coinbase output'as apima `BLOCK_REWARD + totalFees`.
- Rezultatuose rodoma eilutė: `Miner fees collected (MINER_FEE): X coins`.
- Mokesčiai tik perskirstomi (nekuria naujų monetų). Bendrą pasiūlą didina tik `BLOCK_REWARD` per coinbase.

### Automatinis difficulty keitimas

Difficulty automatiškai didėja, jei blokas iškasamas per greitai (<0.2s), ir mažėja, jei per lėtai (>0.4s). Ribos: minimum 1, maksimum 10. Pranešimas rodomas konsolėje kiekvieną kartą, kai difficulty pasikeičia.

```
mineBlock(block):
    start timer
    nonce := 0
    while true:
        block.setNonce(nonce)
        hash := generate_hash(block.toString())
        if hash.startsWith("0" * difficulty):
            block.setHash(hash)
            elapsed := timer.elapsed()
            // --- Auto difficulty adjustment ---
            TARGET_BLOCK_TIME := 0.2   // sekundės
            MIN_DIFFICULTY := 1
            MAX_DIFFICULTY := 10
            if elapsed < TARGET_BLOCK_TIME:
                if difficulty < MAX_DIFFICULTY:
                    difficulty := difficulty + 1
                    print("[Auto] Difficulty increased to", difficulty, "(block mined in", elapsed, "s)")
            else if elapsed > TARGET_BLOCK_TIME * 2:
                if difficulty > MIN_DIFFICULTY:
                    difficulty := difficulty - 1
                    print("[Auto] Difficulty decreased to", difficulty, "(block mined in", elapsed, "s)")
            return
        nonce := nonce + 1
```


## Projekto veikimo demonstracija

### Konsolės išvestis 

```
============================================================
Mining Blocks with Decentralized Process
============================================================


========== Mining Block #1 ==========

=== Parallel Decentralized Mining: 5 candidates ===
Time limit per round: 5.000 seconds

--- Mining Round #1 ---
Time limit: 5.0s

Candidate #1: 100 transactions
Candidate #2: 100 transactions
Candidate #3: 100 transactions
Candidate #4: 100 transactions
Candidate #5: 100 transactions

Mining 5 candidates in parallel...

*** Candidate #1 WON! ***
  Nonce: 7083 | Hash: 000330f51b6071d6... | Time: 0.318 s
Block #1 added to chain with 100 transactions

... [toliau dar 3 blokai iškasami panašiai] ...

============================================================
Results
============================================================

Total mining time: 0.63 seconds
Blocks mined: 5
Transactions remaining in pool: 4697

  Miner (fees + rewards) [MINER_FEE]: 745 coins

============================================================
Blockchain Validation
============================================================

==================================================
BLOCKCHAIN STATISTICS
==================================================
Total blocks  : 6
Difficulty    : 3 (hash starts with 000)
Chain valid   : YES
Average nonce : 630.20
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

### Bloko pavyzdys
```
Block #1 (parallel)
Timestamp : 1761501491
Tx Count  : 100
Tx Root   : c2304964b1f8396b7d2f4a1e0c9b8d7e6f5a4b3c2d1e0f9a8b7c6d5e4f3a2b1c
Difficulty: 3
Nonce     : 2757
Prev Hash : 0003e19da5388965d0631e7219d83897b7c2e3a0b3d3a9dfe4f7b1f1e2a3c4d5
Hash      : 0005e69648968f559092e6d11ca9d61e5f4b3a2c1d0e9f8a7b6c5d4e3f2a1b0c
```

## AI pagalba 

- Bendras blockchain veikimo principo supratimas
- Patarimai dėl projektavimo
- OOP gerųjų praktikų patarimai
- Išvedimo į konsolę formavimas
- Pagalba su Merkle tree įgyvendinimu
- JSON eksportas
- Transakcijų mokesčio integracija
- Transakcijos paieška pagal prefix'ą
- Pagalba su paralelinio kasimo įgyvendinimu
- UTXO realizavimas
- Pagalba su kodo klaidom
- Kodo peržiūra ir patarimai
- `README.md` formavimas

---

# Versijos

Kiekviena versija išsamiai aprašyta jos `README.md` faile.

## v0.1

- Proof-of-Work kasimas su pastovia difficulty=3 (hash pradžia „000")
- Blokų formatas: v1 (paprasti duomenys) ir v2 (transakcijos, Difficulty, tikras Merkle Root)
- Pritaikyta nuosava 256-bit HEX hash funkcija (iš praeito projekto)
- ~1000 vartotojų ir ~10000 transakcijų generavimas
- Transaction Pool ir blokų formavimas iš ~100 transakcijų
- Ledger su UTXO modeliu (`canApply`, `apply`) – saugo neišleistus UTXO, validuoja input sumas
- Grandinės validacija: `prevHash`, perhashinimas, difficulty ir Merkle Root tikrinimas
- Log'ai: `logs/blockchain_log.txt` (blokai) ir `logs/merkle_log.txt` (transakcijos)
- Interaktyvus užklausų meniu (blokų ir transakcijų paieška)


## v0.2
- Paralelinis (decentralizuotas) kasimas su 5 kandidatais:
  - kiekvienas kandidatas kasamas atskirame threade, naudojami `stopFlag` ir `winner`
  - pirmas suradęs sprendimą sustabdo kitus; blokai konsolėje žymimi „Block #N (parallel)“
- Transakcijų mokestis: fiksuotas `TX_FEE = 1` moneta, kredituojamas į `MINER_FEE` sąskaitą
- JSON eksportas visos grandinės į `logs/blockchain_export.json`
- Smulkūs patobulinimai ir pataisymai:
  - užfiksuojama laimėtojo statistika (genesis + laimėję kandidatai)
  - sutvarkyti pagalbiniai kasimo metodai (`mineBlockWithTimeLimit`, `mineBlockWithLimits`)
  - aiškesnė konsolės išvestis su išsamia kasimo statistika 

  ## v0.2.1
  - Automatinis difficulty didinimas ir mažinimas
