#include "server.hpp"
#include <sstream>

/* sha1 hashing */
#include <openssl/sha.h>

static const char* hexmap = "0123456789ABCDEF";

std::string n2hexstr(uint32_t w, bool alpha = false) {
    const uint8_t l = alpha ? 32 : 24;
    std::string rc(l/4, '0');
    for (uint8_t i = 0, x = l/4-1; i < l; x--, i += 4)
        rc[x] = hexmap[(w>>i) & 0x0f];
    return rc;
}

long int js_date_now(){
	struct timeval tp;
	gettimeofday(&tp, NULL);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	return ms;
}

nlohmann::json server::Room::get_json(std::string _id, bool includeppl){
	nlohmann::json j = {
		{"m", "ch"},
		{"ch", {
			{"_id", _id},
			{"count", ids.size()},
			{"settings", {
				{"visible", visible},
				{"chat", chat},
				{"lobby", lobby},
				{"crownsolo", crownsolo},
				{"color", std::string("#")+n2hexstr(color)}
			}}
		}}
	};
	if(includeppl){
		if(!lobby){
			if(crown.owner){
				auto search = ids.find(crown.owner);
				if(search != ids.end()){
					j["ch"]["crown"] = {
						{"participantId", search->second.id},
						{"userId", search->first->get_json()["_id"]}
					};
				}
			} else {
				j["ch"]["crown"] = {
					{"time", crown.time},
					{"startPos", {
						{"x", crown.startpos[0]},
						{"y", crown.startpos[1]}
					}},
					{"endPos", {
						{"x", crown.endpos[0]},
						{"y", crown.endpos[1]}
					}}
				};
			}
		}
		auto ppl = nlohmann::json::array();
		for(auto& c : ids){
			auto inf = c.first->get_json();
			inf["x"] = c.second.x;
			inf["y"] = c.second.y;
			inf["id"] = c.second.id;
			ppl.push_back(inf);
		}
		j["ppl"] = ppl;
	}
	return j;
}

nlohmann::json server::Room::get_chatlog_json(){
	nlohmann::json log = nlohmann::json::array();
	for(auto& msg : chatlog){
		log.push_back(msg);
	}
	return log;
}

void server::Room::push_chat(nlohmann::json j){
	chatlog.push_back(j);
	if(chatlog.size() > 32)
		chatlog.pop_front();
}

nlohmann::json server::Client::get_json(){
	return nlohmann::json::object({
		{"name", name},
		{"color", std::string("#")+n2hexstr(color)},
		{"_id", _id}
	});
}

void server::Room::broadcast(nlohmann::json j, uWS::WebSocket& exclude){
	uWS::WebSocket::PreparedMessage* prep = uWS::WebSocket::prepareMessage(
		(char *)j.dump().c_str(), j.dump().size(), uWS::TEXT, false);
	for(auto& c : ids){
		for(auto sock : c.second.sockets){
			if(sock == exclude) continue;
			sock.sendPrepared(prep);
		}
	}
	uWS::WebSocket::finalizeMessage(prep);
}

void server::Room::part_upd(Client* c){
	auto search = ids.find(c);
	if(search != ids.end()){
		uWS::WebSocket a;
		nlohmann::json j = nlohmann::json::array();
		j[0] = c->get_json();
		j[0]["id"] = search->second.id;
		j[0]["m"] = "p";
		this->broadcast(j, a);
	}
}

clinfo_t* server::Room::get_info(Client* c){
	auto search = ids.find(c);
	if(search != ids.end()){
		return &search->second;
	}
	return NULL;
}

void server::Room::set_param(nlohmann::json j, std::string _id){
	/* TODO: check if user is owner */
	bool updated = false;
	bool nvisible = visible;
	bool nchat = chat;
	bool ncrownsolo = crownsolo;
	uint32_t ncolor = color;
	if(j["visible"].is_boolean()){
		nvisible = j["visible"].get<bool>();
		if(nvisible != visible) updated = true;
	}
	if(j["chat"].is_boolean()){
		nchat = j["chat"].get<bool>();
		if(nchat != chat) updated = true;
	}
	if(j["crownsolo"].is_boolean()){
		ncrownsolo = j["crownsolo"].get<bool>();
		if(ncrownsolo != crownsolo) updated = true;
	}
	if(j["color"].is_string()){
		std::string strcolor = j["color"].get<std::string>();
		if(strcolor.size() > 1 && strcolor[0] == '#'){
			strcolor.erase(0, 1);
			try {
				ncolor = std::stoul(std::string("0x") + strcolor, nullptr, 16);
			} catch(std::invalid_argument) { return; }
			  catch(std::out_of_range) { return; }
		}
		if(ncolor != color) updated = true;
	}
	if(updated){
		color = ncolor;
		visible = nvisible;
		crownsolo = ncrownsolo;
		chat = nchat;
		nlohmann::json j2 = nlohmann::json::array();
		j2[0] = this->get_json(_id, true);
		uWS::WebSocket a; /* ugly */
		this->broadcast(j2, a);
	}
}

