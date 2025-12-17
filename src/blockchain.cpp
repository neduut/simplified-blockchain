#include "blockchain.h"
#include "ownHash.h"
#include "timer.h"
#include "merkle.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <atomic>
#include <unordered_set>
#include <set>
#include <omp.h>
#ifdef _WIN32
#include <direct.h>
#endif

// Transaction fee settings
static constexpr uint64_t TX_FEE = 1; // flat fee per transaction
static const std::string FEE_COLLECTOR = "MINER_FEE"; // miner account key
// Block subsidy (newly minted coins for each mined block)
static constexpr uint64_t BLOCK_REWARD = 50;
// Special sender label for coinbase transactions
static const std::string COINBASE_SENDER = "SYSTEM";

Blockchain::Blockchain(int difficulty)
    : difficulty_(difficulty) 
{
    std::cout << "==============================================\n";
    std::cout << "Simplified Blockchain\n";
    std::cout << "Difficulty: " << difficulty << "\n";
    std::cout << "==============================================\n\n";

    // Genesis blokas
    Block genesis(0, "Genesis Block", "0");
    mineBlock(genesis);
    chain_.push_back(genesis);
    saveToFile(genesis);
    std::cout << "Genesis block created!\n\n";
}

void Blockchain::mineBlock(Block& block) {
    std::string target(difficulty_, '0');
    std::string hash;
    unsigned long long nonce = 0;

    std::cout << "Mining Block #" << block.getIndex() << "...\n";

    Timer timer;

    while (true) {
        block.setNonce(nonce);
        std::string blockData = block.toString();
        hash = generate_hash(blockData);

        if (hash.substr(0, difficulty_) == target) {
            block.setHash(hash);
            double elapsed = timer.elapsed();
            
            miningHistory_.push_back({elapsed, nonce + 1, block.getIndex()});

            std::cout << "Block #" << block.getIndex() << " mined.\n\n";
            // auto difficulty
            constexpr double TARGET_BLOCK_TIME = 0.2; // seconds
            constexpr int MIN_DIFFICULTY = 1;
            constexpr int MAX_DIFFICULTY = 10;
            if (elapsed < TARGET_BLOCK_TIME) {
                if (difficulty_ < MAX_DIFFICULTY) {
                    difficulty_++;
                    std::cout << "[Auto] Difficulty increased to " << difficulty_ << " (block mined in " << elapsed << "s)\n";
                }
            } else if (elapsed > TARGET_BLOCK_TIME * 2) {
                if (difficulty_ > MIN_DIFFICULTY) {
                    difficulty_--;
                    std::cout << "[Auto] Difficulty decreased to " << difficulty_ << " (block mined in " << elapsed << "s)\n";
                }
            }
            return;
        }

        nonce++;
    }
}

// kasimas su laiko limitu
bool Blockchain::mineBlockWithTimeLimit(Block& block, double timeLimitSec, unsigned long long& finalNonce) {
    std::string target(difficulty_, '0');
    std::string hash;
    unsigned long long nonce = 0;
    
    Timer timer;
    
    while (timer.elapsed() < timeLimitSec) {
        block.setNonce(nonce);
        std::string blockData = block.toString();
        hash = generate_hash(blockData);
        
        if (hash.substr(0, difficulty_) == target) {
            block.setHash(hash);
            finalNonce = nonce;
            return true; // success
        }
        
        nonce++;
    }
    
    finalNonce = nonce;
    return false; // time limit exceeded
}

// kasimas su bandymu limitu (arba laiko, jei timeLimitSec > 0)
bool Blockchain::mineBlockWithLimits(Block& block, double timeLimitSec, unsigned long long attemptsLimit, unsigned long long& finalNonce) {
    std::string target(difficulty_, '0');
    std::string hash;
    unsigned long long nonce = 0;

    Timer timer;

    while (true) {
        // tikrina laiko limita (jei nustatytas)
        if (timeLimitSec > 0.0 && timer.elapsed() >= timeLimitSec) {
            finalNonce = nonce;
            return false; // Time limit exceeded
        }
        // tikrinam bandymu limita (jei nustatytas)
        if (attemptsLimit > 0 && nonce >= attemptsLimit) {
            finalNonce = nonce;
            return false; // attempts limit exceeded
        }

        block.setNonce(nonce);
        std::string blockData = block.toString();
        hash = generate_hash(blockData);

        if (hash.substr(0, difficulty_) == target) {
            block.setHash(hash);
            finalNonce = nonce;
            return true; // success
        }
        nonce++;
    }
}

