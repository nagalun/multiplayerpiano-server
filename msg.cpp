#include "server.hpp"
#include <math.h>

using nlohmann::json;

long int js_date_now();

void server::msg::n(server* sv, json j, uWS::WebSocket& s){
	/* TODO: validate note data before sending */
	if(j["n"].is_array() && j["t"].is_number()){
		auto search = sv->clients.find(*(std::string *) s.getData());
		if(search != sv->clients.end()){
			/* this MAY be dangerous? */
			auto ssearch = sv->rooms.find(search->second.sockets.at(s));
			if(ssearch != sv->rooms.end() && (!ssearch->second->is_crownsolo()
				|| ssearch->second->is_owner(search->second.user))){
				clinfo_t* usr = ssearch->second->get_info(search->second.user);
				if(usr && usr->quota.noterate.can_spend()){
					j["p"] = usr->id;
					auto res = json::array();
					res[0] = j;
					ssearch->second->broadcast(res, s);
				}
			}
		}
	}
}

/* NOTE: messages don't have the room id of the user.
 * It doesn't seem to be used by the original client. (_id is sent, though)
 */

void server::msg::a(server* sv, json j, uWS::WebSocket& s){
	auto search = sv->clients.find(*(std::string *) s.getData());
	if(search != sv->clients.end() &&
	   j["message"].is_string() &&
	   j["message"].get<std::string>().size() <= 512){
		auto ssearch = sv->rooms.find(search->second.sockets.at(s));
		if(ssearch != sv->rooms.end() && ssearch->second->chat_on()){
			clinfo_t* usr = ssearch->second->get_info(search->second.user);
			if(usr && usr->quota.chat.can_spend()){
				json res = json::array();
				res[0] = {
					{"m", "a"},
					{"a", j["message"].get<std::string>()},
					{"p", search->second.user->get_json()},
					{"t", js_date_now()}
				};
				ssearch->second->push_chat(res[0]);
				ssearch->second->broadcast(res, s);
				s.send((char *)res.dump().c_str(), res.dump().size(), uWS::TEXT);
			}
		}
	}
}

void server::msg::m(server* sv, json j, uWS::WebSocket& s){
	/* TODO: fix rounding of floats */
	auto res = json::array();
	float x = 0;
	float y = 0;
	if(j["x"].is_string() && j["y"].is_string()) try {
		x = std::stof(j["x"].get<std::string>());
		y = std::stof(j["y"].get<std::string>());
	} catch(std::invalid_argument){ return; }
	  catch(std::out_of_range){ return; }
	else if(j["x"].is_number() && j["y"].is_number()){
		x = j["x"].get<float>();
		y = j["y"].get<float>();
	} else { return; }
	auto search = sv->clients.find(*(std::string *) s.getData());
	if(search != sv->clients.end()){
		auto ssearch = sv->rooms.find(search->second.sockets.at(s));
		if(ssearch != sv->rooms.end()){
			clinfo_t* usr = ssearch->second->get_info(search->second.user);
			if(usr && usr->quota.curs.can_spend()){
				usr->x = x;
				usr->y = y;
				res[0] = json::object({
					{"m", "m"},
					{"id", usr->id},
					{"x", x},
					{"y", y}
				});
				ssearch->second->broadcast(res, s);
			}
		}
	}
}

void server::msg::t(server* sv, json j, uWS::WebSocket& s){
	auto res = json::array();
	if(j["e"].is_number()){
		res[0] = {
			{"m", "t"},
			{"t", js_date_now()},
			{"e", j["e"].get<long int>()}
		};
		s.send((char *)res.dump().c_str(), res.dump().size(), uWS::TEXT);
	}
}
void server::msg::ch(server* sv, json j, uWS::WebSocket& s){
	auto res = json::array();
	auto search = sv->clients.find(*(std::string *) s.getData());
	if(j["_id"].is_string() && search != sv->clients.end() && search->second.user->quota.room.can_spend()){
		std::string nr = j["_id"].get<std::string>();
		nlohmann::json set = {};
		if(j["set"].is_object()) set = j["set"];
		if(nr.size() > 512) return;
		std::string id = sv->set_room(nr, s, search->second, set);
		if(id == "null") return;
		Room* room = sv->rooms.at(nr);
		if(!room) return;
		res[0] = room->get_json(nr, true);
		
		/*room->broadcast(res, s); this can be changed for the following, less data sent. */
		json upd = json::array();
		upd[0] = search->second.user->get_json();
		upd[0]["id"] = id;
		upd[0]["m"] = "p";
		room->broadcast(upd, s);
		
		res[0]["p"] = id;
		res[1] = {
			{"m", "c"},
			{"c", room->get_chatlog_json()}
		};
		s.send((char *)res.dump().c_str(), res.dump().size(), uWS::TEXT);
	}
}



