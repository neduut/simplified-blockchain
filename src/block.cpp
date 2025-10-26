#include "block.h"
#include "ownHash.h"
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

// v0.2: Konstruktorius su transakcijomis
Block::Block(int index, const std::vector<Transaction>& transactions, 
             const std::string& previousHash, int difficulty)
    : index_(index)
    , timestamp_(std::time(nullptr))
    , data_("")  // v0.2 nenaudojame data_
    , previousHash_(previousHash)
    , nonce_(0)
    , hash_("")
    , version_(2)
    , difficulty_(difficulty)
    , transactions_(transactions) {
    
    // Generuojame txRoot - paprastas visų TX ID hash
    std::ostringstream oss;
    for (const auto& tx : transactions_) {
        oss << tx.getId();
    }
    txRoot_ = generate_hash(oss.str());
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
    std::cout << "Block #" << index_ << " (v" << version_ << ")\n";
    std::cout << "Timestamp : " << timestamp_ << "\n";
    
    if (version_ == 1) {
        std::cout << "Data      : " << data_ << "\n";
    } else {
        std::cout << "Tx Count  : " << transactions_.size() << "\n";
        std::cout << "Tx Root   : " << txRoot_.substr(0, 16) << "...\n";
        std::cout << "Difficulty: " << difficulty_ << "\n";
    }
    
    std::cout << "Nonce     : " << nonce_ << "\n";
    std::cout << "Prev Hash : " << previousHash_.substr(0, 16) << "...\n";
    std::cout << "Hash      : " << hash_.substr(0, 32) << "...\n";
    std::cout << "----------------------------------------\n\n";
}