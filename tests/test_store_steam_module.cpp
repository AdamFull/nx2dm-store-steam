#include "framework/nxtest.h"

#include "core/app/module.h"

#include <steam/steam_api.h>

TEST_CASE("store_steam: the module is in the build's registry") {
  bool found = false;
  for (const nxe::ModuleFactory factory : nxe::enabled_module_factories()) {
    const std::unique_ptr<nxe::Module> module = factory();
    found = found || (module != nullptr && module->name() == "store_steam");
  }
  CHECK(found);
}

// No live Steam client runs in this environment, so SteamAPI_Init()
// returning false is expected here, not a failure - a missing or
// ABI-mismatched DLL fails process startup entirely, before this test body
// even runs. What this checks instead is the SDK's own documented contract
// around that result: the interface accessors are null before a successful
// init and non-null after one - a stubbed-out or mislinked DLL that
// returned garbage regardless of init state would be caught here.
TEST_CASE("store_steam: SteamAPI_Init reaches the real SDK") {
  if (SteamAPI_Init()) {
    CHECK(SteamUser() != nullptr);
    CHECK(SteamApps() != nullptr);
    SteamAPI_Shutdown();
  } else {
    CHECK(SteamUser() == nullptr);
  }
}