void server::Room::set_owner(Client* c){
	float x = 50;
	float y = 50;
	auto search = ids.find(crown.owner);
	if(!c && search != ids.end()){
		x = search->second.x;
		y = search->second.y;
	}
	/* the ternary makes sure the crown doesn't land off the screen */
	crown = {c, {x, y}, {x > 95 ? 95 : x < 5 ? 5 : x, y+25 > 95 ? 95 : y+25 < 5 ? 5 : y+25}, js_date_now()};
}

server::Client* server::Room::get_client(std::string id){
	for(auto c : ids)
		if(c.second.id == id)
			return c.first;
	return NULL;
}

bool server::Room::kick_usr(uWS::WebSocket& s, mppconn_t& c, std::string rname){
	std::string ip(s.getAddress().address);
	bool ownupd = false;
	auto ssearch = ids.find(c.user);
	if(ssearch != ids.end()){
		if(ssearch->second.sockets.erase(s) && !ssearch->second.sockets.size()){
			if(!lobby && c.user == crown.owner){
				ownupd = true;
				this->set_owner(NULL);
			}
			std::string id = ssearch->second.id;
			ids.erase(c.user);
			if(!ids.size()){
				return true;
			}
			/* Client left room, notify.
			 * Don't send the first if owner changed because we need
			 * to send the complete room data, and doing this would
			 * waste some bandwidth.
			 ***/
			if(!ownupd){
				nlohmann::json j = nlohmann::json::array();
				j[0] = {
					{"m", "bye"},
					{"p", id}
				};
				this->broadcast(j, s);
			} else {
				nlohmann::json j = nlohmann::json::array();
				j[0] = this->get_json(rname, true);
				this->broadcast(j, s);
			}
		}
	}
	return false;
}

std::string server::Room::join_usr(uWS::WebSocket& s, mppconn_t& c, std::string rname){
	auto search = ids.find(c.user);
	std::string id;
	if(search == ids.end()){
		/* TODO: generate better id? */
		id = std::to_string(js_date_now());
		ids[c.user] = {std::set<uWS::WebSocket>{s}, id, -10, -10};
	} else {
		search->second.sockets.emplace(s);
		id = search->second.id;
	}
	c.sockets.at(s) = rname;
	return id;
}

void server::user_upd(std::string ip){
	/* a set so we don't update the same room more than once */
	std::set<Room*> roomstoupdate;
	Client* c = clients.at(ip).user;
	for(auto& sock : clients.at(ip).sockets){
		auto search = rooms.find(sock.second);
		if(search != rooms.end()){
			roomstoupdate.emplace(search->second);
		}
	}
	for(auto r : roomstoupdate){
		r->part_upd(c);
	}
}

nlohmann::json server::get_roomlist(){
	nlohmann::json res = nlohmann::json::array();
	for(auto room : rooms){
		nlohmann::json j = room.second->get_json(room.first, false)["ch"]; /* ugly */
		if(j["settings"]["visible"].get<bool>()){
			res.push_back(j);
		}
	}
	return res;
}

