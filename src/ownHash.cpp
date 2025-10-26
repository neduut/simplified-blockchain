#include "ownHash.h"
#include "constants.h"

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
    // pavercia sveika skaiciu i base62 simboli
    inline char to_base62(int sk) {
        sk %= 62;
        if (sk < 0) sk += 62;
        return BASE62[sk];
    }

    // pavercia skaičių į HEX simbolį (0-9, a-f)
    inline char to_hex(int sk) {
        constexpr char HEX[] = "0123456789abcdef";
        return HEX[sk % 16];
    }

    // elementai maisomi priklausomai nuo ju vertes
    void value_dependent_shuffle(vector<int>& previous) {
        if (previous.empty()) return;
        
        vector<int> temp = previous; // kopija
        const size_t n = previous.size();
        
        for (size_t i = 0; i < n; ++i) {
            int value = temp[i];
            size_t new_pos;
            
            if (value % 2 == 0) {  // lyginis - keliauja i prieki
                size_t jump = (17 * abs(value) * 7 + i * 23) % n; // pozicijos itaka
                new_pos = (i + jump) % n;
            } else {  // nelyginis - keliauja atgal
                size_t jump = (13 * abs(value) * 11 + i * 19) % n;  // pozicijos itaka
                new_pos = (i + n - (jump % n)) % n;  // apsauga nuo underflow
            }
            
            previous[new_pos] = value;
        }
    }

    // pagrindinis masymas - kiekvienas elementas paveiks 3 kitus
    void three_in_one_mixer(vector<int>& previous) {
        if (previous.empty()) return;
        vector<int> temp = previous; // kopija, kad turet senas reiksmes
        
        for (size_t i = 0; i < temp.size(); ++i) {
            int sk = temp[i];

            size_t e1 = (i + sk) % previous.size();
            size_t e2 = (i + sk * 2) % previous.size();
            size_t e3 = (i + sk * 3) % previous.size();

            // grazinu i 0-255 intervala
            previous[e1] = (previous[e1] + sk) % 256;
            previous[e2] = (previous[e2] + sk * 2) % 256;
            previous[e3] = (previous[e3] + sk * 3) % 256;
        }
    }

    // generuoju seed priklausomai nuo input simboliu
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
        
        // jei liko vienas elementas nelyginiame masyve
        if (current.size() % 2 == 1) {
            size_t last_idx = current.size() - 1;
            size_t pos = last_idx % seed.size();
            seed[pos] = BASE62[(current[last_idx] * 17 + pos * 23) % 62];
        }
        
        return seed;
    }

} // namespace

string generate_hash(const string& user_input) {
    // 1) kiekviena simboli paverciu i ascii koda
    vector<int> current;
    current.reserve(user_input.size());
    for (unsigned char c : user_input) current.push_back((int)c);

    // jei ivestis tuscia, pridedu nuli kad hash vis tiek butu sugeneruotas
    if (current.empty()) {
        current.push_back(0);
    }

    // 2) ivairiausi maisymai hash generavimo vyksta 4 roundai 
    constexpr int ROUNDS = 4;

    // dabartinio roundo duomenys
    vector<int> data = current;

    // paskutinio raundo duomenu kopija
    vector<int> previous;

    for (int r = 0; r < ROUNDS; ++r) {
        // a) naudojam originalius duomenis be papildomo padding
        previous = data;

        // b) maisymas priklausomai nuo elemento reiksmes - efektyvus diffusion
        value_dependent_shuffle(previous);

        // c) maisymas kur vienas elementas paveikia kitus 3 - stipriausias efektas
        three_in_one_mixer(previous);

        // jei tuscia ivestis
        if (previous.empty()) {
            previous.push_back(0);
        }

        // kitas raundas naudos dabartini masyva
        data = previous;
    }

    // 3) generuoju seed priklausomai nuo input simboliu
    string seed = generate_seed(current);
    
    // 4) salt generavimas ir integravimas
    string salt;
    if (!current.empty()) {
        // salt generuojamas is input charakteristiku
        size_t input_sum = 0;
        for (int ascii : current) input_sum += ascii;
        
        // generuoju 4 simboliu salt priklausomai nuo input
        for (int i = 0; i < 4; ++i) {
            int salt_62 = (input_sum * (i + 7) + current[i % current.size()] * 13) % 62;
            salt += to_base62(salt_62);
        }
        
        // integruoju salt i seed
        for (size_t i = 0; i < salt.size(); ++i) {
            int salt_ascii = (int)(unsigned char)salt[i];
            size_t salt_pos = (input_sum + i * salt_ascii) % seed.size();
            int seed_62 = (seed[salt_pos] + salt_ascii + input_sum) % 62;
            seed[salt_pos] = to_base62(seed_62);
        }
    }

    // 5) maisymas su seed 
    for (size_t i = 0; i < previous.size(); ++i) {
        size_t si = i % seed.size(); // seed indeksa sukame ratu
        int rez = ((int)(unsigned char)seed[si] * previous[i] * (i + 1)) % 256; // daugyba + pozicijos poveikis + mod 256
        seed[si] = to_base62(rez % 62); // mod 62 kad griztu i base62 simboli
    }

    // 6) sukuriu galutini hash – 32 baitus (256 bitų), tada konvertuoju į 64 HEX simbolius
    vector<unsigned char> bytes;
    bytes.reserve(32);
    
    // Generuojame 32 baitus su geresniu maišymu
    for (int i = 0; i < 32; ++i) {
        int a = (int)(unsigned char)seed[i % seed.size()];
        int b = previous[i % previous.size()];
        int c = previous[(i * 3) % previous.size()];
        
        // Sudėtingesnis maisymas - kiekvienas baitas priklauso nuo kelių šaltinių
        int byte_val = (a * 31 + b * 17 + c * 13) % 256;
        byte_val ^= (i * 7 + a) % 256;  // XOR su pozicija
        byte_val = (byte_val * 131 + 17) % 256;  // Papildomas maišymas su pirminiu skaičiumi
        
        // Dar vienas XOR su seed
        int seed_idx = (i * 7 + byte_val) % seed.size();
        byte_val ^= (int)(unsigned char)seed[seed_idx];
        
        bytes.push_back(static_cast<unsigned char>(byte_val % 256));
    }
    
    // Papildomas diffusion layer - kiekvienas baitas paveikia kaimynus
    for (size_t i = 0; i < bytes.size(); ++i) {
        size_t next = (i + 1) % bytes.size();
        size_t prev = (i + bytes.size() - 1) % bytes.size();
        
        int mixed = (bytes[i] + bytes[prev] * 3 + bytes[next] * 5) % 256;
        bytes[i] = static_cast<unsigned char>(mixed);
    }
    
    // Konvertuojame 32 baitus į 64 HEX simbolius
    string out;
    out.reserve(64);
    for (unsigned char byte : bytes) {
        out.push_back(to_hex(byte >> 4));    // aukštesnysis nibble (4 bitai)
        out.push_back(to_hex(byte & 0x0F));  // žemesnysis nibble (4 bitai)
    }

    return out;
}