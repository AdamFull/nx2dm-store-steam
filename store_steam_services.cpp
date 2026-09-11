#include "store_steam/store_steam_services.h"

#include "core/foundation/strings/format.h"

#include <utility>

namespace nxm::store_steam {
namespace {

[[nodiscard]] bool parse_app_id(const nx::string_view text, AppId_t &out) {
  if (text.empty())
    return false;
  u32 value = 0;
  for (const char c : text) {
    if (c < '0' || c > '9')
      return false;
    value = value * 10 + static_cast<u32>(c - '0');
  }
  out = static_cast<AppId_t>(value);
  return true;
}

}

// -- SteamCore --------------------------------------------------------

bool SteamCore::is_owned(const nx::string_view dlc_id) const {
  ISteamApps *const apps = SteamApps();
  if (apps == nullptr)
    return false;
  if (dlc_id.empty())
    return apps->BIsSubscribed();
  AppId_t app_id = 0;
  return parse_app_id(dlc_id, app_id) &&
         (apps->BIsSubscribedApp(app_id) || apps->BIsDlcInstalled(app_id));
}

nx::vector<nx::string> SteamCore::owned_dlc_ids() const {
  nx::vector<nx::string> out;
  ISteamApps *const apps = SteamApps();
  if (apps == nullptr)
    return out;
  const int count = apps->GetDLCCount();
  for (int i = 0; i < count; ++i) {
    AppId_t app_id = 0;
    bool available = false;
    char name[256] = {};
    if (!apps->BGetDLCDataByIndex(i, &app_id, &available, name,
                                  static_cast<int>(sizeof(name))))
      continue;
    // `available` is Valve's backend saying this DLC is purchasable, not
    // ownership - that's a separate check.
    if (apps->BIsSubscribedApp(app_id))
      out.push_back(nx::format("{}", static_cast<u32>(app_id)));
  }
  return out;
}

nx::string_view SteamCore::store_name() const noexcept { return "steam"; }

// -- SteamIap -----------------------------------------------------------

nx::vector<store::StoreProduct> SteamIap::products() const {
  nx::vector<store::StoreProduct> out;
  ISteamApps *const apps = SteamApps();
  if (apps == nullptr)
    return out;
  const int count = apps->GetDLCCount();
  for (int i = 0; i < count; ++i) {
    AppId_t app_id = 0;
    bool available = false;
    char name[256] = {};
    if (!apps->BGetDLCDataByIndex(i, &app_id, &available, name,
                                  static_cast<int>(sizeof(name))) ||
        !available)
      continue;
    store::StoreProduct product;
    product.id = nx::format("{}", static_cast<u32>(app_id));
    product.title = nx::string(name);
    out.push_back(std::move(product));
  }
  return out;
}

bool SteamIap::purchase(const nx::string_view product_id) {
  ISteamFriends *const friends = SteamFriends();
  AppId_t app_id = 0;
  if (friends == nullptr || !parse_app_id(product_id, app_id))
    return false;
  friends->ActivateGameOverlayToStore(app_id, k_EOverlayToStoreFlag_None);
  return true;
}

// -- SteamAchievements ----------------------------------------------

bool SteamAchievements::unlock(const nx::string_view id) {
  ISteamUserStats *const stats = SteamUserStats();
  const nx::string name(id);
  return stats != nullptr && stats->SetAchievement(name.c_str()) &&
         stats->StoreStats();
}

bool SteamAchievements::is_unlocked(const nx::string_view id) const {
  ISteamUserStats *const stats = SteamUserStats();
  if (stats == nullptr)
    return false;
  const nx::string name(id);
  bool achieved = false;
  return stats->GetAchievement(name.c_str(), &achieved) && achieved;
}

nx::vector<nx::string> SteamAchievements::achievement_ids() const {
  nx::vector<nx::string> out;
  ISteamUserStats *const stats = SteamUserStats();
  if (stats == nullptr)
    return out;
  const u32 count = stats->GetNumAchievements();
  out.reserve(count);
  for (u32 i = 0; i < count; ++i)
    if (const char *const name = stats->GetAchievementName(i))
      out.emplace_back(name);
  return out;
}

// SetStat/GetStat are overloaded on int32 and float, tied to how the stat
// was declared on Steamworks' backend - a caller here has no way to know
// which, so both are tried in turn.
bool SteamAchievements::set_stat(const nx::string_view id, const f64 value) {
  ISteamUserStats *const stats = SteamUserStats();
  if (stats == nullptr)
    return false;
  const nx::string name(id);
  if (stats->SetStat(name.c_str(), static_cast<int32>(value)))
    return stats->StoreStats();
  return stats->SetStat(name.c_str(), static_cast<float>(value)) &&
         stats->StoreStats();
}

f64 SteamAchievements::stat(const nx::string_view id) const {
  ISteamUserStats *const stats = SteamUserStats();
  if (stats == nullptr)
    return 0.0;
  const nx::string name(id);
  int32 as_int = 0;
  if (stats->GetStat(name.c_str(), &as_int))
    return static_cast<f64>(as_int);
  float as_float = 0.0f;
  return stats->GetStat(name.c_str(), &as_float) ? static_cast<f64>(as_float)
                                                  : 0.0;
}

// -- SteamCloudSaves --------------------------------------------------

bool SteamCloudSaves::write(const nx::string_view key,
                            const nx::string_view value) {
  ISteamRemoteStorage *const storage = SteamRemoteStorage();
  if (storage == nullptr)
    return false;
  const nx::string name(key);
  return storage->FileWrite(name.c_str(), value.data(),
                            static_cast<int32>(value.size()));
}

nx::string SteamCloudSaves::read(const nx::string_view key) const {
  ISteamRemoteStorage *const storage = SteamRemoteStorage();
  if (storage == nullptr)
    return {};
  const nx::string name(key);
  const int32 size = storage->GetFileSize(name.c_str());
  if (size <= 0)
    return {};
  nx::string out;
  out.resize(static_cast<usize>(size));
  const int32 read = storage->FileRead(name.c_str(), out.data(), size);
  if (read <= 0)
    return {};
  out.resize(static_cast<usize>(read));
  return out;
}

bool SteamCloudSaves::exists(const nx::string_view key) const {
  ISteamRemoteStorage *const storage = SteamRemoteStorage();
  if (storage == nullptr)
    return false;
  const nx::string name(key);
  return storage->FileExists(name.c_str());
}

bool SteamCloudSaves::remove(const nx::string_view key) {
  ISteamRemoteStorage *const storage = SteamRemoteStorage();
  if (storage == nullptr)
    return false;
  const nx::string name(key);
  return storage->FileDelete(name.c_str());
}

nx::vector<nx::string> SteamCloudSaves::keys() const {
  nx::vector<nx::string> out;
  ISteamRemoteStorage *const storage = SteamRemoteStorage();
  if (storage == nullptr)
    return out;
  const int32 count = storage->GetFileCount();
  out.reserve(count > 0 ? static_cast<usize>(count) : 0);
  for (int32 i = 0; i < count; ++i) {
    int32 size = 0;
    if (const char *const name = storage->GetFileNameAndSize(i, &size))
      out.emplace_back(name);
  }
  return out;
}

std::pair<u64, u64> SteamCloudSaves::quota() const {
  ISteamRemoteStorage *const storage = SteamRemoteStorage();
  uint64 total = 0;
  uint64 available = 0;
  if (storage == nullptr || !storage->GetQuota(&total, &available))
    return {0, 0};
  return {total, available};
}

u64 SteamCloudSaves::bytes_used() const {
  const auto [total, available] = quota();
  return total >= available ? total - available : 0;
}

u64 SteamCloudSaves::bytes_total() const { return quota().first; }

// -- SteamPresence ------------------------------------------------------

bool SteamPresence::set_status(const nx::string_view text) {
  ISteamFriends *const friends = SteamFriends();
  if (friends == nullptr)
    return false;
  const nx::string value(text);
  return friends->SetRichPresence("status", value.c_str());
}

nx::string_view SteamPresence::own_name() const {
  ISteamFriends *const friends = SteamFriends();
  return friends == nullptr ? nx::string_view{} : friends->GetPersonaName();
}

usize SteamPresence::friend_count() const {
  ISteamFriends *const friends = SteamFriends();
  if (friends == nullptr)
    return 0;
  const int count = friends->GetFriendCount(k_EFriendFlagImmediate);
  return count > 0 ? static_cast<usize>(count) : 0;
}

nx::vector<nx::string> SteamPresence::friend_names() const {
  nx::vector<nx::string> out;
  ISteamFriends *const friends = SteamFriends();
  if (friends == nullptr)
    return out;
  const int count = friends->GetFriendCount(k_EFriendFlagImmediate);
  out.reserve(count > 0 ? static_cast<usize>(count) : 0);
  for (int i = 0; i < count; ++i) {
    const CSteamID id = friends->GetFriendByIndex(i, k_EFriendFlagImmediate);
    out.emplace_back(friends->GetFriendPersonaName(id));
  }
  return out;
}

}
