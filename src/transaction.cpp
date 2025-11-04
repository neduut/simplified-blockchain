#include "transaction.h"
#include "ownHash.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Transaction::Transaction(const std::vector<TxInput>& inputs, const std::vector<TxOutput>& outputs)
    : id_(""), timestamp_(std::time(nullptr)), inputs_(inputs), outputs_(outputs) {
    id_ = computeId(inputs, outputs);
}

Transaction::Transaction(const std::string& from, const std::string& to, uint64_t amount)
    : id_(""), timestamp_(std::time(nullptr)) {
    inputs_.push_back(TxInput("unresolved:" + from, 0));
    outputs_.push_back(TxOutput(to, amount));
    id_ = computeId(inputs_, outputs_);
}

std::string Transaction::computeId(const std::vector<TxInput>& inputs, const std::vector<TxOutput>& outputs) {
    std::ostringstream oss;
    for (const auto& input : inputs) {
        oss << input.prevTxId << input.outputIndex;
    }
    for (const auto& output : outputs) {
        oss << output.receiver << output.amount;
    }
    return generate_hash(oss.str());
}

std::string Transaction::computeId(const Transaction& t, const std::string& salt) {
    std::ostringstream oss;
    for (const auto& input : t.inputs_) {
        oss << input.prevTxId << input.outputIndex;
    }
    for (const auto& output : t.outputs_) {
        oss << output.receiver << output.amount;
    }
    oss << salt;
    return generate_hash(oss.str());
}

bool Transaction::verifyId() const {
    std::string expected = computeId(inputs_, outputs_);
    return (id_ == expected);
}

std::string Transaction::getFrom() const {
    if (isCoinbase()) {
        return "SYSTEM";
    }
    if (!inputs_.empty() && inputs_[0].prevTxId.substr(0, 11) == "unresolved:") {
        return inputs_[0].prevTxId.substr(11);
    }
    return "";
}

std::string Transaction::getTo() const {
    if (!outputs_.empty()) {
        return outputs_[0].receiver;
    }
    return "";
}

uint64_t Transaction::getAmount() const {
    uint64_t total = 0;
    for (const auto& output : outputs_) {
        total += output.amount;
    }
    return total;
}

void Transaction::print() const {
    std::cout << "Transaction ID: " << id_.substr(0, 16) << "...\n";
    std::cout << "Timestamp: " << timestamp_ << "\n";
    if (isCoinbase()) {
        std::cout << "Type: COINBASE\n";
    } else {
        std::cout << "Inputs (" << inputs_.size() << "):\n";
        for (size_t i = 0; i < inputs_.size(); ++i) {
            std::cout << "  [" << i << "] " << inputs_[i].prevTxId.substr(0, 16) 
                      << "... : output[" << inputs_[i].outputIndex << "]\n";
        }
    }
    std::cout << "Outputs (" << outputs_.size() << "):\n";
    for (size_t i = 0; i < outputs_.size(); ++i) {
        std::cout << "  [" << i << "] " << outputs_[i].receiver.substr(0, 16) 
                  << "... : " << outputs_[i].amount << " coins\n";
    }
}
