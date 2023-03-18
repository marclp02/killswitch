enum MType {
    PAIRING_TO_MASTER,
    PAIRING_TO_SLAVE,
    KEEP_ALIVE,
};

struct SlavePairingMessage {
    MType mType = PAIRING_TO_MASTER;
};

struct MasterPairingMessage {
    MType mType = PAIRING_TO_SLAVE;
};

struct KeepAliveMessage {
    MType mType = KEEP_ALIVE;
    int value = 1;
};
