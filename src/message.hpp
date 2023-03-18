enum MType {
    PAIRING_TO_MASTER,
    PAIRING_TO_SLAVE,
    KEEP_ALIVE,
};


struct Message {
    MType type;
    bool alive;
};
