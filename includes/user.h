#pragma once
#include <string>
#include <cstdint>

class User {
private:
    std::string name_;
    std::string publicKey_;
    uint64_t balance_;

public:
    // konstruktoriai
    User() : name_(""), publicKey_(""), balance_(0) {}
    
    User(const std::string& name, const std::string& publicKey, uint64_t balance = 0)
        : name_(name), publicKey_(publicKey), balance_(balance) {}
    
    // Rule of Five: default 
    User(const User&) = default;
    User& operator=(const User&) = default;
    User(User&&) noexcept = default;
    User& operator=(User&&) noexcept = default;
    ~User() = default;

    // getters 
    const std::string& getName() const noexcept { return name_; }
    const std::string& getPublicKey() const noexcept { return publicKey_; }
    uint64_t getBalance() const noexcept { return balance_; }

    // setters
    void setName(const std::string& name) { name_ = name; }
    void setPublicKey(const std::string& publicKey) { publicKey_ = publicKey; }
    void setBalance(uint64_t balance) { balance_ = balance; }

    // balance operacijos
    void addBalance(uint64_t amount) { balance_ += amount; }
    bool deductBalance(uint64_t amount) {
        if (balance_ >= amount) {
            balance_ -= amount;
            return true;
        }
        return false;
    }

    // display
    void print() const;
};
