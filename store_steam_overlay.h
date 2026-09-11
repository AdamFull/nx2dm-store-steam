#pragma once

#include "core/foundation/strings/utf8_string.h"

#include <steam/steam_api.h>

namespace nxm::store_steam {

/// Explicit Steam overlay control beyond the store page - a Steam-only
/// extra (`SteamIap::purchase()`, the neutral service, already handles
/// `ActivateGameOverlayToStore`). All synchronous void Steam calls, so
/// "success" here only means a live `ISteamFriends` (and, for
/// `open_to_friend`, a resolvable friend) was available - Steam gives no
/// completion signal for the overlay itself opening.
class SteamOverlay {
public:
  /// pchDialog per Steam's own docs: "Friends", "Community", "Players",
  /// "Settings", "OfficialGameGroup", "Stats", "Achievements".
  bool open(nx::string_view dialog) const;

  bool open_web_page(nx::string_view url, bool modal) const;

  /// `friend_index` is a friends-list index (same indexing
  /// `StorePresence::friend_names()` already uses), resolved to a
  /// `CSteamID` internally - Luau has no integer type that could carry a
  /// raw 64-bit SteamID as an opaque identity safely.
  bool open_to_friend(int friend_index, nx::string_view dialog) const;
};

}
