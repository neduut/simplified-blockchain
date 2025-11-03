#include "block.h"
#include "ownHash.h"
#include "merkle.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Block::Block(int index, const std::string& data, const std::string& previousHash)
    : index_(index)
    , timestamp_(std::time(nullptr))
    , data_(data)
    , previousHash_(previousHash)
    , nonce_(0)
    , hash_("")
    , version_(1)
    , difficulty_(0)
    , txRoot_("") {}

// konstruktorius su transakcijomis
Block::Block(int index, const std::vector<Transaction>& transactions, 
             const std::string& previousHash, int difficulty)
    : index_(index)
    , timestamp_(std::time(nullptr))
    , data_("")
    , previousHash_(previousHash)
    , nonce_(0)
    , hash_("")
    , version_(2)
    , difficulty_(difficulty)
    , transactions_(transactions) {
    
    // v0.2 generuoja tikra Merkle Root is transakciju id
    std::vector<std::string> leaves;
    leaves.reserve(transactions_.size());
    for (const auto& tx : transactions_) {
        leaves.push_back(tx.getId());
    }
    txRoot_ = MerkleTree::from_leaves(leaves).root();
}

// v0.2 tikrinimas
std::string Block::recomputeTxRoot() const {
    if (version_ != 2) return std::string();
    std::vector<std::string> leaves;
    leaves.reserve(transactions_.size());
    for (const auto& tx : transactions_) leaves.push_back(tx.getId());
    return MerkleTree::from_leaves(leaves).root();
}

bool Block::verifyTxRoot() const {
    if (version_ != 2) return true; // v0.1 neturi txRoot
    return txRoot_ == recomputeTxRoot();
}

std::string Block::toString() const {
    std::ostringstream oss;
    
    if (version_ == 1) {
        // v0.1 formatas
        oss << index_ << timestamp_ << data_ << previousHash_ << nonce_;
    } else {
        // v0.2 formatas
        oss << index_ << timestamp_ << txRoot_ << previousHash_ 
            << nonce_ << difficulty_ << version_;
    }
    
    return oss.str();
}

void Block::printBlock() const {
    std::cout << "----------------------------------------\n";
        if (version_ == 1) {
            std::cout << "Block #" << index_ << "\n";
        } else {
            std::cout << "Block #" << index_ << " (parallel)\n";
        }
    std::cout << "Timestamp : " << timestamp_ << "\n";
    
    if (version_ == 1) {
        std::cout << "Data      : " << data_ << "\n";
    } else {
        std::cout << "Tx Count  : " << transactions_.size() << "\n";
        std::cout << "Tx Root   : " << txRoot_ << "\n";
        std::cout << "Difficulty: " << difficulty_ << "\n";
    }
    
    std::cout << "Nonce     : " << nonce_ << "\n";
    std::cout << "Prev Hash : " << previousHash_ << "\n";
    std::cout << "Hash      : " << hash_ << "\n";
    std::cout << "----------------------------------------\n\n";
}