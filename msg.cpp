#include "server.hpp"

using nlohmann::json;

long int js_date_now();

json server::msg::n(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}

json server::msg::a(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}

json server::msg::m(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}

json server::msg::t(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	if(j["e"].is_number()){
		res = {
			{"m", "t"},
			{"t", js_date_now()},
			{"e", j["e"].get<long int>()}
		};
	}
	return res;
}
json server::msg::ch(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	auto search = sv->clients.find(std::string(s.getAddress().address));
	if(j["_id"].is_string() && search != sv->clients.end()){
		std::string id = sv->set_room(j["_id"].get<std::string>(), s, search->second);
		res = sv->rooms.at(j["_id"].get<std::string>())->get_json(j["_id"].get<std::string>());
		res["p"] = id;
	}
	return res;
}



json server::msg::hi(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	auto search = sv->clients.find(std::string(s.getAddress().address));
	if(search == sv->clients.end() || search->second.sockets.find(s) == search->second.sockets.end()){
		res = {
			{"m", "hi"},
			{"u", sv->genusr(s)},
			{"t", js_date_now()}
		};
	}
	return res;
}

json server::msg::bye(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}



json server::msg::chown(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}

json server::msg::chset(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}

json server::msg::userset(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}



json server::msg::kickban(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}



json server::msg::lsl(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}

json server::msg::lsp(server* sv, json j, uWS::WebSocket s){
	auto res = json::object();
	return res;
}
