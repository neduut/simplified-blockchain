#pragma once
#include "block.h"
#include <vector>
#include <string>

class Blockchain {
public:
    explicit Blockchain(int difficulty);

    void addBlock(const std::string& data);
    bool isChainValid() const;
    void printChain() const;
    void printStatistics() const;

private:
    void mineBlock(Block& block);
    std::string getLastBlockHash() const;
    void saveToFile(const Block& block) const; // nauja

    std::vector<Block> chain_;
    int difficulty_;
};
