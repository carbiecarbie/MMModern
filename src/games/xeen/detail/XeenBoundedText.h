#ifndef MMODERN_GAMES_XEEN_DETAIL_XEEN_BOUNDED_TEXT_H
#define MMODERN_GAMES_XEEN_DETAIL_XEEN_BOUNDED_TEXT_H

#include <cstddef>
#include <string>
#include <string_view>

namespace mmodern::detail {

class XeenBoundedText {
public:
	explicit XeenBoundedText(std::size_t limit) : _limit(limit) {}

	void append(std::string_view token) {
		if (token.empty() || _truncated)
			return;
		std::string addition;
		if (!_value.empty())
			addition.push_back(' ');
		addition.append(token.data(), token.size());
		if (_value.size() + addition.size() <= _limit) {
			_value += addition;
			return;
		}
		_truncated = true;
		if (_limit <= 3) {
			_value.assign(_limit, '.');
			return;
		}
		const std::size_t content = _limit - 3;
		if (_value.size() < content) {
			const std::size_t available = content - _value.size();
			_value.append(addition.data(), available);
		} else {
			_value.resize(content);
		}
		_value += "...";
	}

	void capitalizeFirstAsciiLetter() {
		for (char &c : _value) {
			if (c >= 'a' && c <= 'z') {
				c = static_cast<char>(c - 'a' + 'A');
				return;
			}
			if ((c >= 'A' && c <= 'Z'))
				return;
		}
	}

	const std::string &value() const { return _value; }
	bool truncated() const { return _truncated; }

private:
	std::size_t _limit;
	std::string _value;
	bool _truncated = false;
};

} // namespace mmodern::detail

#endif
