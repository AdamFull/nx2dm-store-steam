#include "framework/nxtest.h"

#include "store_steam/store_steam_config.h"

#include "core/foundation/vfs/vfs.h"

namespace {

using nxm::store_steam::load_project_config;

struct VfsScope {
  VfsScope() { initialized = nx::vfs::initialize(); }
  ~VfsScope() { nx::vfs::shutdown(); }
  bool initialized = false;
};

[[nodiscard]] nx::blob<u8> as_bytes(const nx::string_view text) {
  return nx::blob<u8>(
      {reinterpret_cast<const u8 *>(text.data()), text.size()});
}

} // namespace

TEST_CASE("store_steam config: no file present is not an error") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: a valid file is parsed") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_steam.ini", as_bytes("[store_steam]\napp_id = 480\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  const std::optional<nxm::store_steam::ServiceConfig> config =
      load_project_config();
  REQUIRE(config.has_value());
  CHECK(config->app_id == 480u);

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: missing app_id is refused") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_steam.ini", as_bytes("[store_steam]\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: a non-numeric app_id is refused") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_steam.ini",
             as_bytes("[store_steam]\napp_id = not-a-number\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: malformed ini does not crash") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_steam.ini", as_bytes("this is not [ini at all"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: an explicit path overrides the default") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/somewhere/else.ini", as_bytes("[store_steam]\napp_id = 7\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());
  const std::optional<nxm::store_steam::ServiceConfig> config =
      load_project_config("/somewhere/else.ini");
  REQUIRE(config.has_value());
  CHECK(config->app_id == 7u);

  nx::vfs::unmount(mount);
}
