/*********************************************************************
 * @file   asset_storage.inl
 * @date   2026-05-08
 * @author Travis Gronvold (2018tcg@gmail.com)
 * 
 * @brief  
 *********************************************************************/

#pragma once

// gtl
#include "binary_buffer.hpp"

// std
#include <optional>
#include <fstream>

// this
#include "asset_storage.hpp"

namespace gtl
{
  template <typename AssetType>
  asset_storage<AssetType>::asset_storage(const std::filesystem::path &folder)
    : mFolder(folder)
  {}

  template <typename AssetType>
  uint64_t asset_storage<AssetType>::insert(const AssetType &type, const gtl::uuid &uuid)
  {
    auto idx = find(uuid);
    if (idx)
    {
      at(*idx) = type;
      return *idx;
    }

    asset_info& info = mAssetInfos[uuid];
    info.idx = mAssets.emplace(type);
    return *info.idx;
  }

  template <typename AssetType>
  uint64_t asset_storage<AssetType>::emplace(AssetType&& asset, const gtl::uuid &uuid)
  {
    auto idx = find(uuid);
    if (idx)
    {
      at(*idx) = asset;
      return *idx;
    }

    asset_info& info = mAssetInfos[uuid];
    info.idx = mAssets.emplace(asset);
    return *info.idx;
  }

  template <typename AssetType>
  void asset_storage<AssetType>::erase(const gtl::uuid& uuid)
  {
    auto idx = find(uuid);
    if (!idx)
      return;

    auto path = mFolder / (uuid.to_string() + ".gtlbin");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    mAssetInfos.erase(uuid);
    mAssets.erase(*idx);
  }

  template <typename AssetType>
  std::optional<uint64_t> asset_storage<AssetType>::find(const gtl::uuid &uuid) const
  {
    auto it = mAssetInfos.find(uuid);
    if (it == mAssetInfos.end())
      return std::nullopt;

    const asset_info& info = it->second;

    return info.idx;
  }

  template <typename AssetType>
  const AssetType &asset_storage<AssetType>::at(uint64_t assetIdx) const
  {
    return mAssets.at(assetIdx);
  }

  template <typename AssetType>
  AssetType &asset_storage<AssetType>::at(uint64_t assetIdx)
  {
    return mAssets.at(assetIdx);
  }

  template <typename AssetType>
  bool asset_storage<AssetType>::contains(uint64_t assetIdx) const
  {
    return mAssets.contains(assetIdx);
  }

  template <typename AssetType>
  bool asset_storage<AssetType>::exists(const gtl::uuid &uuid) const
  {
    return mAssetInfos.contains(uuid);
  }

  template <typename AssetType>
  void asset_storage<AssetType>::save(const gtl::uuid& uuid) const
  {
    auto idx = find(uuid);

    if (!idx)
      return;

    const AssetType& asset = at(*idx);
    
    gtl::binary_buffer bin;
    bin.add(asset);

    auto path = mFolder / (uuid.to_string() + ".gtlbin");

    std::filesystem::create_directories(mFolder);

    std::ofstream file{ path , std::ios::binary };
    file << bin;
  }

  template <typename AssetType>
  std::optional<uint64_t> asset_storage<AssetType>::load(const gtl::uuid &uuid)
  {
    auto idx = find(uuid);

    if (idx)
      return idx;
    
    auto path = mFolder / (uuid.to_string() + ".gtlbin");
    
    if (!std::filesystem::exists(path))
      return std::nullopt;

    std::ifstream file{ path , std::ios::binary };
    if (!file.is_open())
      return std::nullopt;
    
    AssetType asset;
    gtl::binary_buffer bin;
    file >> bin;
    bin.get(asset);

    return insert(asset, uuid);
  }

  template <typename AssetType>
  void asset_storage<AssetType>::scan()
  {
    if (!std::filesystem::exists(mFolder))
      return;

    for (const auto it : std::filesystem::directory_iterator(mFolder))
    {
      // if it doesnt have an extension or the extenion isn't gtlbin
      if (!it.path().has_extension() || it.path().extension() != ".gtlbin")
        continue;

      const auto uuidStr = it.path().stem().string();
      const auto uuid = gtl::to_uuid(uuidStr);

      // default construct as nullopt if it doesnt exist
      // otherwise does nothing
      mAssetInfos[uuid]; 
    }
  }
}