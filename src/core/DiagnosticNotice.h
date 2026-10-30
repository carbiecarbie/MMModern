#pragma once
#include <string>
namespace mmodern {
// Presentation reports notices only when the player log has installed a sink.
inline void (*diagnosticNoticeSink)(const std::string &)=nullptr;
inline void diagnosticNotice(const std::string &text) {
 if(diagnosticNoticeSink && text.find("not supported yet")!=std::string::npos)diagnosticNoticeSink(text);
}
}
