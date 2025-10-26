#pragma once

#include <string>
#include <vector>

#include "ownHash.h"

// paprasta merkle medzio realizacija is tx id saraso
// kol kas nenaudojama block header'io txRoot, tik pasirengimas ateiciai (v0.2)
class MerkleTree {
public:
    // sukuria medzio lygius is pradiniu lapu (tx id string'ai)
    // jei lygis turi nelygini kieki elementu, dubliuoja paskutini
    static MerkleTree from_leaves(const std::vector<std::string>& leaves);

    // grazina sakni; jei lygiu nera, grazina tuscia string'a
    std::string root() const;

    // grazina nurodyto lygio hash'u sarasa (0 - lapai)
    const std::vector<std::vector<std::string>>& levels() const { return levels_; }

    // paprastas spausdinimas diagnostikai (neprivaloma naudoti)
    void print() const;

private:
    std::vector<std::vector<std::string>> levels_; // levels_[0] - lapai, paskutinis - saknis
};
