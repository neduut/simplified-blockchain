#include "ledger.h"
#include <iostream>
#include <iomanip>

void Ledger::setBalance(const std::string& publicKey, uint64_t balance) {
    balances_[publicKey] = balance;
}

uint64_t Ledger::getBalance(const std::string& publicKey) const {
    auto it = balances_.find(publicKey);
    if (it != balances_.end()) {
        return it->second;
    }
    return 0; // jei saskaita neegzistuoja, balansas 0
}

bool Ledger::canApply(const Transaction& tx) const {
    // paprasta validacija - tikrina ar uztenka balanco
    uint64_t senderBalance = getBalance(tx.getFrom());
    return senderBalance >= tx.getAmount();
}

void Ledger::apply(const Transaction& tx) {
    // mazinam siuntejo balansa
    if (balances_.find(tx.getFrom()) != balances_.end()) {
        balances_[tx.getFrom()] -= tx.getAmount();
    }
    
    // didinam gavejo balansa
    balances_[tx.getTo()] += tx.getAmount();
}

bool Ledger::applyMultiple(const std::vector<Transaction>& transactions) {
    // patikrinam ar visos transakcijos validzios
    for (const auto& tx : transactions) {
        if (!canApply(tx)) {
            return false;
        }
    }
    
    // jei visos validzios, pritaikom
    for (const auto& tx : transactions) {
        apply(tx);
    }
    
    return true;
}

bool Ledger::exists(const std::string& publicKey) const {
    return balances_.find(publicKey) != balances_.end();
}

uint64_t Ledger::getTotalBalance() const {
    uint64_t total = 0;
    for (const auto& pair : balances_) {
        total += pair.second;
    }
    return total;
}

void Ledger::print() const {
    std::cout << "Ledger (Total: " << getTotalBalance() << " coins)\n";
    std::cout << "----------------------------------------\n";
    
    if (balances_.empty()) {
        std::cout << "  (empty)\n";
        return;
    }
    
    for (const auto& pair : balances_) {
        std::cout << "  " << pair.first.substr(0, 16) << "... : ";
        std::cout << std::setw(8) << pair.second << " coins\n";
    }
}
