#ifndef MMODERN_XEEN_PARTY_LANES_H
#define MMODERN_XEEN_PARTY_LANES_H
#include <algorithm>
#include <array>
#include <initializer_list>
#include <stdexcept>
#include <utility>
namespace mmodern {
// Detached party values retain the DOS six physical lanes. Ordinary iteration
// and access expose only the active prefix; inactive character reads are errors.
template<class T> class XeenPartyLanes {
    std::array<T,6> _lanes{};
    std::size_t _count=6;
public:
    constexpr XeenPartyLanes() = default;
    explicit XeenPartyLanes(std::size_t count) {setCount(count);}
    constexpr XeenPartyLanes(std::initializer_list<T> values) : _count(values.size()) {
        if(_count>6)throw std::invalid_argument("Too many party lanes");
        std::size_t n=0;for(const auto &v:values)_lanes[n++]=v;
    }
    constexpr XeenPartyLanes(const std::array<T,6> &values) : _lanes(values) {}
    void setCount(std::size_t count) {
        if(count>6)throw std::invalid_argument("Too many party lanes");
        _count=count;
    }
    constexpr std::size_t size() const noexcept {return _count;}
    constexpr unsigned mask() const noexcept {return (1u<<_count)-1;}
    T &at(std::size_t n) {if(n>=_count)throw std::out_of_range("Inactive party lane");return _lanes.at(n);}
    const T &at(std::size_t n) const {if(n>=_count)throw std::out_of_range("Inactive party lane");return _lanes.at(n);}
    T &operator[](std::size_t n) {return at(n);}
    const T &operator[](std::size_t n) const {return at(n);}
    auto begin() noexcept {return _lanes.begin();}
    auto end() noexcept {return _lanes.begin()+_count;}
    auto begin() const noexcept {return _lanes.begin();}
    auto end() const noexcept {return _lanes.begin()+_count;}
    void fill(const T &value) {std::fill(begin(),end(),value);}
    void swap(XeenPartyLanes &other) noexcept {using std::swap;_lanes.swap(other._lanes);swap(_count,other._count);}
    friend bool operator==(const XeenPartyLanes &a,const XeenPartyLanes &b) {
        return a._count==b._count && std::equal(a.begin(),a.end(),b.begin());
    }
    friend bool operator!=(const XeenPartyLanes &a,const XeenPartyLanes &b) {return !(a==b);}
};
}
#endif
