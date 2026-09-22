#include "store_steam/store_steam_scripting.h"

#include "store_steam/store_steam_leaderboards.h"
#include "store_steam/store_steam_overlay.h"
#include "store_steam/store_steam_workshop.h"

#include "script/script_host.h"

#include <memory>

namespace nxm::store_steam {
namespace {

/// One leaderboard row as a script reads it.
struct LeaderboardEntry {
  f64 rank = 0.0;
  f64 score = 0.0;
  nx::string_view name;
};

}

void expose_store_steam_extras(nxe::script::Host &host, SteamWorkshop &workshop,
                               SteamLeaderboards &leaderboards,
                               SteamOverlay &overlay) {
  // -- Workshop -----------------------------------------------------------

  host.expose_as("store_steam_workshop_publish",
                 [&workshop](const nx::string_view title,
                             const nx::string_view description,
                             const nx::string_view content_folder,
                             const nx::string_view preview_file) {
                   return workshop.publish(title, description, content_folder,
                                           preview_file);
                 });
  host.expose_as("store_steam_workshop_publish_pending",
                 [&workshop]() { return workshop.publish_pending(); });
  host.expose_as("store_steam_workshop_publish_error", [&workshop]() {
    return workshop.publish_error();
  });
  host.expose_as("store_steam_workshop_published_file_id", [&workshop]() {
    return static_cast<f64>(workshop.published_file_id());
  });

  host.expose_as("store_steam_workshop_subscribe", [&workshop](const f64 file_id) {
    return workshop.subscribe(nx::cast<u64>(file_id));
  });
  host.expose_as("store_steam_workshop_unsubscribe",
                 [&workshop](const f64 file_id) {
                   return workshop.unsubscribe(nx::cast<u64>(file_id));
                 });
  host.expose_as("store_steam_workshop_subscribe_pending",
                 [&workshop]() { return workshop.subscribe_pending(); });
  host.expose_as("store_steam_workshop_subscribe_error", [&workshop]() {
    return workshop.subscribe_error();
  });

  host.expose_as("store_steam_workshop_refresh_subscribed",
                 [&workshop]() { workshop.refresh_subscribed(); return true; });
  host.expose_as("store_steam_workshop_subscribed", [&workshop] {
    nx::vector<f64> out;
    out.reserve(workshop.subscribed_count());
    for (usize i = 0; i < workshop.subscribed_count(); ++i)
      out.push_back(static_cast<f64>(workshop.subscribed_id(i)));
    return out;
  });
  host.expose_as("store_steam_workshop_subscribed_count", [&workshop]() {
    return static_cast<f64>(workshop.subscribed_count());
  });
  host.expose_as("store_steam_workshop_subscribed_id",
                 [&workshop](const f64 index) {
                   return static_cast<f64>(
                       workshop.subscribed_id(nx::cast<usize>(index)));
                 });

  host.expose_as("store_steam_workshop_is_installed",
                 [&workshop](const f64 file_id) {
                   return workshop.is_installed(nx::cast<u64>(file_id));
                 });

  const auto install_path = std::make_shared<nx::string>();
  host.expose_as(
      "store_steam_workshop_install_path",
      [&workshop, install_path](const f64 file_id) -> nx::string_view {
        *install_path = workshop.install_path(nx::cast<u64>(file_id));
        return install_path->view();
      });

  host.expose_as("store_steam_workshop_download", [&workshop](const f64 file_id) {
    return workshop.download(nx::cast<u64>(file_id));
  });
  host.expose_as("store_steam_workshop_download_bytes_downloaded",
                 [&workshop](const f64 file_id) {
                   return static_cast<f64>(
                       workshop.download_bytes_downloaded(nx::cast<u64>(file_id)));
                 });
  host.expose_as("store_steam_workshop_download_bytes_total",
                 [&workshop](const f64 file_id) {
                   return static_cast<f64>(
                       workshop.download_bytes_total(nx::cast<u64>(file_id)));
                 });

  // -- Leaderboards ---------------------------------------------------

  host.expose_as("store_steam_leaderboard_find",
                 [&leaderboards](const nx::string_view name,
                                 const bool create_if_missing) {
                   return leaderboards.find(name, create_if_missing);
                 });
  host.expose_as("store_steam_leaderboard_find_pending", [&leaderboards]() {
    return leaderboards.find_pending();
  });
  host.expose_as("store_steam_leaderboard_found",
                 [&leaderboards]() { return leaderboards.found(); });

  host.expose_as("store_steam_leaderboard_upload_score",
                 [&leaderboards](const f64 score) {
                   return leaderboards.upload_score(nx::cast<i32>(score));
                 });
  host.expose_as("store_steam_leaderboard_upload_pending", [&leaderboards]() {
    return leaderboards.upload_pending();
  });
  host.expose_as("store_steam_leaderboard_upload_succeeded", [&leaderboards]() {
    return leaderboards.upload_succeeded();
  });

  host.expose_as("store_steam_leaderboard_download",
                 [&leaderboards](const f64 range_start, const f64 range_end) {
                   return leaderboards.download(nx::cast<int>(range_start),
                                                nx::cast<int>(range_end));
                 });
  host.expose_as("store_steam_leaderboard_download_pending", [&leaderboards]() {
    return leaderboards.download_pending();
  });
  host.expose_as("store_steam_leaderboard_entries", [&leaderboards] {
    nx::vector<LeaderboardEntry> out;
    out.reserve(leaderboards.entry_count());
    for (usize i = 0; i < leaderboards.entry_count(); ++i)
      out.push_back({static_cast<f64>(leaderboards.entry_rank(i)),
                     static_cast<f64>(leaderboards.entry_score(i)),
                     leaderboards.entry_name(i)});
    return out;
  });
  host.expose_as("store_steam_leaderboard_entry_count", [&leaderboards]() {
    return static_cast<f64>(leaderboards.entry_count());
  });
  host.expose_as("store_steam_leaderboard_entry_rank",
                 [&leaderboards](const f64 index) {
                   return static_cast<f64>(
                       leaderboards.entry_rank(nx::cast<usize>(index)));
                 });
  host.expose_as("store_steam_leaderboard_entry_score",
                 [&leaderboards](const f64 index) {
                   return static_cast<f64>(
                       leaderboards.entry_score(nx::cast<usize>(index)));
                 });
  host.expose_as("store_steam_leaderboard_entry_name",
                 [&leaderboards](const f64 index) {
                   return leaderboards.entry_name(nx::cast<usize>(index));
                 });

  // -- Overlay ----------------------------------------------------------

  host.expose_as("store_steam_overlay_open",
                 [&overlay](const nx::string_view dialog) {
                   return overlay.open(dialog);
                 });
  host.expose_as("store_steam_overlay_open_web_page",
                 [&overlay](const nx::string_view url, const bool modal) {
                   return overlay.open_web_page(url, modal);
                 });
  host.expose_as("store_steam_overlay_open_to_friend",
                 [&overlay](const f64 friend_index,
                            const nx::string_view dialog) {
                   return overlay.open_to_friend(nx::cast<int>(friend_index),
                                                 dialog);
                 });
}

}
