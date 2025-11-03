#include "ledger.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

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

// ===== UTXO mode implementation =====

void Ledger::initializeUTXOFromBalances() {
    utxos_.clear();
    ownerIndex_.clear();
    if (!utxoEnabled_) return;
    for (const auto& p : balances_) {
        const std::string& owner = p.first;
        uint64_t amount = p.second;
        if (amount == 0) continue;
        std::string utxoId = std::string("GENESIS_") + owner;
        UTXO u{ utxoId, owner, amount, false };
        utxos_[utxoId] = u;
        ownerIndex_[owner].push_back(utxoId);
    }
}

bool Ledger::selectUTXO(const std::string& owner, uint64_t needed, UTXOSelection& outSel) const {
    outSel = {};
    auto it = ownerIndex_.find(owner);
    if (it == ownerIndex_.end()) return false;
    std::vector<std::pair<std::string, uint64_t>> candidates; // id, amount
    candidates.reserve(it->second.size());
    for (const auto& id : it->second) {
        auto utxoIt = utxos_.find(id);
        if (utxoIt == utxos_.end()) continue;
        const auto& u = utxoIt->second;
        if (!u.spent && u.amount > 0) {
            candidates.emplace_back(id, u.amount);
        }
    }
    if (candidates.empty()) return false;
    // deterministic: smallest-first selection
    std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b){
        if (a.second != b.second) return a.second < b.second;
        return a.first < b.first; // tie-breaker by id
    });
    uint64_t acc = 0;
    for (auto& kv : candidates) {
        outSel.inputs.push_back(kv.first);
        acc += kv.second;
        if (acc >= needed) break;
    }
    if (acc < needed) {
        outSel = {};
        return false;
    }
    outSel.totalIn = acc;
    outSel.change = acc - needed;
    return true;
}

bool Ledger::canApplyWithFeeUTXO(const Transaction& tx, uint64_t fee) const {
    if (!utxoEnabled_) return canApplyWithFee(tx, fee);
    uint64_t needed = tx.getAmount() + fee;
    UTXOSelection sel;
    return selectUTXO(tx.getFrom(), needed, sel);
}

bool Ledger::applyWithFeeUTXO(const Transaction& tx, const std::string& feeCollector, uint64_t fee) {
    if (!utxoEnabled_) return applyWithFee(tx, feeCollector, fee);
    uint64_t needed = tx.getAmount() + fee;
    UTXOSelection sel;
    if (!selectUTXO(tx.getFrom(), needed, sel)) return false;

    // consume inputs
    for (const auto& id : sel.inputs) {
        auto it = utxos_.find(id);
        if (it == utxos_.end() || it->second.spent) return false; // should not happen
        it->second.spent = true;
    }

    // create outputs: receiver amount, fee to miner, change back to sender
    // new ids can be derived deterministically from tx id and counters
    std::string base = tx.getId();
    // to receiver
    std::string outToId = base + ":0";
    utxos_[outToId] = UTXO{ outToId, tx.getTo(), tx.getAmount(), false };
    ownerIndex_[tx.getTo()].push_back(outToId);
    // fee to miner
    std::string outFeeId = base + ":1";
    utxos_[outFeeId] = UTXO{ outFeeId, feeCollector, fee, false };
    ownerIndex_[feeCollector].push_back(outFeeId);
    // change (if any) back to sender
    if (sel.change > 0) {
        std::string outChId = base + ":2";
        utxos_[outChId] = UTXO{ outChId, tx.getFrom(), sel.change, false };
        ownerIndex_[tx.getFrom()].push_back(outChId);
    }

    // keep balances_ in sync with UTXO outputs (conservative: sum unchanged)
    uint64_t senderBalance = getBalance(tx.getFrom());
    // subtract totalIn from sender, then credit change back, and outputs to recipients
    if (senderBalance >= sel.totalIn) {
        balances_[tx.getFrom()] = senderBalance - sel.totalIn;
    } else {
        // fallback (shouldn't happen): clamp to zero before change re-credit
        balances_[tx.getFrom()] = 0;
    }
    // credit outputs
    balances_[tx.getFrom()] += sel.change;
    balances_[tx.getTo()] += tx.getAmount();
    balances_[feeCollector] += fee;

    return true;
}
