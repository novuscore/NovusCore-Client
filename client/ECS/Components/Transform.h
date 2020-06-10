#pragma once
#include <NovusTypes.h>
#include <glm/matrix.hpp>
#include <glm/gtx/rotate_vector.hpp>

enum class MovementFlags : u32
{
    NONE,
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,

    VERTICAL = FORWARD | BACKWARD,
    HORIZONTAL = LEFT | RIGHT,
    ALL = HORIZONTAL | VERTICAL
};

inline MovementFlags operator &(MovementFlags lhs, MovementFlags rhs)
{
    return static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) &
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );
}
inline MovementFlags operator ^(MovementFlags lhs, MovementFlags rhs)
{
    return static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) ^
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );
}
inline MovementFlags operator ~(MovementFlags rhs)
{
    return static_cast<MovementFlags> (
        ~static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );
}
inline MovementFlags& operator |=(MovementFlags& lhs, MovementFlags rhs)
{
    lhs = static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) |
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );

    return lhs;
}
inline MovementFlags& operator &=(MovementFlags& lhs, MovementFlags rhs)
{
    lhs = static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) &
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );

    return lhs;
}
inline MovementFlags& operator ^=(MovementFlags& lhs, MovementFlags rhs)
{
    lhs = static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) ^
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );

    return lhs;
}

struct Transform
{
    vec3 position;
    vec3 rotation;
    vec3 scale = vec3(1, 1, 1);

    MovementFlags moveFlags;
    inline void AddMoveFlag(MovementFlags flag)
    {
        moveFlags |= flag;
    }
    inline void RemoveMoveFlag(MovementFlags flag)
    {
        moveFlags &= ~flag;
    }
    inline bool HasMoveFlag(MovementFlags flag)
    {
        return (moveFlags & flag) == flag;
    }

    bool isDirty = true;
    
    mat4x4 GetMatrix()
    {
        // When we pass 1 into the constructor, it will construct an identity matrix
        mat4x4 matrix(1);

        // Order is important here (Go lookup how matrices work if confused)
        // Gamedev.net suggest (Rotate, Scale and Translate) this could be wrong
        // Experience tells me (Translate and then scale or rotate in either order)
        matrix = glm::translate(matrix, position);
        matrix = glm::rotate(matrix, Math::DegToRad(rotation.x), vec3(1, 0, 0));
        matrix = glm::rotate(matrix, Math::DegToRad(rotation.y), vec3(0, 1, 0));
        matrix = glm::rotate(matrix, Math::DegToRad(rotation.z), vec3(0, 0, 1));
        matrix = glm::scale(matrix, scale);

        return matrix;
    }
};