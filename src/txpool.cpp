#include "txpool.h"
#include "ledger.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <chrono>
#include <unordered_set>

void TxPool::addTransaction(const Transaction& tx) {
    pool_.push_back(tx);
}

std::vector<Transaction> TxPool::takeRandom(size_t n) {
    std::vector<Transaction> result;
    
    if (pool_.empty()) {
        return result;
    }
    
    // neima daugiau nei yra
    size_t count = std::min(n, pool_.size());
    
    // sukuria indeksu masyva
    std::vector<size_t> indices(pool_.size());
    for (size_t i = 0; i < pool_.size(); ++i) {
        indices[i] = i;
    }
    
    // maiso indeksus
    unsigned seed = static_cast<unsigned>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );
    std::shuffle(indices.begin(), indices.end(), std::default_random_engine(seed));
    
    // ima pirmus count indeksu
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.push_back(pool_[indices[i]]);
    }
    
    return result;
}

void TxPool::eraseByIds(const std::vector<std::string>& ids) {
    if (ids.empty()) return;
    
    // use unordered_set for O(1) lookup instead of O(n) with std::find
    std::unordered_set<std::string> idSet(ids.begin(), ids.end());
    
    // remove all transactions whose ID is in the set
    pool_.erase(
        std::remove_if(pool_.begin(), pool_.end(),
            [&idSet](const Transaction& tx) {
                return idSet.count(tx.getId()) > 0;
            }),
        pool_.end()
    );
}

size_t TxPool::removeInvalid(const Ledger& ledger, uint64_t txFee) {
    size_t initialSize = pool_.size();
    
    // remove all transactions that cannot be applied (invalid UTXO inputs)
    pool_.erase(
        std::remove_if(pool_.begin(), pool_.end(),
            [&ledger, txFee](const Transaction& tx) {
                return !ledger.canApplyWithFee(tx, txFee);
            }),
        pool_.end()
    );
    
    return initialSize - pool_.size(); // number of removed transactions
}

void TxPool::print() const {
    std::cout << "Transaction Pool (size: " << pool_.size() << ")\n";
    std::cout << "----------------------------------------\n";
    
    if (pool_.empty()) {
        std::cout << "  (empty)\n";
        return;
    }
    
    for (size_t i = 0; i < std::min(size_t(5), pool_.size()); ++i) {
        std::cout << "  [" << i + 1 << "] ";
        std::cout << pool_[i].getFrom().substr(0, 8) << "... → ";
        std::cout << pool_[i].getTo().substr(0, 8) << "... : ";
        std::cout << pool_[i].getAmount() << " coins\n";
    }
    
    if (pool_.size() > 5) {
        std::cout << "  ... and " << (pool_.size() - 5) << " more\n";
    }
}
