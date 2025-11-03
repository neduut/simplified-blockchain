#pragma once
#include "transaction.h"
#include <string>
#include <ctime>
#include <vector>

class Block {
public:
    Block(int index, const std::string& data, const std::string& previousHash);
    
    // konstruktorius su transakcijomis
    Block(int index, const std::vector<Transaction>& transactions, 
          const std::string& previousHash, int difficulty = 3);

    std::string toString() const;
    void printBlock() const;

    // getteriai
    int getIndex() const { return index_; }
    std::time_t getTimestamp() const { return timestamp_; }
    const std::string& getData() const { return data_; }
    const std::string& getPreviousHash() const { return previousHash_; }
    const std::string& getHash() const { return hash_; }
    unsigned long long getNonce() const { return nonce_; }
    int getVersion() const { return version_; }
    int getDifficulty() const { return difficulty_; }
    const std::string& getTxRoot() const { return txRoot_; }
    const std::vector<Transaction>& getTransactions() const { return transactions_; }

    // setteriai
    void setNonce(unsigned long long nonce) { nonce_ = nonce; }
    void setHash(const std::string& hash) { hash_ = hash; }
    void setTxRoot(const std::string& txRoot) { txRoot_ = txRoot; }

    // v0.2: diagnostika/validacija
    // Perskaičiuoja Merkle Root iš dabartinių transakcijų ID
    std::string recomputeTxRoot() const;
    // Patikrina, ar saugomas txRoot_ sutampa su perskaičiuotu
    bool verifyTxRoot() const;

private:
    int index_;
    std::time_t timestamp_;
    std::string data_;
    std::string previousHash_;
    unsigned long long nonce_;
    std::string hash_;
    
    // nauji laukai
    int version_;
    int difficulty_;
    std::string txRoot_;
    std::vector<Transaction> transactions_;
};
