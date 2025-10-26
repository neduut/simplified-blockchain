#include "blockchain.h"
#include "timer.h"
#include <iostream>
#include <iomanip>

using namespace std;

void printHeader(const string& title) {
    cout << "\n" << string(60, '=') << "\n";
    cout << title << "\n";
    cout << string(60, '=') << "\n\n";
}

void testBlockchain(int difficulty, int numBlocks) {
    printHeader("TEST: Blockchain with difficulty = " + to_string(difficulty));
    
    Timer totalTimer;
    
    // Sukuriame blockchain su nurodytu difficulty
    Blockchain blockchain(difficulty);
    
    // Pridedame blokus
    for (int i = 1; i <= numBlocks; i++) {
        string data = "Block " + to_string(i) + " - Transaction data: User_" 
                     + to_string(i) + " sent " + to_string(i * 10) + " coins";
        blockchain.addBlock(data);
    }
    
    double totalTime = totalTimer.elapsed();
    
    // Statistika
    blockchain.printStatistics();
    
    cout << "Total mining time: " << fixed << setprecision(2) 
         << totalTime << " seconds\n";
    cout << "Average time per block: " << fixed << setprecision(2) 
         << totalTime / numBlocks << " seconds\n\n";
    
    // Validacija
    printHeader("BLOCKCHAIN VALIDATION");
    if (blockchain.isChainValid()) {
        cout << "✓ Blockchain is VALID!\n\n";
    } else {
        cout << "✗ Blockchain is INVALID!\n\n";
    }
    
    // Išvedame grandinę
    blockchain.printChain();
}

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════╗
║                                                           ║
║        SIMPLIFIED BLOCKCHAIN IMPLEMENTATION               ║
║                  Version 0.1                              ║
║                                                           ║
╚═══════════════════════════════════════════════════════════╝
)" << "\n";

    try {
        // Test 1: Difficulty = 2, 5 blokai
        testBlockchain(2, 5);
        
        cout << "\n" << string(60, '-') << "\n\n";
        
        // Test 2: Difficulty = 3, 5 blokai (ilgiau truks)
        cout << "Press ENTER to continue with difficulty = 3 (this will take longer)...\n";
        cin.get();
        testBlockchain(3, 5);
        
        // Galite pridėti daugiau testų
        // testBlockchain(4, 3); // Labai ilgai truks!
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Program completed successfully!\n";
    cout << string(60, '=') << "\n";
    
    return 0;
}
