#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "common.h"
#include "vault.h"

extern atomic<bool> g_btServerRunning;
extern SOCKET g_btListenSocket;
extern thread* g_btServerThread;

string formatBtAddress(BTH_ADDR addr);
bool parseBtAddress(const string &str, BTH_ADDR &addr);
vector<DiscoveredBtDevice> scanNearbyBluetoothDevices();
void btServerWorker();
void startBluetoothServer();
void stopBluetoothServer();
bool transmitBtPacket(BTH_ADDR targetAddr, const string &packetData, int port = 1);
void bluetoothMeshHub();

#endif

