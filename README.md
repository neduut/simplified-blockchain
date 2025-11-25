# 3-oji (papildoma) užduotis: Bitcoin transakcijų ir blokų analizė su `Libbitcoin` ir `python-bitcoinlib`

WSL2 yra DAUG lėtesnis nei realus Linux, todėl visi instaliavimai vyko labaaai ilgai.

---

## 1 DALIS: Merkle medžio implementacija su Libbitcoin 

<details>
 <summary><strong>Libbitcoin-System įdiegimas</strong></summary>

### Žingsnis 1: Bandymas Windows aplinkoje (NEPAVYKO)

Pirma bandžiau įdiegti Windows 11 su Visual Studio 2022.

Bandžiau per:
- NuGet paketų valdymą
- vcpkg paketų sistemą

**Rezultatas:**

```bash
# NuGet paieška
> Install-Package libbitcoin-system
Error: No packages found

# vcpkg bandymas
> vcpkg install libbitcoin-system:x64-windows
Error: libbitcoin-system does not exist
```

**Kas nutiko:** libbitcoin daugiau nepalaiko Windows. Visi paketai pašalinti iš NuGet ir vcpkg.

**Ką padariau:** Persijungiau į Linux per WSL2.


### Žingsnis 2: WSL2 Ubuntu paruošimas (PAVYKO)

Reikėjo paruošti Linux aplinką kad galėčiau viską sukompiliuoti.

**Veiksmai:**

```bash
# Įdiegiau Ubuntu 24.04 LTS per WSL2
wsl --install -d Ubuntu-24.04

# Atnaujinau sistemą
sudo apt update
sudo apt upgrade -y
```

**Rezultatas:** [PAVYKO] Sistema paruošta, galiu pradėti diegimą.


### Žingsnis 3: Automatinis diegimas su `install.sh` (NEPAVYKO)

Norėjau paleisti oficialų diegimo skriptą, kad viskas įsidiegtų automatiškai.

**Komanda:**

```bash
sudo ./install.sh --build-boost --build-secp256k1
```

**Procesas:**
1. [PAVYKO] Boost 1.86 kompiliaciją pradėjau ir sėkmingai užbaigiau
2. [NEPAVYKO] secp256k1 klonavimas iš GitHub nepavyko:

```
Cloning https://github.com/bitcoin-core/secp256k1.git...
fatal: unable to access 'https://github.com/bitcoin-core/secp256k1.git/': 
       The requested URL returned error: 500 Internal Server Error
```

**Kas nutiko:** GitHub grąžino klaidą (HTTP 500) keletą kartų iš eilės. Automatinis atsisiuntimas neveikė.

**Ką padariau:** Nusprendžiau atsisiųsti ir įdiegti secp256k1 rankiniu būdu.


### Žingsnis 4: Rankinis secp256k1 diegimas (PAVYKO)

Norint apeiti GitHub problemą, atsisiųsiau ZIP failą ir įdiegiau rankiniu būdu.

**Veiksmai:**

```bash
# Atsisiųsti kodą rankiniu būdu
wget https://github.com/bitcoin-core/secp256k1/archive/refs/heads/master.zip
unzip master.zip
cd secp256k1-master

# Kompiliuoti ir įdiegti
./autogen.sh
./configure --enable-module-recovery
make
sudo make install
sudo ldconfig
```

**Rezultatas:** [PAVYKO] secp256k1 biblioteką įdiegiau į `/home/neda/local/lib/libsecp256k1.so`


### Žingsnis 5: Boost kompiliacija su visais gijomis (NEPAVYKO)

Bandžiau sukompiliuoti Boost su įprastais nustatymais.

**Komanda:**

```bash
sudo ./install.sh --build-boost
```

**Procesas:**
- Boost 1.86 kompiliacija pradėta su visomis gijomis (`-j$(nproc)`)
- Kompiliacija progresavo iki ~40%

**Gedimas:**

```
g++: internal compiler error: Killed (program cc1plus)
[Terminated]
```

Terminal užsidarė, WSL procesai sustabdyti.

**Kas nutiko:** 
- `dmesg` parodė: `Out of memory: Killed process`
- WSL2 turi tik 8GB RAM, o Boost kompiliacijai reikia ~12GB

**Ką supratau:** Reikia pridėti swap atmintį (virtualią atmintį).


### Žingsnis 6: 8GB Swap failo sukūrimas (PAVYKO)

Pridėjau 8GB virtualios atminties (swap), kad WSL2 turėtų pakankamai RAM.

**Veiksmai:**

```bash
# Sukurti 8GB swap failą
sudo fallocate -l 8G /swapfile

# Nustatyti leidimus (tik root)
sudo chmod 600 /swapfile

# Inicializuoti swap
sudo mkswap /swapfile

# Aktyvuoti swap
sudo swapon /swapfile

# Patikrinti
free -h
#              total        used        free      shared  buff/cache   available
# Mem:          7.7Gi       2.1Gi       4.2Gi       0.1Gi       1.4Gi       5.3Gi
# Swap:         8.0Gi          0B       8.0Gi
```

