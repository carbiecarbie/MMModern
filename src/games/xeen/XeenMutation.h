#ifndef MMODERN_XEEN_MUTATION_H
#define MMODERN_XEEN_MUTATION_H
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <variant>
#include <ostream>
#include <type_traits>
#include <set>
#include <stdexcept>
#include <atomic>
#include <thread>
namespace mmodern {
// Runtime observation of writes, independent of gameplay values. A restored
// value cannot restore this history. Observers never own gameplay storage.
class XeenMutationWatch {
public:
 struct Range { std::uintptr_t first, last; };
 XeenMutationWatch():ranges(8) { RegistryLock lock;observers().push_back(this); }
 ~XeenMutationWatch() { RegistryLock lock;auto &v=observers();v.erase(std::find(v.begin(),v.end(),this)); }
 XeenMutationWatch(const XeenMutationWatch &)=delete;
 XeenMutationWatch(XeenMutationWatch &&other) {
  RegistryLock lock;ranges=other.ranges;rangeCount=other.rangeCount;changed=other.changed;observers().push_back(this);
 }
 XeenMutationWatch &operator=(const XeenMutationWatch &)=delete;
 void add(const void *p,std::size_t n) noexcept {
  RegistryLock lock;
  addRange(p,n);
 }
 bool current() const noexcept { RegistryLock lock;return !changed; }
 // Allocation is confined to preparation. Observing and publishing never allocate.
 void prepare(std::size_t capacity) { RegistryLock lock;if(capacity>ranges.size())ranges.resize(capacity); }
 void renew() noexcept { RegistryLock lock;changed=false;rangeCount=0; }
 void clearRanges() noexcept { RegistryLock lock;rangeCount=0; }
private:
 friend class XeenWorld;
 // Materializing an immutable cache is not a gameplay write. Extend every
 // observer of its owner before the new storage escapes, without renewing any
 // observer or discarding its history. Allocation precedes cache insertion;
 // there are no callbacks between preparation, insertion and registration.
 static void prepareOwned(const void *owner,std::size_t additional) {
  RegistryLock lock;
  for(auto *observer:observers()) if(observer->contains(owner)) {
   if(additional>observer->ranges.max_size()-observer->rangeCount)
    throw std::length_error("Observed resource range capacity exhausted");
   const auto capacity=observer->rangeCount+additional;
   if(capacity>observer->ranges.size())observer->ranges.resize(capacity);
  }
 }
 static void addOwned(const void *owner,const void *p,std::size_t n) noexcept {
  RegistryLock lock;
  for(auto *observer:observers()) if(observer->contains(owner))observer->addRange(p,n);
 }
 bool contains(const void *p) const noexcept {
  const auto address=reinterpret_cast<std::uintptr_t>(p);
  for(std::size_t i=0;i<rangeCount;++i)
   if(address>=ranges[i].first && address<ranges[i].last)return true;
  return false;
 }
 void addRange(const void *p,std::size_t n) noexcept {
  if(!n)return; const Range range{reinterpret_cast<std::uintptr_t>(p),reinterpret_cast<std::uintptr_t>(p)+n};
  for(std::size_t i=0;i<rangeCount;++i)if(ranges[i].first==range.first && ranges[i].last==range.last)return;
  if(rangeCount==ranges.size()){changed=true;return;} ranges[rangeCount++]=range;
 }
 // Cache eviction retires only addresses, never previously observed writes.
 static void retire(const void *p,std::size_t n) noexcept {
  RegistryLock lock;
  if(!n)return;
  const auto first=reinterpret_cast<std::uintptr_t>(p),last=first+n;
  for(auto *observer:observers()) {
   std::size_t out=0;
   for(std::size_t i=0;i<observer->rangeCount;++i) {
    const auto r=observer->ranges[i];
    if(r.first>=first && r.last<=last)continue;
    observer->ranges[out++]=r;
   }
   observer->rangeCount=out;
  }
 }
public:
 static void write(const void *p) noexcept {
  RegistryLock lock;
  const auto address=reinterpret_cast<std::uintptr_t>(p);
  for(auto *observer:observers()) for(std::size_t i=0;i<observer->rangeCount;++i) { const auto &r=observer->ranges[i];
   if(address>=r.first && address<r.last) { observer->changed=true;break; }
  }
 }
private:
 // Only observer metadata is synchronized. Gameplay remains under its existing
 // Flow owner. A callback cannot hide writes by delegating them to a worker.
 // The spin gate cannot allocate or throw during a checked publication. No
 // operation holding it invokes another observer operation or user callback.
 class RegistryLock {
 public:
  RegistryLock() noexcept { while(gate().test_and_set(std::memory_order_acquire))std::this_thread::yield(); }
  ~RegistryLock() { gate().clear(std::memory_order_release); }
  RegistryLock(const RegistryLock &)=delete;
  RegistryLock &operator=(const RegistryLock &)=delete;
 private:
  static std::atomic_flag &gate() noexcept { static std::atomic_flag value=ATOMIC_FLAG_INIT;return value; }
 };
 static std::vector<XeenMutationWatch *> &observers() { static std::vector<XeenMutationWatch *> v;return v; }
 std::vector<Range> ranges;
 std::size_t rangeCount=0;
 bool changed=false;
};
template<class T> class XeenMutable;
// Private owners can keep standard containers while exporting no mutable
// container alias. Element reads retain the observed element values.
template<class T> class XeenReadOnlyVector {
 const std::vector<T> *values;
public:
 explicit XeenReadOnlyVector(const std::vector<T> &v) noexcept:values(&v) {}
 auto size() const noexcept { return values->size(); } bool empty() const noexcept { return values->empty(); }
 const T &operator[](std::size_t i) const { return (*values)[i]; }
 const T &at(std::size_t i) const { return values->at(i); }
 const T &front() const { return values->front(); } const T &back() const { return values->back(); }
 const T *data() const noexcept { return values->data(); }
 auto begin() const noexcept { return values->begin(); } auto end() const noexcept { return values->end(); }
 operator std::vector<T>() const { return *values; }
 friend bool operator==(XeenReadOnlyVector a,XeenReadOnlyVector b) { return *a.values==*b.values; }
 friend bool operator!=(XeenReadOnlyVector a,XeenReadOnlyVector b) { return !(a==b); }
 friend bool operator==(XeenReadOnlyVector a,const std::vector<T> &b) { return *a.values==b; }
 friend bool operator==(const std::vector<T> &a,XeenReadOnlyVector b) { return b==a; }
 friend bool operator!=(XeenReadOnlyVector a,const std::vector<T> &b) { return !(a==b); }
 friend bool operator!=(const std::vector<T> &a,XeenReadOnlyVector b) { return !(a==b); }
};
// Implicit owner assignment remains observable even when every collection was
// empty and the assigned final values are identical.
struct XeenMutationMarker {
 XeenMutationMarker()=default;
 XeenMutationMarker(const XeenMutationMarker &) noexcept {}
 XeenMutationMarker &operator=(const XeenMutationMarker &) noexcept { XeenMutationWatch::write(this);return *this; }
};
template<class Iterator> class XeenValueIterator {
 Iterator position;
public:
 using iterator_category=std::forward_iterator_tag;
 using value_type=typename std::iterator_traits<Iterator>::value_type;
 using difference_type=std::ptrdiff_t;using reference=value_type;using pointer=void;
 XeenValueIterator()=default;
 explicit XeenValueIterator(Iterator p):position(p) {}
 value_type operator*() const { return *position; }
 XeenValueIterator &operator++() { ++position;return *this; }
 XeenValueIterator operator++(int) { auto copy=*this;++*this;return copy; }
 friend bool operator==(const XeenValueIterator &a,const XeenValueIterator &b) { return a.position==b.position; }
 friend bool operator!=(const XeenValueIterator &a,const XeenValueIterator &b) { return !(a==b); }
};
template<class T> class XeenMutableVector {
 using Stored=std::conditional_t<std::is_arithmetic_v<T> || std::is_enum_v<T>,XeenMutable<T>,T>;
 std::vector<Stored> value;
public:
 XeenMutableVector()=default;
 XeenMutableVector(const std::vector<T> &v):value(v.begin(),v.end()) {}
 XeenMutableVector(std::initializer_list<T> v):value(v.begin(),v.end()) {}
 XeenMutableVector(const XeenMutableVector &v):value(v.value) {}
 XeenMutableVector(XeenMutableVector &&v) noexcept:value(std::move(v.value)) { XeenMutationWatch::write(&v); }
 XeenMutableVector &operator=(const XeenMutableVector &v) { XeenMutationWatch::write(this);value=v.value;return *this; }
 XeenMutableVector &operator=(const std::vector<T> &v) { XeenMutationWatch::write(this);value.assign(v.begin(),v.end());return *this; }
 XeenMutableVector &operator=(XeenMutableVector &&v) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);value=std::move(v.value);return *this; }
 operator std::vector<T>() const { return {value.begin(),value.end()}; }
 auto size() const noexcept { return value.size(); } bool empty() const noexcept { return value.empty(); }
 Stored &operator[](std::size_t i) { return value[i]; } const Stored &operator[](std::size_t i) const { return value[i]; }
 Stored &at(std::size_t i) { return value.at(i); } const Stored &at(std::size_t i) const { return value.at(i); }
 auto begin() noexcept { return value.begin(); } auto end() noexcept { return value.end(); }
 auto begin() const noexcept { return value.begin(); } auto end() const noexcept { return value.end(); }
 Stored *data() noexcept { return value.data(); } const Stored *data() const noexcept { return value.data(); }
 Stored &back() { return value.back(); } const Stored &back() const { return value.back(); }
 Stored &front() { return value.front(); } const Stored &front() const { return value.front(); }
 void clear() noexcept { XeenMutationWatch::write(this);value.clear(); }
 void reserve(std::size_t n) { XeenMutationWatch::write(this);value.reserve(n); }
 void resize(std::size_t n) { XeenMutationWatch::write(this);value.resize(n); }
 void pop_back() { XeenMutationWatch::write(this);value.pop_back(); }
 auto erase(typename std::vector<Stored>::const_iterator at) { XeenMutationWatch::write(this);return value.erase(at); }
 auto erase(typename std::vector<Stored>::const_iterator first,typename std::vector<Stored>::const_iterator last) { XeenMutationWatch::write(this);return value.erase(first,last); }
 template<class I> auto insert(typename std::vector<Stored>::const_iterator at,I first,I last) { XeenMutationWatch::write(this);return value.insert(at,first,last); }
 void push_back(const T &v) { XeenMutationWatch::write(this);value.push_back(v); }
 template<class... A> Stored &emplace_back(A&&... args) { XeenMutationWatch::write(this);return value.emplace_back(std::forward<A>(args)...); }
 void swap(XeenMutableVector &v) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);value.swap(v.value); }
 template<class U=T,std::enable_if_t<std::is_same_v<Stored,U>,int> =0>
 void swap(std::vector<U> &v) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);value.swap(v); }
 friend bool operator==(const XeenMutableVector &a,const XeenMutableVector &b) { return a.value==b.value; }
 friend bool operator!=(const XeenMutableVector &a,const XeenMutableVector &b) { return !(a==b); }
 friend bool operator==(const XeenMutableVector &a,const std::vector<T> &b) { return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin()); }
 friend bool operator==(const std::vector<T> &a,const XeenMutableVector &b) { return b==a; }
 friend bool operator!=(const XeenMutableVector &a,const std::vector<T> &b) { return !(a==b); }
 friend bool operator!=(const std::vector<T> &a,const XeenMutableVector &b) { return !(a==b); }
};
template<class T> class XeenMutableSet {
 std::set<T> values;
public:
 // Set keys are immutable. Iterator dereference returns a detached identity,
 // so no caller can retain a writable alias to a tree node.
 class const_iterator {
  friend class XeenMutableSet;
  template<class> friend class XeenReadOnlySet;
  typename std::set<T>::const_iterator position;
  explicit const_iterator(typename std::set<T>::const_iterator p):position(p) {}
 public:
  using iterator_category=std::forward_iterator_tag;
  using value_type=T;using difference_type=std::ptrdiff_t;using pointer=void;using reference=T;
  const_iterator()=default;
  T operator*() const { return *position; }
  struct Arrow { T value;const T *operator->() const { return &value; } };
  Arrow operator->() const { return {*position}; }
  const_iterator &operator++() { ++position;return *this; }
  const_iterator operator++(int) { auto copy=*this;++*this;return copy; }
  friend bool operator==(const const_iterator &a,const const_iterator &b) { return a.position==b.position; }
  friend bool operator!=(const const_iterator &a,const const_iterator &b) { return !(a==b); }
 };
 XeenMutableSet()=default;
 XeenMutableSet(const std::set<T> &v):values(v) {}
 XeenMutableSet(const XeenMutableSet &v):values(v.values) {}
 XeenMutableSet(XeenMutableSet &&v) noexcept:values(std::move(v.values)) { XeenMutationWatch::write(&v); }
 XeenMutableSet &operator=(const XeenMutableSet &v) { XeenMutationWatch::write(this);values=v.values;return *this; }
 XeenMutableSet &operator=(XeenMutableSet &&v) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);values=std::move(v.values);return *this; }
 XeenMutableSet &operator=(const std::set<T> &v) { XeenMutationWatch::write(this);values=v;return *this; }
 operator std::set<T>() const { return values; }
 auto size() const noexcept { return values.size(); } bool empty() const noexcept { return values.empty(); }
 auto begin() const noexcept { return const_iterator(values.begin()); } auto end() const noexcept { return const_iterator(values.end()); }
 auto count(const T &v) const { return values.count(v); } auto find(const T &v) const { return const_iterator(values.find(v)); }
 auto insert(const T &v) { XeenMutationWatch::write(this);const auto result=values.insert(v);return std::make_pair(const_iterator(result.first),result.second); }
 template<class I> void insert(I first,I last) { XeenMutationWatch::write(this);values.insert(first,last); }
 auto erase(const T &v) { XeenMutationWatch::write(this);return values.erase(v); }
 auto erase(const_iterator at) { XeenMutationWatch::write(this);return const_iterator(values.erase(at.position)); }
 void clear() noexcept { XeenMutationWatch::write(this);values.clear(); }
 void swap(XeenMutableSet &v) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);values.swap(v.values); }
 void swap(std::set<T> &v) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);values.swap(v); }
 friend bool operator==(const XeenMutableSet &a,const XeenMutableSet &b) { return a.values==b.values; }
 friend bool operator!=(const XeenMutableSet &a,const XeenMutableSet &b) { return !(a==b); }
 friend bool operator==(const XeenMutableSet &a,const std::set<T> &b) { return a.values==b; }
 friend bool operator==(const std::set<T> &a,const XeenMutableSet &b) { return b==a; }
 friend bool operator!=(const XeenMutableSet &a,const std::set<T> &b) { return !(a==b); }
 friend bool operator!=(const std::set<T> &a,const XeenMutableSet &b) { return !(a==b); }
};
template<class T> class XeenReadOnlySet {
 const std::set<T> *values;
public:
 using const_iterator=typename XeenMutableSet<T>::const_iterator;
 explicit XeenReadOnlySet(const std::set<T> &v) noexcept:values(&v) {}
 auto size() const noexcept { return values->size(); } bool empty() const noexcept { return values->empty(); }
 auto begin() const noexcept { return const_iterator(values->begin()); } auto end() const noexcept { return const_iterator(values->end()); }
 auto find(const T &v) const { return const_iterator(values->find(v)); }
 auto count(const T &v) const { return values->count(v); }
 operator std::set<T>() const { return *values; }
 friend bool operator==(XeenReadOnlySet a,XeenReadOnlySet b) { return *a.values==*b.values; }
 friend bool operator!=(XeenReadOnlySet a,XeenReadOnlySet b) { return !(a==b); }
 friend bool operator==(XeenReadOnlySet a,const std::set<T> &b) { return *a.values==b; }
 friend bool operator==(const std::set<T> &a,XeenReadOnlySet b) { return b==a; }
 friend bool operator!=(XeenReadOnlySet a,const std::set<T> &b) { return !(a==b); }
 friend bool operator!=(const std::set<T> &a,XeenReadOnlySet b) { return !(a==b); }
};
template<class... T> class XeenMutableVariant:private std::variant<T...> {
 using Base=std::variant<T...>;
public:
 using Base::Base;
 XeenMutableVariant()=default;
 XeenMutableVariant(const XeenMutableVariant &v):Base(v) {}
 template<class V> bool holds() const noexcept { return std::holds_alternative<V>(static_cast<const Base&>(*this)); }
 template<class V> V &get() { return std::get<V>(static_cast<Base&>(*this)); }
 template<class V> const V &get() const { return std::get<V>(static_cast<const Base&>(*this)); }
 template<class V> V *getIf() noexcept { return std::get_if<V>(static_cast<Base*>(this)); }
 template<class V> const V *getIf() const noexcept { return std::get_if<V>(static_cast<const Base*>(this)); }
 std::size_t index() const noexcept { return Base::index(); }
 XeenMutableVariant &operator=(const XeenMutableVariant &v) { XeenMutationWatch::write(this);Base::operator=(v);return *this; }
 template<class V> XeenMutableVariant &operator=(V &&v) { XeenMutationWatch::write(this);Base::operator=(std::forward<V>(v));return *this; }
 template<class V,class... A> V &emplace(A&&... args) { XeenMutationWatch::write(this);return Base::template emplace<V>(std::forward<A>(args)...); }
 template<std::size_t I,class... A> auto &emplace(A&&... args) { XeenMutationWatch::write(this);return Base::template emplace<I>(std::forward<A>(args)...); }
 void swap(XeenMutableVariant &v) noexcept(noexcept(std::declval<Base&>().swap(v))) { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);Base::swap(v); }
};
template<class T> class XeenMutable {
 T value{};
public:
 constexpr XeenMutable()=default;
 constexpr XeenMutable(T v):value(v) {}
 constexpr XeenMutable(const XeenMutable &v) noexcept:value(v.value) {}
 XeenMutable &operator=(const XeenMutable &v) noexcept { return *this=v.value; }
 XeenMutable &operator=(T v) noexcept { XeenMutationWatch::write(this);value=v;return *this; }
 constexpr operator T() const noexcept { return value; }
 template<class U, std::enable_if_t<std::is_arithmetic_v<U> || std::is_enum_v<U>,int> =0>
 explicit constexpr operator U() const noexcept { return static_cast<U>(value); }
 XeenMutable &operator++() noexcept { return *this=T(value+1); }
 T operator++(int) noexcept { const auto old=value;++*this;return old; }
 XeenMutable &operator--() noexcept { return *this=T(value-1); }
 T operator--(int) noexcept { const auto old=value;--*this;return old; }
 template<class V> XeenMutable &operator+=(V v) noexcept { return *this=T(value+v); }
 template<class V> XeenMutable &operator-=(V v) noexcept { return *this=T(value-v); }
 template<class V> XeenMutable &operator|=(V v) noexcept { return *this=T(value|v); }
 template<class V> XeenMutable &operator&=(V v) noexcept { return *this=T(value&v); }
 template<class V> XeenMutable &operator^=(V v) noexcept { return *this=T(value^v); }
};
template<class V,class... T> V &xeenGet(XeenMutableVariant<T...> &v) { return v.template get<V>(); }
template<class V,class... T> const V &xeenGet(const XeenMutableVariant<T...> &v) { return v.template get<V>(); }
template<class V,class... T> V *xeenGetIf(XeenMutableVariant<T...> *v) noexcept { return v?v->template getIf<V>():nullptr; }
template<class V,class... T> const V *xeenGetIf(const XeenMutableVariant<T...> *v) noexcept { return v?v->template getIf<V>():nullptr; }
template<class V,class... T> bool xeenHolds(const XeenMutableVariant<T...> &v) noexcept { return v.template holds<V>(); }
template<class T,std::size_t N> class XeenMutableArray {
 std::array<XeenMutable<T>,N> values{};
public:
 XeenMutableArray()=default;
 XeenMutableArray(std::initializer_list<T> v) { if(v.size()>N)throw std::length_error("Observed array initializer exceeds capacity");std::copy(v.begin(),v.end(),values.begin()); }
 XeenMutableArray(const std::array<T,N> &v) { *this=v; }
 XeenMutableArray &operator=(const std::array<T,N> &v) { for(std::size_t i=0;i<N;++i) values[i]=v[i];return *this; }
 operator std::array<T,N>() const { std::array<T,N> result{};for(std::size_t i=0;i<N;++i)result[i]=values[i];return result; }
 auto &operator[](std::size_t i) { return values[i]; }
 T operator[](std::size_t i) const { return values[i]; }
 auto &at(std::size_t i) { return values.at(i); }
 T at(std::size_t i) const { return values.at(i); }
 constexpr std::size_t size() const noexcept { return N; }
 auto begin() { return values.begin(); } auto end() { return values.end(); }
 auto begin() const { return values.begin(); } auto end() const { return values.end(); }
 void fill(T v) { for(auto &item:values)item=v; }
 friend bool operator==(const XeenMutableArray &a,const XeenMutableArray &b) { return a.values==b.values; }
 friend bool operator!=(const XeenMutableArray &a,const XeenMutableArray &b) { return !(a==b); }
 friend bool operator==(const XeenMutableArray &a,const std::array<T,N> &b) { return static_cast<std::array<T,N>>(a)==b; }
 friend bool operator==(const std::array<T,N> &a,const XeenMutableArray &b) { return b==a; }
 friend bool operator!=(const XeenMutableArray &a,const std::array<T,N> &b) { return !(a==b); }
 friend bool operator!=(const std::array<T,N> &a,const XeenMutableArray &b) { return !(a==b); }
};
template<class T> class XeenMutableOptional {
 std::optional<T> storage;
public:
 XeenMutableOptional()=default;
 XeenMutableOptional(std::nullopt_t) {}
 XeenMutableOptional(const T &v) noexcept(std::is_nothrow_copy_constructible_v<T>):storage(v) {}
 XeenMutableOptional(const std::optional<T> &v):storage(v) {}
 XeenMutableOptional(const XeenMutableOptional &v) noexcept(std::is_nothrow_copy_constructible_v<T>):storage(v.storage) {}
 XeenMutableOptional(XeenMutableOptional &&v) noexcept(std::is_nothrow_move_constructible_v<T>):storage(std::move(v.storage)) { XeenMutationWatch::write(&v); }
 XeenMutableOptional &operator=(XeenMutableOptional &&v) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);storage=std::move(v.storage);return *this; }
 XeenMutableOptional &operator=(const XeenMutableOptional &v) noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>) { XeenMutationWatch::write(this);storage=v.storage;return *this; }
 XeenMutableOptional &operator=(const std::optional<T> &v) { XeenMutationWatch::write(this);storage=v;return *this; }
 XeenMutableOptional &operator=(const T &v) { XeenMutationWatch::write(this);storage=v;return *this; }
 XeenMutableOptional &operator=(std::nullopt_t) { reset();return *this; }
 explicit operator bool() const noexcept { return bool(storage); }
 operator std::optional<T>() const { return storage; }
 T &value() { return storage.value(); } const T &value() const { return storage.value(); }
 bool has_value() const noexcept { return storage.has_value(); }
 const T &operator*() const { return *storage; } T &operator*() { return *storage; }
 const T *operator->() const { return &*storage; } T *operator->() { return &*storage; }
 void reset() noexcept { XeenMutationWatch::write(this);storage.reset(); }
 template<class... A> T &emplace(A&&... args) { XeenMutationWatch::write(this);return storage.emplace(std::forward<A>(args)...); }
 void swap(XeenMutableOptional &v) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>) { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);storage.swap(v.storage); }
 friend void swap(XeenMutableOptional &a,XeenMutableOptional &b) noexcept(noexcept(a.swap(b))) { a.swap(b); }
 friend bool operator==(const XeenMutableOptional &a,const XeenMutableOptional &b) { return a.storage==b.storage; }
 friend bool operator==(const XeenMutableOptional &a,const T &b) { return a.storage==b; }
 friend bool operator==(const T &a,const XeenMutableOptional &b) { return a==b.storage; }
 friend bool operator!=(const XeenMutableOptional &a,const T &b) { return !(a==b); }
 friend bool operator!=(const XeenMutableOptional &a,const XeenMutableOptional &b) { return !(a==b); }
 friend bool operator==(const XeenMutableOptional &a,const std::optional<T> &b) { return a.storage==b; }
 friend bool operator==(const std::optional<T> &a,const XeenMutableOptional &b) { return a==b.storage; }
 friend bool operator!=(const XeenMutableOptional &a,const std::optional<T> &b) { return !(a==b); }
 friend bool operator!=(const std::optional<T> &a,const XeenMutableOptional &b) { return !(a==b); }
};
class XeenMutableString {
 std::string value;
public:
 XeenMutableString()=default;
 XeenMutableString(const std::string &v):value(v) {}
 XeenMutableString(const char *v):value(v) {}
 XeenMutableString(const XeenMutableString &v):value(v.value) {}
 XeenMutableString(XeenMutableString &&v) noexcept:value(std::move(v.value)) { XeenMutationWatch::write(&v); }
 XeenMutableString &operator=(XeenMutableString &&v) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);value=std::move(v.value);return *this; }
 XeenMutableString &operator=(const XeenMutableString &v) { return *this=v.value; }
 XeenMutableString &operator=(const std::string &v) { XeenMutationWatch::write(this);value=v;return *this; }
 XeenMutableString &operator=(const char *v) { return *this=std::string(v); }
 operator std::string() const { return value; }
 bool empty() const noexcept { return value.empty(); } std::size_t size() const noexcept { return value.size(); }
 auto begin() const noexcept { return XeenValueIterator(value.begin()); } auto end() const noexcept { return XeenValueIterator(value.end()); }
 auto find(char c) const noexcept { return value.find(c); }
 void assign(const char *p,std::size_t n) { XeenMutationWatch::write(this);value.assign(p,n); }
 void clear() noexcept { XeenMutationWatch::write(this);value.clear(); }
 friend bool operator==(const XeenMutableString &a,const XeenMutableString &b) { return a.value==b.value; }
 friend bool operator!=(const XeenMutableString &a,const XeenMutableString &b) { return !(a==b); }
 friend bool operator==(const XeenMutableString &a,const char *b) { return a.value==b; }
 friend bool operator!=(const XeenMutableString &a,const char *b) { return !(a==b); }
 friend std::string operator+(const XeenMutableString &a,const char *b) { return a.value+b; }
 friend std::string operator+(const char *a,const XeenMutableString &b) { return a+b.value; }
 friend std::string operator+(const std::string &a,const XeenMutableString &b) { return a+b.value; }
 friend std::string operator+(const XeenMutableString &a,const std::string &b) { return a.value+b; }
 friend std::ostream &operator<<(std::ostream &o,const XeenMutableString &s) { return o<<s.value; }
};
// Diagnostics are read as values; mutation never exposes a raw retained string.
class XeenMutableDiagnostics {
 std::vector<std::string> value;
public:
 XeenMutableDiagnostics()=default;
 XeenMutableDiagnostics(const XeenMutableDiagnostics &v):value(v.value) {}
 XeenMutableDiagnostics &operator=(const XeenMutableDiagnostics &v) { XeenMutationWatch::write(this);value=v.value;return *this; }
 XeenMutableDiagnostics &operator=(const std::vector<std::string> &v) { XeenMutationWatch::write(this);value=v;return *this; }
 operator std::vector<std::string>() const { return value; }
 auto begin() const { return XeenValueIterator(value.begin()); } auto end() const { return XeenValueIterator(value.end()); }
 auto size() const { return value.size(); } bool empty() const { return value.empty(); }
 std::string operator[](std::size_t i) const { return value[i]; }
 void push_back(std::string v) { XeenMutationWatch::write(this);value.push_back(std::move(v)); }
 void clear() { XeenMutationWatch::write(this);value.clear(); }
 void swap(XeenMutableDiagnostics &v) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&v);value.swap(v.value); }
 friend bool operator==(const XeenMutableDiagnostics &a,const XeenMutableDiagnostics &b) { return a.value==b.value; }
 friend bool operator!=(const XeenMutableDiagnostics &a,const XeenMutableDiagnostics &b) { return !(a==b); }
 friend bool operator!=(const XeenMutableDiagnostics &a,const std::vector<std::string> &b) { return a.value!=b; }
 friend bool operator==(const XeenMutableDiagnostics &a,const std::vector<std::string> &b) { return a.value==b; }
};
}
#endif
