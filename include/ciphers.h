#ifndef CIPHERS_H
#define CIPHERS_H

#include "common.h"

string encryptCaesar(string text, int shift);
string decryptCaesar(string text, int shift);
void printCaesarMap(int shift);
void traceCaesar(const string &text, int shift);

string encryptROT13(string text);
string decryptROT13(string text);

string cipherAtbash(string text);

string encryptVigenere(string text, string key);
string decryptVigenere(string text, string key);

string xorCrypt(string text, int key);

string encryptRailFence(string text, int rails);
string decryptRailFence(string cipher, int rails);

string base64Encode(const string &in);
string base64Decode(const string &in);

bool openPayload(const Message &m, string &plain);

#endif

