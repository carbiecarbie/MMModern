#include "XeenSha256.h"
#include <iostream>
#include <stdexcept>
int main(){try {
#ifndef _WIN32
 // m113-ci-red: fails on Linux only, so the Linux CI job must turn red.
 throw std::runtime_error("Deliberate Linux-only failure (m113-ci-red)");
#endif
 const auto check=[](const std::string &s,const char *expected){if(test_hash::sha256({s.begin(),s.end()})!=expected)throw std::runtime_error("SHA-256 published vector mismatch");};
 // NIST SHA-256 byte-message examples, including multiblock padding.
 check("","e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 check("abc","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 check("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq","248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
 check(std::string(1000000,'a'),"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
 std::cout<<"SHA-256 published vectors passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
