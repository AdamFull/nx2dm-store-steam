#include "store_steam/store_steam_overlay.h"

namespace nxm::store_steam {

bool SteamOverlay::open(const nx::string_view dialog) const {
  ISteamFriends *const friends = SteamFriends();
  if (friends == nullptr)
    return false;
  const nx::string value(dialog);
  friends->ActivateGameOverlay(value.c_str());
  return true;
}

bool SteamOverlay::open_web_page(const nx::string_view url,
                                 const bool modal) const {
  ISteamFriends *const friends = SteamFriends();
  if (friends == nullptr)
    return false;
  const nx::string value(url);
  friends->ActivateGameOverlayToWebPage(
      value.c_str(), modal ? k_EActivateGameOverlayToWebPageMode_Modal
                            : k_EActivateGameOverlayToWebPageMode_Default);
  return true;
}

bool SteamOverlay::open_to_friend(const int friend_index,
                                  const nx::string_view dialog) const {
  ISteamFriends *const friends = SteamFriends();
  if (friends == nullptr)
    return false;
  const CSteamID id = friends->GetFriendByIndex(friend_index, k_EFriendFlagImmediate);
  if (!id.IsValid())
    return false;
  const nx::string value(dialog);
  friends->ActivateGameOverlayToUser(value.c_str(), id);
  return true;
}

}
