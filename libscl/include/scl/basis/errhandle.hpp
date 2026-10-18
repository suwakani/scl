#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <array>

#define SCL_ASSERT(cond) { if(!(cond)) { scl::errhandle::debugbreak_noFormat(__FILE__,__LINE__,__PRETTY_FUNCTION__,#cond); } }
#define SCL_ASSERT_MSG(cond,msg,...) { if(!(cond)) { scl::errhandle::debugbreak_withMessage(__FILE__,__LINE__,__PRETTY_FUNCTION__,msg,##__VA_ARGS__); } }

namespace scl {
namespace errhandle {

static inline void debugbreak_noFormat(const char* filename, int line, const char* func, const char* msg) {
	std::printf(
		"---------------------------------------------------------@/\n"
		"assertion fail!\n"
		"%s, line %d: %s error.\n"
		"condition \"%s\" failed.\n",
		filename,line,func,msg
		/*"---------------------------------------------------------@/\n"
		"assertion fail!\n"
		"source/game/scene.cpp, line 300: SceneCheck() error.\n"
		"condition \"scene != NULL\" failed.\n"*/
		
	);
	std::exit(-1);
}

static inline void debugbreak_withMessage(const char* filename, int line, const char* func, const char* msg,...) {
	std::array<char,4096> buffer_fmttext;
	va_list args;
	va_start(args, msg);
	vsnprintf(buffer_fmttext.data(), buffer_fmttext.size(),msg, args);
	va_end(args);

	std::printf(
		"---------------------------------------------------------@/\n"
		"assertion fail!\n"
		"%s, line %d: %s error.\n"
		"%s\n",
		filename,line,func,buffer_fmttext.data()
		/*"---------------------------------------------------------@/\n"
		"assertion fail!\n"
		"source/game/scene.cpp, line 300: SceneCheck() error.\n"
		"condition \"scene != NULL\" failed.\n"*/
		
	);
	std::exit(-1);
}

} // errhandle
} // scl

