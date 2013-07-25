#include "ds/network/tcp_server.h"

#include "ds/debug/debug_defines.h"
#include "ds/debug/logger.h"

namespace ds {
namespace net {

namespace {
	class EchoConnection: public Poco::Net::TCPServerConnection {
	public:
		EchoConnection(	const Poco::Net::StreamSocket& s,
						ds::AsyncQueue<std::string>& q)
				: Poco::Net::TCPServerConnection(s)
				, mQueue(q) {
		}
		
		void run() {
			Poco::Net::StreamSocket&		ss = socket();
			try {
				char buffer[256];
				int n = ss.receiveBytes(buffer, sizeof(buffer));
				while (n > 0) {
					mQueue.push(std::string(buffer, n));
//					std::cout << "handle size=" << n << " me=" << (void*)this << std::endl;
					n = ss.receiveBytes(buffer, sizeof(buffer));
				}
			}
			catch (Poco::Exception&) {
//				std::cerr << "EchoConnection: " << exc.displayText() << std::endl;
			}
		}

		ds::AsyncQueue<std::string>&	mQueue;
	};

	class ConnectionFactory: public Poco::Net::TCPServerConnectionFactory {
	public:
		ConnectionFactory(ds::AsyncQueue<std::string>& q)
				: mQueue(q) {
		}

		Poco::Net::TCPServerConnection* createConnection(const Poco::Net::StreamSocket& socket) {
			return new EchoConnection(socket, mQueue);
		}

		ds::AsyncQueue<std::string>&	mQueue;
	};
}

/**
 * \class ds::TcpServer
 */
TcpServer::TcpServer(ds::ui::SpriteEngine& e, const Poco::Net::SocketAddress& address)
		: ds::AutoUpdate(e)
		, mAddress(address)
		, mServer(new ConnectionFactory(mQueue), Poco::Net::ServerSocket(address)) {
	try {
		mServer.start();
	} catch (std::exception const& ex) {
		DS_LOG_ERROR("TcpServer failed to start (" << address.toString() << ") error=" << ex.what());
	}
}

void TcpServer::add(const std::function<void(const std::string&)>& f) {
	if (!f) return;

	try {
		mListener.push_back(f);
	} catch (std::exception&) {
	}
}

#if 0
// This is not how you'd do this --- if you want to send to the server,
// use a SocketSender like TcpClient
	// Send data through the server
	void							send(const std::string& data);

void TcpServer::send(const std::string& data) {
	if (data.empty()) return;
	try {
		Poco::Net::StreamSocket	socket(mAddress);
		socket.sendBytes(data.data(), data.size());
	} catch (std::exception const& ex) {
		DS_LOG_WARNING("TcpServer::send() error sending data=" << data << " (" << ex.what() << ")");
		DS_DBG_CODE(std::cout << "TcpServer::send() error sending data=" << data << " (" << ex.what() << ")" << std::endl);
	}
}
#endif

void TcpServer::update(const ds::UpdateParams&) {
	const std::vector<std::string>* vec = mQueue.update();
	if (!vec) return;

	for (auto it=mListener.begin(), end=mListener.end(); it != end; ++it) {
		for (auto pit=vec->begin(), pend=vec->end(); pit != pend; ++pit) (*it)(*pit);
	}
}

} // namespace net
} // namespace ds