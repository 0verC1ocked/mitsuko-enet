#include "../server.h"


UdpManager* UdpManager::udpManagerInstance = nullptr;

UdpManager* UdpManager::getUdpManager() {
    if (udpManagerInstance == nullptr) {
        udpManagerInstance = new UdpManager(ENET_HOST_ANY, 7777);
        udpManagerInstance->Initialize();
    }
    return udpManagerInstance;
}

UdpManager::UdpManager(in6_addr ipv6, uint16_t port) {
    this->ipv6 = ipv6;
    this->port = port;
    this->relay_peer = nullptr;
}

UdpManager::~UdpManager() {
}

void UdpManager::Initialize() {
    UdpManager::getUdpManager()->port = this->port;
    UdpManager::getUdpManager()->ipv6 = this->ipv6;
    UdpManager::getUdpManager()->address.port = this->port;
    UdpManager::getUdpManager()->address.ipv6 = this->ipv6;
    UdpManager::getUdpManager()->m_running = true;
}

void UdpManager::attachListeners() {}

int UdpManager::initializeENet() {
    ENetCallbacks callbacks;
    callbacks.malloc = [](size_t size) -> void* {
        return ::operator new(size);
    };
    callbacks.free = [](void* memory) {
        ::operator delete(memory);
    };
    return enet_initialize_with_callbacks(ENET_VERSION_CREATE(1, 3, 0), &callbacks);
}

void UdpManager::deinitializeENet() {
    enet_deinitialize();
    return;
}

ENetHost* UdpManager::createENetHost(size_t channelLimit, uint32_t incomingBandwidth, uint32_t outgoingBandwidth, int bufferSize) {
    UdpManager::getUdpManager()->server = enet_host_create(&UdpManager::getUdpManager()->address, ENET_PROTOCOL_MAXIMUM_PEER_ID, ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT, incomingBandwidth, outgoingBandwidth, bufferSize);
    if (UdpManager::getUdpManager()->server == NULL) {
        Logger::Log(ERROR, "An error occurred while trying to create an ENet server host");
    } else {
        Logger::Log(DEBUG, "Server started on port " + std::to_string(UdpManager::getUdpManager()->port));
        Logger::Log(INFO, "Server started on address " + std::to_string(ENET_HOST_ANY.__in6_u.__u6_addr32[0]));
    }
    return UdpManager::getUdpManager()->server;
}

void UdpManager::destroyENetHost() {
    if (UdpManager::getUdpManager()->server == NULL) {
        Logger::Log(ERROR, "Server is not initialized");
        return;
    }
    enet_host_destroy(UdpManager::getUdpManager()->server);
}

