#include "../include/actions.h"
#include "../include/bluetooth.h"
#include "../include/cryptanalysis.h"

string readName(string prompt) {
    while (true) {
        string name = trim(readLine(prompt));
        if (name.empty()) {
            warn("name cannot be empty.");
        } else if ((int)name.size() > MAX_NAME) {
            warn("name too long.");
        } else if (name.find(DELIM) != string::npos) {
            warn(string("character '") + DELIM + "' is not allowed.");
        } else {
            return name;
        }
    }
}

string readMessageText() {
    while (true) {
        string text = ask("message");
        bool hasContent = false;
        for (int i = 0; i < (int)text.length(); i++) {
            if (text[i] != ' ' && text[i] != '\t') hasContent = true;
        }
        if (!hasContent) {
            warn("empty message.");
        } else if ((int)text.size() > MAX_TEXT) {
            warn("message too long (max " + to_string(MAX_TEXT) + " chars).");
        } else if (text.find(DELIM) != string::npos) {
            warn(string("character '") + DELIM + "' is not allowed.");
        } else {
            return text;
        }
    }
}

string readKeyword() {
    while (true) {
        string key = ask("keyword");
        bool ok = (!key.empty() && key.size() <= 32);
        for (char c : key) if (!isalpha((unsigned char)c)) ok = false;
        if (ok) return key;
        warn("letters only, 1-32 chars.");
    }
}

void composeAndStore(bool &createdNew, Message &outMsg) {
    createdNew = false;
    if (messageCount >= MAX_MESSAGES) {
        warn("vault is full.");
        return;
    }
    if (isQueueFull()) {
        warn("queue is full.");
        return;
    }

    string sender = readName("  sender   ");
    string recipient = readName("  receiver ");
    string text = readMessageText();

    cout << "\n  " << WHT << "ciphers" << RESET << "\n";
    cout << "  " << CYN << "1" << RESET << "  Caesar Shift\n";
    cout << "  " << CYN << "2" << RESET << "  Vigenere\n";
    cout << "  " << CYN << "3" << RESET << "  XOR Stream\n";
    cout << "  " << CYN << "4" << RESET << "  ROT13\n";
    cout << "  " << CYN << "5" << RESET << "  Atbash\n";
    cout << "  " << CYN << "6" << RESET << "  Rail Fence\n";
    cout << "  " << CYN << "7" << RESET << "  Base64\n\n";
    int engine = readInt("  pick ", 1, 7);

    Message m;
    m.shift = 0;
    m.key = "";
    m.created = (long long)time(NULL);
    string stored;

    cout << "\n";
    if (engine == 1) {
        m.algo = 'C';
        m.shift = readInt("  shift (1-25) ", 1, 25);
        stored = encryptCaesar(text, m.shift);
        printCaesarMap(m.shift);
        traceCaesar(text, m.shift);
    } else if (engine == 2) {
        m.algo = 'V';
        m.key = readKeyword();
        stored = encryptVigenere(text, m.key);
    } else if (engine == 3) {
        m.algo = 'X';
        m.shift = readInt("  xor key (1-255) ", 1, 255);
        stored = toHex(xorCrypt(text, m.shift));
    } else if (engine == 4) {
        m.algo = 'R';
        stored = encryptROT13(text);
    } else if (engine == 5) {
        m.algo = 'A';
        stored = cipherAtbash(text);
    } else if (engine == 6) {
        m.algo = 'F';
        m.shift = readInt("  rails (2-10) ", 2, 10);
        stored = encryptRailFence(text, m.shift);
    } else if (engine == 7) {
        m.algo = 'B';
        stored = base64Encode(text);
    }

    cout << "\n  " << GRY << "encrypting..." << RESET << "\n";
    animateSeal(stored, MAG);

    m.id = nextId++;
    m.sender = sender;
    m.recipient = recipient;
    m.encryptedText = stored;
    m.status = "Pending";

    msgArray[messageCount] = m;
    indexPut(m.id, messageCount);
    messageCount++;
    addToList(m.id);
    enqueue(m.id);

    cout << "\n";
    note("saved message #" + to_string(m.id));
    cout << GRY << "  engine: " << algoName(m.algo)
         << "   size: " << text.size() << "B in / " << stored.size() << "B out"
         << "   seal: ";
    printSeal(sealOf(stored));
    cout << RESET << "\n";
    if (saveAllMessages()) note("file updated.");

    createdNew = true;
    outMsg = m;
}

