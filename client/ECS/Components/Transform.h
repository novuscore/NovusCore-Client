#pragma once
#include <NovusTypes.h>
#include <glm/matrix.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>

struct Transform
{
    vec3 position = vec3(0.f, 0.f, 0.f);
    vec3 velocity = vec3(0.f, 0.f, 0.f);
    vec3 scale = vec3(1.f, 1.f, 1.f);

    f32 moveSpeed = 7.1111f;
    f32 fallSpeed = 19.5f;
    f32 fallAcceleration = 7.33f;

    struct MovementFlags
    {
        union
        {
            struct
            {
                u8 FORWARD : 1;
                u8 BACKWARD : 1;
                u8 LEFT : 1;
                u8 RIGHT : 1;
                u8 GROUNDED : 1;
            };

            u16 value;
        };

    } movementFlags;

    // Rotation
    mat4x4 rotationMatrix;
    f32 yaw;
    f32 pitch;

    // Direction vectors
    vec3 front = vec3(0.f, 0.f, 0.f);
    vec3 up = vec3(0.f, 0.f, 0.f);
    vec3 left = vec3(0.f, 0.f, 0.f);

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

    void UpdateRotationMatrix()
    {
        mat4x4 offsetPitchMatrix = glm::yawPitchRoll(0.0f, glm::radians(90.0f), 0.0f);
        mat4x4 offsetYawMatrix = glm::yawPitchRoll(glm::radians(-90.0f), 0.0f, 0.0f);

        rotationMatrix = offsetPitchMatrix * offsetYawMatrix * glm::yawPitchRoll(glm::radians(yaw), glm::radians(pitch), 0.0f);
        UpdateVectors();
    }

    void UpdateVectors()
    {
        left = -rotationMatrix[0];
        up = rotationMatrix[1];
        front = -rotationMatrix[2];
    }
};