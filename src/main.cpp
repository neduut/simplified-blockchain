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

// query funkcijos menu
void queryBlock(const Blockchain& blockchain) {
    int blockNum;
    cout << "Enter block number (0-" << blockchain.getChain().size() - 1 << "): ";
    cin >> blockNum;
    cin.ignore();
    
    const auto& chain = blockchain.getChain();
    if (blockNum >= 0 && blockNum < (int)chain.size()) {
        cout << "\n" << string(60, '=') << "\n";
        cout << "BLOCK #" << blockNum << " DETAILS\n";
        cout << string(60, '=') << "\n";
        chain[blockNum].printBlock();
    } else {
        cout << "Invalid block number!\n";
    }
}

void queryTransaction(const TxPool& pool) {
    string txId;
    cout << "Enter transaction ID (hash): ";
    getline(cin, txId);
    
    const auto& allTxs = pool.getAll();
    bool found = false;
    for (const auto& tx : allTxs) {
        if (tx.getId() == txId) {
            cout << "\n" << string(60, '=') << "\n";
            cout << "TRANSACTION DETAILS\n";
            cout << string(60, '=') << "\n";
            tx.print();
            found = true;
            break;
        }
    }
    
    if (!found) {
        cout << "Transaction not found!\n";
    }
}

// v0.1 Test: Blokai su transakcijomis
void testTransactionBlocks(Blockchain& blockchain, TxPool& pool, vector<User>& users) {
    printHeader("TEST v0.1: Transaction System");
    
    Ledger ledger;
    
    // sukuria ~1000 vartotoju
    cout << "Generating ~1000 users...\n";
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
    
    // rodo pirmus 5 vartotojus kaip pavyzdi konsolej
    cout << "\nSample users (first 5):\n";
    cout << string(40, '-') << "\n";
    for (size_t i = 0; i < min(size_t(5), users.size()); ++i) {
        cout << "  " << users[i].getName() << " : " 
             << users[i].getPublicKey().substr(0, 16) << "... : "
             << users[i].getBalance() << " coins\n";
    }
    cout << "  ... and " << (users.size() - 5) << " more\n";
    
    // sukuria transaction pool
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
    
    // iraso visas transakcijas i merkle_log.txt
    const auto& allTxs = pool.getAll();
    ofstream txLog("merkle_log.txt");
    if (txLog.is_open()) {
        txLog << "==========================================================\n";
        txLog << "ALL TRANSACTIONS (Total: " << allTxs.size() << ")\n";
        txLog << "==========================================================\n\n";
        for (size_t i = 0; i < allTxs.size(); ++i) {
            const auto& tx = allTxs[i];
            txLog << "[" << (i+1) << "] TX ID: " << tx.getId() << "\n";
            txLog << "    From   : " << tx.getFrom() << "\n";
            txLog << "    To     : " << tx.getTo() << "\n";
            txLog << "    Amount : " << tx.getAmount() << " coins\n\n";
        }
        txLog.close();
        cout << "All transactions saved to merkle_log.txt\n";
    }
    
    // konsolėje rodyti tik santrauką
    cout << "\nTransaction Pool (size: " << allTxs.size() << ")\n";
    cout << string(40, '-') << "\n";
    cout << "  Showing first 5 transactions:\n";
    for (size_t i = 0; i < min(size_t(5), allTxs.size()); ++i) {
        const auto& tx = allTxs[i];
        cout << "  [" << (i+1) << "] " 
             << tx.getFrom().substr(0, 8) << "... -> "
             << tx.getTo().substr(0, 8) << "... : "
             << tx.getAmount() << " coins\n";
    }
    if (allTxs.size() > 5) {
        cout << "  ... and " << (allTxs.size() - 5) << " more\n";
    }
    cout << "  (See merkle_log.txt for all transactions)\n\n";
    
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
    
    // rodyti tik santrauka, ne visus balansus
    cout << "Final balance summary:\n";
    cout << "  Total coins in system: " << ledger.getTotalBalance() << "\n";
    cout << "  Balances updated for " << users.size() << " users\n";
    cout << "  (Use Ledger::print() to see all individual balances)\n\n";
    
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
        
        // inicializuojam blockchain, pool ir users
        Blockchain blockchain(3);
        TxPool pool;
        vector<User> users;
        
        // test 1: transaction system
        testTransactionBlocks(blockchain, pool, users);
        
        // interaktyvus query meniu
        printHeader("Query System");
        cout << "You can now query blocks and transactions.\n\n";
        
        while (true) {
            cout << "\nOptions:\n";
            cout << "  1 - Query block by number\n";
            cout << "  2 - Query transaction by ID\n";
            cout << "  0 - Exit\n";
            cout << "Choose option: ";
            
            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(10000, '\n');
                cout << "Invalid input! Please enter a number.\n";
                continue;
            }
            cin.ignore();
            
            if (choice == 0) {
                break;
            } else if (choice == 1) {
                queryBlock(blockchain);
            } else if (choice == 2) {
                queryTransaction(pool);
            } else {
                cout << "Invalid option!\n";
            }
        }
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Program completed successfully!\n";
    cout << string(60, '=') << "\n";
    
    return 0;
}