void sendMessageLocal() {
    clearScreen();
    heading("New Message");
    bool created = false;
    Message dummy;
    composeAndStore(created, dummy);
}

void viewAllMessages() {
    clearScreen();
    heading("Vault Archive");
    if (messageCount == 0) {
        cout << GRY << "  vault is empty." << RESET << "\n";
        return;
    }
    displayList();
    cout << GRY << "  total records: " << WHT << messageCount << GRY
         << "   chain nodes: " << WHT << listLength() << RESET << "\n\n";
    for (int i = 0; i < messageCount; i++) showMessage(msgArray[i]);
}

void searchMessage() {
    clearScreen();
    heading("Search Messages");
    if (messageCount == 0) {
        cout << GRY << "  vault is empty." << RESET << "\n";
        return;
    }
    cout << "  " << CYN << "1" << RESET << "  by id\n";
    cout << "  " << CYN << "2" << RESET << "  by sender or receiver\n\n";
    int mode = readInt("  pick ", 1, 2);
    if (mode == 1) {
        int id = readInt("  id ", 1, 999999999);
        int index = findMessageIndex(id);
        if (index == -1) {
            warn("message #" + to_string(id) + " not found.");
            return;
        }
        cout << "\n";
        note("found via hash index:");
        showMessage(msgArray[index]);
        if (searchList(id)) {
            cout << GRY << "  also confirmed in linked list chain." << RESET << "\n";
        }
        return;
    }

    string needle = trim(readLine("  search name "));
    if (needle.empty()) {
        warn("empty input.");
        return;
    }
    int hits = 0;
    cout << "\n";
    for (int i = 0; i < messageCount; i++) {
        string from = lowerCopy(msgArray[i].sender);
        string to = lowerCopy(msgArray[i].recipient);
        string want = lowerCopy(needle);
        if (from.find(want) != string::npos || to.find(want) != string::npos) {
            showMessage(msgArray[i]);
            hits++;
        }
    }
    if (hits == 0) warn("no matches for \"" + needle + "\"");
    else note(to_string(hits) + " match(es) found.");
}

void decryptSelectedMessage() {
    clearScreen();
    heading("Decrypt Message");
    if (messageCount == 0) {
        cout << GRY << "  vault is empty." << RESET << "\n";
        return;
    }
    int id = readInt("  message id ", 1, 999999999);
    int index = findMessageIndex(id);
    if (index == -1) {
        warn("message #" + to_string(id) + " not found.");
        return;
    }
    string plain;
    if (!openPayload(msgArray[index], plain)) {
        warn("could not decrypt this payload.");
        return;
    }
    cout << "\n";
    showMessage(msgArray[index]);
    cout << "  " << GRY << "decrypted text:" << RESET << "\n";
    animateSeal(plain, GRN);
    cout << GRY << "  length: " << plain.size() << " chars" << RESET << "\n";
}

void viewPendingMessages() {
    clearScreen();
    heading("Pending Messages");
    displayQueue();
}

void processQueue() {
    clearScreen();
    heading("Dispatch Messages");
    if (isQueueEmpty()) {
        cout << GRY << "  queue is empty." << RESET << "\n";
        return;
    }
    cout << GRY << "  pending: " << YEL << queueCount << RESET << "\n\n";
    cout << "  " << CYN << "1" << RESET << "  deliver next message\n";
    cout << "  " << CYN << "2" << RESET << "  deliver all\n\n";
    int mode = readInt("  pick ", 1, 2);
    int todo = (mode == 1) ? 1 : queueCount;
    int deliveredNow = 0;
    cout << "\n";
    for (int n = 0; n < todo; n++) {
        int id = dequeue();
        int index = findMessageIndex(id);
        if (index != -1) {
            msgArray[index].status = "Delivered";
            cout << "  " << GRN << "[OK]" << RESET
                 << "  #" << id << "  "
                 << msgArray[index].sender << GRY << " to " << RESET
                 << msgArray[index].recipient << "\n";
            deliveredNow++;
            sleepMs(40);
        }
    }
    cout << "\n";
    note(to_string(deliveredNow) + " delivered.");
    if (saveAllMessages()) note("saved to file.");
}

