#pragma once
#include <string>

enum class NetMuxMouseButton {
    Left = 0,
    Right = 1,
    Middle = 2
};

enum class NetMuxDriverType {
    Interception,
    ViGEmBus,
    Auto
};

enum class NetMuxPacketType {
    Movement,
    AbsoluteMovement,
    Click,
    Sync,
    Discovery,
    Heartbeat,
    ClipboardSync,
    Handshake,
    FocusUpdate,
    SessionUpdate,
    MasterStateSync,
    Ping,
    InputEvent,
    ResolutionUpdate,
    AuthChallenge,
    AuthResponse,
    SyncCheck,
    SelectionUpdate,
    Disconnect,
    Wheel
};

struct Packet {
    unsigned long long senderId;
    unsigned int groupId;
    unsigned int sequenceNumber;
    double localTimestamp; // To measure E2E latency
    NetMuxPacketType type;
    int x;
    int y;
    int button;
    bool down;
    bool isSelecting;
    int selectionStartX;
    int selectionStartY;
    int wheelDelta;
    bool isHorizontalWheel;
    char payload[1024]; // Dynamic data payload (e.g. clipboard text)
    int payloadSize;
};

struct Config {
    int boundaryX; // Screen coordinate where crossing happens
    int boundaryY;
    bool isLeft;   // Crossing to the left or right
};

struct AppSettings {
    bool isServer;
    std::string remoteIp;
    int port;
    Config inputConfig;
    float cursorScale = 1.0f;
    bool useD3D11 = false;
    NetMuxDriverType driverType = NetMuxDriverType::Auto;
    unsigned int groupId = 0;
    std::string groupName = "DefaultGroup";
    std::string sessionName = "DefaultSession";
    std::string securityKey = "";
    std::string cursorThemePath = "";
    unsigned char selectionColorR = 0;
    unsigned char selectionColorG = 120;
    unsigned char selectionColorB = 215;
};
