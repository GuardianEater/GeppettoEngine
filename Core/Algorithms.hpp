/*****************************************************************//**
 * \file   Algorithms.hpp
 * \brief  various utility functions
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   October 2025
 *********************************************************************/

#pragma once

#include <algorithm>
#include <vector>
#include <numeric>

namespace Gep
{
    // given a vector to sort, sorts both
    template <typename Iter1, typename Iter2, typename Compare>
    void SortParallel(Iter1 first1, Iter1 last1, Iter2 first2, Compare comp)
    {
        size_t n = std::distance(first1, last1);

        // build index indirection
        std::vector<size_t> indices(n);
        std::iota(indices.begin(), indices.end(), 0);

        // sort indices by comparing first range
        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) 
        {
            return comp(*(first1 + a), *(first1 + b));
        });

        // apply permutation to both ranges
        auto apply_permutation = [&](auto iter) 
        {
            using T = typename std::iterator_traits<decltype(iter)>::value_type; // get the type of the iterator

            std::vector<T> tmp(n);
            for (size_t i = 0; i < n; ++i)
                tmp[i] = std::move(*(iter + indices[i]));

            for (size_t i = 0; i < n; ++i)
                *(iter + i) = std::move(tmp[i]);
        };

        apply_permutation(first1);
        apply_permutation(first2);
    }
}