void UdpManager::run(uint32_t timeout) {
    ENetEvent event;
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);
    publisher.bind("tcp://*:5555");

    while (m_running) {
        {
            reset_loop_start();
            while (enet_host_service(UdpManager::getUdpManager()->server, &event, timeout) > 0) {
                try {
                    PayloadBuilder pb;
                    switch (event.type) {
                    case ENET_EVENT_TYPE_CONNECT:
                    {

                        enet_peer_ping_interval(event.peer, ENET_PEER_PING_INTERVAL);
                        enet_peer_timeout(event.peer, ENET_PEER_TIMEOUT_LIMIT, 500, 2000);
                        
                        PeerInfo* pinfo = new PeerInfo();
                        pinfo->address = std::to_string(event.peer->address.ipv6.__in6_u.__u6_addr32[0]);
                        pinfo->port = event.peer->address.port;
                        pinfo->type = static_cast<PeerType>(event.data);
                        event.peer->data = (void*)(pinfo);

                        if (event.data == PeerType::Client) {
                            Logger::Log(INFO, "A new client connected - host: " + pinfo->address + ", port: " + std::to_string(pinfo->port), 8);
                            PAYLOAD::Payload* payload = pb.newPayload();
                            pb.setEvent(PAYLOAD::Events::CONNECT)
                                .build(payload);
                            std::string serializedData = payload->SerializeAsString();
                            
                            
                            // Convert the serialized data to a uint8_t*
                            ENetPacket* packet_to_send = enet_packet_create(reinterpret_cast<const uint8_t*>(serializedData.data()), serializedData.length(), ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(event.peer, 0, packet_to_send);
                            Logger::Log(INFO, getEventString(payload->event()) + " event sent to client", 8);
                        } else if (event.data == PeerType::RelayService) {
                            relay_peer = event.peer;
                            Logger::Log(INFO, "RelayService client connected: " + pinfo->address + ":" + std::to_string(pinfo->port), 8);
                        }
                        break;
                    }
                    case ENET_EVENT_TYPE_RECEIVE:
                    {   
                        if (((PeerInfo*)event.peer->data)->type == PeerType::Client) {
                            PAYLOAD::Payload* receivedPayload = pb.newPayload();
                            receivedPayload->ParseFromArray(event.packet->data, event.packet->dataLength);
                            Logger::Log(INFO, "Received " + getEventString(receivedPayload->event()) + " from " + std::string(receivedPayload->data().userid()) + ":" + std::to_string(((PeerInfo*)event.peer->data)->port) + " in room " + std::string(receivedPayload->data().roomid()));
                            //Gameloop stuff
                            if (receivedPayload->event() == PAYLOAD::Events::CONNECT_ACK) {
                                ((PeerInfo*)event.peer->data)->roomId = receivedPayload->data().roomid();
                                ((PeerInfo*)event.peer->data)->userId = receivedPayload->data().userid();
                            }

                            std::string serializedData = receivedPayload->SerializeAsString();

                            UdpManager::getUdpManager()->queueData(serializedData, publisher);


                        } else if (((PeerInfo*)event.peer->data)->type == PeerType::RelayService) {
                            Logger::Log(INFO, "Received " + std::to_string(event.packet->dataLength) + " bytes from RelayService");
                        }
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    case ENET_EVENT_TYPE_DISCONNECT:
                    {
                        Logger::Log(INFO, "Disconnected - " + std::to_string(event.peer->address.ipv6.__in6_u.__u6_addr32[0]) + ":" + std::to_string(event.peer->address.port), 8);
                        if (((PeerInfo*)event.peer->data)->type == PeerType::Client) {
                            const std::string roomId = ((PeerInfo*)event.peer->data)->roomId;
                            const std::string userId = ((PeerInfo*)event.peer->data)->userId;
                            
                        } else {
                            Logger::Log(INFO, "Skipping disconnect action since its not a client", 8);
                        }
                        /* Reset the peer's client information. */
                        delete event.peer->data;
                        event.peer->data = NULL;

                        break;
                    }
                    case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                    {
                        Logger::Log(INFO, "Disconnected due to timeout - " + std::to_string(event.peer->address.ipv6.__in6_u.__u6_addr32[0]) + ":" + std::to_string(event.peer->address.port), 8);
                        if (((PeerInfo*)event.peer->data)->type == PeerType::Client) {
                            const std::string roomId = ((PeerInfo*)event.peer->data)->roomId;
                            const std::string userId = ((PeerInfo*)event.peer->data)->userId;
                            
                        } else {
                            Logger::Log(INFO, "Skipping disconnect action since its not a client", 8);
                        }
                        /* Reset the peer's client information. */
                        delete event.peer->data;
                        event.peer->data = NULL;

                        break;
                    }
                    case ENET_EVENT_TYPE_NONE:
                    {
                        Logger::Log(INFO, "Event type none packet received");
                        break;
                    }
                    }
                } catch (std::exception& e) {
                    Logger::Log(ERROR, e.what());
                }
            }
            
        }
    }
    UdpManager::getUdpManager()->destroyENetHost();
}

bool UdpManager::enetPeerSend(ENetPeer* peer, uint8_t channel, ENetPacket* packet) {
    try {
        return enet_peer_send(peer, channel, packet) == 0;
    } catch (std::exception& e) {
        Logger::Log(ERROR, e.what());
        return false;
    }
}

void UdpManager::reset_loop_start() {
    loop_start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

long UdpManager::get_loop_elapsed_time() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - loop_start).count();
}

bool UdpManager::queueData(std::string& serializedMessage, zmq::socket_t& publisher) {
    try {
        zmq::message_t payload(serializedMessage.size());
        std::copy(serializedMessage.begin(), serializedMessage.end(), static_cast<char*>(payload.data()));
        publisher.send(payload, zmq::send_flags::none);
        Logger::Log(INFO, "Payload published to subscribers");
        return true;
    } catch (std::exception& e) {
        Logger::Log(ERROR, e.what());
        return false;
    }
}
