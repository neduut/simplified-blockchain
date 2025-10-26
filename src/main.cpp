#include "blockchain.h"
#include "user.h"
#include "transaction.h"
#include "txpool.h"
#include "ledger.h"
#include "ownHash.h"
#include "timer.h"
#include "merkle.h"
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
    
    // sukuria ~1000 vartotoju
    cout << "Generating ~1000 users...\n";
    vector<User> users;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> balanceDist(100, 1000000);
    
    for (int i = 0; i < 1000; ++i) {
        string name = "User_" + to_string(i);
        string pubKey = generatePublicKey(name + to_string(i));
        uint64_t balance = balanceDist(gen);
        users.emplace_back(name, pubKey, balance);
        ledger.setBalance(pubKey, balance);
    }
    
    cout << "Created " << users.size() << " users\n";
    cout << "Total coins in system: " << ledger.getTotalBalance() << "\n";
    
    // sukuria transaction pool
    TxPool pool;
    
    // generuoja ~10000 transakciju
    cout << "\nGenerating ~10000 transactions...\n";
    uniform_int_distribution<> userDist(0, users.size() - 1);
    uniform_int_distribution<> amountDist(10, 1000);
    
    for (int i = 0; i < 10000; ++i) {
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
    
    cout << "Generated " << pool.size() << " transactions\n";
    cout << "(Showing first 5 for brevity)\n";
    
    // rodyti tik pirmas 5 transakcijas
    size_t origSize = pool.size();
    cout << "\nTransaction Pool (size: " << origSize << ")\n";
    cout << string(40, '-') << "\n";
    const auto& allTxs = pool.getAll();
    for (size_t i = 0; i < min(size_t(5), allTxs.size()); ++i) {
        const auto& tx = allTxs[i];
        cout << "  [" << (i+1) << "] " 
             << tx.getFrom().substr(0, 8) << "... -> "
             << tx.getTo().substr(0, 8) << "... : "
             << tx.getAmount() << " coins\n";
    }
    if (origSize > 5) {
        cout << "  ... and " << (origSize - 5) << " more\n";
    }
    cout << "\n";
    
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

    // test 3: merkle tree demo
        cout << "\nPress ENTER to test Merkle Tree...\n";
        cin.get();
        printHeader("TEST v0.1: Merkle Tree Demo");
        
        // keletas fake tx id
        vector<string> txIds = {
            "tx001_abcdef1234567890",
            "tx002_1234567890abcdef",
            "tx003_fedcba0987654321",
            "tx004_0987654321fedcba",
            "tx005_aabbccddeeff1122"
        };
        
        cout << "Sukuriamas Merkle Tree is " << txIds.size() << " transaction ID:\n";
        for (size_t i = 0; i < txIds.size(); ++i) {
            cout << "  [" << i << "] " << txIds[i] << "\n";
        }
        cout << "\n";
        
        MerkleTree mt = MerkleTree::from_leaves(txIds);
        mt.print();
        
        cout << "\nMerkle Root: " << mt.root() << "\n";
        cout << "(Pastaba: Merkle Tree nera integruotas i Block txRoot - tai bus v0.2)\n";
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Program completed successfully!\n";
    cout << string(60, '=') << "\n";
    
    return 0;
}
