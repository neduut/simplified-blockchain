#include "ledger.h"
#include <iostream>
#include <iomanip>

void Ledger::setBalance(const std::string& publicKey, uint64_t balance) {
    balances_[publicKey] = balance;
}

uint64_t Ledger::getBalance(const std::string& publicKey) const noexcept {
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

bool Ledger::canApplyWithFee(const Transaction& tx, uint64_t fee) const {
    uint64_t senderBalance = getBalance(tx.getFrom());
    // reikia padengti suma + mokesti
    return senderBalance >= (tx.getAmount() + fee);
}

bool Ledger::apply(const Transaction& tx) {
    // apsauga nuo underflow, jei balanso neuztenka - nieko nedarom
    uint64_t senderBalance = getBalance(tx.getFrom());
    if (senderBalance < tx.getAmount()) {
        return false;
    }

    // nustato nauja siuntejo balansa be kurimo pagal nutylejima
    balances_[tx.getFrom()] = senderBalance - tx.getAmount();
    
    // didina gavejo balansa (jei nera, sukurs su 0)
    balances_[tx.getTo()] += tx.getAmount();
    return true;
}

bool Ledger::applyWithFee(const Transaction& tx, const std::string& feeCollector, uint64_t fee) {
    uint64_t senderBalance = getBalance(tx.getFrom());
    uint64_t totalOut = tx.getAmount() + fee;
    if (senderBalance < totalOut) {
        return false;
    }
    // nuskaiciuojam suma + mokesti nuo siuntejo
    balances_[tx.getFrom()] = senderBalance - totalOut;
    // pervedam pagrindine suma gavejui
    balances_[tx.getTo()] += tx.getAmount();
    // mokestis atitenka feeCollector (miner)
    balances_[feeCollector] += fee;
    return true;
}

bool Ledger::applyMultiple(const std::vector<Transaction>& transactions) {
    // patikrina ar visos transakcijos validzios
    for (const auto& tx : transactions) {
        if (!canApply(tx)) {
            return false;
        }
    }
    
    // jei visos validzios, pritaiko
    for (const auto& tx : transactions) {
        // kadangi jau patikrino, turetu visada buti true
        (void)apply(tx);
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
