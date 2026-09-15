#include "framework/nxtest.h"

#include "app/engine.h"
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

  [[nodiscard]] const script::Host::ServiceInfo *
  find(const nx::string_view name) const {
    for (const script::Host::ServiceInfo &one : services)
      if (one.name == name)
        return &one;
    return nullptr;
  }
};

} // namespace

// This is the one place a mismatch between what store_steam_scripting.cpp
// actually registers and what modules/store_steam/script-services.json
// declares to Luau would show up - see test_modio_scripting.cpp's identical
// role for modio. No backend is registered in this harness (no live Steam
// client runs), so every callable here just exercises its own "no client"
// refusal path, not real Steam behavior.
TEST_CASE("store_steam scripting: every service is exposed with the shape a "
          "script is told about") {
  const Exposed exposed;

  static constexpr struct {
    nx::string_view name;
    nx::string_view signature;
  } WANT[] = {
      {"store_steam_workshop_publish", "(string,string,string,string)->(boolean)"},
      {"store_steam_workshop_publish_pending", "()->(boolean)"},
      {"store_steam_workshop_publish_error", "()->(string)"},
      {"store_steam_workshop_published_file_id", "()->(number)"},
      {"store_steam_workshop_subscribe", "(number)->(boolean)"},
      {"store_steam_workshop_unsubscribe", "(number)->(boolean)"},
      {"store_steam_workshop_subscribe_pending", "()->(boolean)"},
      {"store_steam_workshop_subscribe_error", "()->(string)"},
      {"store_steam_workshop_refresh_subscribed", "()->(boolean)"},
      {"store_steam_workshop_subscribed_count", "()->(number)"},
      {"store_steam_workshop_subscribed_id", "(number)->(number)"},
      {"store_steam_workshop_is_installed", "(number)->(boolean)"},
      {"store_steam_workshop_install_path", "(number)->(string)"},
      {"store_steam_workshop_download", "(number)->(boolean)"},
      {"store_steam_workshop_download_bytes_downloaded", "(number)->(number)"},
      {"store_steam_workshop_download_bytes_total", "(number)->(number)"},
      {"store_steam_leaderboard_find", "(string,boolean)->(boolean)"},
      {"store_steam_leaderboard_find_pending", "()->(boolean)"},
      {"store_steam_leaderboard_found", "()->(boolean)"},
      {"store_steam_leaderboard_upload_score", "(number)->(boolean)"},
      {"store_steam_leaderboard_upload_pending", "()->(boolean)"},
      {"store_steam_leaderboard_upload_succeeded", "()->(boolean)"},
      {"store_steam_leaderboard_download", "(number,number)->(boolean)"},
      {"store_steam_leaderboard_download_pending", "()->(boolean)"},
      {"store_steam_leaderboard_entry_count", "()->(number)"},
      {"store_steam_leaderboard_entry_rank", "(number)->(number)"},
      {"store_steam_leaderboard_entry_score", "(number)->(number)"},
      {"store_steam_leaderboard_entry_name", "(number)->(string)"},
      {"store_steam_overlay_open", "(string)->(boolean)"},
      {"store_steam_overlay_open_web_page", "(string,boolean)->(boolean)"},
      {"store_steam_overlay_open_to_friend", "(number,string)->(boolean)"},
  };

  CHECK(exposed.services.size() == nx::array_size(WANT));
  for (const auto &want : WANT) {
    const script::Host::ServiceInfo *const found = exposed.find(want.name);
    REQUIRE(found != nullptr);
    CHECK(found->signature == want.signature);
  }
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
