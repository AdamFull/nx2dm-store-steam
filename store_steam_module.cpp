#include "store_steam/store_steam_config.h"
#include "store_steam/store_steam_leaderboards.h"
#include "store_steam/store_steam_overlay.h"
#include "store_steam/store_steam_scripting.h"
#include "store_steam/store_steam_services.h"
#include "store_steam/store_steam_workshop.h"

#include "store/store_service.h"

#include "app/engine.h"
#include "app/module_system/module.h"
#include "app/module_system/module_context.h"

#include "core/foundation/diagnostics/log.h"

#include <steam/steam_api.h>

#include <cstdio>

namespace nxm::store_steam {
namespace {

constexpr nx::string_view PUMP_SYSTEM = "store_steam.pump";
const nx::log::Category log_store_steam = nx::log::category("store_steam");

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

  void on_expose_scripts(nxe::script::Host &host, nxe::ModuleContext &) override {
    expose_store_steam_extras(host, m_workshop, m_leaderboards, m_overlay);
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

  // Steam-specific extras (Workshop, leaderboards, overlay) - never part of
  // store_service.h's neutral interface, never registered through
  // ServiceRegistry (see store_steam_scripting.h).
  SteamWorkshop m_workshop;
  SteamLeaderboards m_leaderboards;
  SteamOverlay m_overlay;
};

} // namespace
} // namespace nxm::store_steam

NX_DECLARE_MODULE(store_steam, nxm::store_steam::StoreSteamModule)
