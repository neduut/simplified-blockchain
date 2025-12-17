# Paralelizmas ir UTXO sistema projekte

## 1. UTXO (Unspent Transaction Output) sistema

### Kas yra UTXO?
UTXO modelis yra Bitcoin naudojamas būdas sekti pinigų balansus. Vietoj to, kad saugotume kiekvieno vartotojo balansą (kaip banke), UTXO modelis saugo **neišleistus transakcijų išvestis**.

**Pagrindinė idėja:**
- Kiekviena transakcija turi **inputs** (iš kur ateina pinigai) ir **outputs** (kur jie eina)
- Input'ai nurodo ankstesnių transakcijų outputs, kurie dar nebuvo išleisti
- Output'ai sukuria naujus UTXO, kuriuos galima panaudoti būsimose transakcijose

### Kur implementuota?

#### 1.1. Duomenų struktūros (`includes/transaction.h`)

```cpp
// === UTXO_MODEL_STRUCTURES ===
// TxInput - nurodo ankstesnės transakcijos output'ą
struct TxInput {
    std::string prevTxId;   // ankstesnės transakcijos ID
    int outputIndex;        // kuris output (nes TX gali turėti kelis)
};

// TxOutput - sukuria naują UTXO
struct TxOutput {
    std::string receiver;   // gavėjo public key
    uint64_t amount;        // suma
};

// Transaction klasė naudoja UTXO modelį
class Transaction {
private:
    std::vector<TxInput> inputs_;   // iš kur ateina pinigai
    std::vector<TxOutput> outputs_; // kur jie eina
    // ...
};
```

**Pavyzdys:**
```
Alice turi UTXO: TX_123, output #0, 100 BTC
Nori siųsti Bob 60 BTC

Nauja transakcija:
  Inputs:  [TX_123:0]              // Išleidžia Alice UTXO
  Outputs: [Bob: 60, Alice: 40]   // Sukuria du naujus UTXO
                                   // (40 = grąža)
```

#### 1.2. Ledger - UTXO valdymas (`includes/ledger.h`, `src/ledger.cpp`)

**Pagrindinis principas:** Ledger saugo tik **neišleistus** outputs (UTXO set).

```cpp
// === UTXO_LEDGER_STORAGE ===
class Ledger {
private:
    // UTXO saugojimas: "txid:outputIndex" -> TxOutput
    std::unordered_map<std::string, TxOutput> utxos_;
    
public:
    // Prideda naują UTXO
    void addUTXO(const std::string& txid, int index, const TxOutput& output);
    
    // Patikrina ar UTXO egzistuoja ir neišleistas
    bool hasUTXO(const std::string& txid, int index) const;
    
    // "Išleidžia" UTXO (pašalina iš set'o)
    void spendUTXO(const std::string& txid, int index);
    
    // Balanso skaičiavimas iš UTXO
    uint64_t getBalance(const std::string& publicKey) const {
        uint64_t total = 0;
        for (const auto& pair : utxos_) {
            if (pair.second.receiver == publicKey) {
                total += pair.second.amount;  // Sumuoja visus UTXO
            }
        }
        return total;
    }
};
```

#### 1.3. Transakcijos validacija (`src/ledger.cpp`)

**Dviejų žingsnių validacija:**

```cpp
// === UTXO_VALIDATION ===
bool Ledger::canApplyWithFee(const Transaction& tx, uint64_t fee) const {
    if (tx.isCoinbase()) return true;  // Coinbase visada validus
    
    // ŽINGSNIS 1: Patikrina ar visi input'ai egzistuoja UTXO set'e
    uint64_t inputSum = 0;
    for (const auto& input : tx.getInputs()) {
        if (!hasUTXO(input.prevTxId, input.outputIndex)) {
            return false;  // UTXO jau išleistas arba neegzistuoja
        }
        TxOutput utxo = getUTXO(input.prevTxId, input.outputIndex);
        inputSum += utxo.amount;
    }
    
    // ŽINGSNIS 2: Patikrina ar pakanka pinigų (inputs >= outputs + fee)
    uint64_t outputSum = 0;
    for (const auto& output : tx.getOutputs()) {
        outputSum += output.amount;
    }
    return inputSum >= (outputSum + fee);
}
```

