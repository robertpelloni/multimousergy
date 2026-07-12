#pragma once
#include <string>
#include <vector>

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
    WebRTCSignaling,
    AuthChallenge,
    AuthResponse,
    SyncCheck,
    SelectionUpdate,
    Disconnect,
    Wheel,
    FileHeader,
    FileData,
    KeyboardEvent,
    DeltaMovement,
    WebRTCOffer,
    WebRTCAnswer,
    ICECandidate,
    VideoFrame
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
    int chunkIndex;
    int totalChunks;
    float dpiScale;     // DPI Scaling factor (e.g. 1.25)
    char payload[4096]; // Dynamic data payload (e.g. clipboard text)
    int payloadSize;

    bool IsHeaderOnly() const {
        return (type == NetMuxPacketType::Movement ||
                type == NetMuxPacketType::AbsoluteMovement ||
                type == NetMuxPacketType::DeltaMovement ||
                type == NetMuxPacketType::Sync ||
                type == NetMuxPacketType::Ping ||
                type == NetMuxPacketType::Heartbeat ||
                type == NetMuxPacketType::SyncCheck);
    }
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
    unsigned char peerColorR = 255;
    unsigned char peerColorG = 0;
    unsigned char peerColorB = 0;
    bool autoConnect = false;
    bool startMinimized = false;
    bool spatialMode = true;
    std::string displayName = "";
    std::vector<std::string> recentServers;
};
