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

using namespace std;

void printHeader(const string& title) {
    cout << "\n" << string(60, '=') << "\n";
    cout << title << "\n";
    cout << string(60, '=') << "\n\n";
}

// Generuoja atsitiktinį public key
string generatePublicKey(const string& name) {
    return generate_hash(name + to_string(time(nullptr)));
}

// v0.1 Test: Paprasti blokai su data
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
        cout << "✓ Blockchain is VALID!\n\n";
    } else {
        cout << "✗ Blockchain is INVALID!\n\n";
    }
    
    blockchain.printChain();
}

// v0.1 Test: Blokai su transakcijomis
void testTransactionBlocks() {
    printHeader("TEST v0.1: Transaction System");
    
    Blockchain blockchain(2);
    Ledger ledger;
    
    // Sukuriame vartotojus
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
    
    // Sukuriame transaction pool
    TxPool pool;
    
    // Generuojame transakcijas
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
    
    // Kasame blokus
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
    
    // Rezultatai
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
        cout << "✅ Blockchain is VALID!\n\n";
    } else {
        cout << "❌ Blockchain is INVALID!\n\n";
    }
    
    blockchain.printChain();
}

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════╗
║                                                           ║
║     SIMPLIFIED BLOCKCHAIN - Transaction System           ║
║                   Version 0.1                             ║
║                                                           ║
╚═══════════════════════════════════════════════════════════╝
)" << "\n";

    try {
        // Test 1: Transaction system (v0.1 išplėsta versija)
        testTransactionBlocks();
        
        cout << "\n" << string(60, '-') << "\n\n";
        
        // Test 2: Simple blocks (backward compatibility)
        cout << "Press ENTER to test simple blocks (backward compatibility)...\n";
        cin.get();
        testSimpleBlocks(2, 3);
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Program completed successfully!\n";
    cout << string(60, '=') << "\n";
    
    return 0;
}
