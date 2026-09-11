#pragma once

namespace nxe::script {
class Host;
}

namespace nxm::store_steam {

class SteamWorkshop;
class SteamLeaderboards;
class SteamOverlay;

/// The Steam-only `host.store_steam_*` surface (Workshop, leaderboards,
/// overlay control) - deliberately separate from store/store_scripting.cpp,
/// which stays neutral-only. Unlike the neutral surface, these three are
/// captured by direct reference rather than looked up through
/// ServiceRegistry: nothing outside store_steam itself will ever need to
/// find them, so there's no "which backend provides this" question to
/// resolve.
void expose_store_steam_extras(nxe::script::Host &host, SteamWorkshop &workshop,
                                SteamLeaderboards &leaderboards,
                                SteamOverlay &overlay);

}
