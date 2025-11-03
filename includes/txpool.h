#pragma once
#include "transaction.h"
#include <vector>
#include <string>
#include <cstddef>

class TxPool {
private:
    std::vector<Transaction> pool_;

public:
    TxPool() = default;
    
    // Rule of Five: default 
    TxPool(const TxPool&) = default;
    TxPool& operator=(const TxPool&) = default;
    TxPool(TxPool&&) noexcept = default;
    TxPool& operator=(TxPool&&) noexcept = default;
    ~TxPool() = default;

    // prideti transakcija i pool
    void addTransaction(const Transaction& tx);

    // pasiimti iki n atsitiktiniu transakciju be pakartojimų
    std::vector<Transaction> takeRandom(size_t n);

    // istrinti transakcijas pagal id
    void eraseByIds(const std::vector<std::string>& ids);

    // grazinti pool dydi 
    size_t size() const noexcept { return pool_.size(); }

    // patikrinti ar pool tuscias 
    bool empty() const noexcept { return pool_.empty(); }

    // isvalyti visa pool 
    void clear() noexcept { pool_.clear(); }

    // Gauti visas transakcijas 
    const std::vector<Transaction>& getAll() const noexcept { return pool_; }

    // Display
    void print() const;
};
