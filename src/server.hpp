#include <uWS.h>
#include <deque>
#include <unordered_map>
#include <map>
#include <functional>
#include <set>
#include <nlohmann/json.hpp>
#include <sys/time.h>
#include <fstream>
#include <sstream>
#include "limiter.hpp"
#include "crossfuncs.hpp"

uint32_t getDefaultRoomColor();

/* Define VANILLA_SERVER if you don't want custom network messages, like non-JSON note data
 * #define VANILLA_SERVER
 */

/* id, connections (multiple tabs),
 * and mouse x & y of the client in a room.
 ***/
struct clinfo_t {
	std::set<uWS::WebSocket<uWS::SERVER> *> sockets;
	RoomLimit quota;
	std::string id;
	float x;
	float y;
};

/* Joined room client info */
struct jroom_clinfo_t {
	std::string id;
	bool newclient;
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
		Database(const std::string& dir) : dir(dir){
			makedir(dir);
		};
		pinfo_t get_usrinfo(std::string);
		void set_usrinfo(std::string, pinfo_t);
	};
	struct mppconn_t {
		Client* user;
		/* Socket, room */
		std::unordered_map<uWS::WebSocket<uWS::SERVER> *, std::string> sockets;
	};
	class Client {
		uint32_t color;
		std::string name;
		std::string _id;
	public:
		Client(std::string filen, std::string n_id, uint32_t clr, std::string nme) :
			color(clr),
			name(nme),
			_id(n_id),
			filen(filen),
			changed(false){};
		nlohmann::json get_json();
		Database::pinfo_t get_dbdata();
		void set_name(std::string n){name=n;};
		void set_color(uint32_t c){color=c;};
		ClientLimit quota;
		std::string filen;
		bool changed;
	};
	class Room {
		struct oinfo_t {
			Client* owner;
			Client* oldowner;
			std::array<float,2> startpos;
			std::array<float,2> endpos;
			int64_t time;
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
			color(getDefaultRoomColor()),
			crown({NULL, NULL, {50, 50}, {50, 50}, 0}){};
		nlohmann::json get_json(std::string, bool);
		nlohmann::json get_chatlog_json();
		void push_chat(nlohmann::json&);
		clinfo_t* get_info(Client*);
		Client* get_client(std::string);
		oinfo_t get_crowninfo(){return crown;};
		bool is_lobby(){return lobby;};
		bool chat_on(){return chat;};
		bool is_crownsolo(){return crownsolo;};
		bool is_visible(){return visible;};
		bool is_owner(Client* c){return c == crown.owner;};
		jroom_clinfo_t join_usr(uWS::WebSocket<uWS::SERVER> *, mppconn_t&, std::string);
		void broadcast(nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);
		void broadcast(const char*, uWS::WebSocket<uWS::SERVER> *, size_t);
		bool kick_usr(uWS::WebSocket<uWS::SERVER> *, mppconn_t&, std::string);
		void set_param(nlohmann::json&, std::string);
		void set_owner(Client*);
		void part_upd(Client*);
	};
	class msg {
	public:
#ifndef VANILLA_SERVER
		static void bin_n(server*, const char*, size_t, uWS::WebSocket<uWS::SERVER> *);
#endif
		static void n(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);
		static void a(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);
		static void m(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);
		static void t(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);
		static void ch(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);

		static void hi(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);

		static void chown(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);
		static void chset(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);
		static void userset(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);

		static void adminmsg(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);
		static void kickban(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *);

		static void lsl(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *); /* -ls */
		static void lsp(server*, nlohmann::json&, uWS::WebSocket<uWS::SERVER> *); /* +ls */
	};
	server(std::string path, uint16_t p, const std::string& pw) : path(path), port(p), h(uWS::NO_DELAY, true, 16384), db("database/"), adminpw(pw){
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
	void reg_evts(uWS::Hub &s);
	void parse_msg(nlohmann::json& msg, uWS::WebSocket<uWS::SERVER> * socket);
	jroom_clinfo_t set_room(std::string, uWS::WebSocket<uWS::SERVER> *, mppconn_t&, nlohmann::json&);
	void user_upd(mppconn_t&);
	void rooml_upd(Room*, std::string);
	nlohmann::json get_roomlist();
	nlohmann::json genusr(uWS::WebSocket<uWS::SERVER> *);
	bool is_adminpw(const std::string p){return p == adminpw;};
private:
	std::string path;
	uint16_t port;
	uWS::Hub h;
	server::Database db;
	std::string adminpw;
	std::unordered_map<std::string, mppconn_t> clients;
	std::unordered_map<std::string, Room*> rooms;
	std::set<uWS::WebSocket<uWS::SERVER> *> roomlist_watchers;
	std::map<std::string, std::function<void(nlohmann::json&,uWS::WebSocket<uWS::SERVER> *)>> funcmap;
};
