#include "crossfuncs.hpp"

#ifndef __WIN32
	#include <sys/types.h>
	#include <sys/stat.h>
#else
	#include <windows.h>
#endif

bool makedir(const std::string & dir) {
#ifndef __WIN32
	return mkdir(dir.c_str(), 0755) == 0;
#else
	/* conversion from windows bool to c++ bool necessary? */
	return CreateDirectory(dir.c_str(), nullptr) == TRUE;
#endif
}
