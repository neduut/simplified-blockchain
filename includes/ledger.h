#pragma once
#include "transaction.h"
#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>

// balansai apskaicuojami is unspent transaction outputs (UTXO)
class Ledger {
private:
    // UTXO storage: key = "txid:outputIndex" -> TxOutput
    std::unordered_map<std::string, TxOutput> utxos_;
    
    // helper to create UTXO key
    static std::string makeUTXOKey(const std::string& txid, int index) {
        return txid + ":" + std::to_string(index);
    }

public:
    // konstruktorius
    Ledger() = default;
    
    // Rule of Five: default (naudoja tik std::unordered_map)
    Ledger(const Ledger&) = default;
    Ledger& operator=(const Ledger&) = default;
    Ledger(Ledger&&) noexcept = default;
    Ledger& operator=(Ledger&&) noexcept = default;
    ~Ledger() = default;
    
    // ========== UTXO methods ==========
    
    // add a new UTXO
    void addUTXO(const std::string& txid, int index, const TxOutput& output);
    
    // check if UTXO exists and is unspent
    bool hasUTXO(const std::string& txid, int index) const;
    
    // get UTXO data
    TxOutput getUTXO(const std::string& txid, int index) const;
    
    // spend (remove) a UTXO
    void spendUTXO(const std::string& txid, int index);
    
    // get balance calculated from UTXOs for a specific address
    uint64_t getBalance(const std::string& publicKey) const;
    
    // validate UTXO transaction
    bool canApply(const Transaction& tx) const;
    bool canApplyWithFee(const Transaction& tx, uint64_t fee) const;
    
    // apply UTXO transaction (spend inputs, create outputs)
    bool apply(const Transaction& tx);
    bool applyWithFee(const Transaction& tx, uint64_t fee);
    
    // apply multiple transactions
    bool applyMultiple(const std::vector<Transaction>& transactions);
    
    // get total balance of all UTXOs
    uint64_t getTotalBalance() const;
    
    // get total unspent outputs count
    size_t getUTXOCount() const noexcept { return utxos_.size(); }
    
    // get all UTXOs (for transaction generation)
    const std::unordered_map<std::string, TxOutput>& getAllUTXOs() const noexcept { return utxos_; }
    
    // check if address has any UTXOs
    bool exists(const std::string& publicKey) const;

    // display
    void print() const;
};
