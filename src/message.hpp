enum MType {
    PAIRING,
    PAIRING_RECV,
    KEEP_ALIVE,
};


struct Message {
    MType type;
    bool alive;
};
