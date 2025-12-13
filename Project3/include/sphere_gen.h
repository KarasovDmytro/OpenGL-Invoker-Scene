#ifndef SPHERE_H
#define SPHERE_H

#include <vector>
#include <glm/glm.hpp>
#include "mesh.h"

class SphereGenerator {
public:
    static Mesh createSphere(float radius = 1.0f, unsigned int sectors = 36, unsigned int stacks = 18) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        float x, y, z, xy;
        float nx, ny, nz, lengthInv = 1.0f / radius;
        float s, t;

        const float PI = 3.14159265359f;
        float sectorStep = 2 * PI / sectors;
        float stackStep = PI / stacks;
        float sectorAngle, stackAngle;

        for (unsigned int i = 0; i <= stacks; ++i) {
            stackAngle = PI / 2 - i * stackStep;
            xy = radius * cosf(stackAngle);
            z = radius * sinf(stackAngle);

            for (unsigned int j = 0; j <= sectors; ++j) {
                sectorAngle = j * sectorStep;

                Vertex vertex;

                x = xy * cosf(sectorAngle);
                y = xy * sinf(sectorAngle);
                vertex.Position = glm::vec3(x, z, y);

                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
                vertex.Normal = glm::vec3(nx, nz, ny);

                s = (float)j / sectors;
                t = (float)i / stacks;
                vertex.TexCoords = glm::vec2(s, t);

                vertices.push_back(vertex);
            }
        }

        // k1--k1+1
        // |  / |
        // | /  |
        // k2--k2+1
        unsigned int k1, k2;
        for (unsigned int i = 0; i < stacks; ++i) {
            k1 = i * (sectors + 1);
            k2 = k1 + sectors + 1;

            for (unsigned int j = 0; j < sectors; ++j, ++k1, ++k2) {

                if (i != 0) {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                if (i != (stacks - 1)) {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }

        return Mesh(vertices, indices, textures);
    }
};
#endif