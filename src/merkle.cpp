#include "merkle.h"

#include <iostream>
#include <cassert>

// adaptuota create_merkle() funkcija is libbitcoin
// naudoja std::vector<std::string> vietoj bc::hash_list
// modifikuoja ivesta vektoriu vietoje (mutating function)
std::string create_merkle_adapted(std::vector<std::string> merkle_hashes) {
    // Stop if hash list is empty or contains one element
    if (merkle_hashes.empty()) {
        return std::string(); // grazina tuscia string'a vietoj bc::null_hash
    }
    else if (merkle_hashes.size() == 1) {
        return merkle_hashes[0];
    }

    // While there is more than 1 hash in the list, keep looping...
    while (merkle_hashes.size() > 1) {
        // If number of hashes is odd, duplicate last hash in the list.
        if (merkle_hashes.size() % 2 != 0) {
            merkle_hashes.push_back(merkle_hashes.back());
        }
        // List size is now even.
        assert(merkle_hashes.size() % 2 == 0);

        // New hash list.
        std::vector<std::string> new_merkle;
        new_merkle.reserve(merkle_hashes.size() / 2);

        // Loop through hashes 2 at a time.
        for (size_t i = 0; i < merkle_hashes.size(); i += 2) {
            // Join both current hashes together (concatenate).
            const std::string& left = merkle_hashes[i];
            const std::string& right = merkle_hashes[i + 1];
            
            // Hash both of the hashes (dvigubas hash'inimas kaip Bitcoin protokole)
            std::string new_root = generate_hash(left + right);
            
            // Add this to the new list.
            new_merkle.push_back(new_root);
        }

        // This is the new list.
        merkle_hashes = std::move(new_merkle);
    }

    // Finally we end up with a single item.
    return merkle_hashes[0];
}

// sukuria visus lygius nuo lapu iki saknies
// dabar naudoja adaptuota create_merkle() algoritma
MerkleTree MerkleTree::from_leaves(const std::vector<std::string>& leaves) {
    MerkleTree tree;
    if (leaves.empty()) {
        return tree;
    }

    tree.levels_.push_back(leaves); // 0-asis lygis - lapai

    // naudoja adaptuota create_merkle logika su lygiu sekimu
    std::vector<std::string> current_level = leaves;
    
    // kuria aukstesnius lygius, kol liks vienas elementas (saknis)
    while (current_level.size() > 1) {
        // If number of hashes is odd, duplicate last hash in the list (Bitcoin taisykle)
        if (current_level.size() % 2 != 0) {
            current_level.push_back(current_level.back());
        }
        
        std::vector<std::string> next_level;
        next_level.reserve(current_level.size() / 2);

        // Loop through hashes 2 at a time (poromis, kaip create_merkle)
        for (size_t i = 0; i < current_level.size(); i += 2) {
            const std::string& left = current_level[i];
            const std::string& right = current_level[i + 1];
            // Hash both of the hashes together
            next_level.push_back(generate_hash(left + right));
        }
        
        tree.levels_.push_back(next_level);
        current_level = std::move(next_level);
    }

    return tree;
}

std::string MerkleTree::root() const {
    if (levels_.empty()) return std::string();
    const auto& top = levels_.back();
    return top.empty() ? std::string() : top.front();
}

void MerkleTree::print() const {
    std::cout << "Merkle Tree lygiai (0 - lapai):\n";
    for (size_t lvl = 0; lvl < levels_.size(); ++lvl) {
        std::cout << "lvl " << lvl << ": ";
        for (size_t j = 0; j < levels_[lvl].size(); ++j) {
            const auto& h = levels_[lvl][j];
            std::cout << h << (j + 1 < levels_[lvl].size() ? "," : "");
        }
        std::cout << "\n";
    }
}
