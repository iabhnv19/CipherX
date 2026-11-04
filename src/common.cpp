#include "../include/common.h"

const char* RESET = "\033[0m";
const char* BOLD  = "\033[1m";
const char* DIM   = "\033[2m";
const char* RED   = "\033[91m";
const char* GRN   = "\033[92m";
const char* YEL   = "\033[93m";
const char* BLU   = "\033[94m";
const char* MAG   = "\033[95m";
const char* CYN   = "\033[96m";
const char* WHT   = "\033[97m";
const char* GRY   = "\033[90m";

void sleepMs(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleTitleA("cipherx");
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

void showCursor() { cout << "\033[?25h"; }
void hideCursor() { cout << "\033[?25l"; }
void clearScreen() { cout << "\033[2J\033[H"; }

void logMsg(const char *tag, const string &text) {
    const char *color = GRN;
    if (tag[0] == '-') color = RED;
    else if (tag[0] == '!') color = YEL;
    else if (tag[0] == '*') color = MAG;
    else if (tag[0] == 'B') color = BLU;
    cout << "  " << GRY << "[" << color << tag << GRY << "] " << RESET << text << "\n";
}

void warn(const string &text) { logMsg("-", text); }
void note(const string &text) { logMsg("+", text); }
void alert(const string &text) { logMsg("!", text); }
void btLog(const string &text) { logMsg("BT", text); }

void heading(const string &title) {
    cout << "\n  " << CYN << BOLD << title << RESET << "\n\n";
}

void printBanner() {
    cout << CYN << R"(
  ██████╗██╗██████╗ ██╗  ██╗███████╗██████╗ ██╗  ██╗
 ██╔════╝██║██╔══██╗██║  ██║██╔════╝██╔══██╗╚██╗██╔╝
 ██║     ██║██████╔╝███████║█████╗  ██████╔╝ ╚███╔╝ 
 ██║     ██║██╔═══╝ ██╔══██║██╔══╝  ██╔══██╗ ██╔██╗ 
 ╚██████╗██║██║     ██║  ██║███████╗██║  ██║██╔╝ ██╗
  ╚═════╝╚═╝╚═╝     ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝)" << RESET << "\n";
    cout << GRY << "   offline crypto tool and bluetooth radio link" << RESET << "\n";
}

string readLine(string prompt) {
    string line;
    cout << prompt;
    cout.flush();
    if (!getline(cin, line)) {
        cout << "\n";
        exit(0);
    }
    if (!line.empty() && line[line.size() - 1] == '\r') {
        line.erase(line.size() - 1);
    }
    return line;
}

void pause() {
    cout << "\n" << GRY << "  [press enter to continue]" << RESET;
    cout.flush();
    string dummy;
    getline(cin, dummy);
}

string ask(const string &label) {
    return readLine("  " + label + GRN + "> " + RESET);
}

bool stringToInt64(string s, long long &value) {
    if (s.empty() || s.length() > 18) return false;
    long long result = 0;
    for (int i = 0; i < (int)s.length(); i++) {
        if (s[i] < '0' || s[i] > '9') return false;
        result = result * 10 + (s[i] - '0');
    }
    value = result;
    return true;
}

bool stringToInt(string s, int &value) {
    long long wide;
    if (!stringToInt64(s, wide)) return false;
    if (wide > 2147483647LL) return false;
    value = (int)wide;
    return true;
}

int readInt(string prompt, int minVal, int maxVal) {
    while (true) {
        string line = readLine(prompt);
        int value;
        if (stringToInt(line, value) && value >= minVal && value <= maxVal) {
            return value;
        }
        warn("enter a value between " + to_string(minVal) + " and " + to_string(maxVal));
    }
}

string trim(string s) {
    int start = 0;
    while (start < (int)s.size() && (s[start] == ' ' || s[start] == '\t')) start++;
    int end = (int)s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) end--;
    return s.substr(start, end - start);
}