// parallel mining 
bool Blockchain::mineBlockWithTimeLimitStop(Block& block, double timeLimitSec, unsigned long long attemptsLimit,
                                              unsigned long long& finalNonce, std::atomic<bool>& stopFlag) {
    std::string target(difficulty_, '0');
    std::string hash;
    unsigned long long nonce = 0;

    Timer timer;
    bool useTime = (timeLimitSec > 0.0);
    bool useAttempts = (attemptsLimit > 0);
    
    while (!stopFlag.load(std::memory_order_relaxed)) {
        // tikrina laiko limita
        if (useTime && timer.elapsed() >= timeLimitSec) break;
        // tikrina bandymu limita
        if (useAttempts && nonce >= attemptsLimit) break;
        
        block.setNonce(nonce);
        std::string blockData = block.toString();
        hash = generate_hash(blockData);
        if (hash.substr(0, difficulty_) == target) {
            block.setHash(hash);
            finalNonce = nonce;
            return true;
        }
        nonce++;
    }
    finalNonce = nonce;
    return false;
}

void Blockchain::addBlock(const std::string& data) {
    int newIndex = static_cast<int>(chain_.size());
    std::string prevHash = getLastBlockHash();

    Block newBlock(newIndex, data, prevHash);
    mineBlock(newBlock);
    chain_.push_back(newBlock);
    saveToFile(newBlock);
}

// blokas is transaction pool
bool Blockchain::formBlockFromPool(TxPool& pool, Ledger& ledger, size_t nTx) {
    if (pool.empty()) {
        std::cout << "Transaction pool is empty!\n";
        return false;
    }
    
    // pasiimam atsitiktines transakcijas
    std::vector<Transaction> selectedTx = pool.takeRandom(nTx);
    
    if (selectedTx.empty()) {
        return false;
    }
    
    std::cout << "\nForming block from " << selectedTx.size() << " transactions...\n";
    
    // dvieju zingsniu verifikacija - ID hash + balansas
    std::vector<Transaction> validTx;
    std::vector<std::string> toRemoveIds;
    int invalidId = 0;
    int insufficientBalance = 0;
    
    for (const auto& tx : selectedTx) {
        // patikrina transakcijos id
        if (!tx.verifyId()) {
            std::cout << "   Invalid TX " << tx.getId().substr(0, 8) 
                     << "... (ID hash mismatch)\n";
            toRemoveIds.push_back(tx.getId());
            invalidId++;
            continue;
        }
        
        // tikrina balansa
        if (ledger.canApplyWithFee(tx, TX_FEE)) {
            validTx.push_back(tx);
            toRemoveIds.push_back(tx.getId());
        } else {
            std::cout << "   Invalid TX " << tx.getId().substr(0, 8) 
                     << "... (insufficient balance incl. fee)\n";
            toRemoveIds.push_back(tx.getId());
            insufficientBalance++;
        }
    }
    
    if (invalidId > 0 || insufficientBalance > 0) {
        std::cout << "   Rejected: " << invalidId << " invalid ID(s), " 
                 << insufficientBalance << " insufficient balance(s)\n";
    }
    
    if (validTx.empty()) {
        std::cout << "No valid transactions to mine!\n";
        pool.eraseByIds(toRemoveIds);
        return false;
    }
    
    std::cout << "   " << validTx.size() << " valid transactions selected\n";
    
    // blokas su transakcijom
    int newIndex = static_cast<int>(chain_.size());
    std::string prevHash = getLastBlockHash();
    
    Block newBlock(newIndex, validTx, prevHash, difficulty_);
    
    // kasam
    mineBlock(newBlock);
    
    // pritaikom ledger (UTXO: fees are implicit in transaction outputs)
    for (const auto& tx : validTx) {
        ledger.apply(tx);
    }
    
    // istrinam is pool
    pool.eraseByIds(toRemoveIds);
    
    // pridedam i grandine
    chain_.push_back(newBlock);
    
    // issaugom
    saveToFile(newBlock);
    
    std::cout << "Block #" << newIndex << " added to chain with " 
             << validTx.size() << " transactions\n\n";
    
    return true;
}

