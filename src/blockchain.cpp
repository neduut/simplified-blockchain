#include "blockchain.h"
#include "ownHash.h"
#include "timer.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <atomic>

Blockchain::Blockchain(int difficulty)
    : difficulty_(difficulty) 
{
    std::cout << "==============================================\n";
    std::cout << "Simplified Blockchain\n";
    std::cout << "Difficulty: " << difficulty << "\n";
    std::cout << "==============================================\n\n";

    // Genesis block
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

    std::cout << "Mining Block #" << block.getIndex() 
              << " (target: '" << target << "')...\n";

    Timer timer;

    while (true) {
        block.setNonce(nonce);
        std::string blockData = block.toString();
        hash = generate_hash(blockData);

        if (hash.substr(0, difficulty_) == target) {
            block.setHash(hash);
            double elapsed = timer.elapsed();

            std::cout << "Block #" << block.getIndex() << " mined!\n";
            std::cout << "   Nonce: " << nonce 
                      << " | Hash: " << hash
                      << " | Time: " 
                      << std::fixed << std::setprecision(3) << elapsed 
                      << " s | Attempts: " << nonce + 1 << "\n\n";
            return;
        }

        nonce++;

        if (nonce % 100000 == 0) {
            std::cout << "   Tried " << nonce << " nonces...\n";
        }
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

// kasimas su laiko IR/ARBA bandymu limitu
bool Blockchain::mineBlockWithLimits(Block& block, double timeLimitSec, unsigned long long attemptsLimit, unsigned long long& finalNonce) {
    std::string target(difficulty_, '0');
    std::string hash;
    unsigned long long nonce = 0;
    
    Timer timer;
    
    // jei abu limitai 0 – error
    if (timeLimitSec <= 0.0 && attemptsLimit == 0) {
        finalNonce = 0;
        return false;
    }
    
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
        if (ledger.canApply(tx)) {
            validTx.push_back(tx);
            toRemoveIds.push_back(tx.getId());
        } else {
            std::cout << "   Invalid TX " << tx.getId().substr(0, 8) 
                     << "... (insufficient balance)\n";
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
    
    // pritaikom ledger
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
        
        for (int i = 0; i < numCandidates; ++i) {
            // pasiima atsitiktines transakcijas
            std::vector<Transaction> selectedTx = pool.takeRandom(nTx);
            if (selectedTx.empty()) break;
            
            //verifikacija
            std::vector<Transaction> validTx;
            std::vector<std::string> toRemoveIds;
            
            for (const auto& tx : selectedTx) {
                if (!tx.verifyId()) {
                    toRemoveIds.push_back(tx.getId());
                    continue;
                }
                if (ledger.canApply(tx)) {
                    validTx.push_back(tx);
                    toRemoveIds.push_back(tx.getId());
                }
            }
            
            if (validTx.empty()) continue;
            
            // sukuria kandidatini bloka
            int newIndex = static_cast<int>(chain_.size());
            std::string prevHash = getLastBlockHash();
            Block candidate(newIndex, validTx, prevHash, difficulty_);
            
            candidates.push_back(candidate);
            candidateTxSets.push_back(validTx);
            candidateRemoveIds.push_back(toRemoveIds);
            
            std::cout << "Candidate #" << (i + 1) << ": " << validTx.size() 
                     << " transactions\n";
        }
        
        if (candidates.empty()) {
            std::cout << "No valid candidate blocks!\n";
            return false;
        }
        
        std::cout << "\nMining " << candidates.size() << " candidates competitively...\n";
        
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
            std::cout << "\n*** Candidate #" << (winner + 1) << " WON! ***\n";
            std::cout << "   Nonce: " << bestNonce 
                     << " | Hash: " << candidates[winner].getHash()
                     << " | Time: " << std::fixed << std::setprecision(3) 
                     << bestTime << " s\n";
            
            // laimetojas
            for (const auto& tx : candidateTxSets[winner]) {
                ledger.apply(tx);
            }
            
            pool.eraseByIds(candidateRemoveIds[winner]);
            chain_.push_back(candidates[winner]);
            saveToFile(candidates[winner]);
            
            std::cout << "Block #" << candidates[winner].getIndex() 
                     << " added to chain with " << candidateTxSets[winner].size() 
                     << " transactions\n\n";
            
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

// paralelinis kandidatų kasimas 
bool Blockchain::mineCandidateBlocksParallel(TxPool& pool, Ledger& ledger, size_t nTx, 
                                              int numCandidates, double timeLimitSec,
                                              unsigned long long attemptsLimit) {
    if (pool.empty()) {
        std::cout << "Transaction pool is empty!\n";
        return false;
    }
    
    std::cout << "\n=== Parallel Decentralized Mining: " << numCandidates << " candidates ===\n";
    std::cout << "Time limit per round: " << timeLimitSec << " seconds\n\n";
    
    int round = 1;
    double currentTimeLimit = timeLimitSec;
    
    while (true) {
        std::cout << "--- Mining Round #" << round << " ---\n";
        std::cout << "Time limit: " << std::fixed << std::setprecision(1) 
                  << currentTimeLimit << "s\n\n";
        
        // sukuria kandidatinius blokus
        std::vector<Block> candidates;
        std::vector<std::vector<Transaction>> candidateTxSets;
        std::vector<std::vector<std::string>> candidateRemoveIds;
        
        for (int i = 0; i < numCandidates; ++i) {
            std::vector<Transaction> selectedTx = pool.takeRandom(nTx);
            if (selectedTx.empty()) break;
            
            std::vector<Transaction> validTx;
            std::vector<std::string> toRemoveIds;
            
            for (const auto& tx : selectedTx) {
                if (!tx.verifyId()) {
                    toRemoveIds.push_back(tx.getId());
                    continue;
                }
                if (ledger.canApply(tx)) {
                    validTx.push_back(tx);
                    toRemoveIds.push_back(tx.getId());
                }
            }
            
            if (validTx.empty()) continue;
            
            int newIndex = static_cast<int>(chain_.size());
            std::string prevHash = getLastBlockHash();
            Block candidate(newIndex, validTx, prevHash, difficulty_);
            
            candidates.push_back(candidate);
            candidateTxSets.push_back(validTx);
            candidateRemoveIds.push_back(toRemoveIds);
            
            std::cout << "Candidate #" << (i + 1) << ": " << validTx.size() 
                     << " transactions\n";
        }
        
        if (candidates.empty()) {
            std::cout << "No valid candidate blocks!\n";
            return false;
        }
        
        std::cout << "\nMining " << candidates.size() << " candidates in parallel...\n";
        
        // paralelinis kasimas su threads
        Timer roundTimer;
        std::atomic<bool> stopFlag{false};
        std::atomic<int> winner{-1};
        std::vector<unsigned long long> nonces(candidates.size(), 0);
        std::vector<double> times(candidates.size(), 0.0);
        std::vector<std::thread> threads;
        threads.reserve(candidates.size());

        for (size_t i = 0; i < candidates.size(); ++i) {
            threads.emplace_back([&, i]() {
                Timer t;
                unsigned long long n = 0;
                bool ok = mineBlockWithTimeLimitStop(candidates[i], currentTimeLimit, attemptsLimit, n, stopFlag);
                times[i] = t.elapsed();
                if (ok) {
                    nonces[i] = n;
                    int expected = -1;
                    if (winner.compare_exchange_strong(expected, static_cast<int>(i))) {
                        stopFlag.store(true, std::memory_order_relaxed);
                    }
                }
            });
        }
        for (auto& th : threads) th.join();

        int winIdx = winner.load();
        unsigned long long bestNonce = 0;
        double bestTime = 0.0;
        if (winIdx >= 0) {
            bestNonce = nonces[static_cast<size_t>(winIdx)];
            bestTime = times[static_cast<size_t>(winIdx)];
        }
        
        if (winIdx >= 0) {
            std::cout << "\n*** Candidate #" << (winIdx + 1) << " WON! ***\n";
            std::cout << "   Nonce: " << bestNonce 
                     << " | Hash: " << candidates[winIdx].getHash()
                     << " | Time: " << std::fixed << std::setprecision(3) 
                     << bestTime << " s\n";
            
            for (const auto& tx : candidateTxSets[winIdx]) {
                ledger.apply(tx);
            }
            
            pool.eraseByIds(candidateRemoveIds[winIdx]);
            chain_.push_back(candidates[winIdx]);
            saveToFile(candidates[winIdx]);
            
            std::cout << "Block #" << candidates[winIdx].getIndex() 
                     << " added to chain with " << candidateTxSets[winIdx].size() 
                     << " transactions\n\n";
            
            return true;
        } else {
            std::cout << "\nNo block mined in " << currentTimeLimit << "s. ";
            currentTimeLimit *= 1.5;
            std::cout << "Increasing time limit to " << std::fixed 
                     << std::setprecision(1) << currentTimeLimit << "s...\n\n";
            round++;
            
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
    std::ofstream file("logs/blockchain_log.txt", std::ios::app);
    if (!file.is_open()) return;

    file << "Block #" << block.getIndex() << "\n";
    file << "Timestamp : " << block.getTimestamp() << "\n";
    file << "Version   : " << block.getVersion() << "\n";
    // jei v1, rasom data; jei v2, rasom tx root ir difficulty
    if (block.getVersion() == 1) {
        file << "Data      : " << block.getData() << "\n";
        file << "Difficulty: " << block.getDifficulty() << "\n";
    } else {
        file << "Tx Root   : " << block.getTxRoot() << "\n";
        file << "Difficulty: " << block.getDifficulty() << "\n";
    }
    file << "Nonce     : " << block.getNonce() << "\n";
    file << "Prev Hash : " << block.getPreviousHash() << "\n";
    file << "Hash      : " << block.getHash() << "\n";
    file << std::string(40, '-') << "\n";
    file.close();
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
            
            // Transactions array
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