void server::msg::hi(server* sv, json j, uWS::WebSocket& s){
	auto res = json::array();
	auto search = sv->clients.find(*(std::string *) s.getData());
	if(search == sv->clients.end() || search->second.sockets.find(s) == search->second.sockets.end()){
		res[0] = json::object({
			{"m", "hi"},
			{"u", sv->genusr(s)},
			{"t", js_date_now()}
		});
		s.send((char *)res.dump().c_str(), res.dump().size(), uWS::TEXT);
	}
}



void server::msg::chown(server* sv, json j, uWS::WebSocket& s){
	auto search = sv->clients.find(*(std::string *) s.getData());
	if(search == sv->clients.end()) return;
	auto ssearch = sv->rooms.find(search->second.sockets.at(s));
	if(ssearch == sv->rooms.end()) return;
	Client* newowner = j["id"].is_string() ? ssearch->second->get_client(j["id"].get<std::string>()) : NULL;
	if(ssearch->second->is_lobby() ||
		(!ssearch->second->is_owner(search->second.user)
		&& !ssearch->second->is_owner(NULL))
		|| ssearch->second->is_owner(newowner))
			return;
	if(js_date_now() - ssearch->second->get_crowninfo().time > 15000 ||
	   ssearch->second->get_crowninfo().oldowner == newowner){
		ssearch->second->set_owner(newowner);
		json res = json::array();
		res[0] = ssearch->second->get_json(ssearch->first, true);
		uWS::WebSocket a;
		ssearch->second->broadcast(res, a);
	}
}

void server::msg::chset(server* sv, json j, uWS::WebSocket& s){
	auto search = sv->clients.find(*(std::string *) s.getData());
	if(search == sv->clients.end()) return;
	if(!search->second.user->quota.room.can_spend()) return; /* ! */
	auto ssearch = sv->rooms.find(search->second.sockets.at(s));
	if(ssearch == sv->rooms.end()) return;
	if(!ssearch->second->is_owner(search->second.user)) return;
	if(j["set"].is_object()){
		ssearch->second->set_param(j["set"], ssearch->first);
	}
}

void server::msg::userset(server* sv, json j, uWS::WebSocket& s){
	if(j["set"].is_object() && j["set"]["name"].is_string()){
		std::string ip = *(std::string *) s.getData();
		auto search = sv->clients.find(ip);
		if(search != sv->clients.end() && search->second.user->quota.name.can_spend()){
			std::string newn = j["set"]["name"].get<std::string>();
			if(newn.size() <= 40){
				search->second.user->set_name(newn);
				sv->user_upd(ip);
			}
		}
	}
}

void server::msg::adminmsg(server* sv, json j, uWS::WebSocket& s){
	if(j["password"].is_string() && j["password"].get<std::string>() == "hunter2" && j["set"].is_object()){
		std::string ip(*(std::string *) s.getData());
		auto search = sv->clients.find(ip);
		if(search == sv->clients.end()) return;
		auto ssearch = sv->rooms.find(search->second.sockets.at(s));
		if(ssearch == sv->rooms.end()) return;
		if(j["set"]["user"].is_object()){
			if(!j["set"]["user"]["id"].is_string()) return;
			Client* selected = ssearch->second->get_client(j["set"]["user"]["id"].get<std::string>());
			if(j["set"]["user"]["color"].is_string()){
				uint32_t ncolor = 0;
				std::string strcolor = j["set"]["user"]["color"].get<std::string>();
				if(strcolor.size() > 1 && strcolor[0] == '#'){
					strcolor.erase(0, 1);
					try {
						ncolor = std::stoul(std::string("0x") + strcolor, nullptr, 16);
					} catch(std::invalid_argument) { return; }
					  catch(std::out_of_range) { return; }
				}
				selected->set_color(ncolor);
				sv->user_upd(ip);
			}
		}
		/*if(j["set"]["channel"].is_object()){
			ssearch->second->set_param(j["set"], ssearch->first);
		}*/
	}
}

void server::msg::kickban(server* sv, json j, uWS::WebSocket& s){
	return;
}



void server::msg::lsl(server* sv, json j, uWS::WebSocket& s){
	sv->roomlist_watchers.erase(s);
}

void server::msg::lsp(server* sv, json j, uWS::WebSocket& s){
	auto search = sv->clients.find(*(std::string *) s.getData());
	if(search == sv->clients.end()) return;
	if(!search->second.user->quota.rmls.can_spend()) return;
	json res = json::array();
	res[0] = {
		{"m", "ls"},
		{"c", true},
		{"u", sv->get_roomlist()}
	};
	sv->roomlist_watchers.emplace(s);
	s.send((char *)res.dump().c_str(), res.dump().size(), uWS::TEXT);
}