void cipherLab() {
    clearScreen();
    heading("Cipher Lab");
    cout << GRY << "  test any cipher here. nothing is saved." << RESET << "\n\n";
    cout << "  " << CYN << "1" << RESET << "  Caesar Shift\n";
    cout << "  " << CYN << "2" << RESET << "  Vigenere\n";
    cout << "  " << CYN << "3" << RESET << "  XOR\n";
    cout << "  " << CYN << "4" << RESET << "  ROT13\n";
    cout << "  " << CYN << "5" << RESET << "  Atbash\n";
    cout << "  " << CYN << "6" << RESET << "  Rail Fence\n";
    cout << "  " << CYN << "7" << RESET << "  Base64\n\n";
    int engine = readInt("  pick ", 1, 7);

    string text = readMessageText();
    string stored;
    string plain;
    bool ok = false;

    cout << "\n";
    if (engine == 1) {
        int shift = readInt("  shift (1-25) ", 1, 25);
        printCaesarMap(shift);
        traceCaesar(text, shift);
        stored = encryptCaesar(text, shift);
        plain = decryptCaesar(stored, shift);
        ok = (plain == text);
    } else if (engine == 2) {
        string key = readKeyword();
        stored = encryptVigenere(text, key);
        plain = decryptVigenere(stored, key);
        ok = (plain == text);
    } else if (engine == 3) {
        int mask = readInt("  xor key (1-255) ", 1, 255);
        stored = toHex(xorCrypt(text, mask));
        string raw;
        ok = fromHex(stored, raw) && xorCrypt(raw, mask) == text;
        plain = ok ? text : "";
    } else if (engine == 4) {
        stored = encryptROT13(text);
        plain = decryptROT13(stored);
        ok = (plain == text);
    } else if (engine == 5) {
        stored = cipherAtbash(text);
        plain = cipherAtbash(stored);
        ok = (plain == text);
    } else if (engine == 6) {
        int rails = readInt("  rails (2-10) ", 2, 10);
        stored = encryptRailFence(text, rails);
        plain = decryptRailFence(stored, rails);
        ok = (plain == text);
    } else if (engine == 7) {
        stored = base64Encode(text);
        plain = base64Decode(stored);
        ok = (plain == text);
    }

    cout << "\n  " << GRY << "ciphertext:" << RESET << "\n";
    animateSeal(stored, MAG);

    cout << "\n  " << GRY << "hexdump:" << RESET << "\n";
    hexDump(stored);

    cout << "\n  " << GRY << "roundtrip test: " << RESET;
    if (ok) cout << GRN << BOLD << "MATCH" << RESET << "\n";
    else cout << RED << BOLD << "FAILED" << RESET << "\n";
}

void showIntel() {
    clearScreen();
    heading("Telemetry");
    int pending = 0;
    int delivered = 0;
    int counts[8] = {0};

    for (int i = 0; i < messageCount; i++) {
        if (msgArray[i].status == "Pending") pending++;
        else if (msgArray[i].status == "Delivered") delivered++;
        switch (msgArray[i].algo) {
            case 'C': counts[0]++; break;
            case 'V': counts[1]++; break;
            case 'X': counts[2]++; break;
            case 'R': counts[3]++; break;
            case 'A': counts[4]++; break;
            case 'F': counts[5]++; break;
            case 'B': counts[6]++; break;
            default:  counts[7]++; break;
        }
    }

    cout << "  vault capacity ";
    meter(messageCount, MAX_MESSAGES, 28);
    cout << "  " << WHT << messageCount << GRY << "/" << MAX_MESSAGES << RESET << "\n";
    cout << "  pending        ";
    meter(pending, MAX_MESSAGES, 28);
    cout << "  " << YEL << pending << RESET << "\n";
    cout << "  delivered      ";
    meter(delivered, MAX_MESSAGES, 28);
    cout << "  " << GRN << delivered << RESET << "\n\n";

    cout << "  " << WHT << "ciphers used" << RESET << "\n";
    const char* labels[] = {"Caesar", "Vigenere", "XOR-Hex", "ROT13", "Atbash", "RailFence", "Base64", "Other"};
    for (int i = 0; i < 7; i++) {
        string l = labels[i];
        if (l.length() < 12) l += string(12 - l.length(), ' ');
        cout << "  " << l;
        meter(counts[i], messageCount == 0 ? 1 : messageCount, 20);
        cout << "  " << counts[i] << "\n";
    }

    int nodes = listLength();
    cout << "\n  " << WHT << "data structures" << RESET << "\n";
    cout << "  linked list nodes: " << WHT << nodes << RESET << "\n";
    cout << "  array records:     " << WHT << messageCount << RESET << "\n";
    cout << "  queue count:       " << YEL << queueCount << RESET << "\n";
    cout << "  hash index slots:  " << GRY << INDEX_SIZE << " (linear probe)" << RESET << "\n";
    cout << "  bluetooth rfcomm:  " << (g_btServerRunning ? string(GRN) + "online" : string(YEL) + "offline") << RESET << "\n";
}

