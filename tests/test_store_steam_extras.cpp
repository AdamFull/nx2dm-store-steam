#include "framework/nxtest.h"

#include "store_steam/store_steam_leaderboards.h"
#include "store_steam/store_steam_overlay.h"
#include "store_steam/store_steam_workshop.h"

// Same guard-path bar as test_store_steam_services.cpp: no live Steam
// client runs here, so every SteamXxx() accessor returns null throughout
// this binary and every operation below must refuse safely.

using namespace nxm::store_steam;

TEST_CASE("store_steam extras: Workshop refuses safely with no client") {
  SteamWorkshop workshop;
  CHECK_FALSE(workshop.publish("My Item", "A description", ".", ""));
  CHECK_FALSE(workshop.publish_pending());
  CHECK(workshop.publish_error().empty());
  CHECK(workshop.published_file_id() == 0u);

  CHECK_FALSE(workshop.subscribe(123));
  CHECK_FALSE(workshop.unsubscribe(123));
  CHECK_FALSE(workshop.subscribe_pending());
  CHECK(workshop.subscribe_error().empty());

  workshop.refresh_subscribed();
  CHECK(workshop.subscribed_count() == 0u);
  CHECK(workshop.subscribed_id(0) == 0u);

  CHECK_FALSE(workshop.is_installed(123));
  CHECK(workshop.install_path(123).empty());
  CHECK_FALSE(workshop.download(123));
  CHECK(workshop.download_bytes_downloaded(123) == 0u);
  CHECK(workshop.download_bytes_total(123) == 0u);
}

TEST_CASE("store_steam extras: Leaderboards refuses safely with no client") {
  SteamLeaderboards leaderboards;
  CHECK_FALSE(leaderboards.find("high_scores", true));
  CHECK_FALSE(leaderboards.find_pending());
  CHECK_FALSE(leaderboards.found());

  // No leaderboard was ever found, so uploading/downloading must refuse too
  // even though SteamUserStats() itself is unreachable either way.
  CHECK_FALSE(leaderboards.upload_score(100));
  CHECK_FALSE(leaderboards.upload_pending());
  CHECK_FALSE(leaderboards.upload_succeeded());

  CHECK_FALSE(leaderboards.download(0, 10));
  CHECK_FALSE(leaderboards.download_pending());
  CHECK(leaderboards.entry_count() == 0u);
  CHECK(leaderboards.entry_rank(0) == 0);
  CHECK(leaderboards.entry_score(0) == 0);
  CHECK(leaderboards.entry_name(0).empty());
}

TEST_CASE("store_steam extras: Overlay refuses safely with no client") {
  const SteamOverlay overlay;
  CHECK_FALSE(overlay.open("Friends"));
  CHECK_FALSE(overlay.open_web_page("https://example.com", false));
  CHECK_FALSE(overlay.open_to_friend(0, "chat"));
}
