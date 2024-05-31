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
    zmq::context_t pub_context(1);
    publisher = zmq::socket_t(pub_context, ZMQ_PUB);
    publisher.bind("tcp://*:5555");
    

    zmq::context_t sub_context(2);
    subscriber = zmq::socket_t(sub_context, ZMQ_SUB);
    subscriber.connect("tcp://localhost:6666");
    

    zmq::pollitem_t items[] = {
        {static_cast<void*>(subscriber), 0, ZMQ_POLLIN, 0}
    };


    subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);

   


    while (m_running) {
        {
            reset_loop_start();
            PayloadBuilder pb;
            processMessages(pb, items);
            while (enet_host_service(UdpManager::getUdpManager()->server, &event, timeout) > 0) {
                try {
                    
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
                        Logger::Log(INFO, "Connected - " + std::to_string(event.peer->address.ipv6.__in6_u.__u6_addr32[0]) + ":" + std::to_string(event.peer->address.port), 8);
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
                            if (receivedPayload->event() == PAYLOAD::Events::CONNECT_ACK) {
                                ((PeerInfo*)event.peer->data)->roomId = receivedPayload->data().roomid();
                                ((PeerInfo*)event.peer->data)->userId = receivedPayload->data().userid();
                                
                                m_peers[receivedPayload->data().userid()] = event.peer;
                            }

                            IPC::IPCMessageType ipcMessageType = IPC::IPCMessageType::IPC_NONE;

                            if (receivedPayload->event() == PAYLOAD::Events::CONNECT_ACK || receivedPayload->event() == PAYLOAD::Events::DISCONNECT || receivedPayload->event() == PAYLOAD::Events::CHAT_EMOJI || receivedPayload->event() == PAYLOAD::Events::MESSAGE || receivedPayload->event() == PAYLOAD::Events::FORFIET_MATCH) {
                                ipcMessageType = IPC::IPCMessageType::IPC_P0_MATCH_REQUEST;
                            } else {
                                ipcMessageType = IPC::IPCMessageType::IPC_P1_MATCH_REQUEST;
                            } 

                            IPC::IPCMessage* ipcMessage = pb.newIPCMessage();

                            ipcMessage->set_type(ipcMessageType);
                            ipcMessage->set_matchid(receivedPayload->data().roomid());
                            ipcMessage->set_data(receivedPayload->SerializeAsString());

                            std::string serializedData = ipcMessage->SerializeAsString();

                            UdpManager::getUdpManager()->queueData(serializedData);


                        } else if (((PeerInfo*)event.peer->data)->type == PeerType::RelayService) {
                            IPC::IPCMessage* ipcMessage = pb.newIPCMessage();
                            ipcMessage->set_type(IPC::IPCMessageType::IPC_CREATE_MATCH_REQUEST);
                            ipcMessage->set_data(event.packet->data, event.packet->dataLength);
                            std::string serializedData = ipcMessage->SerializeAsString();
                            UdpManager::getUdpManager()->queueData(serializedData);
                        }
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    case ENET_EVENT_TYPE_DISCONNECT:
                    {
                        Logger::Log(INFO, "Disconnected - " + ((PeerInfo*)event.peer->data)->userId + " on room " + ((PeerInfo*)event.peer->data)->roomId);
                        if (((PeerInfo*)event.peer->data)->type == PeerType::Client) {
                            const std::string roomId = ((PeerInfo*)event.peer->data)->roomId;
                            const std::string userId = ((PeerInfo*)event.peer->data)->userId;

                            m_peers.erase(userId);
                            
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
                        
                        Logger::Log(INFO, "Disconnected due to timeout - " + ((PeerInfo*)event.peer->data)->userId + " on room " + ((PeerInfo*)event.peer->data)->roomId);
                        if (((PeerInfo*)event.peer->data)->type == PeerType::Client) {
                            const std::string roomId = ((PeerInfo*)event.peer->data)->roomId;
                            const std::string userId = ((PeerInfo*)event.peer->data)->userId;

                            m_peers.erase(userId);
                            
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
            publishData();
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

bool UdpManager::queueData(const std::string& serializedMessage) {
    try {
        m_pub_queue.push_back(serializedMessage);
        return true;
    } catch (std::exception& e) {
        Logger::Log(ERROR, e.what());
        return false;
    }
}

void UdpManager::publishData() {
    try {
        for (auto& message : m_pub_queue) {
            zmq::message_t msg(message.size());
            memcpy(msg.data(), message.c_str(), message.size());
            publisher.send(msg);
            
            Logger::Log(INFO, "Payload published to subscribers");
        }
        m_pub_queue.clear();
    } catch (std::exception& e) {
        Logger::Log(ERROR, e.what());
    }
}

void UdpManager::processMessages(PayloadBuilder& pb, zmq::pollitem_t* items) {
    try {
        
        zmq::poll(&items[0], 1, std::chrono::milliseconds(1000));
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t message;
            if (subscriber.recv(message)) {
                read_buffer.push_back(std::move(message));
            }
        }

        for (const zmq::message_t& message : read_buffer) {
            IPC::IPCMessage* ipcMessage =  pb.newIPCMessage();
            ipcMessage->ParseFromArray(message.data(), message.size());
            Logger::Log(DEBUG, "Received -> " + ipcMessage->ShortDebugString());
            ENetPeer* peer = m_peers[ipcMessage->userid()];
            if (peer == nullptr) {
                Logger::Log(ERROR, "Peer not found");
                return;
            }
            switch (ipcMessage->type()) {
                case IPC::IPCMessageType::IPC_ENET_SEND: {
                    ENetPacket* packet = enet_packet_create(reinterpret_cast<const uint8_t*>(ipcMessage->data().data()), ipcMessage->data().size(), ENET_PACKET_FLAG_RELIABLE);
                    enetPeerSend(peer, 0, packet);
                    Logger::Log(INFO, "ENet packet sent to peer");
                    break;
                }
                case IPC::IPCMessageType::IPC_ENET_STREAM: {
                    break;
                }
                default: {
                    Logger::Log(ERROR, "Unknown IPC message type");
                    break;
                }
            }
        }

        read_buffer.clear();
    } catch (const std::exception& e) {
        Logger::Log(ERROR, e.what());
    }

}