// decentralizuotas kasimas su kandidatiniais blokais
bool Blockchain::mineCandidateBlocks(TxPool& pool, Ledger& ledger, size_t nTx, 
                                     int numCandidates, double timeLimitSec,
                                     unsigned long long attemptsLimit) {
    if (pool.empty()) {
        std::cout << "Transaction pool is empty!\n";
        return false;
    }
    
    std::cout << "\n=== Decentralized Mining: " << numCandidates << " candidates ===\n";
    if (timeLimitSec > 0.0 && attemptsLimit > 0) {
        std::cout << "Limits: " << timeLimitSec << " seconds OR " << attemptsLimit << " attempts\n\n";
    } else if (timeLimitSec > 0.0) {
        std::cout << "Time limit per round: " << timeLimitSec << " seconds\n\n";
    } else if (attemptsLimit > 0) {
        std::cout << "Attempts limit per round: " << attemptsLimit << " attempts\n\n";
    }
    
    int round = 1;
    double currentTimeLimit = timeLimitSec;
    unsigned long long currentAttemptsLimit = attemptsLimit;
    
    while (true) {
        std::cout << "--- Mining Round #" << round << " ---\n";
        if (currentTimeLimit > 0.0 && currentAttemptsLimit > 0) {
            std::cout << "Limits: " << std::fixed << std::setprecision(1) 
                     << currentTimeLimit << "s OR " << currentAttemptsLimit << " attempts\n\n";
        } else if (currentTimeLimit > 0.0) {
            std::cout << "Time limit: " << std::fixed << std::setprecision(1) 
                     << currentTimeLimit << "s\n\n";
        } else if (currentAttemptsLimit > 0) {
            std::cout << "Attempts limit: " << currentAttemptsLimit << "\n\n";
        }
        
        // sukuria kandidatinius blokus
        std::vector<Block> candidates;
        std::vector<std::vector<Transaction>> candidateTxSets;
        std::vector<std::vector<std::string>> candidateRemoveIds;
        
        // Ensure no TX is reused across candidates: track already-picked TX IDs
        std::unordered_set<std::string> usedTxIds;
        
        // Track validation statistics
        int totalSampled = 0;
        int invalidIdCount = 0;
        int insufficientBalanceCount = 0;
        int duplicateCount = 0;
        
        for (int i = 0; i < numCandidates; ++i) {
            // pasiima atsitiktines transakcijas
            std::vector<Transaction> selectedTx = pool.takeRandom(nTx * 2); // oversample to account for filtering
            if (selectedTx.empty()) break;
            
            //verifikacija
            std::vector<Transaction> validTx;
            std::vector<std::string> toRemoveValidIds; // remove only confirmed tx
            // For UTXO: track which inputs are already used in this candidate
            std::set<std::string> usedInputsInCandidate;
            
            for (const auto& tx : selectedTx) {
                totalSampled++;
                // skip if already used by another candidate
                if (usedTxIds.count(tx.getId()) > 0) {
                    duplicateCount++;
                    continue;
                }
                if (!tx.verifyId()) {
                    invalidIdCount++;
                    continue; // discard invalid IDs but don't erase from pool here
                }
                
                // For pure UTXO: check if transaction's inputs are valid and not already used
                bool inputsAlreadyUsed = false;
                for (const auto& input : tx.getInputs()) {
                    std::string inputKey = input.prevTxId + ":" + std::to_string(input.outputIndex);
                    if (usedInputsInCandidate.count(inputKey) > 0) {
                        inputsAlreadyUsed = true;
                        break;
                    }
                }
                
                if (inputsAlreadyUsed) {
                    duplicateCount++;
                    continue;
                }
                
                // Validate using ledger's UTXO validation
                if (ledger.canApplyWithFee(tx, TX_FEE)) {
                    validTx.push_back(tx);
                    toRemoveValidIds.push_back(tx.getId());
                    // Mark inputs as used
                    for (const auto& input : tx.getInputs()) {
                        std::string inputKey = input.prevTxId + ":" + std::to_string(input.outputIndex);
                        usedInputsInCandidate.insert(inputKey);
                    }
                    usedTxIds.insert(tx.getId());
                    if (validTx.size() >= nTx - 1) break; // limit to nTx-1 to account for coinbase
                } else {
                    insufficientBalanceCount++;
                }
            }
            
            if (validTx.empty()) continue;

            // Compose block transactions: prepend coinbase(txReward + fees)
            uint64_t totalFees = static_cast<uint64_t>(validTx.size()) * TX_FEE;
            uint64_t coinbaseAmt = BLOCK_REWARD + totalFees;
            
            // Get block index for coinbase uniqueness and block creation
            int newIndex = static_cast<int>(chain_.size());
            
            // Create coinbase as UTXO transaction
            // Coinbase has special "input" with block index to ensure uniqueness
            std::vector<TxInput> coinbaseInputs;
            coinbaseInputs.push_back(TxInput("coinbase", newIndex)); // Unique per block
            std::vector<TxOutput> coinbaseOutputs;
            coinbaseOutputs.push_back(TxOutput(FEE_COLLECTOR, coinbaseAmt));
            Transaction coinbase(coinbaseInputs, coinbaseOutputs);
            
            std::vector<Transaction> blockTxs;
            blockTxs.reserve(validTx.size() + 1);
            blockTxs.push_back(coinbase);
            for (const auto& t : validTx) blockTxs.push_back(t);
            
            // sukuria kandidatini bloka
            std::string prevHash = getLastBlockHash();
            Block candidate(newIndex, blockTxs, prevHash, difficulty_);
            
            candidates.push_back(candidate);
            candidateTxSets.push_back(blockTxs);
            candidateRemoveIds.push_back(toRemoveValidIds);
            
            // compact console: don't print candidate details
        }
        
        // Print validation statistics
        if (invalidIdCount > 0 || insufficientBalanceCount > 0 || duplicateCount > 0) {
            std::cout << "Validation: sampled " << totalSampled << " tx; ";
            if (invalidIdCount > 0) std::cout << invalidIdCount << " invalid ID, ";
            if (insufficientBalanceCount > 0) std::cout << insufficientBalanceCount << " insufficient balance, ";
            if (duplicateCount > 0) std::cout << duplicateCount << " duplicate";
            std::cout << " (rejected)\n";
        }
        
        if (candidates.empty()) {
            std::cout << "No valid candidate blocks!\n";
            return false;
        }
        
    // compact console: skip verbose mining info
        
        // kasa konkuruojancius blokus 
        Timer roundTimer;
        int winner = -1;
        unsigned long long bestNonce = 0;
        double bestTime = 0.0;
        
        for (size_t i = 0; i < candidates.size(); ++i) {
            unsigned long long nonce = 0;
            Timer candidateTimer;
            
            bool success = mineBlockWithLimits(candidates[i], currentTimeLimit, currentAttemptsLimit, nonce);
            double elapsed = candidateTimer.elapsed();
            
            if (success && winner == -1) {
                winner = static_cast<int>(i);
                bestNonce = nonce;
                bestTime = elapsed;
                break; // kai pirmas iskasa nutraukia cikla
            }
        }
        
        // rezultatai
        if (winner >= 0) {
            // compact console: don't print winner nonce/hash/time
            
            // laimetojas - apply all transactions (including coinbase)
            // In UTXO model, coinbase and fees are handled through outputs
            const auto& wonTxs = candidateTxSets[winner];
            for (const Transaction& tx : wonTxs) {
                ledger.apply(tx);
            }
            
            pool.eraseByIds(candidateRemoveIds[winner]);
            chain_.push_back(candidates[winner]);
            saveToFile(candidates[winner]);
            
            std::cout << "Block #" << candidates[winner].getIndex() 
                     << " mined (" << candidateTxSets[winner].size() 
                     << " tx).\n\n";
            
            return true;
        } else {
            // jei nei viens naiskastu - didinamas laikas/bandymai
            std::cout << "\nNo block mined with current limits. ";
            if (currentTimeLimit > 0.0) {
                currentTimeLimit *= 1.5;
                std::cout << "Increasing time limit to " << std::fixed 
                         << std::setprecision(1) << currentTimeLimit << "s";
            }
            if (currentAttemptsLimit > 0) {
                currentAttemptsLimit = static_cast<unsigned long long>(currentAttemptsLimit * 1.5);
                if (currentTimeLimit > 0.0) std::cout << " and ";
                std::cout << "attempts to " << currentAttemptsLimit;
            }
            std::cout << "...\n\n";
            round++;
            
            // saugiklis nuo begalinio ciklo
            if (round > 10) {
                std::cout << "Too many rounds - aborting.\n";
                return false;
            }
        }
    }
}

