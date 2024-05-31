#pragma once

#include "../../inc/enet.h"
#include "../../inc/zmq.hpp"
#include "../../lib/payloadbuilder/inc/payload-builder.h"
#include <chrono>
#include <string>

class UdpManager {
public:
    static UdpManager* getUdpManager();
    UdpManager(in6_addr ipv6, uint16_t port);
    ~UdpManager();
    void attachListeners();
    int initializeENet();
    void deinitializeENet();
    ENetHost* createENetHost(size_t channelLimit, uint32_t incomingBandwidth, uint32_t outgoingBandwidth, int bufferSize);
    void destroyENetHost();
    void run(uint32_t timeout);

    bool queueData(const std::string &serializedMessage);
    void publishData();

    static bool enetPeerSend(ENetPeer* peer, uint8_t channel, ENetPacket* packet);
  
    bool isRunning() noexcept;
    void reset_loop_start();
    long get_loop_elapsed_time();
    bool m_running{ false };
    ENetHost* server;
protected:
    std::unordered_map<std::string, ENetPeer*> m_peers;
private:
    ENetPeer* relay_peer;
    in6_addr ipv6;
    uint16_t port;
    ENetAddress address;
    static UdpManager* udpManagerInstance;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> loop_start;
    zmq::socket_t publisher;

    zmq::socket_t subscriber;
    

    std::vector<std::string> m_pub_queue;
    std::vector<zmq::message_t> read_buffer;
    void Initialize();

    void processMessages(PayloadBuilder& pb, zmq::pollitem_t* items);
};

enum PeerType : uint32_t {
    Client = 0,
    RelayService = 1
};

struct PeerInfo
{
    std::string address;
    uint16_t port;
    PeerType type;
    std::string roomId;
    std::string userId;
};
