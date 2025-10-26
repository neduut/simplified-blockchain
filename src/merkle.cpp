#include "merkle.h"

#include <iostream>

// sukuria visus lygius nuo lapu iki saknies
MerkleTree MerkleTree::from_leaves(const std::vector<std::string>& leaves) {
    MerkleTree tree;
    if (leaves.empty()) {
        return tree;
    }

    tree.levels_.push_back(leaves); // 0-asis lygis - lapai

    // kuria aukstesnius lygius, kol liks vienas elementas (saknis)
    while (tree.levels_.back().size() > 1) {
        const auto& cur = tree.levels_.back();
        std::vector<std::string> next;
        next.reserve((cur.size() + 1) / 2);

        size_t i = 0;
        while (i < cur.size()) {
            const std::string& left = cur[i];
            const std::string& right = (i + 1 < cur.size()) ? cur[i + 1] : cur[i]; // dubliuojam paskutini jei nelyginis
            // naudoja bendra taisykle: hash(left + right)
            next.push_back(generate_hash(left + right));
            i += 2;
        }
        tree.levels_.push_back(std::move(next));
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
