#include "user.h"
#include <iostream>
#include <iomanip>

void User::print() const {
    std::cout << "User: " << name_ << "\n";
    std::cout << "  Public Key: " << publicKey_.substr(0, 16) << "...\n";
    std::cout << "  Balance: " << balance_ << " coins\n";
}