#### 1.4. Transakcijos pritaikymas (`src/ledger.cpp`)

```cpp
// === UTXO_APPLICATION ===
bool Ledger::apply(const Transaction& tx) {
    // ŽINGSNIS 1: Pašalina input'us iš UTXO set (išleidžia)
    if (!tx.isCoinbase()) {
        for (const auto& input : tx.getInputs()) {
            spendUTXO(input.prevTxId, input.outputIndex);
        }
    }
    
    // ŽINGSNIS 2: Prideda output'us į UTXO set (sukuria naujus)
    for (size_t i = 0; i < tx.getOutputs().size(); ++i) {
        addUTXO(tx.getId(), static_cast<int>(i), tx.getOutputs()[i]);
    }
    return true;
}
```

#### 1.5. Naudojimas blockchain (`src/blockchain.cpp`)

**Validacija prieš kasimą (eilutės ~565-605):**
```cpp
// Patikrina ar transakcijos input'ai egzistuoja ir nebuvo išleisti
if (ledger.canApplyWithFee(tx, TX_FEE)) {
    validTx.push_back(tx);
    // ...
}
```

**Pritaikymas po kasimo (eilutės ~710-713):**

```cpp
// === PARALLEL_WINNER_APPLICATION ===
// Pritaiko visas laimėtojo bloko transakcijas
const auto& wonTxs = candidateTxSets[winIdx];
for (const Transaction& tx : wonTxs) {
    ledger.apply(tx);  // Atnaujina UTXO set
}
```

---

## 2. Paralelizmas (OpenMP)

### Kas yra paralelizmas?
Paralelinis kasimas imituoja decentralizuotą Bitcoin tinklą, kur **keli maineriai vienu metu** konkuruoja kas pirmas suras tinkamą nonce. Vietoj to, kad kiekvienas blokas būtų kasinėjamas eilės tvarka, keli thread'ai kasa skirtingus kandidatinius blokus **vienu metu**.

### Kur implementuota?

#### 2.1. Funkcija (`src/blockchain.cpp`, eilutės ~488-745)

```cpp
// === PARALLEL_MINING_START ===
// Paralelinis kandidatų kasimas su OpenMP
bool Blockchain::mineCandidateBlocksParallel(
    TxPool& pool, 
    Ledger& ledger, 
    size_t nTx,              // transakcijų skaičius bloke
    int numCandidates,       // kiek kandidatų (= thread'ų skaičius)
    double timeLimitSec,     // laiko limitas
    unsigned long long attemptsLimit  // bandymų limitas
) {
    // ... kandidatų formavimas ...
    
    // ===== PARALELINIS KASIMAS =====
    std::atomic<bool> stopFlag{false};  // Shared flag tarp thread'ų
    int winIdx = -1;                     // Laimėtojo indeksas
    
    // OpenMP paralelinis ciklas
    #pragma omp parallel for shared(stopFlag, winIdx, candidates, nonces, times)
    for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
        // Jei kitas thread laimėjo, sustabdo šį thread
        if (stopFlag.load(std::memory_order_relaxed)) continue;
        
        // Kasa savo kandidatą
        bool ok = mineBlockWithTimeLimitStop(
            candidates[i], 
            currentTimeLimit, 
            currentAttemptsLimit, 
            n, 
            stopFlag
        );
        
        if (ok) {
            // KRITINĖ SEKCIJA - tik vienas thread vienu metu
            #pragma omp critical
            {
                if (winIdx == -1) {  // Dar nėra laimėtojo?
                    winIdx = i;      // Šis thread tampa laimėtoju
                    stopFlag.store(true, std::memory_order_relaxed);
                }
            }
        }
    }
    
    // Pritaiko laimėtojo bloką
    if (winIdx >= 0) {
        // ... ledger.apply(), pool cleanup, chain update ...
    }
}
```

