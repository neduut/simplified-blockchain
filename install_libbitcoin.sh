#!/bin/bash
# Libbitcoin-system diegimo skriptas Docker konteineryje

set -e

echo "=== Libbitcoin-system diegimas ==="

# 1. Įdiegti secp256k1
echo "1. Diegiama secp256k1..."
cd /tmp
wget https://github.com/bitcoin-core/secp256k1/archive/refs/heads/master.zip -O secp256k1.zip
unzip secp256k1.zip
cd secp256k1-master
./autogen.sh
./configure --enable-module-recovery
make -j4
make install
ldconfig
echo "✓ secp256k1 įdiegta"

# 2. Įdiegti Boost (jei reikia papildomai)
echo "2. Tikrinu Boost..."
apt-get install -y libboost-all-dev
echo "✓ Boost įdiegtas"

# 3. Klonuoti ir kompiliuoti libbitcoin-system
echo "3. Diegiama libbitcoin-system..."
cd /tmp
git clone https://github.com/libbitcoin/libbitcoin-system.git
cd libbitcoin-system
git checkout v3.8.0  # Naujausia v3 versija

# Kompiliuoti
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF
make -j4
make install
ldconfig

echo "✓ libbitcoin-system įdiegta!"

# 4. Patikrinti diegimą
echo "4. Tikrinu diegimą..."
pkg-config --modversion libbitcoin-system || echo "⚠ pkg-config nerado libbitcoin-system"
ldconfig -p | grep bitcoin || echo "⚠ libbitcoin bibliotekos nerastos"

echo "=== Diegimas baigtas ==="
