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
#include <cstdlib>
#include <set>
#ifdef _WIN32
#include <direct.h>
#endif

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
    // disabled text logging to logs/blockchain_log.txt per request
    return;
}

// sukuria logs kataloga jei jo nera (Windows)
static void ensure_logs_dir() {
#ifdef _WIN32
    _mkdir("logs");
#else
    // galima pridet POSIX mkdir, bet projekte fokusuojuos i Windows
#endif
}

// paprasti blokai su data
void testSimpleBlocks(int difficulty, int numBlocks) {
    printHeader("Simple Blocks Test (difficulty = " + to_string(difficulty) + ")");
    
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
    
    // issami bloko info per uzklausu meniu
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
        
        // paklausti del merkle tree pavaizdavimo
        if (chain[blockNum].getVersion() == 2 && !chain[blockNum].getTransactions().empty()) {
            cout << "Show Merkle tree structure for this block? (y/n): ";
            char choice;
            cin >> choice;
            cin.ignore();
            if (choice == 'y' || choice == 'Y') {
                chain[blockNum].printMerkleTreeStructure();
            }
        }
    } else {
        cout << "Invalid block number!\n";
    }
}

static string to_lower_copy(string s) {
    for (auto& ch : s) ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    return s;
}

// palaiko paieska ne tik pagal id bet ir pagal prefiksa
void queryTransactionInteractive(const Blockchain& blockchain, const TxPool& pool) {
    string input;
    cout << "Enter transaction ID or prefix: ";
    getline(cin, input);
    if (input.empty()) {
        cout << "Empty input.\n";
        return;
    }
    string q = to_lower_copy(input);

    struct TxRef {
        const Transaction* tx;
        string source;
        int blockIndex; // -1 if from pool
        size_t txIndex;
    };

    vector<TxRef> matches;

    // 1) Paieška pool'e (nepatvirtintos)
    const auto& poolTxs = pool.getAll();
    for (size_t i = 0; i < poolTxs.size(); ++i) {
        string id = to_lower_copy(poolTxs[i].getId());
        if (id.rfind(q, 0) == 0) { // prefix match
            matches.push_back(TxRef{&poolTxs[i], "pool", -1, i});
        }
    }

    // 2) Paieška blokų grandinėje (patvirtintos)
    const auto& chain = blockchain.getChain();
    for (const auto& block : chain) {
        if (block.getVersion() != 2) continue;
        const auto& txs = block.getTransactions();
        for (size_t i = 0; i < txs.size(); ++i) {
            string id = to_lower_copy(txs[i].getId());
            if (id.rfind(q, 0) == 0) { // prefix match
                matches.push_back(TxRef{&txs[i], "block", block.getIndex(), i});
            }
        }
    }

    if (matches.empty()) {
        cout << "No transactions found by prefix: '" << input << "'\n";
        return;
    }

    if (matches.size() == 1) {
        cout << "\n" << string(60, '=') << "\n";
        cout << "TRANSACTION DETAILS\n";
        cout << string(60, '=') << "\n";
        cout << "Source: " << (matches[0].blockIndex >= 0 ? ("block #" + to_string(matches[0].blockIndex)) : string("pool")) << "\n";
        matches[0].tx->print();
        return;
    }

    // rodo sarasa su atitikmenim
    cout << "Found " << matches.size() << " transactions. Showing first 20:\n";
    size_t shown = min<size_t>(20, matches.size());
    for (size_t i = 0; i < shown; ++i) {
        cout << "  [" << (i + 1) << "] "
             << matches[i].tx->getId().substr(0, 16) << "...  "
             << (matches[i].blockIndex >= 0 ? ("block #" + to_string(matches[i].blockIndex)) : string("pool"))
             << "  " << matches[i].tx->getFrom().substr(0, 8) << "... -> "
             << matches[i].tx->getTo().substr(0, 8) << "...  amt="
             << matches[i].tx->getAmount() << "\n";
    }
    if (matches.size() > shown) {
        cout << "  ... and " << (matches.size() - shown) << " more\n";
    }
    cout << "Choose number to view details (0 to cancel): ";
    int pick = 0;
    if (!(cin >> pick)) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid input.\n";
        return;
    }
    cin.ignore();
    if (pick <= 0 || pick > static_cast<int>(shown)) return;
    const auto& choice = matches[static_cast<size_t>(pick - 1)];
    cout << "\n" << string(60, '=') << "\n";
    cout << "TRANSACTION DETAILS\n";
    cout << string(60, '=') << "\n";
    cout << "Source: " << (choice.blockIndex >= 0 ? ("block #" + to_string(choice.blockIndex)) : string("pool")) << "\n";
    choice.tx->print();
}

