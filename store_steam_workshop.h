#pragma once

#include "core/foundation/core/foundation.h"
#include "core/foundation/strings/utf8_string.h"

#include <steam/steam_api.h>

#include <utility>

namespace nxm::store_steam {

/// Steam Workshop (ISteamUGC) - a Steam-only extra, never part of
/// store_service.h's neutral interface. Every long-running call here returns
/// a SteamAPICall_t bound through a CCallResult, resolved whenever
/// StoreSteamModule's own "store_steam.pump" system calls
/// SteamAPI_RunCallbacks() - the same pump already driving the five neutral
/// services. One in-flight operation of each kind at a time, not a queue -
/// same documented simplification as StoreIap::purchase_pending().
class SteamWorkshop {
public:
  /// Chains the real 3-step publish flow: CreateItem(), then on success
  /// StartItemUpdate()+setters+SubmitItemUpdate(). Tags/visibility are left
  /// at Steam's defaults - a real scope reduction, not an oversight.
  bool publish(nx::string_view title, nx::string_view description,
               nx::string_view content_folder, nx::string_view preview_file);
  [[nodiscard]] bool publish_pending() const noexcept { return m_publish_pending; }
  [[nodiscard]] nx::string_view publish_error() const noexcept {
    return m_publish_error.view();
  }
  [[nodiscard]] u64 published_file_id() const noexcept {
    return m_published_file_id;
  }

  bool subscribe(u64 file_id);
  bool unsubscribe(u64 file_id);
  [[nodiscard]] bool subscribe_pending() const noexcept {
    return m_subscribe_pending;
  }
  [[nodiscard]] nx::string_view subscribe_error() const noexcept {
    return m_subscribe_error.view();
  }

  void refresh_subscribed();
  [[nodiscard]] usize subscribed_count() const noexcept {
    return m_subscribed.size();
  }
  [[nodiscard]] u64 subscribed_id(usize index) const noexcept;

  [[nodiscard]] bool is_installed(u64 file_id) const;
  [[nodiscard]] nx::string install_path(u64 file_id) const;

  bool download(u64 file_id) const;
  [[nodiscard]] u64 download_bytes_downloaded(u64 file_id) const;
  [[nodiscard]] u64 download_bytes_total(u64 file_id) const;

private:
  void on_create_item(CreateItemResult_t *result, bool io_failure);
  void on_submit_item_update(SubmitItemUpdateResult_t *result, bool io_failure);
  void on_subscribe_result(RemoteStorageSubscribePublishedFileResult_t *result,
                            bool io_failure);
  [[nodiscard]] std::pair<u64, u64> download_progress(u64 file_id) const;

  CCallResult<SteamWorkshop, CreateItemResult_t> m_create_item_result;
  CCallResult<SteamWorkshop, SubmitItemUpdateResult_t> m_submit_update_result;
  CCallResult<SteamWorkshop, RemoteStorageSubscribePublishedFileResult_t>
      m_subscribe_result;

  bool m_publish_pending = false;
  nx::string m_publish_error;
  u64 m_published_file_id = 0;
  nx::string m_publish_title;
  nx::string m_publish_description;
  nx::string m_publish_content_folder;
  nx::string m_publish_preview_file;

  bool m_subscribe_pending = false;
  nx::string m_subscribe_error;

  nx::vector<u64> m_subscribed;
};

}
