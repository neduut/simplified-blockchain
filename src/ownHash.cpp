#include "ownHash.h"

#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>

using std::vector;
using std::string;
using std::cout;
using std::cin;
using std::cerr;

namespace {
    // int -> base62 char
    inline char to_base62(int sk) {
        sk %= 62;
        if (sk < 0) sk += 62;
        return BASE62[sk];
    }

    // int -> hex char
    inline char to_hex(int sk) {
        constexpr char HEX[] = "0123456789abcdef";
        return HEX[sk % 16];
    }

    // maisymas pagal vertes
    void value_dependent_shuffle(vector<int>& previous) {
        if (previous.empty()) return;
        
        vector<int> temp = previous;
        const size_t n = previous.size();
        
        for (size_t i = 0; i < n; ++i) {
            int value = temp[i];
            size_t new_pos;
            
            if (value % 2 == 0) {
                size_t jump = (17 * abs(value) * 7 + i * 23) % n;
                new_pos = (i + jump) % n;
            } else {
                size_t jump = (13 * abs(value) * 11 + i * 19) % n;
                new_pos = (i + n - (jump % n)) % n;
            }
            
            previous[new_pos] = value;
        }
    }

    // vienas elementas keicia 3 kitus
    void three_in_one_mixer(vector<int>& previous) {
        if (previous.empty()) return;
        vector<int> temp = previous;
        
        for (size_t i = 0; i < temp.size(); ++i) {
            int sk = temp[i];

            size_t e1 = (i + sk) % previous.size();
            size_t e2 = (i + sk * 2) % previous.size();
            size_t e3 = (i + sk * 3) % previous.size();

            previous[e1] = (previous[e1] + sk) % 256;
            previous[e2] = (previous[e2] + sk * 2) % 256;
            previous[e3] = (previous[e3] + sk * 3) % 256;
        }
    }

    // pradzios seed
    string generate_seed(const vector<int>& current) {
        string seed = "Kx9mN3vL8qR5wY1pZ7jT2bF6hC4nA0sD";
        
        int matrix[2][2] = {{7, 13}, {11, 5}};
        
        for (size_t i = 0; i + 1 < current.size(); i += 2) {
            int a = current[i];
            int b = current[i + 1];
            
            size_t pos1 = i % seed.size();
            size_t pos2 = (i + 1) % seed.size();
            
            seed[pos1] = BASE62[(matrix[0][0] * a + matrix[0][1] * b) % 62];
            seed[pos2] = BASE62[(matrix[1][0] * a + matrix[1][1] * b) % 62];
        }
        
        if (current.size() % 2 == 1) {
            size_t last_idx = current.size() - 1;
            size_t pos = last_idx % seed.size();
            seed[pos] = BASE62[(current[last_idx] * 17 + pos * 23) % 62];
        }
        
        return seed;
    }

} // namespace

string generate_hash(const string& user_input) {
    // simboliai -> ascii kodai
    vector<int> current;
    current.reserve(user_input.size());
    for (unsigned char c : user_input) current.push_back((int)c);

    if (current.empty()) {
        current.push_back(0);
    }

    // 4 roundai
    constexpr int ROUNDS = 4;
    vector<int> data = current;
    vector<int> previous;

    for (int r = 0; r < ROUNDS; ++r) {
        previous = data;
        value_dependent_shuffle(previous);
        three_in_one_mixer(previous);

        if (previous.empty()) {
            previous.push_back(0);
        }

        data = previous;
    }

    // seed generavimas
    string seed = generate_seed(current);
    
    // salt generavimas
    string salt;
    if (!current.empty()) {
        size_t input_sum = 0;
        for (int ascii : current) input_sum += ascii;
        
        for (int i = 0; i < 4; ++i) {
            int salt_62 = (input_sum * (i + 7) + current[i % current.size()] * 13) % 62;
            salt += to_base62(salt_62);
        }
        
        // integruoju salt
        for (size_t i = 0; i < salt.size(); ++i) {
            int salt_ascii = (int)(unsigned char)salt[i];
            size_t salt_pos = (input_sum + i * salt_ascii) % seed.size();
            int seed_62 = (seed[salt_pos] + salt_ascii + input_sum) % 62;
            seed[salt_pos] = to_base62(seed_62);
        }
    }

    // maisymas su seed 
    for (size_t i = 0; i < previous.size(); ++i) {
        size_t si = i % seed.size();
        int rez = ((int)(unsigned char)seed[si] * previous[i] * (i + 1)) % 256;
        seed[si] = to_base62(rez % 62);
    }

    // 32 baitai = 256 bitu
    vector<unsigned char> bytes;
    bytes.reserve(32);
    
    for (int i = 0; i < 32; ++i) {
        int a = (int)(unsigned char)seed[i % seed.size()];
        int b = previous[i % previous.size()];
        int c = previous[(i * 3) % previous.size()];
        
        int byte_val = (a * 31 + b * 17 + c * 13) % 256;
        byte_val ^= (i * 7 + a) % 256;
        byte_val = (byte_val * 131 + 17) % 256;
        
        int seed_idx = (i * 7 + byte_val) % seed.size();
        byte_val ^= (int)(unsigned char)seed[seed_idx];
        
        bytes.push_back(static_cast<unsigned char>(byte_val % 256));
    }
    
    // diffusion - kaimynu maisymas
    for (size_t i = 0; i < bytes.size(); ++i) {
        size_t next = (i + 1) % bytes.size();
        size_t prev = (i + bytes.size() - 1) % bytes.size();
        
        int mixed = (bytes[i] + bytes[prev] * 3 + bytes[next] * 5) % 256;
        bytes[i] = static_cast<unsigned char>(mixed);
    }
    
    // 32 baitai -> 64 HEX simboliai
    string out;
    out.reserve(64);
    for (unsigned char byte : bytes) {
        out.push_back(to_hex(byte >> 4));
        out.push_back(to_hex(byte & 0x0F));
    }

    return out;
}