// === PARALLEL_MINING_START ===
// Paralelinis kandidatų kasimas su OpenMP
// Imituoja decentralizuotą Bitcoin tinklą, kur keli "maineriai" (threads) konkuruoja tarpusavyje
// Kiekvienas thread'as kasa savo kandidatinį bloką su skirtingomis transakcijomis
// Pirmas sėkmingai iškanavęs thread'as laimi - jo blokas pridedamas į grandinę
bool Blockchain::mineCandidateBlocksParallel(TxPool& pool, Ledger& ledger, size_t nTx, 
                                              int numCandidates, double timeLimitSec,
                                              unsigned long long attemptsLimit) {
    if (pool.empty()) {
        std::cout << "Transaction pool is empty!\n";
        return false;
    }
    
    std::cout << "\n=== Parallel Decentralized Mining: " << numCandidates << " candidates ===\n";
    if (timeLimitSec > 0.0 && attemptsLimit > 0) {
        std::cout << "Limits: " << timeLimitSec << " seconds OR " << attemptsLimit << " attempts\n\n";
    } else if (timeLimitSec > 0.0) {
        std::cout << "Time limit per round: " << timeLimitSec << " seconds\n\n";
    } else if (attemptsLimit > 0) {
        std::cout << "Attempts limit per round: " << attemptsLimit << " attempts\n\n";
    }
    
    // Kasimo raundai - jei nei vienas kandidatas neiškasinėja per limitą, 
    // limitai padidinami ir bandoma iš naujo
    int round = 1;
    double currentTimeLimit = timeLimitSec;
    unsigned long long currentAttemptsLimit = attemptsLimit;
    
    while (true) {
        std::cout << "--- Mining Round #" << round << " ---\n";
        if (currentTimeLimit > 0.0 && currentAttemptsLimit > 0) {
            std::cout << "Limits: " << std::fixed << std::setprecision(1)
                      << currentTimeLimit << "s OR " << currentAttemptsLimit << " attempts\n\n";
        } else if (currentTimeLimit > 0.0) {
            std::cout << "Time limit: " << std::fixed << std::setprecision(1)
                      << currentTimeLimit << "s\n\n";
        } else if (currentAttemptsLimit > 0) {
            std::cout << "Attempts limit: " << currentAttemptsLimit << "\n\n";
        }
        
        // ===== ŽINGSNIS 1: Kandidatinių blokų formavimas =====
        // Kiekvienas "maineris" (thread) formuoja savo bloką su unikaliomis transakcijomis
        std::vector<Block> candidates;
        std::vector<std::vector<Transaction>> candidateTxSets;
        std::vector<std::vector<std::string>> candidateRemoveIds;
        
        // Užtikrina, kad tos pačios TX nepatektų į kelis kandidatus
        // (imituoja realų Bitcoin tinklą, kur maineriai nekoordinuoja tarpusavyje)
        std::unordered_set<std::string> usedTxIds;
        
        // Validacijos statistika
        int totalSampled = 0;
        int invalidIdCount = 0;
        int insufficientBalanceCount = 0;
        int duplicateCount = 0;
        
        // Kiekvienas kandidatas formuojamas atskirai
        for (int i = 0; i < numCandidates; ++i) {
            // Pasiima atsitiktines transakcijas (oversample, nes dalis bus atmesta validacijos metu)
            std::vector<Transaction> selectedTx = pool.takeRandom(nTx * 2);
            if (selectedTx.empty()) break;
            
            // ===== VALIDACIJA: Dviejų žingsnių transakcijų tikrinimas =====
            std::vector<Transaction> validTx;
            std::vector<std::string> toRemoveValidIds; // Patvirtintos TX ID, kurias pašalinti iš pool
            
            // UTXO modelis: sekame kurie input'ai jau panaudoti šiame kandidate
            // (vienoje transakcijoje negali būti du kartus panaudotas tas pats UTXO)
            std::set<std::string> usedInputsInCandidate;
            
            for (const auto& tx : selectedTx) {
                totalSampled++;
                
                // Tikrina ar TX jau naudojama kitame kandidate
                if (usedTxIds.count(tx.getId()) > 0) {
                    duplicateCount++;
                    continue;
                }
                
                // VALIDACIJA 1: Transakcijos ID hash patikrinimas
                if (!tx.verifyId()) {
                    invalidIdCount++;
                    continue;
                }
                
                // VALIDACIJA 2: UTXO double-spending tikrinimas šiame kandidate
                // Patikrina ar transakcijos input'ai dar nebuvo panaudoti
                bool inputsAlreadyUsed = false;
                for (const auto& input : tx.getInputs()) {
                    std::string inputKey = input.prevTxId + ":" + std::to_string(input.outputIndex);
                    if (usedInputsInCandidate.count(inputKey) > 0) {
                        inputsAlreadyUsed = true;
                        break;
                    }
                }
                
                if (inputsAlreadyUsed) {
                    duplicateCount++;
                    continue;
                }
                
                // VALIDACIJA 3: Ledger tikrinimas (ar egzistuoja UTXO, ar pakanka balanso + fee)
                if (ledger.canApplyWithFee(tx, TX_FEE)) {
                    validTx.push_back(tx);
                    toRemoveValidIds.push_back(tx.getId());
                    
                    // Pažymi input'us kaip panaudotus šiame kandidate
                    for (const auto& input : tx.getInputs()) {
                        std::string inputKey = input.prevTxId + ":" + std::to_string(input.outputIndex);
                        usedInputsInCandidate.insert(inputKey);
                    }
                    
                    // Pažymi TX kaip panaudotą (negalės būti kituose kandidatuose)
                    usedTxIds.insert(tx.getId());
                    
                    // Riboja kandidato dydį (nTx-1, nes dar pridės coinbase)
                    if (validTx.size() >= nTx - 1) break;
                } else {
                    insufficientBalanceCount++;
                }
            }
            
            if (validTx.empty()) continue;

            // ===== COINBASE transakcija: Mainerio atlygis =====
            // Apskaičiuoja mainerio atlygį: Block Reward + sumos iš visų TX fee
            uint64_t totalFees = static_cast<uint64_t>(validTx.size()) * TX_FEE;
            uint64_t coinbaseAmt = BLOCK_REWARD + totalFees;
            
            // Gauna bloko indeksą (reikalingas coinbase unikalumui)
            int newIndex = static_cast<int>(chain_.size());
            
            // Sukuria coinbase TX pagal UTXO modelį
            // Coinbase turi specialų "input" su bloko numeriu (užtikrina unikalumą)
            std::vector<TxInput> coinbaseInputs;
            coinbaseInputs.push_back(TxInput("coinbase", newIndex)); // Unikalus kiekvienam blokui
            std::vector<TxOutput> coinbaseOutputs;
            coinbaseOutputs.push_back(TxOutput(FEE_COLLECTOR, coinbaseAmt)); // Atlygis maineriui
            Transaction coinbase(coinbaseInputs, coinbaseOutputs);
            
            // Formuoja galutinį transakcijų sąrašą: coinbase + validuotos TX
            std::vector<Transaction> blockTxs;
            blockTxs.reserve(validTx.size() + 1);
            blockTxs.push_back(coinbase);        // Pirma visada coinbase
            for (const auto& t : validTx) blockTxs.push_back(t);
            
            // ===== Sukuria kandidatinį bloką =====
            std::string prevHash = getLastBlockHash();
            Block candidate(newIndex, blockTxs, prevHash, difficulty_);
            
            // Išsaugo kandidatą ir jo duomenis
            candidates.push_back(candidate);
            candidateTxSets.push_back(blockTxs);
            candidateRemoveIds.push_back(toRemoveValidIds);
        }
        
        // Validacijos statistikos spausdinimas
        if (invalidIdCount > 0 || insufficientBalanceCount > 0 || duplicateCount > 0) {
            std::cout << "Validation: sampled " << totalSampled << " tx; ";
            if (invalidIdCount > 0) std::cout << invalidIdCount << " invalid ID, ";
            if (insufficientBalanceCount > 0) std::cout << insufficientBalanceCount << " insufficient balance, ";
            if (duplicateCount > 0) std::cout << duplicateCount << " duplicate";
            std::cout << " (rejected)\n";
        }
        
        if (candidates.empty()) {
            std::cout << "No valid candidate blocks!\n";
            return false;
        }
        
        // ===== ŽINGSNIS 2: PARALELINIS KASIMAS SU OpenMP =====
        // Kiekvienas thread kasa savo kandidatinį bloką konkuruojant su kitais
        // Pirmas thread'as, kuris suranda tinkamą nonce, laimi ir sustabdo kitus
        
        Timer roundTimer;
        std::atomic<bool> stopFlag{false};  // Shared flag: kai true, visi thread'ai sustoja
        int winIdx = -1;                     // Laimėtojo indeksas (-1 = dar nėra laimėtojo)
        std::vector<unsigned long long> nonces(candidates.size(), 0);  // Kiekvieno thread'o nonce
        std::vector<double> times(candidates.size(), 0.0);              // Kiekvieno thread'o laikas

        // === PARALLEL_OMP_LOOP ===
        // ===== OpenMP PARALELINIS CIKLAS =====
        // #pragma omp parallel for - sukuria kelis thread'us, kiekvienas kasa savo kandidatą
        // shared(...) - visi thread'ai mato tas pačias kintamąsias (stopFlag, candidates, etc.)
        #pragma omp parallel for shared(stopFlag, winIdx, candidates, nonces, times)
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
            // Jei kitas thread'as jau laimėjo, šis thread'as tuoj pat baigiasi
            if (stopFlag.load(std::memory_order_relaxed)) continue;
            
            // Kasa bloką su laiko/bandymų limitais
            Timer t;
            unsigned long long n = 0;
            bool ok = mineBlockWithTimeLimitStop(candidates[i], currentTimeLimit, currentAttemptsLimit, n, stopFlag);
            times[i] = t.elapsed();
            
            // Jei šis thread'as sėkmingai iškasė bloką
            if (ok) {
                nonces[i] = n;
                
                // === PARALLEL_CRITICAL_SECTION ===
                // KRITINĖ SEKCIJA: tik vienas thread'as vienu metu gali čia būti
                // Užtikrina, kad tik PIRMAS sėkmingai iškanavęs thread'as bus laimėtojas
                #pragma omp critical
                {
                    if (winIdx == -1) {  // Dar nėra laimėtojo?
                        winIdx = i;      // Šis thread'as tampa laimėtoju
                        stopFlag.store(true, std::memory_order_relaxed); // Sustabdo kitus thread'us
                    }
                }
            }
        }
        
        // Paralelinio kasimo pabaiga - surenkame rezultatus
        unsigned long long bestNonce = 0;
        double bestTime = 0.0;
        if (winIdx >= 0) {
            bestNonce = nonces[static_cast<size_t>(winIdx)];
            bestTime = times[static_cast<size_t>(winIdx)];
        }
        
        // ŽINGSNIS 3: rezultatų apdorojimas 
        if (winIdx >= 0) {
            // === PARALLEL_WINNER_APPLICATION ===
            // LAIMĖTOJAS RASTAS! Pritaikoma jo blokas į blockchain
            
            // Pritaiko visas laimėtojo bloko transakcijas į ledger (UTXO modelis)
            // Coinbase ir fee yra apdorojami per transaction outputs
            const auto& wonTxs = candidateTxSets[winIdx];
            for (const Transaction& tx : wonTxs) {
                ledger.apply(tx);  // Atnaujina UTXO set
            }
            
            // Pašalina panaudotas transakcijas iš pool
            pool.eraseByIds(candidateRemoveIds[winIdx]);
            
            // Valo invalid transakcijas (kurių UTXO buvo išleisti šiame bloke)
            pool.removeInvalid(ledger, TX_FEE);
            
            // Prideda laimėtojo bloką į blockchain
            chain_.push_back(candidates[winIdx]);
            saveToFile(candidates[winIdx]);
            
            // Išsaugo kasimo statistiką
            miningHistory_.push_back({bestTime, bestNonce, candidates[winIdx].getIndex()});
            
            std::cout << "Block #" << candidates[winIdx].getIndex() 
                     << " mined (" << candidateTxSets[winIdx].size() 
                     << " tx, parallel).\n\n";
            
            return true;
        } else {
            // NĖ VIENAS KANDIDATAS NEIŠKASĖ per limitą
            // Padidinami limitai ir bandoma iš naujo kitame raunde
            std::cout << "\nNo block mined with current limits. ";
            bool printed = false;
            if (currentTimeLimit > 0.0) {
                currentTimeLimit *= 1.5;  // +50% laiko
                std::cout << "Increasing time limit to " << std::fixed
                          << std::setprecision(1) << currentTimeLimit << "s";
                printed = true;
            }
            if (currentAttemptsLimit > 0) {
                currentAttemptsLimit = static_cast<unsigned long long>(currentAttemptsLimit * 1.5);  // +50% bandymų
                if (printed) std::cout << " and ";
                std::cout << "attempts to " << currentAttemptsLimit;
            }
            std::cout << "...\n\n";
            round++;

            // Saugiklis nuo begalinio ciklo (max 10 raundų)
            if (round > 10) {
                std::cout << "Too many rounds - aborting.\n";
                return false;
            }
        }
    }
}

