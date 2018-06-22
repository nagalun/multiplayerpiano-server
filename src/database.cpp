#include "server.hpp"
#include <iostream>

server::Database::pinfo_t server::Database::get_usrinfo(uint32_t hash){
	server::Database::pinfo_t ret = {false, 0, {}};
	std::fstream file(std::string(dir + std::to_string(hash)), std::fstream::in | std::fstream::out | std::fstream::binary);
	if(!file) return ret;
	std::streampos old = file.tellg();
	file.seekg(0, std::fstream::end);
	long int size = file.tellg();
	if(size < 5) return ret;
	ret.found = true;
	file.seekg(old);
	file.read((char*)&ret.color, 4);
	ret.name.resize(size - 4);
	file.read((char*)&ret.name[0], size - 4);
	file.close();
	return ret;
}

void server::Database::set_usrinfo(uint32_t hash, pinfo_t usr){
	std::fstream file(std::string(dir + std::to_string(hash)), std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
	if(!file){
		std::cout << "Could not create file!" << std::endl;
		return;
	}
	file.write((char*)&usr.color, sizeof(usr.color));
	file.write(usr.name.c_str(), usr.name.size());
	file.close();
}
