#pragma once
#include"net_common.h"
#include"net_tsqueue.h"
#include"net_message.h"

namespace olc
{
	namespace net
	{
		template<typename T>
		class connection : public std::enable_shared_from_this<connection<T>>
		{
		public:
			enum class owner
			{
				server,
				client
			};

			connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn)
				:m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
			{
				m_nOwnerType = parent;
			}
			virtual ~connection() {}

			uint32_t GetID() const
			{
				return id;
			}

		public:
			void ConnectToClient(uint32_t uid = 0)
			{
				if (m_nOwnerType == owner::server)
				{
					if (m_socket.is_open())
					{
						id = uid;
						ReadHeader();
					}
				}
			}

			void ConnectToServer(asio::ip::tcp::resolver::results_type& endpoints)
			{
				if (m_nOwnerType == owner::client)
				{
					asio::async_connect(m_socket, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
					{
						if (!ec)
						{
							ReadHeader();
						}
					});
				}
			}
			
			void Disconnect()
			{
				if (IsConnected())
				{
					asio::post(m_asioContext, [this]() {m_socket.close(); });
				}
			}
			
			bool IsConnected() const
			{
				return m_socket.is_open();
			}

		public:
			void Send(const message<T>& msg)
			{
				asio::post(m_asioContext,
					[this, msg]()
				{
					bool bWritingMessage = !m_qMessagesOut.empty();
					m_qMessagesOut.push_back(msg);
					if (!bWritingMessage)
					{
						WriteHeader();
					}
				});
			}

		private:
			void ReadHeader()
			{
				asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
					[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_msgTemporaryIn.header.size > 0)
						{
							size_t body_size = m_msgTemporaryIn.header.size - sizeof(message_header<T>);
							m_msgTemporaryIn.body.resize(body_size);
							ReadBody();
						}
						else
						{
							AddToIncomingMessageQueue();
						}
					}
					else
					{
						std::cout << "[" << id << "] Read Head Failed \n";
						m_socket.close();
					}
				});
			}

			void ReadBody()
			{
				asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
					[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						AddToIncomingMessageQueue();
					}
					else
					{
						std::cout << "[" << id << "] Read Body Failed \n";
						m_socket.close();
					}
				});
			}

			void WriteHeader()
			{
				asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
					[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_qMessagesOut.front().body.size() > 0)
						{
							WriteBody();
						}
						else
						{
							m_qMessagesOut.pop_front();
							if (!m_qMessagesOut.empty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						std::cout << "[" << id << "] Wrire Head Failed \n";
						m_socket.close();
					}
				});
			}

			void WriteBody()
			{
				asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
					[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_qMessagesOut.pop_front();
						if (!m_qMessagesOut.empty())
						{
							WriteHeader();
						}
					}
					else
					{
						std::cout << "[" << id << "] Wrire Body Failed \n";
						m_socket.close();
					}
				});
			}

			void AddToIncomingMessageQueue()
			{
				if (m_nOwnerType == owner::server)
					m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
				else
					m_qMessagesIn.push_back({ nullptr, m_msgTemporaryIn });

				ReadHeader();
			}

		protected:
			asio::ip::tcp::socket m_socket;

			asio::io_context& m_asioContext;

			tsqueue<message<T>> m_qMessagesOut;

			tsqueue<owned_message<T>>& m_qMessagesIn;

			message<T> m_msgTemporaryIn;
			
			owner m_nOwnerType = owner::server;
			uint32_t id = 0;
		};
	}
}
#pragma once
