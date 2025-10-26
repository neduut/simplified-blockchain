teko pakeist is base62 i hex nes per 41700000... nonces nesugebejo iskast net dvieju nuliu hasho


aprasyt situs:
kodo struktūra ir objektinio programavimo principų taikymas (enkapsuliacija, klasės, konstruktoriai ir pan.);
blokų ir transakcijų kūrimo, kasimo ir validavimo logikos teisingumas;
tinkamas Proof-of-Work mechanizmo veikimas (maišos sudėtingumas, nonce iteracijos ir t. t.);

Proof-of-Work (PoW)
- tikslas: rasti nonce taip, kad bloko hash prasidetu su N nuliais. v0.1 difficulty=3, taigi tikslas yra "000...".
- hash funkcija: custom `generate_hash()` (256-bit HEX). isvedime rodau pilnus hash'us.
- hash'inami laukai:
	- v1 blokas: index, timestamp, data, prevHash, nonce
	- v2 blokas: index, timestamp, version, difficulty, txRoot (v0.1 paprastas visu tx id hash), prevHash, nonce
- kasimas: didinu nonce nuo 0; po kiekvieno bandymo skaičiuoju hash(toString()). kai hash prasideda reikiamu kiekiu nuliu, blokas laikomas iskastu.
- progresas: kas 100000 bandymu isvedamas bandymu skaicius; po kasimo parodytas Nonce | Hash | Time | Attempts.
- validacija: `isChainValid()` tikrina prev hash nuorodas, perhashuoja bloko turini ir patikrina, kad hash atitiktu difficulty ("000...").
- difficulty: nustatomas paleidžiant `Blockchain(3)`. galima pakeisti i didesni/mazesni.
- pastaba v0.1: `txRoot` nera tikras Merkle Root; naudojamas paprastas visu tx id sujungimo hash. tikras Merkle bus v0.2.

projekto veikimo demonstracija (konsolės išvestis, bloko / transakcijos pavyzdžiai);
kodo tipo iskarpa is konsoles/prt sc