#ifndef VAULT_H
#define VAULT_H

#include "common.h"

extern Message msgArray[MAX_MESSAGES];
extern int messageCount;
extern int nextId;
extern Node* head;
extern int queueArr[QUEUE_SIZE];
extern int queueFront;
extern int queueRear;
extern int queueCount;
extern int idIndex[INDEX_SIZE];
extern mutex g_vaultMutex;

void indexReset();
void indexPut(int id, int arrayIndex);
int indexGet(int id);
int findMessageIndex(int id);

void addToList(int id);
int listLength();
void displayList();
bool searchList(int id);
void deleteList();

bool isQueueEmpty();
bool isQueueFull();
bool enqueue(int id);
int dequeue();
void displayQueue();

void showMessage(const Message &m);
bool saveAllMessages();
bool parseLine(string line, Message &m);
void loadMessages(int &loaded, int &skipped, bool &missing);
string serializePacket(const Message &m);
bool ingestIncomingPacket(const string &packet);

unsigned hashPassword(const string &pass);
bool checkVaultAuth();
void changeMasterPassword();

void shutdownVault();

#endif

