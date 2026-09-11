#include "store_steam/store_steam_config.h"

#include "core/foundation/diagnostics/log.h"
#include "core/foundation/serialization/ini.h"
#include "core/foundation/vfs/vfs.h"

namespace nxm::store_steam {
namespace {

const nx::log::Category log_store_steam = nx::log::category("store_steam");

[[nodiscard]] bool parse_u32(const nx::string_view text, u32 &out) noexcept {
  if (text.empty())
    return false;
  u32 value = 0;
  for (const char c : text) {
    if (c < '0' || c > '9')
      return false;
    value = value * 10 + static_cast<u32>(c - '0');
  }
  out = value;
  return true;
}

} // namespace

std::optional<ServiceConfig> load_project_config(const nx::string_view path) {
  const auto text = nx::vfs::read_text(path);
  if (!text) {
    if (text.error().kind != nx::fs::io_error::NotFound)
      nx::logw(log_store_steam, "config: could not read '{}': {}", path,
               nx::fs::to_string(text.error().kind));
    return {};
  }

  const auto parsed = nx::ini::parse(text->view());
  if (!parsed) {
    const nx::ini::ParseError &error = parsed.error();
    nx::logw(log_store_steam, "config: {}:{}:{}: {}", path, error.line,
              error.column, error.message());
    return {};
  }

  const nx::string *const app_id_text = parsed->find("store_steam", "app_id");
  if (app_id_text == nullptr) {
    nx::logw(log_store_steam, "config: '{}' is missing [store_steam] app_id",
              path);
    return {};
  }

  ServiceConfig config;
  if (!parse_u32(app_id_text->view(), config.app_id)) {
    nx::logw(log_store_steam,
              "config: '{}' [store_steam] app_id '{}' is not a positive integer",
              path, *app_id_text);
    return {};
  }

  return config;
}

}
