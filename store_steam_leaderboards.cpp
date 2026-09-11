#include "store_steam/store_steam_leaderboards.h"

namespace nxm::store_steam {

bool SteamLeaderboards::find(const nx::string_view name,
                             const bool create_if_missing) {
  ISteamUserStats *const stats = SteamUserStats();
  if (stats == nullptr)
    return false;
  const nx::string leaderboard_name(name);
  m_find_pending = true;
  m_leaderboard = 0;
  const SteamAPICall_t call =
      create_if_missing
          ? stats->FindOrCreateLeaderboard(leaderboard_name.c_str(),
                                           k_ELeaderboardSortMethodDescending,
                                           k_ELeaderboardDisplayTypeNumeric)
          : stats->FindLeaderboard(leaderboard_name.c_str());
  m_find_result.Set(call, this, &SteamLeaderboards::on_find);
  return true;
}

void SteamLeaderboards::on_find(LeaderboardFindResult_t *const result,
                                const bool io_failure) {
  m_find_pending = false;
  if (!io_failure && result->m_bLeaderboardFound)
    m_leaderboard = result->m_hSteamLeaderboard;
}

bool SteamLeaderboards::upload_score(const i32 score) {
  ISteamUserStats *const stats = SteamUserStats();
  if (stats == nullptr || m_leaderboard == 0)
    return false;
  m_upload_pending = true;
  m_upload_succeeded = false;
  const SteamAPICall_t call = stats->UploadLeaderboardScore(
      m_leaderboard, k_ELeaderboardUploadScoreMethodForceUpdate, score,
      nullptr, 0);
  m_upload_result.Set(call, this, &SteamLeaderboards::on_upload);
  return true;
}

void SteamLeaderboards::on_upload(LeaderboardScoreUploaded_t *const result,
                                  const bool io_failure) {
  m_upload_pending = false;
  m_upload_succeeded = !io_failure && result->m_bSuccess != 0;
}

bool SteamLeaderboards::download(const int range_start, const int range_end) {
  ISteamUserStats *const stats = SteamUserStats();
  if (stats == nullptr || m_leaderboard == 0)
    return false;
  m_download_pending = true;
  m_entries.clear();
  const SteamAPICall_t call = stats->DownloadLeaderboardEntries(
      m_leaderboard, k_ELeaderboardDataRequestGlobal, range_start, range_end);
  m_download_result.Set(call, this, &SteamLeaderboards::on_download);
  return true;
}

void SteamLeaderboards::on_download(LeaderboardScoresDownloaded_t *const result,
                                    const bool io_failure) {
  m_download_pending = false;
  if (io_failure)
    return;
  ISteamUserStats *const stats = SteamUserStats();
  ISteamFriends *const friends = SteamFriends();
  if (stats == nullptr)
    return;
  for (int i = 0; i < result->m_cEntryCount; ++i) {
    LeaderboardEntry_t entry{};
    if (!stats->GetDownloadedLeaderboardEntry(result->m_hSteamLeaderboardEntries,
                                              i, &entry, nullptr, 0))
      continue;
    Entry out;
    out.rank = entry.m_nGlobalRank;
    out.score = entry.m_nScore;
    if (friends != nullptr)
      out.name = nx::string(friends->GetFriendPersonaName(entry.m_steamIDUser));
    m_entries.push_back(std::move(out));
  }
}

i32 SteamLeaderboards::entry_rank(const usize index) const noexcept {
  return index < m_entries.size() ? m_entries[index].rank : 0;
}

i32 SteamLeaderboards::entry_score(const usize index) const noexcept {
  return index < m_entries.size() ? m_entries[index].score : 0;
}

nx::string_view SteamLeaderboards::entry_name(const usize index) const noexcept {
  return index < m_entries.size() ? m_entries[index].name.view()
                                   : nx::string_view{};
}

}
