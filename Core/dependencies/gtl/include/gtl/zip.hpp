/*********************************************************************
 * @file   zip.hpp
 * @date   2026-04-04
 * @author Travis Gronvold (2018tcg@gmail.com)
 * 
 * @brief  utility to iterate over multiple containers at once
 *********************************************************************/

namespace gtl
{
  template <typename... Args>
  struct zipped
  {
    struct iterator
    {

    };

    struct const_iterator
    {

    };

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cbegin() const;
  };

  template <typename... Args>
  auto zip(Args&... containers)
  {

  }
} // namespace gtl
