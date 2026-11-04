#include "../include/bluetooth.h"
#include "../include/actions.h"

atomic<bool> g_btServerRunning(false);
SOCKET g_btListenSocket = INVALID_SOCKET;
thread* g_btServerThread = NULL;

string formatBtAddress(BTH_ADDR addr) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned int)((addr >> 40) & 0xFF),
             (unsigned int)((addr >> 32) & 0xFF),
             (unsigned int)((addr >> 24) & 0xFF),
             (unsigned int)((addr >> 16) & 0xFF),
             (unsigned int)((addr >> 8) & 0xFF),
             (unsigned int)(addr & 0xFF));
    return string(buf);
}

bool parseBtAddress(const string &str, BTH_ADDR &addr) {
    unsigned int bytes[6];
    if (sscanf(str.c_str(), "%x:%x:%x:%x:%x:%x",
               &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]) != 6 &&
        sscanf(str.c_str(), "%x-%x-%x-%x-%x-%x",
               &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]) != 6) {
        return false;
    }
    addr = 0;
    for (int i = 0; i < 6; i++) {
        addr = (addr << 8) | (bytes[i] & 0xFF);
    }
    return true;
}

vector<DiscoveredBtDevice> scanNearbyBluetoothDevices() {
    vector<DiscoveredBtDevice> devices;
#ifdef _WIN32
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = {
        sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
        1,
        1,
        1,
        1,
        1,
        4,
        NULL
    };

    BLUETOOTH_DEVICE_INFO deviceInfo = { sizeof(BLUETOOTH_DEVICE_INFO), {0} };
    HBLUETOOTH_DEVICE_FIND hFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (hFind != NULL) {
        do {
            DiscoveredBtDevice dev;
            char nameBuf[256];
            WideCharToMultiByte(CP_UTF8, 0, deviceInfo.szName, -1, nameBuf, sizeof(nameBuf), NULL, NULL);
            dev.name = string(nameBuf);
            if (dev.name.empty()) dev.name = "Unknown Bluetooth Node";
            dev.address = deviceInfo.Address.ullLong;
            dev.addrString = formatBtAddress(dev.address);
            devices.push_back(dev);
        } while (BluetoothFindNextDevice(hFind, &deviceInfo));
        BluetoothFindDeviceClose(hFind);
    }
#endif
    return devices;
}

void btServerWorker() {
#ifdef _WIN32
    g_btListenSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (g_btListenSocket == INVALID_SOCKET) {
        g_btServerRunning = false;
        return;
    }

    SOCKADDR_BTH sa = { 0 };
    sa.addressFamily = AF_BTH;
    sa.port = BT_PORT_ANY;

    if (bind(g_btListenSocket, (SOCKADDR*)&sa, sizeof(sa)) == SOCKET_ERROR) {
        closesocket(g_btListenSocket);
        g_btListenSocket = INVALID_SOCKET;
        g_btServerRunning = false;
        return;
    }

    int saLen = sizeof(sa);
    getsockname(g_btListenSocket, (SOCKADDR*)&sa, &saLen);

    if (listen(g_btListenSocket, 2) == SOCKET_ERROR) {
        closesocket(g_btListenSocket);
        g_btListenSocket = INVALID_SOCKET;
        g_btServerRunning = false;
        return;
    }

    g_btServerRunning = true;

    while (g_btServerRunning) {
        SOCKADDR_BTH clientSa = { 0 };
        int clientLen = sizeof(clientSa);
        SOCKET clientSock = accept(g_btListenSocket, (SOCKADDR*)&clientSa, &clientLen);
        if (clientSock == INVALID_SOCKET) {
            break;
        }

        char buffer[1024];
        int bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            string packet(buffer);
            if (!packet.empty() && packet[packet.size() - 1] == '\n') {
                packet.erase(packet.size() - 1);
            }
            if (!packet.empty() && packet[packet.size() - 1] == '\r') {
                packet.erase(packet.size() - 1);
            }
            if (ingestIncomingPacket(packet)) {
                send(clientSock, "ACK\n", 4, 0);
            } else {
                send(clientSock, "ERR\n", 4, 0);
            }
        }
        closesocket(clientSock);
    }
#endif
}

void startBluetoothServer() {
    if (g_btServerRunning) return;
    g_btServerThread = new thread(btServerWorker);
}

void stopBluetoothServer() {
    if (!g_btServerRunning && g_btListenSocket == INVALID_SOCKET) return;
    g_btServerRunning = false;
    if (g_btListenSocket != INVALID_SOCKET) {
        closesocket(g_btListenSocket);
        g_btListenSocket = INVALID_SOCKET;
    }
    if (g_btServerThread != NULL) {
        if (g_btServerThread->joinable()) {
            g_btServerThread->join();
        }
        delete g_btServerThread;
        g_btServerThread = NULL;
    }
}

