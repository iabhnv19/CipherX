#ifndef ACTIONS_H
#define ACTIONS_H

#include "common.h"
#include "ciphers.h"
#include "vault.h"

string readName(string prompt);
string readMessageText();
string readKeyword();

void composeAndStore(bool &createdNew, Message &outMsg);
void sendMessageLocal();
void viewAllMessages();
void searchMessage();
void decryptSelectedMessage();
void viewPendingMessages();
void processQueue();
void cipherLab();
void showIntel();
void statusBar();
void menuItem(const char *key, const string &name, const char *hint);
void showMenu();
void boot();

#endif

