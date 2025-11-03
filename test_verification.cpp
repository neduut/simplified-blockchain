#include "transaction.h"
#include "ownHash.h"
#include <iostream>

// Mini testas transakcijų ID verifikacijai
int main() {
    std::cout << "==============================================\n";
    std::cout << "Transaction Verification Test\n";
    std::cout << "==============================================\n\n";
    
    // 1. Sukuriame teisingą transakciją
    Transaction validTx("sender_pubkey_123", "receiver_pubkey_456", 100);
    
    std::cout << "Test 1: Valid transaction\n";
    std::cout << "  TX ID: " << validTx.getId().substr(0, 16) << "...\n";
    std::cout << "  verifyId() = " << (validTx.verifyId() ? "PASS" : "FAIL") << "\n\n";
    
    // 2. Sukuriame transakciją ir pakeičiame ID rankiniu būdu (sugadinimas)
    Transaction corruptedTx("sender_abc", "receiver_xyz", 250);
    std::string originalId = corruptedTx.getId();
    
    std::cout << "Test 2: Corrupted transaction (modified ID)\n";
    std::cout << "  Original ID: " << originalId.substr(0, 16) << "...\n";
    
    // Pakeičiam ID į kažką kita
    corruptedTx.setId("fake_hash_12345678");
    std::cout << "  Modified ID: " << corruptedTx.getId().substr(0, 16) << "...\n";
    std::cout << "  verifyId() = " << (corruptedTx.verifyId() ? "PASS" : "FAIL") << "\n\n";
    
    // 3. Patikrinam, kad recompute grąžina teisingą ID
    std::string recomputed = Transaction::computeId(corruptedTx);
    std::cout << "Test 3: Recomputed ID check\n";
    std::cout << "  Recomputed ID: " << recomputed.substr(0, 16) << "...\n";
    std::cout << "  Matches original? " << (recomputed == originalId ? "YES" : "NO") << "\n\n";
    
    std::cout << "==============================================\n";
    std::cout << "All tests completed!\n";
    std::cout << "==============================================\n";
    
    return 0;
}
