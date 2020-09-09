#pragma once
#include <NovusTypes.h>
#include <glm/matrix.hpp>
#include <glm/gtx/rotate_vector.hpp>

enum class MovementFlags : u32
{
    NONE = 1 << 0,
    FORWARD = 1 << 1,
    BACKWARD = 1 << 2,
    LEFT = 1 << 3,
    RIGHT = 1 << 4,
    GROUNDED = 1 << 5, // Special Flag to check if we are grounded

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

struct MovementData
{
    MovementFlags flags;
    f32 speed = 7.1111f;

    inline void AddMoveFlag(MovementFlags flag)
    {
        flags |= flag;
    }
    inline void RemoveMoveFlag(MovementFlags flag)
    {
        flags &= ~flag;
    }
    inline bool HasMoveFlag(MovementFlags flag)
    {
        return (flags & flag) == flag;
    }
};

struct Transform
{
    vec3 position = vec3(0, 0, 0);
    vec3 velocityDirection = vec3(0, 0, 0);
    vec3 velocity = vec3(0, 0, 0);
    vec3 scale = vec3(1, 1, 1);
    
    // Rotation
    mat4x4 rotationMatrix;
    f32 yaw;
    f32 pitch;

    // Direction vectors
    vec3 front = vec3(0, 0, 0);
    vec3 up = vec3(0, 0, 0);
    vec3 left = vec3(0, 0, 0);

    MovementData movementData;
    bool isDirty = true;

    vec3 GetRotation() const { return vec3(0, yaw, pitch); }
    mat4x4 GetMatrix()
    {
        // When we pass 1 into the constructor, it will construct an identity matrix
        mat4x4 matrix(1);

        // Order is important here (Go lookup how matrices work if confused)
        // Gamedev.net suggest (Rotate, Scale and Translate) this could be wrong
        // Experience tells me (Translate and then scale or rotate in either order)
        matrix = glm::translate(matrix, position);
        matrix *= rotationMatrix;
        matrix = glm::scale(matrix, scale);

        return matrix;
    }

    void UpdateVectors()
    {
        left = rotationMatrix[0];
        up = rotationMatrix[1];
        front = -rotationMatrix[2];
    }
};