#### 2.2. Kaip veikia?

**Žingsnis po žingsnio:**

1. **Kandidatų formavimas (eilutės ~530-635):**
   - Sukuriami N kandidatiniai blokai
   - Kiekvienas turi skirtingas transakcijas (imituoja skirtingus mainerius)
   - SVARBU: Tos pačios TX nepateks į kelis kandidatus (`usedTxIds` set)

2. **Paralelinis kasimas (eilutės ~663-695):**
   ```cpp
   #pragma omp parallel for
   ```
   - OpenMP sukuria N thread'ų (pvz., 5)
   - Kiekvienas thread kasa savo kandidatą nepriklausomai
   - Visi thread'ai dirba VIENU METU (lygiagrečiai)

3. **Laimėtojo nustatymas:**
   ```cpp
   if (stopFlag.load(...)) continue;  // Tikrina ar kas nors laimėjo
   ```
   - Pirmas thread, kuris suranda tinkamą nonce, nustato `stopFlag = true`
   - Visi kiti thread'ai mato `stopFlag` ir sustoja
   
4. **Kritinė sekcija:**
   
   ```cpp
   // === PARALLEL_CRITICAL_SECTION ===
   #pragma omp critical
   {
       if (winIdx == -1) {  // Thread-safe patikrinimas
           winIdx = i;
           stopFlag.store(true, ...);
       }
   }
   ```
   - Tik VIENAS thread vienu metu gali būti čia
   - Užtikrina, kad tik pirmas sėkmingai iškanavęs thread bus laimėtojas

#### 2.3. Naudojimas (`src/main.cpp`, eilutės ~397)

```cpp
// Kasa bloką su 5 konkuruojančiais thread'ais
blockchain.mineCandidateBlocksParallel(
    pool,       // transaction pool
    ledger,     // UTXO ledger
    100,        // ~100 TX per bloką
    5,          // 5 kandidatai (= 5 thread'ai)
    5.0,        // 5 sekundžių limitas
    0           // be bandymų limito
);
```



## 3. Kaip tai veikia kartu?

### Pilnas scenarijus:

```
1. Transaction Pool formuojamas su UTXO transakcijomis
   └─ Kiekviena TX turi inputs (ankstesnių UTXO) ir outputs (naujus UTXO)

2. Paralelinis kasimas prasideda:
   ├─ Thread 1: formuoja Candidate #1 su TX[1-100]
   ├─ Thread 2: formuoja Candidate #2 su TX[101-200]
   ├─ Thread 3: formuoja Candidate #3 su TX[201-300]
   ├─ Thread 4: formuoja Candidate #4 su TX[301-400]
   └─ Thread 5: formuoja Candidate #5 su TX[401-500]

3. UTXO validacija kiekvienam kandidatui:
   ├─ Patikrina ar input'ai egzistuoja ledger UTXO set'e
   ├─ Patikrina ar nėra double-spending (tas pats UTXO du kartus)
   └─ Patikrina ar pakanka balanso (inputs >= outputs + fee)

4. Visi thread'ai VIENU METU kasa savo kandidatus:
   ├─ Thread 1: nonce=0, 1, 2, 3... (kasa Candidate #1)
   ├─ Thread 2: nonce=0, 1, 2, 3... (kasa Candidate #2)
   ├─ Thread 3: nonce=0, 1, 2, 3... (kasa Candidate #3)
   ├─ Thread 4: nonce=0, 1, 2, 3... (kasa Candidate #4)
   └─ Thread 5: nonce=0, 1, 2, 3... (kasa Candidate #5)

5. Thread 3 suranda tinkamą nonce pirmasis!
   └─ Nustato stopFlag = true → visi kiti thread'ai sustoja

6. Thread 3 blokas pridedamas į blockchain:
   ├─ UTXO atnaujinimas:
   │  ├─ Pašalina visus input'us iš UTXO set (išleidžia)
   │  └─ Prideda visus output'us į UTXO set (sukuria naujus)
   ├─ Pool cleanup: pašalina panaudotas TX
   └─ Blockchain: chain_.push_back(candidate #3)
```

