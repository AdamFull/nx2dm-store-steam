#pragma once

#include "core/foundation/core/foundation.h"
#include "core/foundation/strings/utf8_string.h"

#include <steam/steam_api.h>

namespace nxm::store_steam {

/// Steam leaderboards (ISteamUserStats) - a Steam-only extra. Tracks one
/// "current" leaderboard at a time, matching how most simple games use
/// exactly one, rather than a handle registry. Same CCallResult-through-
/// the-existing-pump shape as SteamWorkshop.
class SteamLeaderboards {
public:
  bool find(nx::string_view name, bool create_if_missing);
  [[nodiscard]] bool find_pending() const noexcept { return m_find_pending; }
  [[nodiscard]] bool found() const noexcept { return m_leaderboard != 0; }

  bool upload_score(i32 score);
  [[nodiscard]] bool upload_pending() const noexcept { return m_upload_pending; }
  [[nodiscard]] bool upload_succeeded() const noexcept {
    return m_upload_succeeded;
  }

  bool download(int range_start, int range_end);
  [[nodiscard]] bool download_pending() const noexcept {
    return m_download_pending;
  }
  [[nodiscard]] usize entry_count() const noexcept { return m_entries.size(); }
  [[nodiscard]] i32 entry_rank(usize index) const noexcept;
  [[nodiscard]] i32 entry_score(usize index) const noexcept;
  [[nodiscard]] nx::string_view entry_name(usize index) const noexcept;

private:
  void on_find(LeaderboardFindResult_t *result, bool io_failure);
  void on_upload(LeaderboardScoreUploaded_t *result, bool io_failure);
  void on_download(LeaderboardScoresDownloaded_t *result, bool io_failure);

  CCallResult<SteamLeaderboards, LeaderboardFindResult_t> m_find_result;
  CCallResult<SteamLeaderboards, LeaderboardScoreUploaded_t> m_upload_result;
  CCallResult<SteamLeaderboards, LeaderboardScoresDownloaded_t> m_download_result;

  SteamLeaderboard_t m_leaderboard = 0;
  bool m_find_pending = false;
  bool m_upload_pending = false;
  bool m_upload_succeeded = false;
  bool m_download_pending = false;

  struct Entry {
    i32 rank = 0;
    i32 score = 0;
    nx::string name;
  };
  nx::vector<Entry> m_entries;
};

}