string lowerCopy(string s) {
    for (int i = 0; i < (int)s.size(); i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
    return s;
}

string upperCopy(string s) {
    for (int i = 0; i < (int)s.size(); i++) {
        s[i] = (char)toupper((unsigned char)s[i]);
    }
    return s;
}

unsigned sealOf(const string &text) {
    unsigned hash = 2166136261u;
    for (int i = 0; i < (int)text.size(); i++) {
        hash ^= (unsigned char)text[i];
        hash *= 16777619u;
    }
    return hash;
}

void printSeal(unsigned value) {
    const char *digits = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        cout << digits[(value >> (i * 4)) & 0xF];
    }
}

string algoName(char algo) {
    switch (algo) {
        case 'C': return "Caesar";
        case 'V': return "Vigenere";
        case 'X': return "XOR-Hex";
        case 'R': return "ROT13";
        case 'A': return "Atbash";
        case 'F': return "RailFence";
        case 'B': return "Base64";
        default:  return "Unknown";
    }
}

string formatWhen(long long stamp) {
    if (stamp <= 0) return "unrecorded";
    time_t when = (time_t)stamp;
    tm *parts = localtime(&when);
    if (parts == NULL) return "unrecorded";
    char buf[40];
    if (strftime(buf, sizeof(buf), "%d %b %Y %H:%M", parts) == 0) {
        return "unrecorded";
    }
    return buf;
}

void meter(int value, int maxValue, int width) {
    if (maxValue < 1) maxValue = 1;
    int filled = value * width / maxValue;
    if (value > 0 && filled < 1) filled = 1;
    if (filled > width) filled = width;
    cout << GRY << "[";
    for (int i = 0; i < width; i++) {
        if (i < filled) cout << GRN << "#";
        else cout << GRY << ".";
    }
    cout << GRY << "]" << RESET;
}

void animateSeal(const string &text, const char *color) {
    int limit = (int)text.size();
    bool cut = false;
    if (limit > 60) {
        limit = 60;
        cut = true;
    }
    hideCursor();
    cout << "  " << color;
    for (int i = 0; i < limit; i++) {
        cout << text[i];
        cout.flush();
        sleepMs(5);
    }
    if (cut) cout << GRY << "... [truncated]";
    cout << RESET << "\n";
    showCursor();
}

void hexDump(const string &raw) {
    const char *digits = "0123456789ABCDEF";
    int total = (int)raw.size();
    int show = total > 64 ? 64 : total;
    for (int row = 0; row < show; row += 16) {
        cout << GRY << "  " << digits[(row >> 12) & 0xF]
             << digits[(row >> 8) & 0xF]
             << digits[(row >> 4) & 0xF]
             << digits[row & 0xF] << "  ";
        for (int col = 0; col < 16; col++) {
            int pos = row + col;
            if (pos < show) {
                unsigned char c = (unsigned char)raw[pos];
                cout << YEL << digits[c >> 4] << digits[c & 0xF] << " ";
            } else {
                cout << "   ";
            }
            if (col == 7) cout << " ";
        }
        cout << GRY << " ";
        for (int col = 0; col < 16; col++) {
            int pos = row + col;
            if (pos >= show) break;
            unsigned char c = (unsigned char)raw[pos];
            if (c >= 32 && c < 127) cout << (char)c;
            else cout << ".";
        }
        cout << RESET << "\n";
    }
    if (total > show) {
        cout << GRY << "  ... " << (total - show) << " remaining bytes" << RESET << "\n";
    }
}

string toHex(const string &raw) {
    const char *digits = "0123456789ABCDEF";
    string hex;
    for (int i = 0; i < (int)raw.size(); i++) {
        unsigned char c = (unsigned char)raw[i];
        hex += digits[c >> 4];
        hex += digits[c & 0x0F];
    }
    return hex;
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

bool fromHex(const string &hex, string &raw) {
    if (hex.empty() || hex.size() % 2 != 0) return false;
    raw = "";
    for (int i = 0; i < (int)hex.size(); i += 2) {
        int hi = hexValue(hex[i]);
        int lo = hexValue(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        raw += (char)((hi << 4) | lo);
    }
    return true;
}
