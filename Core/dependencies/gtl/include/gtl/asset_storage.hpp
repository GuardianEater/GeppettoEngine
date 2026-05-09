/*********************************************************************
 * @file   asset_storage.hpp
 * @date   2026-05-07
 * @author Travis Gronvold (2018tcg@gmail.com)
 * 
 * @brief  storage specifically used to represent items that can be saved to disk
 *         abstracts away filepaths so only uuids are used.
 *********************************************************************/

#pragma once

// gtl
#include "keyed_vector.hpp"
#include "uuid.hpp"

// std
#include <optional>
#include <unordered_map>
#include <filesystem>

namespace gtl
{
  template <typename AssetType>
  class asset_storage
  {
  public:
    // given a folder determines where a file should be saved and loaded from
    explicit asset_storage(const std::filesystem::path& folder);

    // adds a asset to be tracked inside the asset storage, if the passed uuid already exists: will overwrite it
    uint64_t insert(const AssetType& asset, const gtl::uuid& uuid = gtl::generate_uuid());

    // adds a asset to be tracked inside the asset storage, if the passed uuid already exists: will overwrite it
    uint64_t emplace(AssetType&& asset, const gtl::uuid& uuid = gtl::generate_uuid());

    // removes an asset from the storage
    void erase(const gtl::uuid& uuid);

    // locates an assetIdx by uuid
    std::optional<uint64_t> find(const gtl::uuid& uuid) const;

    // gets an asset at the index returned by insert (const)
    const AssetType& at(uint64_t assetIdx) const;

    // gets an asset at the index returned by insert
    AssetType& at(uint64_t assetIdx);

    // checks whether an asset is loaded
    bool contains(uint64_t assetIdx) const;

    // checks if an asset exists on disk or in memory
    bool exists(const gtl::uuid& uuid) const;

    // given an assets idx saves it to this storages folder with the uuid as a name
    void save(const gtl::uuid& uuid) const;

    // loads the file assocated with the given uuid
    // if the passed uuid already exists will simply behave as find
    std::optional<uint64_t> load(const gtl::uuid& uuid);

    // scans the asset storage working folder and adds all existing file names into mAssets
    void scan();

  private:
    // the existance of this asset_info signals that an assets exists either on disk or in memory
    struct asset_info
    {
      std::optional<uint64_t> idx = std::nullopt; // if this is nullopt then it exists on disk, if not its an idx into mAssets
    };

    std::filesystem::path mFolder; // the folder that this asset storage is responsible for
    gtl::keyed_vector<AssetType> mAssets; // the actual asset
    std::unordered_map<gtl::uuid, asset_info> mAssetInfos; // asset uuid to meta information about that asset
  };
}

#include "asset_storage.inl"