// pagrindine funkcija
void runBlockchainSimulation(Blockchain& blockchain, TxPool& pool, vector<User>& users) {
    printHeader("Transaction System");
    
        Ledger ledger;
        
        // sukuria 1000 vartotoju
        cout << "Generating 1000 users...\n";
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> balanceDist(100, 1000000);
        
        // sukuria vartotojus su random balansu
        for (int i = 0; i < 1000; ++i) {
            string name = "User_" + to_string(i);
            string pubKey = generatePublicKey(name + to_string(i));
            uint64_t balance = balanceDist(gen);
            users.emplace_back(name, pubKey, balance);
            
            // sukuria pradini UTXO ledgery
            std::string initTxId = "initial:" + pubKey;
            TxOutput initOutput(pubKey, balance);
            ledger.addUTXO(initTxId, 0, initOutput);
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
        
    // sukuria transaction pool - generate ~10000 transactions upfront for all blocks
    cout << "\nGenerating ~10000 UTXO transactions...\n";
    
    // random generators for transaction generation
    uniform_int_distribution<> userDist(0, users.size() - 1);
    uniform_int_distribution<> amountDist(10, 1000);
    
    int txGenerated = 0;
    int attempts = 0;
    const int TARGET_TX = 10000;
    const int MAX_ATTEMPTS = 15000; 
    
    while (txGenerated < TARGET_TX && attempts < MAX_ATTEMPTS) {
        attempts++;
        
        int fromIdx = userDist(gen);
        int toIdx = userDist(gen);
        
        while (toIdx == fromIdx) {
            toIdx = userDist(gen);
        }
        
        string sender = users[fromIdx].getPublicKey();
        string receiver = users[toIdx].getPublicKey();
        uint64_t amount = amountDist(gen);
        
        // find sender's first available UTXO (any UTXO they own)
        std::string txId;
        int outputIdx = -1;
        uint64_t utxoAmount = 0;
        
        // search for any sender's UTXO (allows duplicates - mining will filter)
        const auto& allUtxos = ledger.getAllUTXOs();
        for (const auto& [key, output] : allUtxos) {
            if (output.receiver == sender) {
                // parse key "txid:index"
                // find last colon (in case txid contains colons like "initial:<addr>")
                size_t colonPos = key.rfind(':');
                if (colonPos != std::string::npos) {
                    txId = key.substr(0, colonPos);
                    try {
                        outputIdx = std::stoi(key.substr(colonPos + 1));
                        utxoAmount = output.amount;
                        // check if this UTXO has enough for the transaction
                        const uint64_t TX_FEE = 1;
                        if (utxoAmount >= amount + TX_FEE) {
                            break; // found suitable UTXO
                        } else {
                            outputIdx = -1; // not enough, keep searching
                        }
                    } catch (...) {
                        continue; // skip malformed keys
                    }
                }
            }
        }
        
        if (outputIdx == -1) {
            continue; // no suitable UTXO found
        }
        
        // create UTXO transaction
        std::vector<TxInput> inputs;
        std::vector<TxOutput> outputs;
        
        // input: reference to sender's UTXO
        inputs.push_back(TxInput(txId, outputIdx));
        
        // output 1: amount to receiver
        outputs.push_back(TxOutput(receiver, amount));
        
        // output 2: change back to sender (accounting for TX_FEE = 1)
        const uint64_t TX_FEE = 1;
        uint64_t change = utxoAmount - amount - TX_FEE;
        if (change > 0) {
            outputs.push_back(TxOutput(sender, change));
        }
        
        Transaction tx(inputs, outputs);
        pool.addTransaction(tx);
        txGenerated++;
    }
    
    cout << "Generated " << pool.size() << " valid UTXO transactions\n";
    
    // validate transaction ID correctness (proof for assignment)
        // optional diagnostics: Transaction ID validation sample
        bool showTests = false;
        if (const char* envTests = std::getenv("SHOW_TESTS")) {
            showTests = (std::string(envTests) == "1" || std::string(envTests) == "true");
        }
        const auto& allTxs = pool.getAll();
        if (showTests) {
            cout << "\n=== Transaction ID Validation (Sample) ===\n";
            int validatedCount = 0;
            int sampleSize = min(size_t(10), allTxs.size());
            for (size_t i = 0; i < sampleSize; ++i) {
                const auto& tx = allTxs[i];
                bool valid = tx.verifyId();
                if (valid) validatedCount++;
                if (i < 3) { // show first 3 in detail
                    cout << "  TX #" << (i+1) << ": " << tx.getId().substr(0, 16) << "... ";
                    cout << (valid ? "\u2713 VALID" : "\u2717 INVALID") << "\n";
                }
            }
            cout << "  Validated " << validatedCount << "/" << sampleSize << " sample transactions\n";
            cout << "  All " << pool.size() << " transactions have cryptographic IDs: ID = hash(from + to + amount + timestamp)\n";
        }
    
    // konsolej tik santrauka
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
    cout << "\n";
    
    // kasa blokus su kandidatais (v0.2: decentralizuotas kasimas)
    printHeader("Mining Blocks with Decentralized Process");
    
    Timer totalTimer;
    int blocksToMine = 5;
    if (const char* envBlocks = std::getenv("BLOCKS_TO_MINE")) {
        try {
            int v = std::stoi(envBlocks);
            if (v > 0) blocksToMine = v;
        } catch (...) {}
    }
    int successfulBlocks = 0;
    
    for (int i = 0; i < blocksToMine; ++i) {
        cout << "\n========== Mining Block #" << i + 1 << " ==========\n";
        
        // paralelinis kandidatu kasimas: 5 threads kasa 5 kandidatus vienu metu
        // galima naudoti tik laiko limita, tik bandymu limita, arba abu:
        // - 5.0, 0 = tik 5s limitas
        // - 0.0, 50000 = tik 50k bandymų limitas
        // - 5.0, 50000 = 5s ARBA 50k bandymu (kuris pirmas)
        if (blockchain.mineCandidateBlocksParallel(pool, ledger, 100, 5, 5.0, 0)) {
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
    
    // UTXO model statistics
    cout << "Final UTXO summary:\n";
    cout << "  Total coins in system: " << ledger.getTotalBalance() << "\n";
    cout << "  Unspent Outputs (UTXO count): " << ledger.getUTXOCount() << "\n";
    cout << "  Active addresses with UTXOs: " << users.size() << "\n";
    cout << "  (Use Ledger::print() to see all UTXO details)\n\n";
    // miner earnings (fees + block rewards)
    cout << "  Miner (fees + rewards) [MINER_FEE]: " << ledger.getBalance("MINER_FEE") << " coins\n\n";
    
    printHeader("Blockchain Validation");
    blockchain.printStatistics();
    
    if (blockchain.isChainValid()) {
        cout << "Blockchain is VALID!\n\n";
    } else {
        cout << "Blockchain is INVALID!\n\n";
    }
    
    // Ssppressed full chain dump; view block details via Query menu option 1.
    
        // detailed mining stats only when diagnostics are enabled
        if (showTests) {
            blockchain.printDetailedStatistics();
        }
}

int main() {
    try {
        // ensure logs folder, then pazymi sesijos pradzia log faile
        ensure_logs_dir();
        log_session_start();
        
        // inicializuoja blockchain, pool ir users
        Blockchain blockchain(3);
        TxPool pool;
        vector<User> users;
        
        // Main simulation: setup and mine blocks
        runBlockchainSimulation(blockchain, pool, users);
        
    // JSON eksportas: atskiri failai kiekvienam blokui tiesiai i logs/
        bool exportLogs = false;
        if (const char* envExport = std::getenv("EXPORT_LOGS")) {
            exportLogs = (std::string(envExport) == "1" || std::string(envExport) == "true");
        }
        if (exportLogs) {
            blockchain.exportBlocksToJsonDir("logs");
        }
        
        // interaktyvus query meniu
        printHeader("Query System");
    cout << "You can now query blocks and transactions.\n\n";
        
        while (true) {
            cout << "\nOptions:\n";
            cout << "  1 - Query block by number\n";
            cout << "  2 - Query transaction by ID/prefix\n";
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
                queryTransactionInteractive(blockchain, pool);
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
