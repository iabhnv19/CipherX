#ifndef COMMON_H
#define COMMON_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2bth.h>
#include <bthsdpdef.h>
#include <bluetoothapis.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#include <iostream>
#include <string>
#include <fstream>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

extern const char* RESET;
extern const char* BOLD;
extern const char* DIM;
extern const char* RED;
extern const char* GRN;
extern const char* YEL;
extern const char* BLU;
extern const char* MAG;
extern const char* CYN;
extern const char* WHT;
extern const char* GRY;

const int MAX_MESSAGES = 250;
const int MAX_TEXT = 512;
const int MAX_NAME = 32;
const int QUEUE_SIZE = 250;
const int INDEX_SIZE = 509;
const char DELIM = '|';
const string FILE_NAME = "messages.txt";
const string KEY_FILE = "vault.key";

struct Message {
    int id;
    string sender;
    string recipient;
    string encryptedText;
    int shift;
    string status;
    char algo;
    string key;
    long long created;
};

struct Node {
    int id;
    Node* next;
};

struct DiscoveredBtDevice {
    string name;
    BTH_ADDR address;
    string addrString;
};

void sleepMs(int ms);
void setupConsole();
void showCursor();
void hideCursor();
void clearScreen();

void logMsg(const char *tag, const string &text);
void warn(const string &text);
void note(const string &text);
void alert(const string &text);
void btLog(const string &text);
void heading(const string &title);
void printBanner();

string readLine(string prompt);
void pause();
string ask(const string &label);
bool stringToInt64(string s, long long &value);
bool stringToInt(string s, int &value);
int readInt(string prompt, int minVal, int maxVal);
string trim(string s);
string lowerCopy(string s);
string upperCopy(string s);

unsigned sealOf(const string &text);
void printSeal(unsigned value);
string algoName(char algo);
string formatWhen(long long stamp);
void meter(int value, int maxValue, int width);
void animateSeal(const string &text, const char *color);
void hexDump(const string &raw);
string toHex(const string &raw);
int hexValue(char c);
bool fromHex(const string &hex, string &raw);

#endif

