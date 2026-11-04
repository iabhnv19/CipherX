#include "../include/vault.h"

Message msgArray[MAX_MESSAGES];
int messageCount = 0;
int nextId = 1;
Node* head = NULL;
int queueArr[QUEUE_SIZE];
int queueFront = 0;
int queueRear = -1;
int queueCount = 0;
int idIndex[INDEX_SIZE];
mutex g_vaultMutex;

void indexReset() {
    for (int i = 0; i < INDEX_SIZE; i++) idIndex[i] = -1;
}

void indexPut(int id, int arrayIndex) {
    int start = id % INDEX_SIZE;
    if (start < 0) start += INDEX_SIZE;
    for (int step = 0; step < INDEX_SIZE; step++) {
        int slot = (start + step) % INDEX_SIZE;
        if (idIndex[slot] == -1) {
            idIndex[slot] = arrayIndex;
            return;
        }
    }
}

int indexGet(int id) {
    int start = id % INDEX_SIZE;
    if (start < 0) start += INDEX_SIZE;
    for (int step = 0; step < INDEX_SIZE; step++) {
        int slot = (start + step) % INDEX_SIZE;
        int found = idIndex[slot];
        if (found == -1) return -1;
        if (found >= 0 && found < messageCount && msgArray[found].id == id) {
            return found;
        }
    }
    return -1;
}

int findMessageIndex(int id) {
    int hit = indexGet(id);
    if (hit != -1) return hit;
    for (int i = 0; i < messageCount; i++) {
        if (msgArray[i].id == id) return i;
    }
    return -1;
}

void addToList(int id) {
    Node* node = new Node;
    node->id = id;
    node->next = NULL;
    if (head == NULL) {
        head = node;
        return;
    }
    Node* temp = head;
    while (temp->next != NULL) temp = temp->next;
    temp->next = node;
}

int listLength() {
    int count = 0;
    Node* temp = head;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}

void displayList() {
    if (head == NULL) {
        cout << GRY << "  history chain is empty" << RESET << "\n";
        return;
    }
    cout << GRY << "  chain  " << RESET;
    Node* temp = head;
    int shown = 0;
    while (temp != NULL) {
        if (shown > 0 && shown % 10 == 0) cout << "\n         ";
        cout << CYN << "[" << WHT << temp->id << CYN << "]";
        if (temp->next != NULL) cout << GRY << " -> ";
        temp = temp->next;
        shown++;
    }
    cout << GRY << " -> null" << RESET << "\n";
}

bool searchList(int id) {
    Node* temp = head;
    while (temp != NULL) {
        if (temp->id == id) return true;
        temp = temp->next;
    }
    return false;
}

void deleteList() {
    Node* temp = head;
    while (temp != NULL) {
        Node* nextNode = temp->next;
        delete temp;
        temp = nextNode;
    }
    head = NULL;
}

bool isQueueEmpty() { return queueCount == 0; }
bool isQueueFull() { return queueCount == QUEUE_SIZE; }

bool enqueue(int id) {
    if (isQueueFull()) return false;
    queueRear = (queueRear + 1) % QUEUE_SIZE;
    queueArr[queueRear] = id;
    queueCount++;
    return true;
}

int dequeue() {
    if (isQueueEmpty()) return -1;
    int id = queueArr[queueFront];
    queueFront = (queueFront + 1) % QUEUE_SIZE;
    queueCount--;
    return id;
}

void displayQueue() {
    if (isQueueEmpty()) {
        cout << GRY << "  queue is clear. nothing is waiting." << RESET << "\n";
        return;
    }
    cout << GRY << "  front " << queueFront
         << "   rear " << queueRear
         << "   occupancy " << queueCount << "/" << QUEUE_SIZE << RESET << "\n\n  ";
    meter(queueCount, QUEUE_SIZE, 24);
    cout << "\n\n";
    for (int i = 0; i < queueCount; i++) {
        int id = queueArr[(queueFront + i) % QUEUE_SIZE];
        int index = findMessageIndex(id);
        cout << "  " << YEL << (i == 0 ? "FRONT" : "     ") << RESET;
        cout << "  " << CYN << "#" << id << RESET;
        if (index != -1) {
            cout << "   " << msgArray[index].sender
                 << GRY << " to " << RESET
                 << msgArray[index].recipient
                 << GRY << "   " << algoName(msgArray[index].algo) << RESET;
        }
        if (i == queueCount - 1) cout << YEL << "   REAR" << RESET;
        cout << "\n";
    }
}

