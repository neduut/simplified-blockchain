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

    // nustatyti pradini balansa
    void setBalance(const std::string& publicKey, uint64_t balance);

    // gauti balansa
    uint64_t getBalance(const std::string& publicKey) const;

    // patikrinti ar transakcija galima pritaikyti
    bool canApply(const Transaction& tx) const;

    // pritaikyti transakcija (mazina sender, didina receiver)
    void apply(const Transaction& tx);

    // pritaikyti kelias transakcijas
    bool applyMultiple(const std::vector<Transaction>& transactions);

    // patikrinti ar saskaita egzistuoja
    bool exists(const std::string& publicKey) const;

    // gauti visu balansu suma
    uint64_t getTotalBalance() const;

    // display
    void print() const;
};
