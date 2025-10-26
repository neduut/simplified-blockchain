#pragma once
#include "transaction.h"
#include <vector>
#include <string>
#include <cstddef>

class TxPool {
private:
    std::vector<Transaction> pool_;

public:
public:
    TxPool() = default;

    // prideti transakcija i pool
    void addTransaction(const Transaction& tx);

    // pasiimti iki n atsitiktiniu transakciju be pakartojimų
    std::vector<Transaction> takeRandom(size_t n);

    // istrinti transakcijas pagal id
    void eraseByIds(const std::vector<std::string>& ids);

    // grazinti pool dydi
    size_t size() const { return pool_.size(); }

    // patikrinti ar pool tuscias
    bool empty() const { return pool_.empty(); }

    // isvalyti visa pool
    void clear() { pool_.clear(); }

    // Gauti visas transakcijas (const)
    const std::vector<Transaction>& getAll() const { return pool_; }

    // Display
    void print() const;
};
