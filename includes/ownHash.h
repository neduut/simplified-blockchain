#pragma once
#include <string>

// base62 simboliu masyvas
// ar man dar jis reikalinas??
const char BASE62[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

// pagrindine hash funkcija
std::string generate_hash(const std::string& user_input);
