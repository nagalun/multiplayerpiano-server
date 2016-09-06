#include "server.hpp"
#include <sstream>

long int js_date_now(){
	struct timeval tp;
	gettimeofday(&tp, NULL);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	return ms;
}

nlohmann::json server::Room::get_json(std::string _id){
	auto ppl = nlohmann::json::array();
	for(auto& c : ids){
		auto inf = c.first->get_json();
		inf["x"] = c.second.x;
		inf["y"] = c.second.y;
		inf["id"] = c.second.id;
		ppl.push_back(inf);
	}
	return nlohmann::json::object({
		{"m", "ch"},
		{"ch", {
			{"_id", _id},
			{"count", ppl.size()},
			{"settings", {
				{"visible", visible},
				{"chat", chat},
				{"lobby", lobby},
				{"crownsolo", crownsolo},
				{"color", std::string("#")+n2hexstr(color)}
			}}
		}},
		{"ppl", ppl}
	});
}

nlohmann::json server::Client::get_json(){
	return nlohmann::json::object({
		{"name", name},
		{"color", std::string("#")+n2hexstr(color)},
		{"_id", _id}
	});
}

bool server::Room::kick_usr(uWS::WebSocket s, mppconn_t& c){
	std::string ip(s.getAddress().address);
	auto ssearch = ids.find(c.user);
	if(ssearch != ids.end()){
		if(ssearch->second.sockets.erase(s) && !ssearch->second.sockets.size() && ids.erase(c.user)){
			if(!ids.size()){
				std::cout << "Deleted room: " << c.sockets.at(s) << std::endl;
				delete this; /* not a meme */
				return true;
			}
			/* Client left room, notify. */
		}
	}
	return false;
}

std::string server::Room::join_usr(uWS::WebSocket s, mppconn_t& c, std::string rname){
	auto search = ids.find(c.user);
	std::string id;
	if(search == ids.end()){
		/* TODO: generate better id? */
		id = std::to_string(js_date_now());
		ids[c.user] = {std::set<uWS::WebSocket>{s}, id, 0, 0};
	} else {
		search->second.sockets.emplace(s);
		id = search->second.id;
	}
	c.sockets.at(s) = rname;
	return id;
}

std::string server::set_room(std::string newroom, uWS::WebSocket s, mppconn_t& m){
	auto thissocket = m.sockets.find(s);
	if(thissocket != m.sockets.end()){
		auto old = rooms.find(thissocket->second);
		
		if(old != rooms.end() && old->second->kick_usr(s, m)){
			rooms.erase(old);
		}
		auto newr = rooms.find(newroom);
		if(newr == rooms.end()){
			std::cout << "Created new room: " << newroom << std::endl;
			rooms.emplace(newroom, new server::Room());
		}
		
		return rooms[newroom]->join_usr(s, m, newroom);
	}
	return "null";
}

nlohmann::json server::genusr(uWS::WebSocket s){
	std::string ip(s.getAddress().address);
	std::string _id = ip;
	auto search = clients.find(ip);
	if(search == clients.end()){
		std::cout << "New client: " << ip << std::endl;
		clients[ip] = {new server::Client(_id), {{s, ""}}};
	} else {
		search->second.sockets.emplace(s, "");
	}
	return clients[ip].user->get_json();
}

void server::run(){
	uWS::Server s(port);
	reg_evts(s);
	s.run();
}

void server::reg_evts(uWS::Server &s){
	s.onConnection([this](uWS::WebSocket socket){
	
	});
	
	s.onDisconnection([this](uWS::WebSocket socket, int code, const char *message, size_t length){
		std::string ip(socket.getAddress().address);
		auto search = clients.find(ip);
		if(search != clients.end()){
			auto ssearch = search->second.sockets.find(socket);
			if(ssearch != search->second.sockets.end()){
				auto tsearch = rooms.find(ssearch->second);
				if(tsearch != rooms.end() && tsearch->second->kick_usr(socket, search->second)){
					rooms.erase(tsearch);
				}
				search->second.sockets.erase(socket);
			}
			if(!search->second.sockets.size()){
				/* !!!!!!!!!!!!!!!! Does not delete from room ids */
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
		} catch(std::invalid_argument) { return; }
	});
}

void server::parse_msg(nlohmann::json &msg, uWS::WebSocket socket){
	auto response = nlohmann::json::array();
	for(auto& m : msg){
		if(m["m"].is_string()){
			auto s = funcmap.find(m["m"].get<std::string>());
			if(s != funcmap.end()){
				auto r = s->second(m, socket);
				if(r["m"].is_string())
					response.push_back(r);
			}
		}
	}
	if(response.size()){
		std::cout << "> " << response << std::endl;
		socket.send((char *)response.dump().c_str(), response.dump().size(), uWS::TEXT);
	}
}

int main(){
	server s(1234);
	s.run();
}
