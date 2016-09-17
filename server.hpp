#include <uWS.h>
#include <deque>
#include <unordered_map>
#include <map>
#include <functional>
#include <set>
#include "json.hpp"
#include <sys/time.h>
#include "limiter.hpp"
#include <fstream>

/* id, connections (multiple tabs),
 * and mouse x & y of the client in a room.
 ***/
struct clinfo_t {
	std::set<uWS::WebSocket> sockets;
	RoomLimit quota;
	std::string id;
	float x;
	float y;
};

class server {
public:
	class Client;
	class Room;
	class Database {
		std::string dir;
	public:
		struct pinfo_t {
			bool found;
			uint32_t color;
			std::string name;
		};
		Database(const std::string& dir) : dir(dir){ mkdir(dir.c_str(), 0700); };
		pinfo_t get_usrinfo(uint32_t);
		void set_usrinfo(uint32_t, pinfo_t);
	};
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
		Client(uint32_t hsh, std::string n_id, uint32_t clr, std::string nme) :
			color(clr),
			name(nme),
			_id(n_id),
			dhash(hsh),
			changed(false){};
		nlohmann::json get_json();
		Database::pinfo_t get_dbdata();
		void set_name(std::string n){name=n;};
		void set_color(uint32_t c){color=c;};
		ClientLimit quota;
		uint32_t dhash;
		bool changed;
	};
	class Room {
		struct oinfo_t {
			Client* owner;
			Client* oldowner;
			std::array<float,2> startpos;
			std::array<float,2> endpos;
			long int time;
		};
		bool lobby, visible, chat, crownsolo;
		uint32_t color;
		oinfo_t crown;
		std::unordered_map<Client*, clinfo_t> ids;
		std::deque<nlohmann::json> chatlog;
	public:
		Room(bool lby) :
			lobby(lby),
			visible(true),
			chat(true),
			crownsolo(false),
			color(0x242464),
			crown({NULL, NULL, {50, 50}, {50, 50}, 0}){};
		nlohmann::json get_json(std::string, bool);
		nlohmann::json get_chatlog_json();
		void push_chat(nlohmann::json);
		clinfo_t* get_info(Client*);
		Client* get_client(std::string);
		oinfo_t get_crowninfo(){return crown;};
		bool is_lobby(){return lobby;};
		bool chat_on(){return chat;};
		bool is_crownsolo(){return crownsolo;};
		bool is_owner(Client* c){return c == crown.owner;};
		std::string join_usr(uWS::WebSocket&, mppconn_t&, std::string);
		void broadcast(nlohmann::json, uWS::WebSocket&);
		bool kick_usr(uWS::WebSocket&, mppconn_t&, std::string);
		void set_param(nlohmann::json, std::string);
		void set_owner(Client*);
		void part_upd(Client*);
	};
	class msg {
	public:
		static void n(server*, nlohmann::json, uWS::WebSocket&);
		static void a(server*, nlohmann::json, uWS::WebSocket&);
		static void m(server*, nlohmann::json, uWS::WebSocket&);
		static void t(server*, nlohmann::json, uWS::WebSocket&);
		static void ch(server*, nlohmann::json, uWS::WebSocket&);
		
		static void hi(server*, nlohmann::json, uWS::WebSocket&);
		
		static void chown(server*, nlohmann::json, uWS::WebSocket&);
		static void chset(server*, nlohmann::json, uWS::WebSocket&);
		static void userset(server*, nlohmann::json, uWS::WebSocket&);
		
		static void adminmsg(server*, nlohmann::json, uWS::WebSocket&);
		static void kickban(server*, nlohmann::json, uWS::WebSocket&);
		
		static void lsl(server*, nlohmann::json, uWS::WebSocket&); /* -ls */
		static void lsp(server*, nlohmann::json, uWS::WebSocket&); /* +ls */
	};
	server(uint16_t p) : port(p), db("database/"){
		funcmap = {
			{"n", std::bind(msg::n, this, std::placeholders::_1, std::placeholders::_2)},
			{"a", std::bind(msg::a, this, std::placeholders::_1, std::placeholders::_2)},
			{"m", std::bind(msg::m, this, std::placeholders::_1, std::placeholders::_2)},
			{"t", std::bind(msg::t, this, std::placeholders::_1, std::placeholders::_2)},
			{"ch", std::bind(msg::ch, this, std::placeholders::_1, std::placeholders::_2)},
			{"hi", std::bind(msg::hi, this, std::placeholders::_1, std::placeholders::_2)},
			{"chown", std::bind(msg::chown, this, std::placeholders::_1, std::placeholders::_2)},
			{"chset", std::bind(msg::chset, this, std::placeholders::_1, std::placeholders::_2)},
			{"userset", std::bind(msg::userset, this, std::placeholders::_1, std::placeholders::_2)},
			{"adminmsg", std::bind(msg::adminmsg, this, std::placeholders::_1, std::placeholders::_2)},
			{"kickban", std::bind(msg::kickban, this, std::placeholders::_1, std::placeholders::_2)},
			{"-ls", std::bind(msg::lsl, this, std::placeholders::_1, std::placeholders::_2)},
			{"+ls", std::bind(msg::lsp, this, std::placeholders::_1, std::placeholders::_2)}
		};
	}
	void run();
	void reg_evts(uWS::Server &s);
	void parse_msg(nlohmann::json &msg, uWS::WebSocket& socket);
	std::string set_room(std::string, uWS::WebSocket&, mppconn_t&, nlohmann::json);
	void user_upd(std::string);
	void rooml_upd(Room*, std::string);
	nlohmann::json get_roomlist();
	nlohmann::json genusr(uWS::WebSocket&);
private:
	uint16_t port;
	server::Database db;
	std::unordered_map<std::string, mppconn_t> clients;
	std::unordered_map<std::string, Room*> rooms;
	std::set<uWS::WebSocket> roomlist_watchers;
	std::map<std::string, std::function<void(nlohmann::json,uWS::WebSocket&)>> funcmap;
};
