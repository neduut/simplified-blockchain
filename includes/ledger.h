#pragma once
#include "transaction.h"
#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>

class Ledger {
private:
    std::unordered_map<std::string, uint64_t> balances_;

public:
    // konstruktorius
    Ledger() = default;
    
    // Rule of Five: default (naudoja tik std::unordered_map)
    Ledger(const Ledger&) = default;
    Ledger& operator=(const Ledger&) = default;
    Ledger(Ledger&&) noexcept = default;
    Ledger& operator=(Ledger&&) noexcept = default;
    ~Ledger() = default;

    // nustatyti pradini balansa
    void setBalance(const std::string& publicKey, uint64_t balance);

    // gauti balansa 
    uint64_t getBalance(const std::string& publicKey) const noexcept;

    // patikrinti ar transakcija galima pritaikyti
    bool canApply(const Transaction& tx) const;
    // patikrina ar transakcija galima pritaikyti su mokesciu
    bool canApplyWithFee(const Transaction& tx, uint64_t fee) const;

    // pritaikyti transakcija (mazina sender, didina receiver) - grazina true jei pritaike
    bool apply(const Transaction& tx);
    // pritaiko transakcija su mokesciu; mokestis pervedamas i feeCollector
    bool applyWithFee(const Transaction& tx, const std::string& feeCollector, uint64_t fee);

    // pritaikyti kelias transakcijas
    bool applyMultiple(const std::vector<Transaction>& transactions);

    // patikrinti ar saskaita egzistuoja
    bool exists(const std::string& publicKey) const;

    // gauti visu balansu suma
    uint64_t getTotalBalance() const;

    // display
    void print() const;
};
