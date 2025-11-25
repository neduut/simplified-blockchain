# 3-oji (papildoma) užduotis: Bitcoin transakcijų ir blokų analizė su 'Libbitcoin' ir 'python-bitcoinlib'

---

## 1 DALIS: Merkle medžio implementacija su Libbitcoin 

### 1 užduotis: Libbitcoin-System įdiegimas

Čia aprašiau kaip įdiegiau **libbitcoin-system** biblioteką WSL2 Ubuntu aplinkoje. Surašiau visas problemas su kuriomis susidūriau ir kaip jas išsprendžiau.

---

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

---

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

---

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

---

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

**Rezultatas:** [PAVYKO] secp256k1 biblioteką įdiegiau į `/usr/local/lib/libsecp256k1.so`

---

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

---

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

---

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

---

### Žingsnis 8: Diegimas į sistemą su sudo (PAVYKO)

Dabar reikėjo įdiegti sukompiliuotas bibliotekas į sistemą (`/usr/local/`).

**Problema:**

```bash
make install
# Error: Permission denied — cannot write to /usr/local/lib/
```

**Sprendimas:**

```bash
cd build-libbitcoin-system/libbitcoin-system
sudo make install
```

**Rezultatas:** [PAVYKO] Bibliotekas įdiegiau:
- `/usr/local/lib/libbitcoin-system.so`
- `/usr/local/include/bitcoin/system/`
- `/usr/local/lib/pkgconfig/libbitcoin-system.pc`

---


