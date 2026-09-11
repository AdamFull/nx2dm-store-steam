#include "store_steam/store_steam_workshop.h"

#include "core/foundation/strings/format.h"

namespace nxm::store_steam {

bool SteamWorkshop::publish(const nx::string_view title,
                            const nx::string_view description,
                            const nx::string_view content_folder,
                            const nx::string_view preview_file) {
  ISteamUGC *const ugc = SteamUGC();
  ISteamUtils *const utils = SteamUtils();
  if (ugc == nullptr || utils == nullptr || m_publish_pending)
    return false;
  m_publish_title = nx::string(title);
  m_publish_description = nx::string(description);
  m_publish_content_folder = nx::string(content_folder);
  m_publish_preview_file = nx::string(preview_file);
  m_publish_pending = true;
  m_publish_error.clear();
  m_published_file_id = 0;
  const SteamAPICall_t call =
      ugc->CreateItem(utils->GetAppID(), k_EWorkshopFileTypeCommunity);
  m_create_item_result.Set(call, this, &SteamWorkshop::on_create_item);
  return true;
}

void SteamWorkshop::on_create_item(CreateItemResult_t *const result,
                                   const bool io_failure) {
  if (io_failure || result->m_eResult != k_EResultOK) {
    m_publish_pending = false;
    m_publish_error = io_failure
                          ? nx::string("network error")
                          : nx::format("Steam error {}",
                                       static_cast<int>(result->m_eResult));
    return;
  }
  m_published_file_id = static_cast<u64>(result->m_nPublishedFileId);

  ISteamUGC *const ugc = SteamUGC();
  ISteamUtils *const utils = SteamUtils();
  if (ugc == nullptr || utils == nullptr) {
    m_publish_pending = false;
    m_publish_error = "no Steam UGC interface";
    return;
  }
  const UGCUpdateHandle_t handle =
      ugc->StartItemUpdate(utils->GetAppID(), result->m_nPublishedFileId);
  ugc->SetItemTitle(handle, m_publish_title.c_str());
  ugc->SetItemDescription(handle, m_publish_description.c_str());
  ugc->SetItemContent(handle, m_publish_content_folder.c_str());
  if (!m_publish_preview_file.empty())
    ugc->SetItemPreview(handle, m_publish_preview_file.c_str());
  const SteamAPICall_t call = ugc->SubmitItemUpdate(handle, "");
  m_submit_update_result.Set(call, this, &SteamWorkshop::on_submit_item_update);
}

void SteamWorkshop::on_submit_item_update(SubmitItemUpdateResult_t *const result,
                                          const bool io_failure) {
  m_publish_pending = false;
  if (io_failure || result->m_eResult != k_EResultOK)
    m_publish_error = io_failure
                          ? nx::string("network error")
                          : nx::format("Steam error {}",
                                       static_cast<int>(result->m_eResult));
}

bool SteamWorkshop::subscribe(const u64 file_id) {
  ISteamUGC *const ugc = SteamUGC();
  if (ugc == nullptr)
    return false;
  m_subscribe_pending = true;
  m_subscribe_error.clear();
  const SteamAPICall_t call =
      ugc->SubscribeItem(static_cast<PublishedFileId_t>(file_id));
  m_subscribe_result.Set(call, this, &SteamWorkshop::on_subscribe_result);
  return true;
}

bool SteamWorkshop::unsubscribe(const u64 file_id) {
  ISteamUGC *const ugc = SteamUGC();
  if (ugc == nullptr)
    return false;
  m_subscribe_pending = true;
  m_subscribe_error.clear();
  const SteamAPICall_t call =
      ugc->UnsubscribeItem(static_cast<PublishedFileId_t>(file_id));
  m_subscribe_result.Set(call, this, &SteamWorkshop::on_subscribe_result);
  return true;
}

void SteamWorkshop::on_subscribe_result(
    RemoteStorageSubscribePublishedFileResult_t *const result,
    const bool io_failure) {
  m_subscribe_pending = false;
  if (io_failure || result->m_eResult != k_EResultOK)
    m_subscribe_error = io_failure
                             ? nx::string("network error")
                             : nx::format("Steam error {}",
                                          static_cast<int>(result->m_eResult));
}

void SteamWorkshop::refresh_subscribed() {
  m_subscribed.clear();
  ISteamUGC *const ugc = SteamUGC();
  if (ugc == nullptr)
    return;
  const uint32 count = ugc->GetNumSubscribedItems(true);
  if (count == 0)
    return;
  nx::vector<PublishedFileId_t> ids;
  ids.resize(count);
  ugc->GetSubscribedItems(ids.data(), count, true);
  m_subscribed.reserve(count);
  for (const PublishedFileId_t id : ids)
    m_subscribed.push_back(static_cast<u64>(id));
}

u64 SteamWorkshop::subscribed_id(const usize index) const noexcept {
  return index < m_subscribed.size() ? m_subscribed[index] : 0;
}

bool SteamWorkshop::is_installed(const u64 file_id) const {
  ISteamUGC *const ugc = SteamUGC();
  if (ugc == nullptr)
    return false;
  const uint32 state =
      ugc->GetItemState(static_cast<PublishedFileId_t>(file_id));
  return (state & k_EItemStateInstalled) != 0;
}

nx::string SteamWorkshop::install_path(const u64 file_id) const {
  ISteamUGC *const ugc = SteamUGC();
  if (ugc == nullptr)
    return {};
  uint64 size_on_disk = 0;
  char folder[1024] = {};
  uint32 timestamp = 0;
  if (!ugc->GetItemInstallInfo(static_cast<PublishedFileId_t>(file_id),
                                &size_on_disk, folder,
                                static_cast<uint32>(sizeof(folder)), &timestamp))
    return {};
  return nx::string(folder);
}

bool SteamWorkshop::download(const u64 file_id) const {
  ISteamUGC *const ugc = SteamUGC();
  return ugc != nullptr &&
         ugc->DownloadItem(static_cast<PublishedFileId_t>(file_id), false);
}

std::pair<u64, u64> SteamWorkshop::download_progress(const u64 file_id) const {
  ISteamUGC *const ugc = SteamUGC();
  uint64 downloaded = 0;
  uint64 total = 0;
  if (ugc == nullptr ||
      !ugc->GetItemDownloadInfo(static_cast<PublishedFileId_t>(file_id),
                                &downloaded, &total))
    return {0, 0};
  return {downloaded, total};
}

u64 SteamWorkshop::download_bytes_downloaded(const u64 file_id) const {
  return download_progress(file_id).first;
}

u64 SteamWorkshop::download_bytes_total(const u64 file_id) const {
  return download_progress(file_id).second;
}

}
