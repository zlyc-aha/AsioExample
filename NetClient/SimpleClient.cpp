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

void test1()
{
	olc::net::message<CustomMsgTypes> msg;
	msg.header.id = CustomMsgTypes::ServerAccept;

	int a = 1;
	bool b = true;
	float c = 3.14159f;

	struct
	{
		float x = 1.0f;
		float y = 2.0f;
	}d[5];

	msg << a << b << c << d;

	a = 99;
	b = false;
	c = 99.0f;

	for (auto& p : d)
	{
		p.x = 3.0f;
		p.y = 4.0f;
	}

	msg >> d >> c >> b >> a;
}

class CustomClient : public olc::net::client_interface<CustomMsgTypes>
{
public:
	void PingServer()
	{
		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerPing;

		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		msg << timeNow;
		Send(msg);
	}

	void MessageAll()
	{
		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::MessageAll;
		msg << GetID();
		Send(msg);
	}
};

int main()
{
	CustomClient c;
	c.Connect("127.0.0.1", 60000);

	bool bQuit = false;

	bool key[3] = { false, false, false };
	bool old_key[3] = { false, false, false };

	while (!bQuit)
	{
		if (GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}

		if (key[0] && !old_key[0])
			c.PingServer();
		
		if (key[1] && !old_key[1])
			c.MessageAll();

		if (key[2] && !old_key[2])
			bQuit = true;

		for (int i = 0; i < 3; ++i)
			old_key[i] = key[i];

		if (c.IsConnected())
		{
			if (!c.Incoming().empty())
			{
				auto msg = c.Incoming().pop_front().msg;
				switch (msg.header.id)
				{
				case CustomMsgTypes::ServerPing:
				{
					std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point timeThen;
					msg >> timeThen;
					std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << std::endl;
				}
				break;
				case CustomMsgTypes::ServerAccept:
				{
					uint32_t id;
					msg >> id;
					c.SetID(id);
					std::cout << "[" << id << "] Connect Server" << std::endl;
				}
				break;
				case CustomMsgTypes::MessageAll:
				{
					uint32_t ID;
					msg >> ID;
					std::cout << "["<<ID<<"] Hello"<< std::endl;
				}
				break;
				}
			}
		}
	}

	system("pause");
	return 0;
}