#include "games/xeen/XeenMutation.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool v,const char *message) { if(!v)throw std::runtime_error(message); }
struct Value { XeenMutable<int> number=0; };
template<class T,class=void>struct HasClear:std::false_type{};
template<class T>struct HasClear<T,std::void_t<decltype(std::declval<T&>().clear())>>:std::true_type{};
template<class T,class F> void rejectsWrite(T &value,F operation) {
 XeenMutationWatch watch;watch.add(&value,sizeof(value));operation();
 check(!watch.current(),"write history was lost");
}
}
int main() {
 try {
  XeenMutable<int> scalar=7;
  rejectsWrite(scalar,[&]{scalar=8;scalar=7;});check(scalar==7,"scalar value");
  rejectsWrite(scalar,[&]{std::thread worker([&]{scalar=8;scalar=7;});worker.join();});
  XeenMutableArray<bool,4> array;auto &retained=array[1];
  rejectsWrite(array,[&]{retained=true;retained=false;});
  const std::array<bool,4> plain=array;check(plain==std::array<bool,4>{},"array detached value");
  XeenMutableOptional<Value> optional;optional.emplace();
  rejectsWrite(optional,[&]{const auto before=optional;optional.reset();optional=before;});
  const std::optional<Value> detached=optional;check(detached.has_value(),"optional detached value");
  XeenMutableVector<Value> vector;vector.resize(2);
  rejectsWrite(vector,[&]{const auto before=vector;vector.clear();vector=before;});
  std::vector<Value> staged(3);rejectsWrite(vector,[&]{vector.swap(staged);vector.swap(staged);});
  check(vector.size()==2 && staged.size()==3,"vector swaps preserve values");
  XeenMutableSet<int> set;set.insert(1);
  rejectsWrite(set,[&]{set.erase(1);set.insert(1);});
  std::set<int> stagedSet{2};rejectsWrite(set,[&]{set.swap(stagedSet);set.swap(stagedSet);});
  check(set==std::set<int>{1} && stagedSet==std::set<int>{2},"set swaps preserve values");
  std::vector<Value> actorStorage(2);XeenReadOnlyVector<Value> actorView(actorStorage);
  static_assert(!HasClear<decltype(actorView)>::value && !std::is_convertible_v<decltype(actorView),std::vector<Value>&>);
  std::vector<Value> actorCopy=actorView;actorCopy.clear();check(actorStorage.size()==2,"actor view leaked container mutation");
  std::set<int> overlayStorage{1};XeenReadOnlySet<int> overlayView(overlayStorage);
  static_assert(!std::is_reference_v<decltype(*overlayView.begin())>);
  auto key=*overlayView.begin();key=2;check(overlayStorage.count(1)==1,"set view leaked key mutation");
  struct Session { XeenMutationMarker marker;std::vector<Value> actors; } session;
  session.actors.resize(2);
  rejectsWrite(session,[&]{const auto before=session;session={};session=before;});
  XeenMutableVariant<Value,int> variant;variant.get<Value>().number=7;
  rejectsWrite(variant,[&]{variant.emplace<int>(3);variant.emplace<Value>().number=7;});
  check(variant.holds<Value>() && variant.get<Value>().number==7,"variant retained value");
  XeenMutableDiagnostics diagnostics;diagnostics.push_back("before");
  static_assert(!std::is_reference_v<decltype(*diagnostics.begin())>);
  static_assert(!std::is_reference_v<decltype(diagnostics[0])>);
  auto diagnosticCopy=diagnostics[0];diagnosticCopy="after";check(diagnostics[0]=="before","diagnostic alias escaped");
  XeenMutableString text="before";
  static_assert(!std::is_reference_v<decltype(*text.begin())>);
  auto character=*text.begin();character='x';check(text=="before","string character alias escaped");
  XeenMutationWatch before;before.add(&scalar,sizeof(scalar));scalar=8;
  before.clearRanges();before.add(&scalar,sizeof(scalar));check(!before.current(),"range refresh revived authority");
  XeenMutationWatch retire;retire.add(&scalar,sizeof(scalar));retire.clearRanges();
  scalar=9;check(retire.current()&&!before.current(),"address retirement altered earlier write history");
  static_assert(std::is_nothrow_copy_assignable_v<XeenMutable<int>>);
  static_assert(std::is_nothrow_copy_assignable_v<XeenMutableOptional<Value>>);
  static_assert(noexcept(vector.swap(staged)) && noexcept(set.swap(stagedSet)));
  std::cout<<"Observed value semantics passed\n";return 0;
 } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
