#pragma once
#include "block.h"
#include "txpool.h"
#include "ledger.h"
#include <vector>
#include <string>

class Blockchain {
public:
    explicit Blockchain(int difficulty);

    // v0.1: Simple block with data
    void addBlock(const std::string& data);
    
    // v0.2: Block su transakcijomis iš pool'o
    bool formBlockFromPool(TxPool& pool, Ledger& ledger, size_t nTx = 100);
    
    bool isChainValid() const;
    void printChain() const;
    void printStatistics() const;
    
    // Getters
    size_t getChainSize() const { return chain_.size(); }
    const Block& getBlock(size_t index) const { return chain_.at(index); }
    int getDifficulty() const { return difficulty_; }

private:
    void mineBlock(Block& block);
    std::string getLastBlockHash() const;
    void saveToFile(const Block& block) const;

    std::vector<Block> chain_;
    int difficulty_;
};
