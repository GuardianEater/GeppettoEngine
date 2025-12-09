// wraps a couple functions from enet for quality of life

#pragma once

#include <string>

#include "enet/enet.h"
#undef min
#undef max
namespace Gep::Net
{
    // initializes ENet, must be called before anything else
    void InitializeENet();

    // connects a host to a server returning the connection
    ENetPeer* ConnectToServer(ENetHost* client, const std::string& ip, uint16_t port);

    // disconects a given connection
    void DisconnectFromServer(ENetPeer* server);

    // creates a client for the local system
    ENetHost* CreateClient();

    // sends a message across a connection
    void MessageSend(ENetPeer* connection, const std::string& msg);

    ENetEvent PollConnection(ENetPeer* connection);

    template<typename OnConnectFunc, typename OnDisconnectFunc, typename OnMessageRecievedFunc>
        requires std::is_invocable_v<OnMessageRecievedFunc, const std::string&>
    void HandleEvent(const ENetEvent& event, const OnConnectFunc& onConnect, const OnDisconnectFunc& onDisconnect, const OnMessageRecievedFunc& onMessage);
    
    void ExitENet();
}

namespace Gep::Net
{
    template<typename OnConnectFunc, typename OnDisconnectFunc, typename OnMessageRecievedFunc>
        requires std::is_invocable_v<OnMessageRecievedFunc, const std::string&>
    void HandleEvent(const ENetEvent& event, const OnConnectFunc& onConnect, const OnDisconnectFunc& onDisconnect, const OnMessageRecievedFunc& onMessage)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                if constexpr (std::is_invocable_v<OnConnectFunc, const ENetEvent&>)
                    onConnect(event);
                else if constexpr (std::is_invocable_v<OnConnectFunc, ENetPeer*>)
                    onConnect(event.peer);
                else if constexpr (std::is_invocable_v<OnConnectFunc>)
                    onConnect();
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                if constexpr (std::is_invocable_v<OnDisconnectFunc, const ENetEvent&>)
                    onDisconnect(event);
                else if constexpr (std::is_invocable_v<OnDisconnectFunc, ENetPeer*>)
                    onDisconnect(event.peer);
                else if constexpr (std::is_invocable_v<OnDisconnectFunc>)
                    onDisconnect();
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
            {
                if (event.packet && event.packet->data && event.packet->dataLength > 0)
                {
                    const char* data = reinterpret_cast<const char*>(event.packet->data);
                    const size_t size = static_cast<size_t>(event.packet->dataLength);

                    std::string msg(data, size);

                    // may include a trailing null
                    if (!msg.empty() && msg.back() == '\0')
                        msg.pop_back();

                    onMessage(msg);
                }

                if (event.packet)
                    enet_packet_destroy(event.packet);
                break;
            }
        }
    }
}