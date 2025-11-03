#pragma once
#include <string>
#include <cstdint>
#include <ctime>

class Transaction {
private:
    std::string id_;// unikalus transakcijos id (hash)
    std::string from_; // siuntejo public key
    std::string to_;// gavejo public key
    uint64_t amount_;// suma
    std::time_t timestamp_; // kada sukurta

public:
    // konstruktoriai
    Transaction() : id_(""), from_(""), to_(""), amount_(0), timestamp_(0) {}
    
    Transaction(const std::string& from, const std::string& to, uint64_t amount);
    
    // Rule of Five: default 
    Transaction(const Transaction&) = default;
    Transaction& operator=(const Transaction&) = default;
    Transaction(Transaction&&) noexcept = default;
    Transaction& operator=(Transaction&&) noexcept = default;
    ~Transaction() = default;

    // getters 
    const std::string& getId() const noexcept { return id_; }
    const std::string& getFrom() const noexcept { return from_; }
    const std::string& getTo() const noexcept { return to_; }
    uint64_t getAmount() const noexcept { return amount_; }
    std::time_t getTimestamp() const noexcept { return timestamp_; }

    // setters
    void setId(const std::string& id) { id_ = id; }

    // static metodas - apskaiciuoja transaction id naudojant ownhash
    static std::string computeId(const Transaction& t, const std::string& salt = "");

    // perskaičiuoja id is lauku ir patikrina, ar sutampa su saugomu id_
    bool verifyId() const;

    // serializacija i string (hash generavimui)
    std::string toString() const;

    // display
    void print() const;
};
