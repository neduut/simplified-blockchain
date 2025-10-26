#pragma once
#include <string>
#include <ctime>

class Block {
public:
    Block(int index, const std::string& data, const std::string& previousHash);

    std::string toString() const;
    void printBlock() const;

    // Getteriai
    int getIndex() const { return index_; }
    std::time_t getTimestamp() const { return timestamp_; }
    const std::string& getData() const { return data_; }
    const std::string& getPreviousHash() const { return previousHash_; }
    const std::string& getHash() const { return hash_; }
    unsigned long long getNonce() const { return nonce_; }

    // Setteriai
    void setNonce(unsigned long long nonce) { nonce_ = nonce; }
    void setHash(const std::string& hash) { hash_ = hash; }

private:
    int index_;
    std::time_t timestamp_;
    std::string data_;
    std::string previousHash_;
    unsigned long long nonce_;
    std::string hash_;
};
