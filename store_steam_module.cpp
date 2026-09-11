#include "store_steam/store_steam_config.h"

#include "store/store_service.h"

#include "core/app/engine.h"
#include "core/app/module.h"
#include "core/app/module_context.h"

#include "core/foundation/diagnostics/log.h"
#include "core/foundation/strings/format.h"

#include <steam/steam_api.h>

#include <cstdio>

namespace nxm::store_steam {
namespace {

constexpr nx::string_view PUMP_SYSTEM = "store_steam.pump";
const nx::log::Category log_store_steam = nx::log::category("store_steam");

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

/// Steam checks for this file in the process's current working directory
/// when SteamAPI_Init() is called outside the Steam client (local/dev runs);
/// written from the configured app_id so a developer only maintains one
/// value, in store_steam.ini, rather than a second copy of it in a loose
/// text file.
void write_steam_appid_file(const u32 app_id) {
  if (FILE *const file = std::fopen("steam_appid.txt", "w")) {
    std::fprintf(file, "%u", app_id);
    std::fclose(file);
  } else {
    nx::logw(log_store_steam, "could not write steam_appid.txt in '{}'",
              ".");
  }
}

// -- The five services, backed by the real Steamworks interfaces ----------

class SteamCore final : public store::StoreCore {
public:
  [[nodiscard]] bool is_owned(const nx::string_view dlc_id) const override {
    ISteamApps *const apps = SteamApps();
    if (apps == nullptr)
      return false;
    if (dlc_id.empty())
      return apps->BIsSubscribed();
    AppId_t app_id = 0;
    return parse_app_id(dlc_id, app_id) &&
           (apps->BIsSubscribedApp(app_id) || apps->BIsDlcInstalled(app_id));
  }

  [[nodiscard]] nx::string_view store_name() const noexcept override {
    return "steam";
  }
};

class SteamAchievements final : public store::StoreAchievements {
public:
  bool unlock(const nx::string_view id) override {
    ISteamUserStats *const stats = SteamUserStats();
    const nx::string name(id);
    return stats != nullptr && stats->SetAchievement(name.c_str()) &&
           stats->StoreStats();
  }

  [[nodiscard]] bool is_unlocked(const nx::string_view id) const override {
    ISteamUserStats *const stats = SteamUserStats();
    if (stats == nullptr)
      return false;
    const nx::string name(id);
    bool achieved = false;
    return stats->GetAchievement(name.c_str(), &achieved) && achieved;
  }
};

/// Steam has no in-game "buy now" call for DLC: `purchase()` opens the Steam
/// Store overlay to the product page (`ActivateGameOverlayToStore`) and the
/// purchase itself happens in Steam's own UI - there is no completion
/// callback tied to it, so `purchase_pending()`/`purchase_error()` are
/// always false/empty here. A game is expected to re-check `is_owned()`
/// afterward, not poll a purchase result.
class SteamIap final : public store::StoreIap {
public:
  [[nodiscard]] nx::vector<store::StoreProduct> products() const override {
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

  bool purchase(const nx::string_view product_id) override {
    ISteamFriends *const friends = SteamFriends();
    AppId_t app_id = 0;
    if (friends == nullptr || !parse_app_id(product_id, app_id))
      return false;
    friends->ActivateGameOverlayToStore(app_id, k_EOverlayToStoreFlag_None);
    return true;
  }

  [[nodiscard]] bool purchase_pending() const override { return false; }
  [[nodiscard]] nx::string_view purchase_error() const override { return {}; }
};

class SteamCloudSaves final : public store::StoreCloudSaves {
public:
  bool write(const nx::string_view key, const nx::string_view value) override {
    ISteamRemoteStorage *const storage = SteamRemoteStorage();
    if (storage == nullptr)
      return false;
    const nx::string name(key);
    return storage->FileWrite(name.c_str(), value.data(),
                              static_cast<int32>(value.size()));
  }