std::string Blockchain::getLastBlockHash() const {
    if (chain_.empty()) return "0";
    return chain_.back().getHash();
}

bool Blockchain::isChainValid() const {
    std::string target(difficulty_, '0');
    for (size_t i = 1; i < chain_.size(); i++) {
        const Block& curr = chain_[i];
        const Block& prev = chain_[i - 1];

        if (curr.getPreviousHash() != prev.getHash()) {
            std::cout << "Invalid chain: previous hash mismatch at block " << i << "\n";
            return false;
        }

        std::string recalculated = generate_hash(curr.toString());
        if (curr.getHash() != recalculated) {
            std::cout << "Invalid chain: hash mismatch at block " << i << "\n";
            return false;
        }

        if (curr.getHash().substr(0, difficulty_) != target) {
            std::cout << "Invalid chain: difficulty not met at block " << i << "\n";
            return false;
        }

        // papildoma validacija — patikriname Merkle Root atitikima
        if (curr.getVersion() == 2 && !curr.verifyTxRoot()) {
            std::cout << "Invalid chain: txRoot mismatch at block " << i << "\n";
            return false;
        }
    }
    return true;
}

void Blockchain::printChain() const {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "BLOCKCHAIN (Total blocks: " << chain_.size() << ")\n";
    std::cout << std::string(50, '=') << "\n\n";
    for (const auto& block : chain_) {
        block.printBlock();
    }
}