bool transmitBtPacket(BTH_ADDR targetAddr, const string &packetData, int port) {
#ifdef _WIN32
    SOCKET s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (s == INVALID_SOCKET) return false;

    SOCKADDR_BTH sa = { 0 };
    sa.addressFamily = AF_BTH;
    sa.btAddr = targetAddr;
    sa.port = (port > 0) ? (ULONG)port : 1;

    DWORD timeout = 3000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    if (connect(s, (SOCKADDR*)&sa, sizeof(sa)) == SOCKET_ERROR) {
        closesocket(s);
        return false;
    }

    int sent = send(s, packetData.c_str(), (int)packetData.length(), 0);
    if (sent <= 0) {
        closesocket(s);
        return false;
    }

    char reply[16] = {0};
    recv(s, reply, sizeof(reply) - 1, 0);
    closesocket(s);
    return true;
#else
    return false;
#endif
}

void bluetoothMeshHub() {
    clearScreen();
    heading("Bluetooth Link");
    cout << GRY << "  server status: " 
         << (g_btServerRunning ? string(GRN) + "listening" : string(RED) + "stopped") 
         << RESET << "\n\n";

    cout << "  " << CYN << "1" << RESET << "  scan for devices\n";
    cout << "  " << CYN << "2" << RESET << "  encrypt and send over bluetooth\n";
    cout << "  " << CYN << "3" << RESET << "  send existing message from vault\n";
    cout << "  " << CYN << "4" << RESET << "  toggle receiver\n\n";
    int op = readInt("  pick ", 1, 4);

    if (op == 1) {
        cout << "\n" << GRY << "  scanning nearby devices (~4s)..." << RESET << "\n";
        vector<DiscoveredBtDevice> devs = scanNearbyBluetoothDevices();
        if (devs.empty()) {
            warn("no devices found.");
        } else {
            cout << "\n";
            for (size_t i = 0; i < devs.size(); i++) {
                cout << "  " << CYN << "[" << (i + 1) << "]" << RESET
                     << "  " << YEL << devs[i].addrString << RESET << "  "
                     << WHT << devs[i].name << RESET << "\n";
            }
            cout << "\n";
            note(to_string(devs.size()) + " devices found.");
        }
    } else if (op == 2) {
        bool created = false;
        Message msg;
        composeAndStore(created, msg);
        if (!created) return;

        cout << "\n  " << WHT << "target device" << RESET << "\n";
        cout << "  " << CYN << "1" << RESET << " pick from scan\n";
        cout << "  " << CYN << "2" << RESET << " enter mac manually (e.g. 00:11:22:33:44:55)\n\n";
        int sel = readInt("  pick ", 1, 2);

        BTH_ADDR targetAddr = 0;
        if (sel == 1) {
            cout << GRY << "  scanning..." << RESET << "\n";
            vector<DiscoveredBtDevice> devs = scanNearbyBluetoothDevices();
            if (devs.empty()) {
                warn("no devices found.");
                return;
            }
            cout << "\n";
            for (size_t i = 0; i < devs.size(); i++) {
                cout << "  " << CYN << (i + 1) << RESET << ". " 
                     << YEL << devs[i].addrString << RESET << "  " << devs[i].name << "\n";
            }
            cout << "\n";
            int choice = readInt("  pick ", 1, (int)devs.size());
            targetAddr = devs[choice - 1].address;
        } else {
            string macStr = ask("mac address");
            if (!parseBtAddress(macStr, targetAddr)) {
                warn("invalid mac address format.");
                return;
            }
        }

        string packet = serializePacket(msg);
        cout << GRY << "  connecting..." << RESET << "\n";
        if (transmitBtPacket(targetAddr, packet)) {
            note("message sent.");
            int idx = findMessageIndex(msg.id);
            if (idx != -1) msgArray[idx].status = "Delivered";
            saveAllMessages();
        } else {
            warn("failed to connect or send.");
        }
    } else if (op == 3) {
        if (messageCount == 0) {
            warn("vault is empty.");
            return;
        }
        int id = readInt("  message id ", 1, 999999999);
        int idx = findMessageIndex(id);
        if (idx == -1) {
            warn("id not found.");
            return;
        }

        string macStr = ask("destination mac (leave empty to scan)");
        BTH_ADDR targetAddr = 0;
        if (!parseBtAddress(macStr, targetAddr)) {
            cout << GRY << "  scanning..." << RESET << "\n";
            vector<DiscoveredBtDevice> devs = scanNearbyBluetoothDevices();
            if (devs.empty()) {
                warn("no devices found.");
                return;
            }
            cout << "\n";
            for (size_t i = 0; i < devs.size(); i++) {
                cout << "  " << CYN << (i + 1) << RESET << ". " 
                     << YEL << devs[i].addrString << RESET << "  " << devs[i].name << "\n";
            }
            cout << "\n";
            int choice = readInt("  pick ", 1, (int)devs.size());
            targetAddr = devs[choice - 1].address;
        }

        string packet = serializePacket(msgArray[idx]);
        cout << GRY << "  sending..." << RESET << "\n";
        if (transmitBtPacket(targetAddr, packet)) {
            note("delivered.");
            msgArray[idx].status = "Delivered";
            saveAllMessages();
        } else {
            warn("connection failed.");
        }
    } else if (op == 4) {
        if (g_btServerRunning) {
            stopBluetoothServer();
            note("bluetooth listener stopped.");
        } else {
            startBluetoothServer();
            note("bluetooth listener started.");
        }
    }
}
