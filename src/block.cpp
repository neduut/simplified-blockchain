#include "block.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Block::Block(int index, const std::string& data, const std::string& previousHash)
    : index_(index)
    , timestamp_(std::time(nullptr))
    , data_(data)
    , previousHash_(previousHash)
    , nonce_(0)
    , hash_("") {}

std::string Block::toString() const {
    std::ostringstream oss;
    oss << index_ << timestamp_ << data_ << previousHash_ << nonce_;
    return oss.str();
}

void Block::printBlock() const {
    std::cout << "----------------------------------------\n";
    std::cout << "Block #" << index_ << "\n";
    std::cout << "Timestamp : " << timestamp_ << "\n";
    std::cout << "Data      : " << data_ << "\n";
    std::cout << "Nonce     : " << nonce_ << "\n";
    std::cout << "Prev Hash : " << previousHash_.substr(0, 16) << "...\n";
    std::cout << "Hash      : " << hash_.substr(0, 32) << "...\n";
    std::cout << "----------------------------------------\n\n";
}
