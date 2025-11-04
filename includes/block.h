#pragma once
#include "transaction.h"
#include <string>
#include <ctime>
#include <vector>

class Block {
public:
    // konstruktoriai 
    Block(int index, const std::string& data, const std::string& previousHash);
    
    // konstruktorius su transakcijomis
    Block(int index, const std::vector<Transaction>& transactions, 
          const std::string& previousHash, int difficulty = 3);

    // Rule of Five: default, nes naudoju tik STL konteinerius (automatinis RAII)
    Block(const Block&) = default;
    Block& operator=(const Block&) = default;
    Block(Block&&) noexcept = default;
    Block& operator=(Block&&) noexcept = default;
    ~Block() = default;

    std::string toString() const;
    void printBlock() const;

    // getteriai 
    int getIndex() const noexcept { return index_; }
    std::time_t getTimestamp() const noexcept { return timestamp_; }
    const std::string& getData() const noexcept { return data_; }
    const std::string& getPreviousHash() const noexcept { return previousHash_; }
    const std::string& getHash() const noexcept { return hash_; }
    unsigned long long getNonce() const noexcept { return nonce_; }
    int getVersion() const noexcept { return version_; }
    int getDifficulty() const noexcept { return difficulty_; }
    const std::string& getTxRoot() const noexcept { return txRoot_; }
    const std::vector<Transaction>& getTransactions() const noexcept { return transactions_; }

    // setteriai
    void setNonce(unsigned long long nonce) { nonce_ = nonce; }
    void setHash(const std::string& hash) { hash_ = hash; }
    void setTxRoot(const std::string& txRoot) { txRoot_ = txRoot; }

    // patikrinimas
    // perskaičiuoja Merkle Root is dabartiniu transakciju id
    std::string recomputeTxRoot() const;
    // patikrina ar saugomas txRoot_ sutampa su perskaiciuotu
    bool verifyTxRoot() const;
    
    // merkle tree i bloku json faila log kataloge
    void printMerkleTreeStructure() const;

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
