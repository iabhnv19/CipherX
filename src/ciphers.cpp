#include "../include/ciphers.h"

static const string B64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

string base64Encode(const string &in) {
    string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(B64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(B64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

string base64Decode(const string &in) {
    string out;
    vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[B64_CHARS[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

string encryptCaesar(string text, int shift) {
    string result = text;
    shift = (shift % 26 + 26) % 26;
    for (int i = 0; i < (int)result.length(); i++) {
        char c = result[i];
        if (c >= 'A' && c <= 'Z') {
            result[i] = (char)((c - 'A' + shift) % 26 + 'A');
        } else if (c >= 'a' && c <= 'z') {
            result[i] = (char)((c - 'a' + shift) % 26 + 'a');
        }
    }
    return result;
}

string decryptCaesar(string text, int shift) {
    return encryptCaesar(text, 26 - (shift % 26));
}

void printCaesarMap(int shift) {
    shift = (shift % 26 + 26) % 26;
    cout << GRY << "  Plain  : ABCDEFGHIJKLMNOPQRSTUVWXYZ" << RESET << "\n";
    cout << YEL << "  Cipher : ";
    for (char c = 'A'; c <= 'Z'; c++) {
        cout << (char)((c - 'A' + shift) % 26 + 'A');
    }
    cout << RESET << "\n";
}

void traceCaesar(const string &text, int shift) {
    cout << GRY << "  Shift trace: " << RESET;
    int limit = (text.size() > 16) ? 16 : (int)text.size();
    for (int i = 0; i < limit; i++) {
        char c = text[i];
        if (isalpha((unsigned char)c)) {
            char base = isupper((unsigned char)c) ? 'A' : 'a';
            char mapped = (char)((c - base + shift) % 26 + base);
            cout << c << "->" << mapped << " ";
        }
    }
    if (text.size() > 16) cout << "...";
    cout << "\n";
}

string encryptROT13(string text) { return encryptCaesar(text, 13); }
string decryptROT13(string text) { return encryptCaesar(text, 13); }

string cipherAtbash(string text) {
    string result = text;
    for (int i = 0; i < (int)result.length(); i++) {
        char c = result[i];
        if (c >= 'A' && c <= 'Z') {
            result[i] = (char)('Z' - (c - 'A'));
        } else if (c >= 'a' && c <= 'z') {
            result[i] = (char)('z' - (c - 'a'));
        }
    }
    return result;
}

string encryptVigenere(string text, string key) {
    string result = text;
    int keyPos = 0;
    int keyLen = (int)key.length();
    if (keyLen == 0) return result;
    for (int i = 0; i < (int)result.length(); i++) {
        char c = result[i];
        if (isalpha((unsigned char)c)) {
            int shift = tolower((unsigned char)key[keyPos % keyLen]) - 'a';
            char base = isupper((unsigned char)c) ? 'A' : 'a';
            result[i] = (char)((c - base + shift) % 26 + base);
            keyPos++;
        }
    }
    return result;
}

string decryptVigenere(string text, string key) {
    string result = text;
    int keyPos = 0;
    int keyLen = (int)key.length();
    if (keyLen == 0) return result;
    for (int i = 0; i < (int)result.length(); i++) {
        char c = result[i];
        if (isalpha((unsigned char)c)) {
            int shift = tolower((unsigned char)key[keyPos % keyLen]) - 'a';
            char base = isupper((unsigned char)c) ? 'A' : 'a';
            result[i] = (char)((c - base - shift + 26) % 26 + base);
            keyPos++;
        }
    }
    return result;
}

string xorCrypt(string text, int key) {
    string result = text;
    unsigned char mask = (unsigned char)(key & 255);
    for (int i = 0; i < (int)result.length(); i++) {
        result[i] = (char)((unsigned char)result[i] ^ mask);
    }
    return result;
}

string encryptRailFence(string text, int rails) {
    if (rails <= 1 || text.empty()) return text;
    vector<string> rail(rails);
    int currRail = 0;
    bool down = false;
    for (char c : text) {
        rail[currRail].push_back(c);
        if (currRail == 0 || currRail == rails - 1) down = !down;
        currRail += down ? 1 : -1;
    }
    string res = "";
    for (int i = 0; i < rails; i++) res += rail[i];
    return res;
}

string decryptRailFence(string cipher, int rails) {
    if (rails <= 1 || cipher.empty()) return cipher;
    int n = (int)cipher.length();
    vector<vector<char>> fence(rails, vector<char>(n, '\n'));
    int currRail = 0;
    bool down = false;
    for (int i = 0; i < n; i++) {
        fence[currRail][i] = '*';
        if (currRail == 0 || currRail == rails - 1) down = !down;
        currRail += down ? 1 : -1;
    }
    int idx = 0;
    for (int i = 0; i < rails; i++) {
        for (int j = 0; j < n; j++) {
            if (fence[i][j] == '*' && idx < n) {
                fence[i][j] = cipher[idx++];
            }
        }
    }
    string res = "";
    currRail = 0;
    down = false;
    for (int i = 0; i < n; i++) {
        res.push_back(fence[currRail][i]);
        if (currRail == 0 || currRail == rails - 1) down = !down;
        currRail += down ? 1 : -1;
    }
    return res;
}

bool openPayload(const Message &m, string &plain) {
    if (m.algo == 'C') {
        plain = decryptCaesar(m.encryptedText, m.shift);
        return true;
    }
    if (m.algo == 'V') {
        if (m.key.empty()) return false;
        plain = decryptVigenere(m.encryptedText, m.key);
        return true;
    }
    if (m.algo == 'X') {
        string raw;
        if (!fromHex(m.encryptedText, raw)) return false;
        plain = xorCrypt(raw, m.shift);
        return true;
    }
    if (m.algo == 'R') {
        plain = decryptROT13(m.encryptedText);
        return true;
    }
    if (m.algo == 'A') {
        plain = cipherAtbash(m.encryptedText);
        return true;
    }
    if (m.algo == 'F') {
        plain = decryptRailFence(m.encryptedText, m.shift);
        return true;
    }
    if (m.algo == 'B') {
        plain = base64Decode(m.encryptedText);
        return true;
    }
    return false;
}

