#pragma once

// Mock interfaces to allow compilation without the massive libwebrtc binary dependency during development.

namespace webrtc {
    class PeerConnectionInterface {
    public:
        virtual ~PeerConnectionInterface() = default;
    };

    class DataChannelInterface {
    public:
        virtual ~DataChannelInterface() = default;
    };

    class VideoTrackSourceInterface {
    public:
        virtual ~VideoTrackSourceInterface() = default;
    };

    class AudioTrackSourceInterface {
    public:
        virtual ~AudioTrackSourceInterface() = default;
    };

    class VideoTrackInterface {
    public:
        virtual ~VideoTrackInterface() = default;
    };

    class AudioTrackInterface {
    public:
        virtual ~AudioTrackInterface() = default;
    };

    class MediaStreamInterface {
    public:
        virtual ~MediaStreamInterface() = default;
    };

    class PeerConnectionFactoryInterface {
    public:
        virtual ~PeerConnectionFactoryInterface() = default;
    };
}

namespace rtc {
    class Thread {
    public:
        virtual ~Thread() = default;
    };
}
