#ifndef MMODERN_TESTS_PROBE_FIRED_H
#define MMODERN_TESTS_PROBE_FIRED_H
// Link-time (--wrap) probes are silently inert if the wrapped call is renamed,
// inlined or never reached. A probe calls hit(name) when it intercepts a call;
// the executable that relies on it calls expect(name). At process exit every
// expected probe that never fired fails the process (exit code 97). Setting
// MMODERN_PROBE_REPORT prints the hit counts at exit.
#include <cstdio>
#include <cstdlib>
#include <map>
#include <initializer_list>
#include <set>
#include <string>
namespace probe_fired {
struct Registry {
	std::map<std::string,unsigned long long> hits;
	std::set<std::string> expected;
	bool installed=false;
};
inline Registry &registry() { static Registry value; return value; }
inline void verify() {
	auto &r=registry();
	bool missing=false;
	for(const auto &name:r.expected)
		if(!r.hits.count(name)) { std::fprintf(stderr,"PROBE NEVER FIRED: %s\n",name.c_str()); missing=true; }
	if(std::getenv("MMODERN_PROBE_REPORT")) {
		for(const auto &name:r.expected) std::fprintf(stderr,"PROBE expected %s\n",name.c_str());
		for(const auto &entry:r.hits) std::fprintf(stderr,"PROBE hit %s %llu\n",entry.first.c_str(),entry.second);
	}
	std::fflush(stderr);
	if(missing) std::_Exit(97);
}
inline void install() { auto &r=registry(); if(!r.installed) { r.installed=true; std::atexit(verify); } }
inline void hit(const char *name) { install(); ++registry().hits[name]; }
inline void expect(const char *name) { registry().expected.insert(name); install(); }
// File-scope form for probes whose own translation unit runs before main.
struct Expect { Expect(std::initializer_list<const char *> names) { for(auto name:names) expect(name); } };
}
#endif