std::string server::set_room(std::string newroom, uWS::WebSocket& s, mppconn_t& m, nlohmann::json set){
	auto thissocket = m.sockets.find(s);
	if(thissocket != m.sockets.end() && thissocket->second != newroom && newroom.size() <= 512){
		auto old = rooms.find(thissocket->second);
		
		if(old != rooms.end()){
			bool d = old->second->kick_usr(s, m, old->first);
			rooml_upd(old->second, old->first);
			if(d){
				std::cout << "Deleted room: " << old->first << std::endl;
				delete old->second;
				rooms.erase(old);
			}
		}
		auto newr = rooms.find(newroom);
		Room* room;
		if(newr == rooms.end()){
			bool islobby = !newroom.compare(0, 5, "lobby") || !newroom.compare(0, 5, "test/");
			std::cout << "Created new room: " << newroom << std::endl;
			rooms.emplace(newroom, new server::Room(islobby));
			room = rooms[newroom];
			if(!islobby) {
				room->set_param(set, newroom);
				room->set_owner(m.user);
			}
		} else {
			room = newr->second;
		}
		std::string id = room->join_usr(s, m, newroom);
		rooml_upd(room, newroom);
		return id;
	}
	return "null";
}

void server::rooml_upd(Room* r, std::string _id){
	if(roomlist_watchers.size()){
		nlohmann::json res = nlohmann::json::array();
		res[0] = {
			{"m", "ls"},
			{"c", false},
			{"u", nlohmann::json::array()}
		};
		res[0]["u"][0] = r->get_json(_id, false)["ch"];
		for(auto sock : roomlist_watchers){
			sock.send((char *)res.dump().c_str(), res.dump().size(), uWS::TEXT);
		}
	}
}

nlohmann::json server::genusr(uWS::WebSocket& s){
	std::string ip(s.getAddress().address);
	auto search = clients.find(ip);
	if(search == clients.end()){
		unsigned char hash[20];
		std::string _id(20, '0');
		SHA1((unsigned char*)ip.c_str(), ip.size(), hash);
		for(uint8_t i = 10; i--;){
			_id[2 * i] = hexmap[(hash[i] & 0xF0) >> 4];
			_id[2 * i + 1] = hexmap[hash[i] & 0x0F];
		}
		uint32_t color = (uint32_t)hash[11] << 24 |
		                 (uint32_t)hash[12] << 16 |
		                 (uint16_t)hash[13] << 8 |
		                           hash[14];
		color += 0x222222; /* colors seem to be pretty dark otherwise */
		std::cout << "New client: " << ip << std::endl;
		clients[ip] = {new server::Client(_id, color), {{s, ""}}};
	} else {
		search->second.sockets.emplace(s, "");
	}
	return clients[ip].user->get_json();
}

void server::run(){
	uWS::EventSystem es(uWS::MASTER);
	uWS::Server s(es, port, uWS::PERMESSAGE_DEFLATE | uWS::NO_DELAY, 32768);
	reg_evts(s);
	es.run();
}

void server::reg_evts(uWS::Server &s){
	s.onConnection([this](uWS::WebSocket socket){
	
	});
	
	s.onDisconnection([this](uWS::WebSocket socket, int code, const char *message, size_t length){
		std::string ip(socket.getAddress().address);
		roomlist_watchers.erase(socket);
		auto search = clients.find(ip);
		if(search != clients.end()){
			auto ssearch = search->second.sockets.find(socket);
			if(ssearch != search->second.sockets.end()){
				auto tsearch = rooms.find(ssearch->second);
				if(tsearch != rooms.end() && tsearch->second->kick_usr(socket, search->second, tsearch->first)){
					std::cout << "Deleted room: " << tsearch->first << std::endl;
					delete tsearch->second;
					rooms.erase(tsearch);
				}
				search->second.sockets.erase(socket);
			}
			if(!search->second.sockets.size()){
				std::cout << "Deleted client: " << ip << std::endl;
				delete search->second.user;
				clients.erase(ip);
			}
		}
	});
	
	s.onMessage([this, &s](uWS::WebSocket socket, const char *message, size_t length, uWS::OpCode opCode){
		if(opCode == uWS::TEXT) try {
			auto msg = nlohmann::json::parse(std::string(message, length));
			if(msg.is_array())
				parse_msg(msg, socket);
		} catch(std::invalid_argument) {
			/* kick his ass */
			socket.close();
			return;
		}
	});
}

void server::parse_msg(nlohmann::json &msg, uWS::WebSocket& socket){
	for(auto& m : msg){
		if(m["m"].is_string()){
			/* we don't want to continue reading messages if the client said bye */
			if(m["m"].get<std::string>() == "bye"){ 
				socket.close();
				break;
			}
			auto str = funcmap.find(m["m"].get<std::string>());
			if(str != funcmap.end()){
				str->second(m, socket);
			}
		}
	}
}

int main(){
	server s(1234);
	s.run();
}