**Rezultatas:** [PAVYKO] WSL2 dabar turi 8GB + 8GB = 16GB bendrą atmintį.


### Žingsnis 7: Boost perkompiliavimas su mažiau gijų (PAVYKO)

Bandžiau dar kartą, bet šį kartą sumažinau paralelių procesų skaičių, kad RAM naudojimas būtų mažesnis.

**Komanda:**

```bash
PARALLEL=4 PREFIX=$HOME/local ./install.sh --build-boost
```

**Kaip vyko:**
- Boost kompiliacija su tik 4 gijomis (vietoj 24)
- Užtruko ilgiau (~20 min), bet atmintis naudojosi stabiliai
- Kompiliacija užsibaigė be crash'ų

**Rezultatas:**
```
Building libbitcoin-system...
[100%] Built target bitcoin-system
Running tests...
Test project /home/user/libbitcoin-system/build
      Start  1: blockchain_tests
      ...
      Start 4064: version_tests
100% tests passed, 0 tests failed out of 4064
```

**[PAVYKO] Visos 4064 testai praėjo sėkmingai!**


### Žingsnis 8: Diegimas į sistemą su sudo (PAVYKO)

Dabar reikėjo įdiegti sukompiliuotas bibliotekas į sistemą (`/usr/local/`).

**Problema:**

```bash
make install
# Error: Permission denied — cannot write to /home/neda/local/lib/
```

**Sprendimas:**

```bash
cd build-libbitcoin-system/libbitcoin-system
sudo make install
```

**Rezultatas:** [PAVYKO] Bibliotekas įdiegiau:
- `/home/neda/local/lib/libbitcoin-system.so`
- `/home/neda/local/include/bitcoin/system/`
- `/home/neda/local/lib/pkgconfig/libbitcoin-system.pc`


### Žingsnis 9: Patikrinimas (VEIKIA)

Į Ubuntu terminalą įvedžiau:
`pkg-config --cflags --libs libbitcoin-system`

Gavau:
`home/neda/local/include -L/usr/local/lib -lbitcoin-system -L/home/neda/local/lib -lboost_iostreams -lboost_locale -lboost_program_options -lboost_thread -lboost_url -lpthread -lrt -ldl -lsecp256k1`

Išvada: `libbitcoin-system` biblioteka instaliuot sėkmingai.

</details>

---

<details>
 <summary><strong>1.2 Užduoty pateiktos create_merkle() funkcijos analizė</strong></summary>

`create_merkle()` funkcija realizuoja Merkle tree konstrukciją pagal Bitcoin protokolo specifikaciją. Funkcija priima transakcijų hash'ų sąrašą (`bc::hash_list`) ir grąžina vieną hash'ą – Merkle root, naudojamą bloko header'yje.

#### Funkcijos kodas:

```cpp
//merkle.cpp
#include <bitcoin/bitcoin.hpp>

// Merkle Root Hash
bc::hash_digest create_merkle(bc::hash_list& merkle) {
    // Stop if hash list is empty or contains one element
    if (merkle.empty())
        return bc::null_hash;
    else if (merkle.size() == 1)
        return merkle[0];

    // While there is more than 1 hash in the list, keep looping...
    while (merkle.size() > 1)
    {
        // If number of hashes is odd, duplicate last hash in the list.
        if (merkle.size() % 2 != 0)
            merkle.push_back(merkle.back());
        // List size is now even.
        assert(merkle.size() % 2 == 0);

        // New hash list.
        bc::hash_list new_merkle;
        // Loop through hashes 2 at a time.
        for (auto it = merkle.begin(); it != merkle.end(); it += 2)
        {
            // Join both current hashes together (concatenate).
            bc::data_chunk concat_data(bc::hash_size * 2);
            auto concat = bc::serializer<
                decltype(concat_data.begin())>(concat_data.begin());
            concat.write_hash(*it);
            concat.write_hash(*(it + 1));
            // Hash both of the hashes.
            bc::hash_digest new_root = bc::bitcoin_hash(concat_data);
            // Add this to the new list.
            new_merkle.push_back(new_root);
        }
        // This is the new list.
        merkle = new_merkle;

        // DEBUG output
        std::cout << "Current merkle hash list:" << std::endl;
        for (const auto& hash: merkle)
            std::cout << "  " << bc::encode_base16(hash) << std::endl;
        std::cout << std::endl;
    }

    // Finally we end up with a single item.
    return merkle[0];
}
```

#### 1. Edge-case apdorojimas

```cpp
if (merkle.empty())
    return bc::null_hash;
else if (merkle.size() == 1)
    return merkle[0];
```