void showMessage(const Message &m) {
    string meta = algoName(m.algo);
    if (m.algo == 'C') meta += " (shift " + to_string(m.shift) + ")";
    else if (m.algo == 'V') meta += " (key " + m.key + ")";
    else if (m.algo == 'X') meta += " (mask 0x" + to_string(m.shift) + ")";
    else if (m.algo == 'F') meta += " (rails " + to_string(m.shift) + ")";

    const char* stateColor = (m.status == "Pending") ? YEL : GRN;
    cout << "  " << WHT << BOLD << "#" << m.id << RESET
         << "  " << MAG << meta << RESET << "\n";
    cout << "  status   " << stateColor << m.status << RESET << "\n";
    cout << "  from     " << m.sender << "\n";
    cout << "  to       " << m.recipient << "\n";
    cout << "  date     " << GRY << formatWhen(m.created) << RESET << "\n";
    cout << "  payload  " << MAG << m.encryptedText << RESET << "\n";
    cout << "  seal     " << GRY;
    printSeal(sealOf(m.encryptedText));
    cout << RESET << "\n\n";
}

bool saveAllMessages() {
    lock_guard<mutex> lock(g_vaultMutex);
    ofstream out(FILE_NAME.c_str());
    if (!out) {
        warn("could not open " + FILE_NAME + " to save records.");
        return false;
    }
    for (int i = 0; i < messageCount; i++) {
        out << msgArray[i].id << DELIM
            << msgArray[i].sender << DELIM
            << msgArray[i].recipient << DELIM
            << msgArray[i].encryptedText << DELIM
            << msgArray[i].shift << DELIM
            << msgArray[i].status << DELIM
            << msgArray[i].algo << DELIM
            << msgArray[i].key << DELIM
            << msgArray[i].created << "\n";
    }
    return true;
}

bool parseLine(string line, Message &m) {
    int delims = 0;
    for (int i = 0; i < (int)line.size(); i++) {
        if (line[i] == DELIM) delims++;
    }
    int expected = 0;
    if (delims == 5) expected = 6;
    else if (delims == 7) expected = 8;
    else if (delims == 8) expected = 9;
    else return false;

    string fields[9];
    int start = 0;
    for (int i = 0; i < expected - 1; i++) {
        int pos = (int)line.find(DELIM, start);
        if (pos == (int)string::npos) return false;
        fields[i] = line.substr(start, pos - start);
        start = pos + 1;
    }
    fields[expected - 1] = line.substr(start);

    int id = 0;
    int shift = 0;
    if (!stringToInt(fields[0], id) || id < 1) return false;
    if (!stringToInt(fields[4], shift)) return false;
    if (fields[1].empty() || fields[2].empty() || fields[3].empty()) return false;
    if (fields[5] != "Pending" && fields[5] != "Delivered") return false;

    m.algo = 'C';
    m.key = "";
    m.created = 0;
    if (expected >= 8) {
        if (fields[6].size() != 1) return false;
        m.algo = fields[6][0];
        m.key = fields[7];
    }
    if (expected == 9) {
        if (!stringToInt64(fields[8], m.created)) return false;
    }

    m.id = id;
    m.sender = fields[1];
    m.recipient = fields[2];
    m.encryptedText = fields[3];
    m.shift = shift;
    m.status = fields[5];
    return true;
}

void loadMessages(int &loaded, int &skipped, bool &missing) {
    loaded = 0;
    skipped = 0;
    missing = false;
    ifstream in(FILE_NAME.c_str());
    if (!in) {
        missing = true;
        return;
    }
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        if (line.empty()) continue;
        if (messageCount >= MAX_MESSAGES) {
            skipped++;
            continue;
        }
        Message m;
        if (!parseLine(line, m) || findMessageIndex(m.id) != -1) {
            skipped++;
            continue;
        }
        msgArray[messageCount] = m;
        indexPut(m.id, messageCount);
        messageCount++;
        addToList(m.id);
        if (m.status == "Pending") enqueue(m.id);
        if (m.id >= nextId) nextId = m.id + 1;
        loaded++;
    }
}

