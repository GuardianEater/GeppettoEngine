#include "pch.hpp"
#include "NetworkingHelp.hpp"

#include "enet/enet.h"

namespace Gep::Net
{
    void InitializeENet()
    {
        // Init ENet once in your program
        if (enet_initialize() != 0)
        {
            Gep::Log::Error("ENet failed to initialize");
        }
    }

    ENetPeer* ConnectToServer(ENetHost* self, const std::string& ip, uint16_t port)
    {
        if (!self)
        {
            Gep::Log::Error("Null host attempting to connect to server");
            return nullptr;
        }

        ENetAddress address;
        enet_address_set_host(&address, ip.c_str());    // example: "127.0.0.1"
        address.port = port;

        ENetPeer* serverPeer = enet_host_connect(self, &address, 2, 0);
        if (!serverPeer)
        {
            Gep::Log::Error("ENet failed to create connection to server");
            return nullptr;
        }

        ENetEvent event;
        if (enet_host_service(self, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
        {
            Gep::Log::Important("Successfully connected: [", ip, "][", port, "]");
            return serverPeer; // success
        }

        Gep::Log::Error("ENet failed to complete connection with the server: [", ip, "]");
        return nullptr;
    }

    void DisconnectFromServer(ENetPeer* server)
    {
        if (!server) return;

        enet_peer_disconnect(server, 0);

        // wait for the server to acknowledge (optional but recommended)
        ENetEvent event;
        if (enet_host_service(server->host, &event, 3000) > 0 && event.type == ENET_EVENT_TYPE_DISCONNECT)
        {
            Gep::Log::Important("Successfully disconnected:");
            return; // done
        }

        // If no response, force it
        enet_peer_reset(server);
    }

    ENetHost* CreateClient()
    {
        return enet_host_create(
            nullptr,  // no address = this is a client
            1,        // max connections
            2,        // channels
            0, 0
        );
    }

    // server is assumed to be valid
    void MessageSend(ENetPeer* server, const std::string& msg)
    {
        ENetPacket* packet = enet_packet_create(
            msg.c_str(),
            msg.size() + 1,
            ENET_PACKET_FLAG_RELIABLE
        );

        enet_peer_send(server, 0, packet);
        enet_host_flush(server->host);
    }

    ENetEvent PollConnection(ENetPeer* connection)
    {
        ENetEvent event;
        int eventStatus = enet_host_service(connection->host, &event, 1); // do not wait
        return event;
    }

    void ExitENet()
    {
        enet_deinitialize();
    }
}

