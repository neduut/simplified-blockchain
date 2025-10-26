#include "blockchain.h"
#include "ownHash.h"
#include "timer.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

Blockchain::Blockchain(int difficulty)
    : difficulty_(difficulty) 
{
    std::cout << "==============================================\n";
    std::cout << "Simplified Blockchain v0.1\n";
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
                      << " | Hash: " << hash.substr(0, 20) 
                      << "... | Time: " 
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
    
    // filtruojam tik valid
    std::vector<Transaction> validTx;
    std::vector<std::string> toRemoveIds;
    
    for (const auto& tx : selectedTx) {
        if (ledger.canApply(tx)) {
            validTx.push_back(tx);
            toRemoveIds.push_back(tx.getId());
        } else {
            std::cout << "   Invalid TX " << tx.getId().substr(0, 8) 
                     << "... (insufficient balance)\n";
            toRemoveIds.push_back(tx.getId());
        }
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
    std::ofstream file("blockchain_log.txt", std::ios::app);
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
