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

    // getters
    std::string getId() const { return id_; }
    std::string getFrom() const { return from_; }
    std::string getTo() const { return to_; }
    uint64_t getAmount() const { return amount_; }
    std::time_t getTimestamp() const { return timestamp_; }

    // setters
    void setId(const std::string& id) { id_ = id; }

    // static metodas - apskaiciuoja transaction id naudojant ownhash
    static std::string computeId(const Transaction& t, const std::string& salt = "");

    // serializacija i string (hash generavimui)
    std::string toString() const;

    // display
    void print() const;
};