- **Jei sąrašas tuščias:** grąžinamas `null_hash` (nulinis hash'as)
- **Jei jame tik vienas hash'as:** jis jau yra galutinis Merkle root
- **Paskirtis:** apsauga nuo neteisingo įvesties dydžio ir taisyklingo apdorojimo užtikrinimas

#### 2. Nelyginio hash'ų skaičiaus tvarkymas

```cpp
if (merkle.size() % 2 != 0)
    merkle.push_back(merkle.back());
```

- **Bitcoin taisyklė:** jeigu kuriame Merkle lygmenyje hash'ų skaičius nelyginis, paskutinis hash'as duplikuojamas, kad būtų galima sudaryti pilnas poras
- **Pavyzdys:** `[A, B, C]` → `[A, B, C, C]`
- **Kodėl svarbu:** tai užtikrina deterministinį, nuoseklų Merkle medžio kūrimą, kuris visada duoda tą patį rezultatą su tais pačiais hash'ais

#### 3. Hash'ų porų sujungimas ir dvigubas hash'inimas

```cpp
for (auto it = merkle.begin(); it != merkle.end(); it += 2)
{
    // Sujungiami (concatenate) du hash'ai
    concat.write_hash(*it);
    concat.write_hash(*(it + 1));
    
    // Hash'inami dvigubu SHA-256 (bitcoin_hash)
    bc::hash_digest new_root = bc::bitcoin_hash(concat_data);
    
    // Rezultatas įdedamas į naują sąrašą
    new_merkle.push_back(new_root);
}
```

Kiekvienoje iteracijoje hash'ai apdorojami poromis (`it += 2`):

1. **Sujungiami du hash'ai:** kiekviena pora konkatenojama į vieną duomenų bloką
2. **Taikomas dvigubas SHA-256:** naudojama `bitcoin_hash()` funkcija (SHA-256 du kartus)
3. **Rezultatas įrašomas:** naujas hash'as pridedamas į kito lygmens sąrašą

Tokiu būdu sukuriamas kitas Merkle tree lygmuo.

#### 4. Iteratyvus Merkle lygmenų konstravimas

```cpp
merkle = new_merkle;
```

- Naujai sudarytas hash'ų lygmuo tampa įvestimi kitai iteracijai
- Ciklas kartojamas tol, kol lieka tik vienas elementas
- Kiekviena iteracija sukuria naują Merkle medžio lygmenį, judant nuo lapų link šaknies

#### 5. Galutinio rezultato grąžinimas

```cpp
return merkle[0];
```

Kai sąraše yra vienas hash'as, jis yra **Merkle root** – galutinis rezultatas, naudojamas bloko antraštėje.

#### Algoritmo vizualizacija:

```

┌───┐     ┌───┐                ┌───┐        ┌───┐
│ A │     │ B │                │ C │        │ D │
└───┘     └───┘                └───┘        └───┘
tx0       tx1                  tx2          tx3
 │           │                      │           │
 └─────┬─────┘                      └─────┬─────┘
       │                                  │
       ▼                                  ▼
┌──────────────┐                 ┌──────────────┐
│ hash(AB)     │                 │ hash(CD)     │
└──────────────┘                 └──────────────┘
      │                                  │
      └────────────────┬─────────────────┘
                       │
                       ▼
           ┌───────────────────────────────┐
           │         Merkle Root           │
           │     hash(ABCD_level_2)        │
           └───────────────────────────────┘
```

#### Išvados:

- Funkcija tiksliai atitinka Bitcoin Merkle medžio specifikaciją
- Naudoja dvigubą SHA-256 hash'inimą (`bitcoin_hash`)
- Teisingai apdoroja nelyginį skaičių hash'ų (duplikuoja paskutinį)
- Iteratyvus algoritmas efektyviai sudaro Merkle medį be rekursijos
- Galutinis Merkle root naudojamas bloko antraštėje transakcijų vientisumo patikrinimui

</details>

---

<details>
 <summary><strong>1.3 Kodo kompiliavimas ir testavimas</strong></summary>

Pateiktas kodas:  
```
$ clang++ -std=c++11 -o merkle merkle.cpp $(pkg-config --cflags --libs libbitcoin)    $ ./merkle
```
Pakoreguota kodas pagal mano įdiegimą:
```
$ clang++ -std=c++11 -o merkle merkle.cpp $(pkg-config --cflags --libs libbitcoin-system)
$ ./merkle
```
Įvedus kodą gavau klaidą:
```
neda@jessica:~$ clang++ -std=c++11 -o merkle merkle.cpp $(pkg-config --cflags --libs libbitcoin-system)
Command 'clang++' not found, but can be installed with:
sudo apt install clang
```

Todėl įdiegiau clang++ į Ubuntu:
```
sudo apt update
sudo apt install clang -y
```
<details>