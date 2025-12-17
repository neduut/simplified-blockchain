#pragma once
#include <string>
#include <cstdint>
#include <ctime>
#include <vector>

// === UTXO_MODEL_STRUCTURES ===
// v0.2: UTXO model
// TxInput references a previous transaction's output
struct TxInput {
    std::string prevTxId;   // previous transaction ID
    int outputIndex;        // which output of that transaction
    
    TxInput() : prevTxId(""), outputIndex(0) {}
    TxInput(const std::string& txId, int idx) : prevTxId(txId), outputIndex(idx) {}
};

// TxOutput specifies receiver and amount
struct TxOutput {
    std::string receiver;   // public key
    uint64_t amount;        // amount
    
    TxOutput() : receiver(""), amount(0) {}
    TxOutput(const std::string& recv, uint64_t amt) : receiver(recv), amount(amt) {}
};

class Transaction {
private:
    std::string id_;        // unique transaction id (hash)
    std::time_t timestamp_; // when created
    
    // UTXO model: inputs and outputs
    std::vector<TxInput> inputs_;
    std::vector<TxOutput> outputs_;

public:
    // constructors
    Transaction() : id_(""), timestamp_(0) {}
    
    // direct UTXO constructor
    Transaction(const std::vector<TxInput>& inputs, const std::vector<TxOutput>& outputs);
    
    // compatibility constructor: creates a simple UTXO transaction
    Transaction(const std::string& from, const std::string& to, uint64_t amount);
    
    // Rule of Five: default 
    Transaction(const Transaction&) = default;
    Transaction& operator=(const Transaction&) = default;
    Transaction(Transaction&&) noexcept = default;
    Transaction& operator=(Transaction&&) noexcept = default;
    ~Transaction() = default;

    // getters
    const std::string& getId() const noexcept { return id_; }
    std::time_t getTimestamp() const noexcept { return timestamp_; }
    const std::vector<TxInput>& getInputs() const noexcept { return inputs_; }
    const std::vector<TxOutput>& getOutputs() const noexcept { return outputs_; }
    
    // compatibility getters for old code
    // returns the first output receiver (or empty if coinbase)
    std::string getFrom() const;
    // returns the first output receiver
    std::string getTo() const;
    // returns sum of all outputs
    uint64_t getAmount() const;
    
    // check if transaction is coinbase (no inputs)
    bool isCoinbase() const noexcept { 
        return inputs_.empty() || 
               (!inputs_.empty() && inputs_[0].prevTxId == "coinbase"); 
    }

    // setters
    void setId(const std::string& id) { id_ = id; }

    // compute transaction ID from inputs and outputs
    static std::string computeId(const std::vector<TxInput>& inputs, const std::vector<TxOutput>& outputs);
    
    // for compatibility with old code
    static std::string computeId(const Transaction& t, const std::string& salt = "");

    // verify ID matches computed hash
    bool verifyId() const;

    // display
    void print() const;
};