void statusBar() {
    int pending = queueCount;
    int delivered = 0;
    for (int i = 0; i < messageCount; i++) {
        if (msgArray[i].status == "Delivered") delivered++;
    }
    cout << GRY << "  vault: " << WHT << messageCount << GRY << "/" << MAX_MESSAGES
         << "   pending: " << YEL << pending
         << GRY << "   delivered: " << GRN << delivered
         << GRY << "   bluetooth: " << (g_btServerRunning ? string(GRN) + "active" : string(RED) + "off")
         << GRY << "   next id: " << CYN << "#" << nextId
         << RESET << "\n";
}

void menuItem(const char *key, const string &name, const char *hint) {
    string label = name;
    if ((int)label.size() < 16) label += string(16 - label.size(), ' ');
    cout << "  " << MAG << BOLD << key << RESET
         << "   " << WHT << label << RESET
         << GRY << hint << RESET << "\n";
}

void showMenu() {
    clearScreen();
    printBanner();
    cout << "\n";
    statusBar();
    cout << "\n";
    menuItem("1", "Compose",        "encrypt and save to vault");
    menuItem("2", "Bluetooth Link", "scan, send, or receive via rfcomm");
    menuItem("3", "Decrypt",        "open message by id");
    menuItem("4", "Archive",        "view message chain");
    menuItem("5", "Search",         "find by id or name");
    menuItem("6", "Queue",          "view pending deliveries");
    menuItem("7", "Dispatch",       "mark messages delivered");
    menuItem("8", "Cipher Lab",     "sandbox testing");
    menuItem("9", "Cracker",        "frequency analysis and brute force");
    menuItem("0", "Telemetry",      "data structure stats");
    menuItem("P", "Security",       "change master password");
    menuItem("Q", "Exit",           "save and quit");
    cout << "\n";
}

void boot() {
    clearScreen();
    printBanner();
    cout << "\n";
    const char *steps[6] = {
        "utf-8 and terminal colors",
        "cipher engines",
        "hash table index",
        "linked list history",
        "circular queue",
        "bluetooth rfcomm socket"
    };
    for (int i = 0; i < 6; i++) {
        cout << GRN << "  [ok]  " << RESET << steps[i] << "\n";
        sleepMs(20);
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) {
        startBluetoothServer();
        cout << GRN << "  [ok]  " << RESET << "bluetooth rfcomm listener ready\n";
    } else {
        cout << RED << "  [--]  " << RESET << "could not start bluetooth subsystem\n";
    }
#endif

    int loaded = 0;
    int skipped = 0;
    bool missing = false;
    loadMessages(loaded, skipped, missing);
    if (missing) {
        cout << YEL << "  [--]  " << RESET << "no vault file found, starting fresh.\n";
    } else {
        cout << GRN << "  [ok]  " << RESET << "loaded " << loaded << " messages from vault.\n";
    }
    if (skipped > 0) {
        warn(to_string(skipped) + " lines were invalid and skipped.");
    }
    sleepMs(150);
}
