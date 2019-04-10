/*
    MIT License

    Copyright (c) 2018-2019 NovusCore

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#pragma once
#include <NovusTypes.h>
#include <entt.hpp>

#include "../NovusEnums.h"
#include "../Components/PlayerPositionComponent.h"
#include "../Components/UnitStatusComponent.h"
#include "../Components/Singletons/SingletonComponent.h"
#include "../Components/Singletons/PlayerUpdatesQueueSingleton.h"
#include "../Components/Singletons/PlayerPacketQueueSingleton.h"

namespace ClientUpdateSystem
{
	void Update(entt::registry &registry) 
    {
        SingletonComponent& singleton = registry.ctx<SingletonComponent>();
        PlayerUpdatesQueueSingleton& playerUpdatesQueue = registry.ctx<PlayerUpdatesQueueSingleton>();
        PlayerPacketQueueSingleton& playerPacketQueue = registry.ctx<PlayerPacketQueueSingleton>();
        NovusConnection& novusConnection = *singleton.connection;

        auto view = registry.view<PlayerConnectionComponent, PlayerUpdateDataComponent>();
        view.each([&novusConnection, playerUpdatesQueue](const auto, PlayerConnectionComponent& clientConnection, PlayerUpdateDataComponent& clientUpdateData)
        {
            for (PlayerUpdatePacket playerUpdatePacket : playerUpdatesQueue.playerUpdatePacketQueue)
            {
                if ((playerUpdatePacket.updateType == UPDATETYPE_CREATE_OBJECT ||
                    playerUpdatePacket.updateType == UPDATETYPE_CREATE_OBJECT2) &&
                    playerUpdatePacket.characterGuid != clientConnection.characterGuid)
                {
                    if (std::find(clientUpdateData.visibleGuids.begin(), clientUpdateData.visibleGuids.end(), playerUpdatePacket.characterGuid) == clientUpdateData.visibleGuids.end())
                    {
                        novusConnection.SendPacket(clientConnection.accountGuid, playerUpdatePacket.data, playerUpdatePacket.opcode);
                        clientUpdateData.visibleGuids.push_back(playerUpdatePacket.characterGuid);
                    }
                }
                else if (playerUpdatePacket.updateType == UPDATETYPE_VALUES)
                {
                    // So far we have not observed any issues with sending private field flags to any other client but themselves, this offers a good speed increase but if we see issues in the future we should recheck this.
                    // if (playerUpdatePacket.characterGuid != clientConnection.characterGuid)
                    {
                        novusConnection.SendPacket(clientConnection.accountGuid, playerUpdatePacket.data, playerUpdatePacket.opcode);
                    }
                }
            }

            for (MovementPacket movementPacket : playerUpdatesQueue.playerMovementPacketQueue)
            {
                if (clientConnection.characterGuid != movementPacket.characterGuid)
                {
                    novusConnection.SendPacket(clientConnection.accountGuid, movementPacket.data, movementPacket.opcode);
                }
            }

            for (ChatPacket chatPacket : playerUpdatesQueue.playerChatPacketQueue)
            {
                novusConnection.SendPacket(clientConnection.accountGuid, chatPacket.data, Common::Opcode::SMSG_MESSAGECHAT);
            }
        });

        // These packets should already include headers and data.
        Common::ByteBuffer packet;
        while (playerPacketQueue.packetQueue->try_dequeue(packet))
        {
            novusConnection.SendPacket(packet);
        }

        // Clear Queues
        if (playerUpdatesQueue.playerUpdatePacketQueue.size() != 0)
            playerUpdatesQueue.playerUpdatePacketQueue.clear();

        if (playerUpdatesQueue.playerMovementPacketQueue.size() != 0)
            playerUpdatesQueue.playerMovementPacketQueue.clear();

        if (playerUpdatesQueue.playerChatPacketQueue.size() != 0)
            playerUpdatesQueue.playerChatPacketQueue.clear();
	}
}
