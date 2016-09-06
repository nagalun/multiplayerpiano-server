#include <uWS.h>
#include <unordered_map>
#include <map>
#include <functional>
#include <set>
#include "json.hpp"
#include <sys/time.h>

template <typename I> std::string n2hexstr(I w, size_t hex_len = sizeof(I)<<1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}

struct clinfo_t {
	std::set<uWS::WebSocket> sockets;
	std::string id;
	float x;
	float y;
};

class server {
public:
	class Client;
	class Room;
	struct mppconn_t {
		Client* user;
		/* Socket, room */
		std::unordered_map<uWS::WebSocket, std::string> sockets;
	};
	class Client {
		uint32_t color;
		std::string name;
		std::string _id;
	public:
		Client(std::string n_id){name="Anonymoose"; color=0xFF0000FF; _id=n_id;};
		nlohmann::json get_json();
		void set_name(std::string n){name=n;};
		/*void set_color(uint32_t); /* related to this client */
	};
	class Room {
		/* Client pointer and it's id */
		bool lobby, visible, chat, crownsolo;
		uint32_t color;
		std::unordered_map<Client*, clinfo_t> ids;
	public:
		Room(){color=0x7E0404FF; lobby=false; visible=true; chat=true; crownsolo=false;};
		nlohmann::json get_json(std::string);
		std::string join_usr(uWS::WebSocket, mppconn_t&, std::string);
		bool kick_usr(uWS::WebSocket, mppconn_t&);
		/*void set_param(void){};*/
	};
	class msg {
	public:
		static nlohmann::json n(server*, nlohmann::json, uWS::WebSocket);
		static nlohmann::json a(server*, nlohmann::json, uWS::WebSocket);
		static nlohmann::json m(server*, nlohmann::json, uWS::WebSocket);
		static nlohmann::json t(server*, nlohmann::json, uWS::WebSocket);
		static nlohmann::json ch(server*, nlohmann::json, uWS::WebSocket);
		
		static nlohmann::json hi(server*, nlohmann::json, uWS::WebSocket);
		static nlohmann::json bye(server*, nlohmann::json, uWS::WebSocket);
		
		static nlohmann::json chown(server*, nlohmann::json, uWS::WebSocket);
		static nlohmann::json chset(server*, nlohmann::json, uWS::WebSocket);
		static nlohmann::json userset(server*, nlohmann::json, uWS::WebSocket);
		
		static nlohmann::json kickban(server*, nlohmann::json, uWS::WebSocket);
		
		static nlohmann::json lsl(server*, nlohmann::json, uWS::WebSocket); /* -ls */
		static nlohmann::json lsp(server*, nlohmann::json, uWS::WebSocket); /* +ls */
	};
	server(uint16_t p){
		funcmap = {
			{"n", std::bind(msg::n, this, std::placeholders::_1, std::placeholders::_2)},
			{"a", std::bind(msg::a, this, std::placeholders::_1, std::placeholders::_2)},
			{"m", std::bind(msg::m, this, std::placeholders::_1, std::placeholders::_2)},
			{"t", std::bind(msg::t, this, std::placeholders::_1, std::placeholders::_2)},
			{"ch", std::bind(msg::ch, this, std::placeholders::_1, std::placeholders::_2)},
			{"hi", std::bind(msg::hi, this, std::placeholders::_1, std::placeholders::_2)},
			{"bye", std::bind(msg::bye, this, std::placeholders::_1, std::placeholders::_2)},
			{"chown", std::bind(msg::chown, this, std::placeholders::_1, std::placeholders::_2)},
			{"chset", std::bind(msg::chset, this, std::placeholders::_1, std::placeholders::_2)},
			{"userset", std::bind(msg::userset, this, std::placeholders::_1, std::placeholders::_2)},
			{"kickban", std::bind(msg::kickban, this, std::placeholders::_1, std::placeholders::_2)},
			{"-ls", std::bind(msg::lsl, this, std::placeholders::_1, std::placeholders::_2)},
			{"+ls", std::bind(msg::lsp, this, std::placeholders::_1, std::placeholders::_2)}
		};
		port = p;
	}
	void run();
	void reg_evts(uWS::Server &s);
	void parse_msg(nlohmann::json &msg, uWS::WebSocket socket);
	std::string set_room(std::string, uWS::WebSocket, mppconn_t&);
	nlohmann::json genusr(uWS::WebSocket);
private:
	uint16_t port;
	std::unordered_map<std::string, mppconn_t> clients;
	std::unordered_map<std::string, Room*> rooms;
	std::map<std::string, std::function<nlohmann::json(nlohmann::json,uWS::WebSocket)>> funcmap;
};
