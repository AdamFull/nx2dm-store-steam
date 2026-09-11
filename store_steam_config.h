#pragma once

#include "core/foundation/strings/utf8_string.h"

#include <optional>

namespace nxm::store_steam {

/// VFS path StoreSteamModule::on_attach() checks automatically. A project
/// enables the module by cooking a file to this path (author it at
/// assets/config/store_steam.ini) with its own Steam App ID - e.g.:
///
///   [store_steam]
///   app_id = 480
///
/// `app_id` is required. Valve's own public SDK test App ID (480, Spacewar -
/// the same one `steamworksexample` ships with) is safe to commit for
/// development; replace it with the real App ID before shipping.
inline constexpr nx::string_view kDefaultConfigPath = "/config/store_steam.ini";

struct ServiceConfig {
  u32 app_id = 0;
};

/// Reads and validates an INI file at @p path. Empty if the file does not
/// exist, or (with a logged warning) if it exists but fails to parse or is
/// missing `app_id` - callers should treat both the same way, as "nothing to
/// auto-configure with".
[[nodiscard]] std::optional<ServiceConfig>
load_project_config(nx::string_view path = kDefaultConfigPath);

}
