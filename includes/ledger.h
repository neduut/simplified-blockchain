#pragma once
#include "transaction.h"
#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>

class Ledger {
private:
    std::unordered_map<std::string, uint64_t> balances_;
    // UTXO mode
    struct UTXO {
        std::string id;         // unique id (txid:index or GENESIS_owner)
        std::string owner;      // public key
        uint64_t amount{0};
        bool spent{false};
    };
    bool utxoEnabled_ { false };
    // utxo id -> utxo
    std::unordered_map<std::string, UTXO> utxos_;
    // owner -> list of utxo ids for quick selection
    std::unordered_map<std::string, std::vector<std::string>> ownerIndex_;

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

    // ===== UTXO mode API =====
    void enableUTXO(bool enabled = true) noexcept { utxoEnabled_ = enabled; }
    bool isUTXOEnabled() const noexcept { return utxoEnabled_; }
    // Initialize UTXO set from current balances (one UTXO per account)
    void initializeUTXOFromBalances();

    // Selection result for spending
    struct UTXOSelection {
        std::vector<std::string> inputs; // utxo ids used
        uint64_t totalIn{0};
        uint64_t change{0};
    };

    // Deterministically select UTXOs to cover 'needed' amount
    bool selectUTXO(const std::string& owner, uint64_t needed, UTXOSelection& outSel) const;

    // UTXO-based checks and application with fee
    bool canApplyWithFeeUTXO(const Transaction& tx, uint64_t fee) const;
    bool applyWithFeeUTXO(const Transaction& tx, const std::string& feeCollector, uint64_t fee);
};
