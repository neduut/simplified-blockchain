#pragma once
#include "block.h"
#include "txpool.h"
#include "ledger.h"
#include <vector>
#include <string>

class Blockchain {
public:
    explicit Blockchain(int difficulty);

    // simple block
    void addBlock(const std::string& data);
    
    // block su transakcijom is pool
    bool formBlockFromPool(TxPool& pool, Ledger& ledger, size_t nTx = 100);
    
    // v0.2: decentralizuotas kasimas su kandidatiniais blokais
    bool mineCandidateBlocks(TxPool& pool, Ledger& ledger, size_t nTx = 100, 
                             int numCandidates = 5, double timeLimitSec = 5.0);
    
    bool isChainValid() const;
    void printChain() const;
    void printStatistics() const;
    
    // getters
    size_t getChainSize() const { return chain_.size(); }
    const Block& getBlock(size_t index) const { return chain_.at(index); }
    const std::vector<Block>& getChain() const { return chain_; }
    int getDifficulty() const { return difficulty_; }

private:
    void mineBlock(Block& block);
    
    // v0.2: kasimas su laiko limitu (grąžina true jei iškastu, false jei per lėtas)
    bool mineBlockWithTimeLimit(Block& block, double timeLimitSec, unsigned long long& finalNonce);
    
    std::string getLastBlockHash() const;
    void saveToFile(const Block& block) const;

    std::vector<Block> chain_;
    int difficulty_;
};