  [[nodiscard]] nx::string read(const nx::string_view key) const override {
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
};

class SteamPresence final : public store::StorePresence {
public:
  bool set_status(const nx::string_view text) override {
    ISteamFriends *const friends = SteamFriends();
    if (friends == nullptr)
      return false;
    const nx::string value(text);
    return friends->SetRichPresence("status", value.c_str());
  }

  [[nodiscard]] usize friend_count() const override {
    ISteamFriends *const friends = SteamFriends();
    if (friends == nullptr)
      return 0;
    const int count = friends->GetFriendCount(k_EFriendFlagImmediate);
    return count > 0 ? static_cast<usize>(count) : 0;
  }
};

constexpr nxe::ModuleService PROVIDED_SERVICES[] = {
    {.id = store::kCoreService, .version = {1, 0, 0}},
    {.id = store::kIapService, .version = {1, 0, 0}},
    {.id = store::kAchievementsService, .version = {1, 0, 0}},
    {.id = store::kCloudSavesService, .version = {1, 0, 0}},
    {.id = store::kPresenceService, .version = {1, 0, 0}},
};

class StoreSteamModule final : public nxe::Module {
public:
  [[nodiscard]] nxe::ModuleDescriptor descriptor() const noexcept override {
    nxe::ModuleDescriptor out{};
    out.id = "store_steam";
    out.version = {1, 0, 0};
    out.provided_services = PROVIDED_SERVICES;
    // Steamworks is Windows/Linux/macOS only - no mobile, no console.
    out.platforms = nxe::ModulePlatform::Windows | nxe::ModulePlatform::Linux |
                    nxe::ModulePlatform::MacOS;
    return out;
  }

  bool on_register(nxe::ModuleContext &ctx) override {
    nxe::ServiceRegistrar registrar = ctx.service_registrar();
    store::StoreCore &core = m_core;
    store::StoreIap &iap = m_iap;
    store::StoreAchievements &achievements = m_achievements;
    store::StoreCloudSaves &cloud_saves = m_cloud_saves;
    store::StorePresence &presence = m_presence;
    return registrar.provide(store::kCoreService, PROVIDED_SERVICES[0].version, core) &&
           registrar.provide(store::kIapService, PROVIDED_SERVICES[1].version, iap) &&
           registrar.provide(store::kAchievementsService,
                              PROVIDED_SERVICES[2].version, achievements) &&
           registrar.provide(store::kCloudSavesService,
                              PROVIDED_SERVICES[3].version, cloud_saves) &&
           registrar.provide(store::kPresenceService, PROVIDED_SERVICES[4].version,
                              presence);
  }

  bool on_attach(nxe::ModuleContext &ctx) override {
    if (!ctx.schedule().try_define(
            PUMP_SYSTEM, nxe::sys::SystemFn([this](const nxe::sys::Context &) {
              if (m_initialized)
                SteamAPI_RunCallbacks();
            }))) {
      nx::logw(log_store_steam, "system '{}' is already owned by another module",
                PUMP_SYSTEM);
      return false;
    }
    ctx.schedule().add(nxe::sys::Stage::Update, PUMP_SYSTEM);

    if (const std::optional<ServiceConfig> config = load_project_config();
        config.has_value()) {
      write_steam_appid_file(config->app_id);
      m_initialized = SteamAPI_Init();
      if (m_initialized)
        nx::logi(log_store_steam, "attached, App ID {}", config->app_id);
      else
        nx::logw(log_store_steam,
                  "SteamAPI_Init failed - no Steam client running?");
    } else {
      nx::logi(log_store_steam, "no {} found; staying idle", kDefaultConfigPath);
    }

    return true;
  }

  void on_detach(nxe::ModuleContext &) override {
    if (m_initialized)
      SteamAPI_Shutdown();
  }

private:
  SteamCore m_core;
  SteamIap m_iap;
  SteamAchievements m_achievements;
  SteamCloudSaves m_cloud_saves;
  SteamPresence m_presence;
  bool m_initialized = false;
};

} // namespace
} // namespace nxm::store_steam

NX_DECLARE_MODULE(store_steam, nxm::store_steam::StoreSteamModule)
