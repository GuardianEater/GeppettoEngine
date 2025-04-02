/*****************************************************************//**
 * \file   BVHTree.hpp
 * \brief  bounding volume hierarchy tree
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   April 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "Shapes.hpp"
#include "CollisionChecking.hpp"
#include <vector>
#include <unordered_map>

namespace Gep
{
    template <typename KeyType, typename ValueType>
    class bvh_tree
    {
    public:
        void insert(const KeyType& key, const ValueType& userData, const AABB& aabb);
        void erase(const KeyType& key);
        void clear();
        std::vector<ValueType*> cast_ray(const Ray& ray);
        std::vector<ValueType*> cast_frustum(const Frustum& frustum);

    private:
        struct bvh_node
        {
            ValueType userData;
            AABB aabb{};
            bvh_node* parent = nullptr;
            bvh_node* left = nullptr;
            bvh_node* right = nullptr;
            size_t lastAxis = 0;
            size_t height = 0;

            bvh_node* small_child();
            bool is_right();
        };

        std::unordered_map<KeyType, bvh_node*> mNodeMap;
        bvh_node* mRoot = nullptr;

        bvh_node* find_partner(bvh_node* newNode);
        bool is_leaf(bvh_node* node);
        void recalculate_nodes(bvh_node* node);
        void balance_node(bvh_node* node);
        void deallocate_node_recursive(bvh_node* node);
        void deallocate_node(bvh_node* node);
        void cast_ray_recursive(const Ray& ray, bvh_node* node, std::vector<ValueType*>& results);
        void cast_frustum_recursive(const Frustum& frustum, bvh_node* node, std::vector<ValueType*>& results);
        void cast_frustum_recursive_all(bvh_node* node, std::vector<ValueType*>& results);
    };
}

#include "BVHTree.inl"
