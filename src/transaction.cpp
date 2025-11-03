#include "transaction.h"
#include "ownHash.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

Transaction::Transaction(const std::string& from, const std::string& to, uint64_t amount)
    : from_(from), to_(to), amount_(amount), timestamp_(std::time(nullptr)) {
    // automatiskai generuoja id
    id_ = computeId(*this);
}

std::string Transaction::computeId(const Transaction& t, const std::string& salt) {
    std::ostringstream oss;
    oss << t.from_ << t.to_ << t.amount_ << t.timestamp_ << salt;
    return generate_hash(oss.str());
}

// verifikacija - perskaiciuoja id ir lygina su saugomu
bool Transaction::verifyId() const {
    std::string recomputed = computeId(*this);
    return (id_ == recomputed);
}

std::string Transaction::toString() const {
    std::ostringstream oss;
    oss << from_ << to_ << amount_ << timestamp_;
    return oss.str();
}

void Transaction::print() const {
    std::cout << "Transaction [" << id_.substr(0, 12) << "...]\n";
    std::cout << "  From: " << from_.substr(0, 16) << "...\n";
    std::cout << "  To:   " << to_.substr(0, 16) << "...\n";
    std::cout << "  Amount: " << amount_ << " coins\n";
    std::cout << "  Time: " << timestamp_ << "\n";
}