void Blockchain::printStatistics() const {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "BLOCKCHAIN STATISTICS\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "Total blocks  : " << chain_.size() << "\n";
    std::cout << "Difficulty    : " << difficulty_ << " (hash starts with " 
              << std::string(difficulty_, '0') << ")\n";
    std::cout << "Chain valid   : " << (isChainValid() ? "YES" : "NO") << "\n";

    if (chain_.size() > 1) {
        unsigned long long totalNonces = 0;
        for (size_t i = 1; i < chain_.size(); i++) {
            totalNonces += chain_[i].getNonce();
        }
        double avgNonce = static_cast<double>(totalNonces) / (chain_.size() - 1);
        std::cout << "Average nonce : " << avgNonce << "\n";
    }
    std::cout << std::string(50, '=') << "\n\n";
}

// issaugojimas i faila
void Blockchain::saveToFile(const Block& block) const {
    (void)block;
}

// JSON eksportas
void Blockchain::exportToJson(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }
    
    file << "{\n";
    file << "  \"difficulty\": " << difficulty_ << ",\n";
    file << "  \"chainLength\": " << chain_.size() << ",\n";
    file << "  \"chain\": [\n";
    
    for (size_t i = 0; i < chain_.size(); ++i) {
        const auto& block = chain_[i];
        file << "    {\n";
        file << "      \"index\": " << block.getIndex() << ",\n";
        file << "      \"timestamp\": " << block.getTimestamp() << ",\n";
        file << "      \"version\": " << block.getVersion() << ",\n";
        file << "      \"hash\": \"" << block.getHash() << "\",\n";
        file << "      \"previousHash\": \"" << block.getPreviousHash() << "\",\n";
        file << "      \"nonce\": " << block.getNonce() << ",\n";
        file << "      \"difficulty\": " << block.getDifficulty() << ",\n";
        
        if (block.getVersion() == 1) {
            file << "      \"data\": \"" << block.getData() << "\",\n";
        } else {
            file << "      \"txRoot\": \"" << block.getTxRoot() << "\",\n";
            file << "      \"transactionCount\": " << block.getTransactions().size() << ",\n";
            
            // transactions array
            file << "      \"transactions\": [\n";
            const auto& txs = block.getTransactions();
            for (size_t j = 0; j < txs.size(); ++j) {
                file << "        {\n";
                file << "          \"id\": \"" << txs[j].getId() << "\",\n";
                file << "          \"from\": \"" << txs[j].getFrom() << "\",\n";
                file << "          \"to\": \"" << txs[j].getTo() << "\",\n";
                file << "          \"amount\": " << txs[j].getAmount() << ",\n";
                file << "          \"timestamp\": " << txs[j].getTimestamp() << "\n";
                file << "        }" << (j < txs.size() - 1 ? "," : "") << "\n";
            }
            file << "      ]\n";
        }
        
        file << "    }" << (i < chain_.size() - 1 ? "," : "") << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    file.close();
    
    std::cout << "Blockchain exported to " << filename << "\n";
}

