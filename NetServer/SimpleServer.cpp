#include<iostream>
#include<olc_net.h>

enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

class CustomServer : public olc::net::server_interface<CustomMsgTypes>
{
public:
	CustomServer(uint16_t nPort) : olc::net::server_interface<CustomMsgTypes>(nPort) {}

protected:
	virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client)
	{
		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerAccept;
		msg << client->GetID();
		
		client->Send(msg);
		return true;
	}

	virtual void OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client)
	{
		std::cout << "Removing Client [" << client->GetID() << "]" << std::endl;
	}

	virtual void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client, olc::net::message<CustomMsgTypes> msg)
	{
		switch (msg.header.id)
		{
		case CustomMsgTypes::ServerPing:
		{
			std::cout << "[" << client->GetID() << "] Ping Server" << std::endl;
			client->Send(msg);
		}
		break;
		case CustomMsgTypes::MessageAll:
		{
			std::cout << "[" << client->GetID() << "] MassageAll" << std::endl;
			MessageAllClients(msg, client);
		}
		break;
		}
	}
};

int main()
{
	CustomServer server(60000);
	server.Start();

	while (true)
	{
		server.Update();
	}
	return 0;
}