string serializePacket(const Message &m) {
    return to_string(m.id) + DELIM +
           m.sender + DELIM +
           m.recipient + DELIM +
           m.encryptedText + DELIM +
           to_string(m.shift) + DELIM +
           m.status + DELIM +
           string(1, m.algo) + DELIM +
           m.key + DELIM +
           to_string(m.created) + "\n";
}

bool ingestIncomingPacket(const string &packet) {
    lock_guard<mutex> lock(g_vaultMutex);
    Message m;
    if (!parseLine(packet, m)) return false;
    
    m.id = nextId++;
    m.status = "Pending";
    if (messageCount >= MAX_MESSAGES) return false;

    msgArray[messageCount] = m;
    indexPut(m.id, messageCount);
    messageCount++;
    addToList(m.id);
    enqueue(m.id);

    ofstream out(FILE_NAME.c_str(), ios::app);
    if (out) {
        out << m.id << DELIM
            << m.sender << DELIM
            << m.recipient << DELIM
            << m.encryptedText << DELIM
            << m.shift << DELIM
            << m.status << DELIM
            << m.algo << DELIM
            << m.key << DELIM
            << m.created << "\n";
    }
    return true;
}

unsigned hashPassword(const string &pass) {
    unsigned h = 0x811c9dc5;
    for (size_t i = 0; i < pass.size(); i++) {
        h ^= (unsigned char)pass[i];
        h *= 0x01000193;
        h = (h << 7) | (h >> 25);
    }
    return h;
}

bool checkVaultAuth() {
    ifstream keyIn(KEY_FILE.c_str());
    if (!keyIn) {
        clearScreen();
        printBanner();
        heading("Setup Master Password");
        note("set a password to encrypt this local vault:");
        while (true) {
            string p1 = ask("password");
            if (p1.length() < 4) {
                warn("use at least 4 characters.");
                continue;
            }
            string p2 = ask("confirm");
            if (p1 != p2) {
                warn("passwords do not match.");
                continue;
            }
            unsigned h = hashPassword(p1);
            ofstream keyOut(KEY_FILE.c_str());
            if (keyOut) {
                keyOut << h << "\n";
                note("master password saved.");
                sleepMs(400);
                return true;
            } else {
                warn("failed to create key file.");
                return false;
            }
        }
    }

    unsigned savedHash = 0;
    keyIn >> savedHash;
    keyIn.close();

    int attempts = 3;
    while (attempts > 0) {
        clearScreen();
        printBanner();
        heading("Unlock Vault");
        cout << GRY << "  attempts left: " << YEL << attempts << RESET << "\n\n";
        string inputPass = ask("password");
        if (hashPassword(inputPass) == savedHash) {
            note("unlocked.");
            sleepMs(300);
            return true;
        }
        attempts--;
        warn("wrong password.");
        sleepMs(500);
    }
    warn("too many attempts, exiting.");
    exit(1);
    return false;
}

void changeMasterPassword() {
    clearScreen();
    heading("Change Password");
    ifstream keyIn(KEY_FILE.c_str());
    unsigned savedHash = 0;
    if (keyIn) keyIn >> savedHash;
    keyIn.close();

    string curr = ask("current password");
    if (hashPassword(curr) != savedHash) {
        warn("wrong password.");
        return;
    }

    while (true) {
        string p1 = ask("new password");
        if (p1.length() < 4) {
            warn("minimum 4 characters.");
            continue;
        }
        string p2 = ask("confirm");
        if (p1 != p2) {
            warn("passwords do not match.");
            continue;
        }
        unsigned h = hashPassword(p1);
        ofstream keyOut(KEY_FILE.c_str());
        if (keyOut) {
            keyOut << h << "\n";
            note("password updated.");
        } else {
            warn("could not write key file.");
        }
        break;
    }
}
