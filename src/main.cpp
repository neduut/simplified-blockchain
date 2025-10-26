#include "blockchain.h"
#include "user.h"
#include "transaction.h"
#include "txpool.h"
#include "ledger.h"
#include "ownHash.h"
#include "timer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <fstream>
#include <sstream>

using namespace std;

void printHeader(const string& title) {
    cout << "\n" << string(60, '=') << "\n";
    cout << title << "\n";
    cout << string(60, '=') << "\n\n";
}

// generuoja random public key
string generatePublicKey(const string& name) {
    return generate_hash(name + to_string(time(nullptr)));
}

// pagalbinis: dabartinis utc laikas kaip tekstas
static string utc_now_str() {
    std::time_t t = std::time(nullptr);
    std::tm* g = std::gmtime(&t);
    std::ostringstream oss;
    if (g) {
        oss << std::put_time(g, "%Y-%m-%d %H:%M:%S");
    }
    return oss.str();
}

// i log faila pazymim sesijos pradzia su date_utc
static void log_session_start() {
    std::ofstream file("blockchain_log.txt", std::ios::app);
    if (!file.is_open()) return;
    file << std::string(40, ' ') << "\n";
    file << std::string(40, '-') << "\n";
    file << "date_utc : " << utc_now_str() << "\n";
    file << std::string(40, '-') << "\n";
}

// paprasti blokai su data
void testSimpleBlocks(int difficulty, int numBlocks) {
    printHeader("TEST v0.1: Simple Blocks (difficulty = " + to_string(difficulty) + ")");
    
    Timer totalTimer;
    Blockchain blockchain(difficulty);
    
    for (int i = 1; i <= numBlocks; i++) {
        string data = "Block " + to_string(i) + " - Transaction data: User_" 
                     + to_string(i) + " sent " + to_string(i * 10) + " coins";
        blockchain.addBlock(data);
    }
    
    double totalTime = totalTimer.elapsed();
    
    blockchain.printStatistics();
    cout << "Total mining time: " << fixed << setprecision(2) 
         << totalTime << " seconds\n";
    cout << "Average time per block: " << fixed << setprecision(2) 
         << totalTime / numBlocks << " seconds\n\n";
    
    printHeader("BLOCKCHAIN VALIDATION");
    if (blockchain.isChainValid()) {
        cout << "Blockchain is VALID!\n\n";
    } else {
        cout << "Blockchain is INVALID!\n\n";
    }
    
    blockchain.printChain();
}

// v0.1 Test: Blokai su transakcijomis
void testTransactionBlocks() {
    printHeader("TEST v0.1: Transaction System");
    
    Blockchain blockchain(3);
    Ledger ledger;
    
    // sukuria vartotojus
    cout << "Creating users...\n";
    vector<User> users;
    users.emplace_back("Alice", generatePublicKey("Alice"), 1000);
    users.emplace_back("Bob", generatePublicKey("Bob"), 800);
    users.emplace_back("Charlie", generatePublicKey("Charlie"), 1200);
    users.emplace_back("Diana", generatePublicKey("Diana"), 500);
    users.emplace_back("Eve", generatePublicKey("Eve"), 950);
    
    for (const auto& user : users) {
        ledger.setBalance(user.getPublicKey(), user.getBalance());
    }
    
    cout << "\nInitial balances:\n";
    ledger.print();
    
    // sukuria transaction pool
    TxPool pool;
    
    // generuoja transakcijas
    cout << "\nGenerating random transactions...\n";
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> userDist(0, users.size() - 1);
    uniform_int_distribution<> amountDist(10, 100);
    
    for (int i = 0; i < 250; ++i) {
        int fromIdx = userDist(gen);
        int toIdx = userDist(gen);
        
        while (toIdx == fromIdx) {
            toIdx = userDist(gen);
        }
        
        uint64_t amount = amountDist(gen);
        Transaction tx(users[fromIdx].getPublicKey(), 
                      users[toIdx].getPublicKey(), 
                      amount);
        pool.addTransaction(tx);
    }
    
    cout << "Generated " << pool.size() << " transactions\n\n";
    pool.print();
    
    // kasa blokus
    printHeader("Mining Blocks with Transactions");
    
    Timer totalTimer;
    int blocksToMine = 3;
    int successfulBlocks = 0;
    
    for (int i = 0; i < blocksToMine; ++i) {
        cout << "\n--- Mining Block #" << i + 1 << " ---\n";
        
        if (blockchain.formBlockFromPool(pool, ledger, 80)) {
            successfulBlocks++;
        } else {
            cout << "Failed to mine block\n";
            break;
        }
    }
    
    double totalTime = totalTimer.elapsed();
    
    // rezultatai
    printHeader("Results");
    
    cout << "Total mining time: " << fixed << setprecision(2) 
         << totalTime << " seconds\n";
    cout << "Blocks mined: " << successfulBlocks << "\n";
    cout << "Transactions remaining in pool: " << pool.size() << "\n\n";
    
    cout << "Final balances:\n";
    ledger.print();
    
    printHeader("Blockchain Validation");
    blockchain.printStatistics();
    
    if (blockchain.isChainValid()) {
        cout << "Blockchain is VALID!\n\n";
    } else {
        cout << "Blockchain is INVALID!\n\n";
    }
    
    blockchain.printChain();
}

int main() {
    try {
        // pazymim sesijos pradzia log faile
        log_session_start();
        // test 1: transaction system
        testTransactionBlocks();
        
        cout << "\n" << string(60, '-') << "\n\n";
        
    // test 2: simple blocks (backward compatibility)
        cout << "Press ENTER to test simple blocks (backward compatibility)...\n";
        cin.get();
    testSimpleBlocks(3, 3);
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Program completed successfully!\n";
    cout << string(60, '=') << "\n";
    
    return 0;
}
