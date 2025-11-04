#include "ledger.h"
#include <iostream>
#include <iomanip>

void Ledger::addUTXO(const std::string& txid, int index, const TxOutput& output) {
    std::string key = makeUTXOKey(txid, index);
    utxos_[key] = output;
}

bool Ledger::hasUTXO(const std::string& txid, int index) const {
    std::string key = makeUTXOKey(txid, index);
    return utxos_.find(key) != utxos_.end();
}

TxOutput Ledger::getUTXO(const std::string& txid, int index) const {
    std::string key = makeUTXOKey(txid, index);
    auto it = utxos_.find(key);
    if (it != utxos_.end()) {
        return it->second;
    }
    return TxOutput();
}

void Ledger::spendUTXO(const std::string& txid, int index) {
    std::string key = makeUTXOKey(txid, index);
    utxos_.erase(key);
}

uint64_t Ledger::getBalance(const std::string& publicKey) const {
    uint64_t total = 0;
    for (const auto& pair : utxos_) {
        if (pair.second.receiver == publicKey) {
            total += pair.second.amount;
        }
    }
    return total;
}

bool Ledger::canApply(const Transaction& tx) const {
    if (tx.isCoinbase()) return true;
    
    // UTXO validation
    uint64_t inputSum = 0;
    for (const auto& input : tx.getInputs()) {
        if (!hasUTXO(input.prevTxId, input.outputIndex)) {
            return false;
        }
        TxOutput utxo = getUTXO(input.prevTxId, input.outputIndex);
        inputSum += utxo.amount;
    }
    uint64_t outputSum = 0;
    for (const auto& output : tx.getOutputs()) {
        outputSum += output.amount;
    }
    return inputSum >= outputSum;
}

bool Ledger::canApplyWithFee(const Transaction& tx, uint64_t fee) const {
    if (tx.isCoinbase()) return true;
    
    // UTXO validation
    uint64_t inputSum = 0;
    for (const auto& input : tx.getInputs()) {
        if (!hasUTXO(input.prevTxId, input.outputIndex)) {
            return false;
        }
        TxOutput utxo = getUTXO(input.prevTxId, input.outputIndex);
        inputSum += utxo.amount;
    }
    uint64_t outputSum = 0;
    for (const auto& output : tx.getOutputs()) {
        outputSum += output.amount;
    }
    return inputSum >= (outputSum + fee);
}

bool Ledger::apply(const Transaction& tx) {
    // UTXO application
    if (!tx.isCoinbase()) {
        for (const auto& input : tx.getInputs()) {
            spendUTXO(input.prevTxId, input.outputIndex);
        }
    }
    for (size_t i = 0; i < tx.getOutputs().size(); ++i) {
        addUTXO(tx.getId(), static_cast<int>(i), tx.getOutputs()[i]);
    }
    return true;
}

bool Ledger::applyWithFee(const Transaction& tx, uint64_t fee) {
    (void)fee;
    return apply(tx);
}

bool Ledger::applyMultiple(const std::vector<Transaction>& transactions) {
    for (const auto& tx : transactions) {
        if (!apply(tx)) {
            return false;
        }
    }
    return true;
}

uint64_t Ledger::getTotalBalance() const {
    uint64_t total = 0;
    for (const auto& pair : utxos_) {
        total += pair.second.amount;
    }
    return total;
}

bool Ledger::exists(const std::string& publicKey) const {
    for (const auto& pair : utxos_) {
        if (pair.second.receiver == publicKey) {
            return true;
        }
    }
    return false;
}

void Ledger::print() const {
    std::cout << "Ledger (UTXO Model)\n";
    std::cout << "Unspent outputs: " << utxos_.size() << "\n";
    std::cout << "Total balance: " << getTotalBalance() << " coins\n";
    std::cout << "----------------------------------------\n";
}