---

## 4. Raktiniai failai ir eilutės

### UTXO sistema:
| Failas | CTRL+F | Aprašymas |
|--------|-----------------|-----------|
| `includes/transaction.h` | `UTXO_MODEL_STRUCTURES` | TxInput, TxOutput struktūros |
| `includes/ledger.h` | `UTXO_LEDGER_STORAGE` | Ledger klasė su UTXO metodais |
| `src/ledger.cpp` | `UTXO_VALIDATION` | UTXO validacija (canApplyWithFee) |
| `src/ledger.cpp` | `UTXO_APPLICATION` | UTXO pritaikymas (apply) |

### Paralelizmas:
| Failas | CTRL+F | Aprašymas |
|--------|-----------------|-----------|
| `src/blockchain.cpp` | `PARALLEL_MINING_START` | mineCandidateBlocksParallel() funkcija |
| `src/blockchain.cpp` | `PARALLEL_OMP_LOOP` | OpenMP paralelinis ciklas |
| `src/blockchain.cpp` | `PARALLEL_CRITICAL_SECTION` | Kritinė sekcija (thread safety) |
| `src/blockchain.cpp` | `PARALLEL_WINNER_APPLICATION` | Laimėtojo bloko pritaikymas |

---

## 5. Demonstracija

1. **UTXO struktūros** (`includes/transaction.h`):
   - **Ctrl+F:** `UTXO_MODEL_STRUCTURES`
   - Paaiškinti TxInput (prevTxId, outputIndex)
   - Paaiškinti TxOutput (receiver, amount)
   - Pavyzdys: "Alice siunčia Bob 60 BTC, gauna 40 grąžą"

2. **Ledger UTXO set** (`includes/ledger.h`, `src/ledger.cpp`):
   - **Ctrl+F:** `UTXO_LEDGER_STORAGE`
   - Parodyti `utxos_` map struktūrą ("txid:index" → output)
   - **Ctrl+F:** `UTXO_VALIDATION`
   - Paaiškinti `canApplyWithFee()` - kaip validuoja
   - **Ctrl+F:** `UTXO_APPLICATION`
   - Paaiškinti `apply()` - kaip atnaujina UTXO set

3. **Paralelinis kasimas** (`src/blockchain.cpp`):
   - **Ctrl+F:** `PARALLEL_MINING_START`
   - Parodyti funkcijos pradžią
   - **Ctrl+F:** `PARALLEL_OMP_LOOP`
   - Parodyti `#pragma omp parallel for`
   - **Ctrl+F:** `PARALLEL_CRITICAL_SECTION`
   - Parodyti `#pragma omp critical` - thread safety
   - Paaiškinti `stopFlag` - kaip thread'ai komunikuoja

4. **Integruotas veikimas**:
   - **Ctrl+F:** `PARALLEL_WINNER_APPLICATION`
   - Parodyti kaip laimėtojas pritaikomas į ledger
   - Pool cleanup su `removeInvalid()`
   - Paaiškinti `canApplyWithFee()` - kaip validuoja
   - Paaiškinti `apply()` - kaip atnaujina UTXO set

3. **Paralelinis kasimas** (`src/blockchain.cpp`):
   - Parodyti `#pragma omp parallel for` (eilutė 669)
   - Paaiškinti `stopFlag` - kaip thread'ai komunikuoja
   - Parodyti `#pragma omp critical` - thread safety

4. **Integruotas veikimas**:
   - **Ctrl+F:** `PARALLEL_WINNER_APPLICATION`
   - Parodyti kaip laimėtojas pritaikomas į ledger
   - Pool cleanup su `removeInvalid()`



