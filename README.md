# Simplified Blockchain v0.1

Paprastas blockchain projektas su Proof-of-Work algoritmu, transakcijomis ir balansu sistema.

## Apie projektą

v0.1 versija realizuoja centralizuotą blokų grandinę su:
- ~1000 vartotojų, sugeneruotais su atsitiktiniais balansais (100-1 000 000 monetomis)
- ~10 000 transakcijomis tarp vartotojų
- Proof-of-Work kasimo procesu (difficulty = 3, hash turi prasidėti "000...")
- Account model balansų skaičiavimu ir validacija
- Custom 256-bit maišos funkcija (HEX formatu)
- Blokų grandinės validacija

Tikslas: sukurti minimalią, bet pilnavertę blockchain sistemą, kuri demonstruoja pagrindines koncepcijas (PoW, grandinę, transakcijas, validaciją).

Programa automatiškai sugeneruos 1000 vartotojų, 10000 transakcijų, ir kas blokus su ~100 transakcijų iki kol baseinas ištuštėja.

## Architektura

Projektas naudoja OOP principus ir suskirstytas į aiškias klases:

### Pagrindinės klasės

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
- Merkle medžio struktūra (paruošta v0.2)
- `from_leaves()` - kuria medį iš lapų (tx id)
- `root()` - grąžina šaknį
- v0.1 dar nenaudojamas Block txRoot skaičiavime

### OOP principai

- **Enkapsuliacija**: visi duomenų laukai `private`, prieiga per getterius/setterius
- **Konstruktoriai**: parametrizuoti konstruktoriai visiems objektams (pvz., `Block(index, transactions, prevHash, difficulty)`)
- **Const correctness**: getteriai pažymėti `const`, metodai nemodifikuojantys būsenos - `const`
- **RAII**: automatinis resursų valdymas (STL konteineriai, Timer klasė)
- **Single Responsibility**: kiekviena klasė atsakinga už vieną dalyką

## Proof-of-Work (PoW)
- tikslas: rasti nonce taip, kad bloko hash prasidėtų su N nulių. v0.1 difficulty=3, taigi tikslas yra "000...".
- hash funkcija: custom `generate_hash()` (256-bit HEX). išvedime rodau pilnus hash'us.
- hash'inami laukai:
	- v1 blokas: index, timestamp, data, prevHash, nonce
	- v2 blokas: index, timestamp, version, difficulty, txRoot (v0.1 paprastas visų tx id hash), prevHash, nonce
- kasimas: didinu nonce nuo 0; po kiekvieno bandymo skaičiuoju hash(toString()). kai hash prasideda reikiamu kiekiu nulių, blokas laikomas iškastu.
- progresas: kas 100000 bandymų išvedamas bandymų skaičius; po kasimo parodytas Nonce | Hash | Time | Attempts.
- validacija: `isChainValid()` tikrina prev hash nuorodas, perhashuoja bloko turinį ir patikrina, kad hash atitiktų difficulty ("000...").
- difficulty: nustatomas paleidžiant `Blockchain(3)`. galima pakeisti į didesnį/mažesnį.
- pastaba v0.1: `txRoot` nėra tikras Merkle Root; naudojamas paprastas visų tx id sujungimo hash. tikras Merkle bus v0.2.

## Blokų ir transakcijų kūrimas, kasimas, validacija

### Transakcijų kūrimas
1. Sugeneruojami 1000 vartotojų su `User(name, publicKey, balance)`
2. publicKey = `hash(name + unique_salt)`
3. balance - atsitiktinis [100..1 000 000]
4. Sugeneruojamos 10000 transakcijų: `Transaction(from, to, amount)`
5. Transaction ID automatiškai: `id = hash(from + to + amount + timestamp)`

### Bloko formavimas
1. Iš TxPool pasirenkamos ~100 atsitiktinių transakcijų
2. Kiekviena validuojama per `Ledger::canApply()` (ar sender turi pakankamai)
3. Valid transakcijos įdedamos į naują `Block(index, validTx, prevHash, difficulty)`
4. Block konstruktorius automatiškai skaičiuoja `txRoot = hash(concat(tx.id))`

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
4. Blokas išsaugomas į `blockchain_log.txt`

### Validacija
`isChainValid()` tikrina:
- Ar dabartinis blokas `prevHash == previous.hash`
- Ar blokas perhashintas teisingai: `hash(block.toString()) == block.hash`
- Ar hash atitinka difficulty: `hash.substr(0, difficulty) == "000"`

## Sprendimai

### Custom hash funkcija
- Pradžioje naudojau Base62, bet su difficulty=2 per 41M+ nonces nerado "00" pradžios, todėl pakeičiau į HEX 

### Account model
- Vietoje UTXO naudoju Account model (Ledger su balansais)
- Paprastesnis implementuoti v0.1
- `canApply()` pre-check + `apply()` su underflow apsauga

### Logging
- Kiekvienas blokas išsaugomas į `blockchain_log.txt` su pilnais headeriais
- Sesijos pradžioje: `date_utc` žymė
- Bloko info: Version, Tx Root, Difficulty, Nonce, Prev Hash, Hash

### MerkleTree pasiruošimas
- Jau sukurta `MerkleTree` klasė su `from_leaves()` ir `root()`
- v0.1 nenaudojama Block txRoot (naudojamas paprastas hash)
- v0.2 tiesiog pakeisiu `txRoot = MerkleTree::from_leaves(txIds).root()`

## Projekto veikimo demonstracija

### Konsolės išvestis (pavyzdys)

```
==============================================
Simplified Blockchain v0.1
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
(Showing first 5 for brevity)

Transaction Pool (size: 10000)
----------------------------------------
  [1] e98f1cba... -> b1355516... : 65 coins
  [2] 78028d76... -> e98f1cba... : 67 coins
  [3] e98f1cba... -> b1355516... : 32 coins
  [4] 78028d76... -> 1d9e9c93... : 14 coins
  [5] 78028d76... -> 73138d67... : 100 coins
  ... and 9995 more

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

[... kartojasi ~100 bloku ...]

==================================================
BLOCKCHAIN STATISTICS
==================================================
Total blocks  : 101
Difficulty    : 3 (hash starts with 000)
Chain valid   : YES
Average nonce : 2443.33
==================================================
```

### Bloko išklotinė

```
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

### Transakcijos pavyzdys

```
Transaction
  ID:    7a9c2f1e4d6b8a0c1f3e5d7c9b2a4e6f8d0c1b2a3e4f5d6c7b8a9c0d1e2f3a4
  From:  e98f1cbac0e44e34a1b2c3d4e5f60718a9b0c1d2e3f405162738495a6b7c8d9
  To:    b1355516d249c7ef1234567890abcdef1234567890abcdef1234567890abcd
  Amount: 65 coins
  Time: 1761501491
```

## Ateities planai (v0.2)

- **Tikras Merkle Root**: vietoje paprasto hash naudosiu MerkleTree.root()
- **Transakcijų verifikacija**: TxID hash tikrinimas, griežtesnis balansų tikrinimas
- **Decentralizuotas kasimas**: 5 kandidatiniai blokai, ribota laiko (5s) arba bandymų kiekis
- **Papildoma validacija**: pilna grandinės ir transakcijų verifikacija

---

**Versija**: v0.1  
**Data**: 2025-10-28  
**Branch**: `v0.1`