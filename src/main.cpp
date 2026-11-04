#include "../include/common.h"
#include "../include/ciphers.h"
#include "../include/vault.h"
#include "../include/bluetooth.h"
#include "../include/cryptanalysis.h"
#include "../include/actions.h"

void shutdownVault() {
    stopBluetoothServer();
    showCursor();
    saveAllMessages();
    deleteList();
#ifdef _WIN32
    WSACleanup();
#endif
}

int main() {
    setupConsole();
    indexReset();
    if (!checkVaultAuth()) return 1;
    boot();

    string choiceStr = "";
    while (choiceStr != "Q" && choiceStr != "q") {
        showMenu();
        choiceStr = trim(readLine(string("  ") + CYN + ">" + RESET + " "));
        if (choiceStr.empty()) continue;

        if (choiceStr == "1") sendMessageLocal();
        else if (choiceStr == "2") bluetoothMeshHub();
        else if (choiceStr == "3") decryptSelectedMessage();
        else if (choiceStr == "4") viewAllMessages();
        else if (choiceStr == "5") searchMessage();
        else if (choiceStr == "6") viewPendingMessages();
        else if (choiceStr == "7") processQueue();
        else if (choiceStr == "8") cipherLab();
        else if (choiceStr == "9") cryptanalysisTools();
        else if (choiceStr == "0") showIntel();
        else if (choiceStr == "P" || choiceStr == "p") changeMasterPassword();
        else if (choiceStr == "Q" || choiceStr == "q") break;
        else {
            warn("invalid option.");
        }
        pause();
    }

    clearScreen();
    printBanner();
    cout << "\n";
    note("vault saved, shutting down.");
    shutdownVault();
    return 0;
}