// kiekviena bloka i atskira json faila 
void Blockchain::exportBlocksToJsonDir(const std::string& dirPath) const {
    // uztikrina kad butu katalogas 
#ifdef _WIN32
    _mkdir(dirPath.c_str());
#endif

    // kiekviena bloka i atskira faila
    for (const auto& block : chain_) {
        std::string filePath = dirPath + "/block_" + std::to_string(block.getIndex()) + ".json";
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filePath << "\n";
            continue;
        }

        file << "{\n";
        file << "  \"index\": " << block.getIndex() << ",\n";
        file << "  \"timestamp\": " << block.getTimestamp() << ",\n";
        file << "  \"version\": " << block.getVersion() << ",\n";
        file << "  \"hash\": \"" << block.getHash() << "\",\n";
        file << "  \"previousHash\": \"" << block.getPreviousHash() << "\",\n";
        file << "  \"nonce\": " << block.getNonce() << ",\n";
        file << "  \"difficulty\": " << block.getDifficulty() << ",\n";

        if (block.getVersion() == 1) {
            file << "  \"data\": \"" << block.getData() << "\"\n";
        } else {
            file << "  \"txRoot\": \"" << block.getTxRoot() << "\",\n";
            file << "  \"transactionCount\": " << block.getTransactions().size() << ",\n";
            
            //  merkle tree i json faila bloku
            const auto& txs = block.getTransactions();
            if (!txs.empty()) {
                std::vector<std::string> leaves;
                leaves.reserve(txs.size());
                for (const auto& tx : txs) {
                    leaves.push_back(tx.getId());
                }
                MerkleTree tree = MerkleTree::from_leaves(leaves);
                const auto& levels = tree.levels();
                
                file << "  \"merkleTree\": {\n";
                file << "    \"root\": \"" << tree.root() << "\",\n";
                file << "    \"levels\": [\n";
                for (size_t lvl = 0; lvl < levels.size(); ++lvl) {
                    file << "      [\n";
                    for (size_t h = 0; h < levels[lvl].size(); ++h) {
                        file << "        \"" << levels[lvl][h] << "\"";
                        if (h + 1 < levels[lvl].size()) file << ",";
                        file << "\n";
                    }
                    file << "      ]";
                    if (lvl + 1 < levels.size()) file << ",";
                    file << "\n";
                }
                file << "    ],\n";
                file << "    \"levelCount\": " << levels.size() << ",\n";
                file << "    \"description\": \"Level 0 = transaction IDs (leaves), Level " << (levels.size() - 1) << " = root\"\n";
                file << "  },\n";
            }
            
            file << "  \"transactions\": [\n";
            for (size_t j = 0; j < txs.size(); ++j) {
                file << "    {\n";
                file << "      \"id\": \"" << txs[j].getId() << "\",\n";
                file << "      \"timestamp\": " << txs[j].getTimestamp() << ",\n";
                
                // UTXO inputs
                file << "      \"inputs\": [\n";
                const auto& inputs = txs[j].getInputs();
                for (size_t k = 0; k < inputs.size(); ++k) {
                    file << "        {\"prevTxId\": \"" << inputs[k].prevTxId << "\", ";
                    file << "\"outputIndex\": " << inputs[k].outputIndex << "}";
                    if (k + 1 < inputs.size()) file << ",";
                    file << "\n";
                }
                file << "      ],\n";
                
                // UTXO outputs
                file << "      \"outputs\": [\n";
                const auto& outputs = txs[j].getOutputs();
                for (size_t k = 0; k < outputs.size(); ++k) {
                    file << "        {\"receiver\": \"" << outputs[k].receiver << "\", ";
                    file << "\"amount\": " << outputs[k].amount << "}";
                    if (k + 1 < outputs.size()) file << ",";
                    file << "\n";
                }
                file << "      ]\n";
                
                file << "    }" << (j + 1 < txs.size() ? "," : "") << "\n";
            }
            file << "  ]\n";
        }

        file << "}\n";
        file.close();
    }

    std::cout << "Exported " << chain_.size() << " blocks to directory: " << dirPath << "\n";
}

// detailed mining statistics 
void Blockchain::printDetailedStatistics() const {
    if (miningHistory_.empty()) {
        std::cout << "No mining statistics available.\n";
        return;
    }
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "DETAILED MINING STATISTICS\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    std::cout << "Mining Times:\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (const auto& stat : miningHistory_) {
        std::cout << "Block #" << std::setw(2) << stat.blockIndex << " | ";
        std::cout << std::fixed << std::setprecision(3) << stat.miningTime << "s";
        std::cout << " (" << stat.attempts << " attempts)\n";
    }
    
    std::cout << std::string(60, '-') << "\n";
    std::cout << "\n";
}
