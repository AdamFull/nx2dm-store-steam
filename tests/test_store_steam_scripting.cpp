#include "framework/nxtest.h"

#include "app/engine.h"
#include "core/foundation/platform/filesystem.h"
#include "script/luau/luau_backend.h"
#include "script/luau/luau_bindings.h"
#include "script/script_host.h"
#include "store_steam/store_steam_leaderboards.h"
#include "store_steam/store_steam_overlay.h"
#include "store_steam/store_steam_scripting.h"
#include "store_steam/store_steam_workshop.h"

namespace {

using namespace nxm::store_steam;
namespace script = nxe::script;

struct Exposed {
  SteamWorkshop workshop;
  SteamLeaderboards leaderboards;
  SteamOverlay overlay;
  script::Host host;
  nx::vector<script::Host::ServiceInfo> services;

  Exposed() {
    expose_store_steam_extras(host, workshop, leaderboards, overlay);
    services = host.services();
  }
};

} // namespace

TEST_CASE("store_steam scripting: every service is exposed as script-services.json "
          "declares it") {
  const Exposed exposed;
  const auto manifest = nx::fs::file_read_text(
      nx::fs::path_view(NX_MODULE_SERVICES_MANIFEST));
  REQUIRE(manifest);

  nx::string error;
  if (!script::luau_manifest_agrees(manifest.value(), exposed.services, error))
    FAIL(error.c_str());
}

TEST_CASE("store_steam scripting: the module hands them over on its own") {
  std::unique_ptr<nxe::Module> found;
  for (const nxe::ModuleFactory factory : nxe::enabled_module_factories()) {
    std::unique_ptr<nxe::Module> module = factory();
    if (module != nullptr && module->name() == "store_steam")
      found = std::move(module);
  }
  REQUIRE(found != nullptr);

  nxe::Engine engine{nxe::Game{}};
  nxe::ModuleContext ctx{engine};
  script::Host host;
  found->on_expose_scripts(host, ctx);

  script::Host direct;
  SteamWorkshop workshop;
  SteamLeaderboards leaderboards;
  SteamOverlay overlay;
  expose_store_steam_extras(direct, workshop, leaderboards, overlay);
  CHECK(host.exposed_count() == direct.exposed_count());
  CHECK(host.exposed_count() > 0u);
}

// No Steam client runs here, so both lists are empty - but they arrive as
// tables a script can walk, agreeing with the counts beside them.
TEST_CASE("store_steam scripting: the lists come back as tables") {
  SteamWorkshop workshop;
  SteamLeaderboards leaderboards;
  SteamOverlay overlay;
  script::Host host;
  REQUIRE(host.set_backend(script::luau_backend()));
  expose_store_steam_extras(host, workshop, leaderboards, overlay);
  REQUIRE(host.bind());
  const nx::string_view source = R"(
local subscribed = host.store_steam_workshop_subscribed()
assert(type(subscribed) == "table", "subscribed")
assert(#subscribed == host.store_steam_workshop_subscribed_count(), "count")
local entries = host.store_steam_leaderboard_entries()
assert(type(entries) == "table", "entries")
assert(#entries == host.store_steam_leaderboard_entry_count(), "entry count")
return {}
)";
  CHECK(host.load("steam_lists",
                  {reinterpret_cast<const std::byte *>(source.data()),
                   source.size()}));
}
