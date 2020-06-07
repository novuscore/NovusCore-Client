#pragma once
#include <NovusTypes.h>
#include <glm/matrix.hpp>
#include <glm/gtx/rotate_vector.hpp>

struct Transform
{
    vec3 position;
    vec3 rotation;
    vec3 scale;
    bool isDirty = false;
    
    mat4x4 GetMatrix()
    {
        // When we pass 1 into the constructor, it will construct an identity matrix
        mat4x4 matrix(1);

        // Order is important here (Go lookup how matrices work if confused)
        // Gamedev.net suggest (Rotate, Scale and Translate) this could be wrong
        matrix = glm::rotate(matrix, rotation.x, vec3(1, 0, 0));
        matrix = glm::rotate(matrix, rotation.y, vec3(0, 1, 0));
        matrix = glm::rotate(matrix, rotation.z, vec3(0, 0, 1));
        matrix = glm::scale(matrix, scale);
        matrix = glm::translate(matrix, position);

        return matrix;
    }
};