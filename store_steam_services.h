#pragma once

#include "store/store_service.h"

#include <steam/steam_api.h>

#include <utility>

namespace nxm::store_steam {

/// The five neutral services (store_service.h), backed by the real
/// Steamworks interfaces. Each is a plain class rather than nested inside
/// store_steam_module.cpp so it's directly testable - see
/// tests/test_store_steam_services.cpp for the guard-path coverage (every
/// operation refuses safely, never crashes, when Steam isn't initialized).

class SteamCore final : public store::StoreCore {
public:
  [[nodiscard]] bool is_owned(nx::string_view dlc_id = {}) const override;
  [[nodiscard]] nx::vector<nx::string> owned_dlc_ids() const override;
  [[nodiscard]] nx::string_view store_name() const noexcept override;
};

/// Steam has no in-game "buy now" call for DLC: `purchase()` opens the Steam
/// Store overlay to the product page (`ActivateGameOverlayToStore`) and the
/// purchase itself happens in Steam's own UI - there is no completion
/// callback tied to it, so `purchase_pending()`/`purchase_error()` are
/// always false/empty here. A game is expected to re-check `is_owned()`
/// afterward, not poll a purchase result.
class SteamIap final : public store::StoreIap {
public:
  [[nodiscard]] nx::vector<store::StoreProduct> products() const override;
  bool purchase(nx::string_view product_id) override;
  [[nodiscard]] bool purchase_pending() const override { return false; }
  [[nodiscard]] nx::string_view purchase_error() const override { return {}; }
};

class SteamAchievements final : public store::StoreAchievements {
public:
  bool unlock(nx::string_view id) override;
  [[nodiscard]] bool is_unlocked(nx::string_view id) const override;
  [[nodiscard]] nx::vector<nx::string> achievement_ids() const override;
  bool set_stat(nx::string_view id, f64 value) override;
  [[nodiscard]] f64 stat(nx::string_view id) const override;
};

class SteamCloudSaves final : public store::StoreCloudSaves {
public:
  bool write(nx::string_view key, nx::string_view value) override;
  [[nodiscard]] nx::string read(nx::string_view key) const override;
  [[nodiscard]] bool exists(nx::string_view key) const override;
  bool remove(nx::string_view key) override;
  [[nodiscard]] nx::vector<nx::string> keys() const override;
  [[nodiscard]] u64 bytes_used() const override;
  [[nodiscard]] u64 bytes_total() const override;

private:
  [[nodiscard]] std::pair<u64, u64> quota() const;
};

class SteamPresence final : public store::StorePresence {
public:
  bool set_status(nx::string_view text) override;
  [[nodiscard]] nx::string_view own_name() const override;
  [[nodiscard]] usize friend_count() const override;
  [[nodiscard]] nx::vector<nx::string> friend_names() const override;
};

}
