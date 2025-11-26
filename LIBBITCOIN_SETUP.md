# Libbitcoin bandymas su Docker

## Žingsnis 1: Sukurti Docker image

```powershell
docker build -f Dockerfile.libbitcoin -t libbitcoin-test .
```

## Žingsnis 2: Paleisti konteinerį

```powershell
docker run -it -v ${PWD}:/workspace libbitcoin-test
```

## Žingsnis 3: Įdiegti libbitcoin konteineryje

```bash
chmod +x install_libbitcoin.sh
./install_libbitcoin.sh
```

## Žingsnis 4: Kompiliuoti test programą

```bash
g++ -std=c++20 test_libbitcoin_merkle.cpp $(pkg-config --cflags --libs libbitcoin-system) -o test_merkle
```

## Žingsnis 5: Paleisti

```bash
./test_merkle
```

## Alternatyva: Per WSL2 (paprasčiau)

Jei Docker per sudėtingas, galime tiesiog iš naujo įdiegti į WSL2:

```bash
# WSL2 terminale
cd ~
chmod +x /mnt/c/Users/nedad/OneDrive\ -\ Vilnius\ University/Desktop/2k_1p/blokchain/simplified_blockchain/install_libbitcoin.sh
sudo bash /mnt/c/Users/nedad/OneDrive\ -\ Vilnius\ University/Desktop/2k_1p/blokchain/simplified_blockchain/install_libbitcoin.sh
```

Ar nori bandyti su Docker, ar su WSL2?
