#include "framework/nxtest.h"

#include "store_steam/store_steam_services.h"

// No live Steam client runs in this environment, so SteamApps()/SteamUser()/
// SteamUserStats()/SteamRemoteStorage()/SteamFriends() all return null
// throughout this binary (SteamAPI_Init() never actually succeeds - see
// test_store_steam_module.cpp). Every method below must refuse safely
// (false/empty, never a crash) under that guard path - the same bar
// test_modio_service.cpp already established for "every operation refuses
// correctly before Service::ready()".

using namespace nxm::store_steam;

TEST_CASE("store_steam services: SteamCore refuses safely with no client") {
  SteamCore core;
  CHECK_FALSE(core.is_owned());
  CHECK_FALSE(core.is_owned("480"));
  CHECK(core.owned_dlc_ids().empty());
  CHECK(core.store_name() == "steam");
}

TEST_CASE("store_steam services: SteamIap refuses safely with no client") {
  SteamIap iap;
  CHECK(iap.products().empty());
  CHECK_FALSE(iap.purchase("480"));
  CHECK_FALSE(iap.purchase_pending());
  CHECK(iap.purchase_error().empty());
}

TEST_CASE(
    "store_steam services: SteamAchievements refuses safely with no client") {
  SteamAchievements achievements;
  CHECK_FALSE(achievements.unlock("first_win"));
  CHECK_FALSE(achievements.is_unlocked("first_win"));
  CHECK(achievements.achievement_ids().empty());
  CHECK_FALSE(achievements.set_stat("enemies_killed", 5.0));
  CHECK(achievements.stat("enemies_killed") == 0.0);
}

TEST_CASE(
    "store_steam services: SteamCloudSaves refuses safely with no client") {
  SteamCloudSaves saves;
  CHECK_FALSE(saves.write("slot1", "data"));
  CHECK(saves.read("slot1").empty());
  CHECK_FALSE(saves.exists("slot1"));
  CHECK_FALSE(saves.remove("slot1"));
  CHECK(saves.keys().empty());
  CHECK(saves.bytes_used() == 0u);
  CHECK(saves.bytes_total() == 0u);
}

TEST_CASE(
    "store_steam services: SteamPresence refuses safely with no client") {
  SteamPresence presence;
  CHECK_FALSE(presence.set_status("in menu"));
  CHECK(presence.own_name().empty());
  CHECK(presence.friend_count() == 0u);
  CHECK(presence.friend_names().empty());
}
