#pragma once
#include "block.h"
#include "txpool.h"
#include "ledger.h"
#include <vector>
#include <string>
#include <atomic>

class Blockchain {
public:
    explicit Blockchain(int difficulty);
    
    // Rule of Five: isjungia kopijavima
    Blockchain(const Blockchain&) = delete;
    Blockchain& operator=(const Blockchain&) = delete;
    
    // move 
    Blockchain(Blockchain&&) noexcept = default;
    Blockchain& operator=(Blockchain&&) noexcept = default;
    
    ~Blockchain() = default;

    // simple block
    void addBlock(const std::string& data);
    
    // block su transakcijom is pool
    bool formBlockFromPool(TxPool& pool, Ledger& ledger, size_t nTx = 100);
    
    // decentralizuotas kasimas su kandidatiniais blokais 
    bool mineCandidateBlocks(TxPool& pool, Ledger& ledger, size_t nTx = 100, 
                             int numCandidates = 5, double timeLimitSec = 5.0,
                             unsigned long long attemptsLimit = 0); // 0 = neribota
    
    // paralelinis kandidatu kasimas 
    bool mineCandidateBlocksParallel(TxPool& pool, Ledger& ledger, size_t nTx = 100, 
                                      int numCandidates = 5, double timeLimitSec = 5.0,
                                      unsigned long long attemptsLimit = 0);
    
    bool isChainValid() const;
    void printChain() const;
    void printStatistics() const;
    void printDetailedStatistics() const; // Detalesnė statistika su histograma
    
    // JSON eksportas
    void exportToJson(const std::string& filename) const;
    
    // getters 
    size_t getChainSize() const noexcept { return chain_.size(); }
    const Block& getBlock(size_t index) const { return chain_.at(index); }
    const std::vector<Block>& getChain() const noexcept { return chain_; }
    int getDifficulty() const noexcept { return difficulty_; }

private:
    void mineBlock(Block& block);
    
    // kasimas su laiko limitu (graziina true jei iskasa, false jei per letas)
    bool mineBlockWithTimeLimit(Block& block, double timeLimitSec, unsigned long long& finalNonce);
    // kasimas su bandymu limitu (arba laiko, jei timeLimitSec > 0)
    bool mineBlockWithLimits(Block& block, double timeLimitSec, unsigned long long attemptsLimit, unsigned long long& finalNonce);
    // kasimas su laiko/bandymu limitu ir bendru stop signalu (naudojama paraleliniam kasimui)
    bool mineBlockWithTimeLimitStop(Block& block, double timeLimitSec, unsigned long long attemptsLimit, 
                                     unsigned long long& finalNonce, std::atomic<bool>& stopFlag);
    
    std::string getLastBlockHash() const;
    void saveToFile(const Block& block) const;

    std::vector<Block> chain_;
    int difficulty_;
    
    // Mining statistics tracking
    struct MiningStats {
        double miningTime;
        unsigned long long attempts;
        int blockIndex;
    };
    std::vector<MiningStats> miningHistory_;

};
