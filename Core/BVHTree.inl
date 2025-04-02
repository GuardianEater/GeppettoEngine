/*****************************************************************//**
 * \file   BVHTree.inl
 * \brief  template implementation for BVHTree.hpp
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   April 2025
 *********************************************************************/

#pragma once

#include "BVHTree.hpp"

namespace Gep
{
    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::insert(const KeyType& key, const ValueType& userData, const AABB& aabb)
    {
        bvh_node* newNode = new bvh_node();
        newNode->userData = userData;
        newNode->aabb = aabb;
        mNodeMap[key] = newNode;

        if (mRoot == nullptr)
        {
            mRoot = newNode;
            return;
        }

        bvh_node* partner = find_partner(newNode);
        bvh_node* newParent = new bvh_node();

        if (partner->parent)
        {
            if (partner->parent->left == partner)
                partner->parent->left = newParent;
            else
                partner->parent->right = newParent;

            newParent->parent = partner->parent;
        }
        else
        {
            mRoot = newParent;
        }

        newParent->left = partner;
        newParent->right = newNode;

        partner->parent = newParent;
        newNode->parent = newParent;

        recalculate_nodes(newNode);
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::erase(const KeyType& key)
    {
        bvh_node* deleteNode = mNodeMap.at(key);
        bvh_node* parent = deleteNode->parent;

        if (parent)
        {
            bvh_node* sibling = parent->left == deleteNode ? parent->right : parent->left;
            bvh_node* grandParent = parent->parent;

            if (grandParent)
            {
                if (grandParent->left == parent)
                    grandParent->left = sibling;
                else
                    grandParent->right = sibling;

                sibling->parent = grandParent;
            }
            else
            {
                mRoot = sibling;
                sibling->parent = nullptr;
            }

            deallocate_node(parent);
            deallocate_node(deleteNode);
            recalculate_nodes(sibling);
        }
        else
        {
            deallocate_node(deleteNode);
            mRoot = nullptr;
        }
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::clear()
    {
        deallocate_node_recursive(mRoot);
        mRoot = nullptr;
    }

    template <typename KeyType, typename ValueType>
    std::vector<ValueType*> bvh_tree<KeyType, ValueType>::cast_ray(const Ray& ray)
    {
        std::vector<ValueType*> results;
        cast_ray_recursive(ray, mRoot, results);
        return results;
    }

    template <typename KeyType, typename ValueType>
    std::vector<ValueType*> bvh_tree<KeyType, ValueType>::cast_frustum(const Frustum& frustum)
    {
        std::vector<ValueType*> results;
        cast_frustum_recursive(frustum, mRoot, results);
        return results;
    }

    template <typename KeyType, typename ValueType>
    typename bvh_tree<KeyType, ValueType>::bvh_node* bvh_tree<KeyType, ValueType>::bvh_node::small_child()
    {
        if (!left || !right) return nullptr;
        return left->height < right->height ? left : right;
    }

    template <typename KeyType, typename ValueType>
    bool bvh_tree<KeyType, ValueType>::bvh_node::is_right()
    {
        if (!parent) return false;
        return parent->right == this;
    }

    template <typename KeyType, typename ValueType>
    typename bvh_tree<KeyType, ValueType>::bvh_node* bvh_tree<KeyType, ValueType>::find_partner(bvh_node* newNode)
    {
        bvh_node* currentNode = mRoot;

        while (!is_leaf(currentNode))
        {
            bvh_node* left = currentNode->left;
            bvh_node* right = currentNode->right;

            AABB& leftAabb = left->aabb;
            AABB& rightAabb = right->aabb;
            AABB& newAabb = newNode->aabb;

            float leftSA = leftAabb.GetSurfaceArea();
            float rightSA = rightAabb.GetSurfaceArea();

            float leftDeltaSA = AABB::Combine(leftAabb, newAabb).GetSurfaceArea() - leftSA;
            float rightDeltaSA = AABB::Combine(rightAabb, newAabb).GetSurfaceArea() - rightSA;

            if (leftDeltaSA < rightDeltaSA)
                currentNode = left;
            else
                currentNode = right;
        }

        return currentNode;
    }

    template <typename KeyType, typename ValueType>
    bool bvh_tree<KeyType, ValueType>::is_leaf(bvh_node* node)
    {
        return node->left == nullptr && node->right == nullptr;
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::recalculate_nodes(bvh_node* node)
    {
        bvh_node* currentNode = node->parent;
        while (currentNode)
        {
            currentNode->height = 1 + std::max(currentNode->left->height, currentNode->right->height);
            currentNode->aabb = AABB::Combine(currentNode->left->aabb, currentNode->right->aabb);
            currentNode = currentNode->parent;
        }
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::balance_node(bvh_node* node)
    {
        if (!node || !node->left || !node->right) return;

        int balance = node->right->height - node->left->height;

        bvh_node* pivot = nullptr;
        if (balance > 1) // right heavy
        {
            pivot = node->right;
            node->right = nullptr;
        }
        else if (balance < -1) // left heavy
        {
            pivot = node->left;
            node->left = nullptr;
        }
        else
        {
            return;
        }

        bvh_node* smallChild = pivot->small_child();
        bool smallChildIsRight = smallChild->is_right();

        pivot->parent = nullptr;

        if (smallChildIsRight)
            pivot->right = nullptr;
        else
            pivot->left = nullptr;

        smallChild->parent = nullptr;

        bvh_node* grandparent = node->parent;
        if (grandparent)
        {
            if (grandparent->left == node)
                grandparent->left = pivot;
            else
                grandparent->right = pivot;

            pivot->parent = grandparent;
        }
        else
        {
            mRoot = pivot;
        }

        node->parent = nullptr;

        if (smallChildIsRight)
        {
            pivot->right = node;
        }
        else
        {
            pivot->left = node;
        }

        node->parent = pivot;

        if (!node->left)
            node->left = smallChild;
        else
            node->right = smallChild;

        smallChild->parent = node;

        node->height = 1 + std::max(node->left->height, node->right->height);
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::deallocate_node_recursive(bvh_node* node)
    {
        if (!node) return;
        deallocate_node_recursive(node->left);
        deallocate_node_recursive(node->right);
        deallocate_node(node);
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::deallocate_node(bvh_node* node)
    {
        if (!node) return;

        if (node->left)
            node->left->parent = nullptr;
        if (node->right)
            node->right->parent = nullptr;

        if (node->parent)
        {
            if (node->is_right())
                node->parent->right = nullptr;
            else
                node->parent->left = nullptr;
        }
        delete node;
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::cast_ray_recursive(const Ray& ray, bvh_node* node, std::vector<ValueType*>& results)
    {
        if (!node) return;

        float t{};
        if (!RayAABB(ray, node->aabb, t)) return;

        if (is_leaf(node))
        {
            results.push_back(&node->userData);
        }
        else
        {
            cast_ray_recursive(ray, node->left, results);
            cast_ray_recursive(ray, node->right, results);
        }
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::cast_frustum_recursive(const Frustum& frustum, bvh_node* node, std::vector<ValueType*>& results)
    {
        if (!node) return;

        FrustumIntersectionType intersection = FrustumAABB(frustum, node->aabb, node->lastAxis);

        if (intersection == FrustumIntersectionType::Outside) return;

        if (intersection == FrustumIntersectionType::Inside)
        {
            cast_frustum_recursive_all(node, results);
            return;
        }

        if (is_leaf(node))
        {
            results.push_back(&node->userData);
        }
        else
        {
            cast_frustum_recursive(frustum, node->left, results);
            cast_frustum_recursive(frustum, node->right, results);
        }
    }

    template <typename KeyType, typename ValueType>
    void bvh_tree<KeyType, ValueType>::cast_frustum_recursive_all(bvh_node* node, std::vector<ValueType*>& results)
    {
        if (!node) return;

        if (is_leaf(node))
        {
            results.push_back(&node->userData);
        }
        else
        {
            cast_frustum_recursive_all(node->left, results);
            cast_frustum_recursive_all(node->right, results);
        }
    }
}

