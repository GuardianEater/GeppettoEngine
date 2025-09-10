/*********************************************************************
 * file:   Quaternion.hpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   September 5, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  implementation for a quaternion structure
 *********************************************************************/

namespace Gep
{
    struct Quat
    {
        friend Quat operator+(const Quat& q0, const Quat& q1);

        float x, y, z, s;
    };


}
