#include "framework/nxtest.h"

#include "store_steam/store_steam_config.h"

#include "core/foundation/serialization/ini.h"
#include "core/foundation/threading/thread_pool.h"
#include "core/foundation/vfs/async_io.h"
#include "core/foundation/vfs/vfs.h"

namespace {

nx::vfs::AsyncIoService &test_io() {
  static nx::thread_pool pool(
      {.thread_count = 1, .name = "nx-test-io", .blocking_threads = 1});
  static nx::vfs::AsyncIoService io(pool);
  return io;
}

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

  CHECK_FALSE(load_project_config(test_io()).has_value());

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
      load_project_config(test_io());
  REQUIRE(config.has_value());
  CHECK(config->app_id == 480u);

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: a cooked file is read before its source") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  const auto cooked = nx::ini::parse("[store_steam]\napp_id = 730\n");
  REQUIRE(cooked);
  const nx::vector<u8> bytes = nx::ini::encode(*cooked);
  memory->add("/config/store_steam.ini.nxb",
              nx::blob<u8>({bytes.data(), bytes.size()}));
  memory->add("/config/store_steam.ini",
              as_bytes("[store_steam]\napp_id = 480\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  const std::optional<nxm::store_steam::ServiceConfig> config =
      load_project_config(test_io());
  REQUIRE(config.has_value());
  CHECK(config->app_id == 730u);

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: missing app_id is refused") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_steam.ini", as_bytes("[store_steam]\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config(test_io()).has_value());

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

  CHECK_FALSE(load_project_config(test_io()).has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: malformed ini does not crash") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_steam.ini", as_bytes("this is not [ini at all"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config(test_io()).has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_steam config: an explicit path overrides the default") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/somewhere/else.ini", as_bytes("[store_steam]\napp_id = 7\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config(test_io()).has_value());
  const std::optional<nxm::store_steam::ServiceConfig> config =
      load_project_config(test_io(), "/somewhere/else.ini");
  REQUIRE(config.has_value());
  CHECK(config->app_id == 7u);

  nx::vfs::unmount(mount);
}
