#include "../src/Bridge/BuildPolicy.hpp"
#include "../src/Bridge/Protocol.hpp"
#include "../src/Core/SDK/SupportedBuild.hpp"
#include <cassert>
#include <limits>
int main() {
 using namespace MenuBridge;
 assert(BuildPolicy::Matches(3258,3258,3258));
 assert(BuildPolicy::Matches(3889,3889,3889));
 for(auto value:{0u,1604u,3095u,3888u,3890u,std::numeric_limits<unsigned>::max()}) {
  assert(!BuildPolicy::Supported(value));
  assert(!BuildPolicy::Matches(value,value,value));
  assert(!BuildPolicy::Matches(3889,value,3889));
  assert(!BuildPolicy::Matches(3889,3889,value));
 }
 assert(!BuildPolicy::Matches(3889,3258,3889));
 assert(!BuildPolicy::Matches(3889,3889,3258));
 assert(!BuildPolicy::Matches(3258,3258,3889));
 assert(!BuildPolicy::Matches(3889,3889,0x100000F31ULL));
 assert(BuildPolicy::Image3889(1793798832u,65387520u));
 assert(BuildPolicy::Image3889(1793965275u,65387520u));
 assert(!BuildPolicy::Image3889(1793965276u,65387520u));
 assert(!BuildPolicy::Image3889(1793965275u,65387521u));
 assert(ClientBuild::Detect("FiveM_b3889_GTAProcess.exe")==3889);
 assert(ClientBuild::Detect("FiveM_b3258_GameProcess.exe")==3258);
 assert(ClientBuild::Detect("FiveM_b3889_GTAProcess.exe.bak")==0);
 assert(ClientBuild::Detect("FiveM_b38890_GTAProcess.exe")==0);
 static_assert(version==8,"EXE and DLL require protocol 8");
 Command command{};command.id=1;command.action=Action::TeleportWaypoint;
 assert(Valid(command));command.id=0;assert(!Valid(command));
 command.id=1;command.action=static_cast<Action>(999);assert(!Valid(command));
}
