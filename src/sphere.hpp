#include "Angel.h"
#include "light.hpp"
#include <cmath>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace std;

typedef vec4 color4;
typedef vec4 point4;

// Sphere constants
#define SECTOR_COUNT 50
#define STACK_COUNT 50

// Object size modifier

// The number of vertices in each object
const int NumSphereVertices = SECTOR_COUNT * STACK_COUNT * 6;

inline void
update_shader_lighting_parameters_sphere(color4 material_ambient, color4 material_diffuse, color4 material_specular,
                                         GLfloat material_shininess, Light light, GLuint program) {
    // Initialize shader lighting parameters

    color4 ambient_product = light.get_ambient() * material_ambient;
    color4 diffuse_product = light.get_diffuse() * material_diffuse;
    color4 specular_product = light.get_specular() * material_specular;

    glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, ambient_product);
    glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse_product);
    glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, specular_product);
    glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1, light.get_position());

    glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);
}

class Sphere {
    point4 sphere_points[NumSphereVertices];
    vec3 sphere_normals[NumSphereVertices];
    vec2 sphere_tex_2d_coordinates[NumSphereVertices];
    GLfloat sphere_tex_1d_coordinates[NumSphereVertices];
    GLuint vao;
    GLuint sphere_buffer;
    vec3 displacement;
    GLfloat scalar = 1.0f;
    GLfloat rotate = 0.0f;
    vec3 velocity = vec3(0.5, 0.0, 0.0);
    vec3 start_velocity = vec3(0.5, 0.0, 0.0);
    vec3 start_displacement = vec3(-3, 3, -11);
    GLfloat y_acceleration = -9.8f;
    GLfloat dt = 0.01f;
    GLfloat radius = 0.5;
    vector<color4> materials;
    GLfloat Shininess;
    GLboolean directionX;
    GLboolean directionY;
    GLboolean directionZ;
    mat4 modelview;

public:
    Sphere(GLfloat r, vec3 start_displacement, vector<color4> m, GLfloat s) {
        radius = r;
        materials = m;
        Shininess = s;
        create_sphere();
        displacement = start_displacement;
        directionX = true;
        directionY = false;
        directionZ = false;
        modelview = Translate(start_displacement);
    }

    void draw(GLuint program, Light &light) {
        update_shader_lighting_parameters_sphere(materials[0], materials[1], materials[2], Shininess, light, program);

        glBindVertexArray(vao);

        GLuint ModelView = glGetUniformLocation(program, "ModelView");

        glUniformMatrix4fv(ModelView, 1, GL_TRUE, modelview);
        glPolygonMode(GL_FRONT, GL_FILL);
        glDrawArrays(GL_TRIANGLES, 0, NumSphereVertices);
    }

    void load(GLuint program) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &sphere_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);

        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(sphere_points) + sizeof(sphere_normals) + sizeof(sphere_tex_2d_coordinates) +
                     sizeof(sphere_tex_1d_coordinates), nullptr, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphere_points), sphere_points);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphere_points), sizeof(sphere_normals), sphere_normals);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphere_points) + sizeof(sphere_normals),
                        sizeof(sphere_tex_2d_coordinates), sphere_tex_2d_coordinates);
        glBufferSubData(GL_ARRAY_BUFFER,
                        sizeof(sphere_points) + sizeof(sphere_normals) + sizeof(sphere_tex_2d_coordinates),
                        sizeof(sphere_tex_1d_coordinates), sphere_tex_1d_coordinates);

        // set up vertex arrays
        GLuint vPosition = glGetAttribLocation(program, "vPosition");
        glEnableVertexAttribArray(vPosition);
        glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

        GLuint vNormal = glGetAttribLocation(program, "vNormal");
        glEnableVertexAttribArray(vNormal);
        glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(sphere_points)));

        GLuint vTexCoord2D = glGetAttribLocation(program, "vTexCoord2D");
        glEnableVertexAttribArray(vTexCoord2D);
        glVertexAttribPointer(vTexCoord2D, 2, GL_FLOAT, GL_FALSE, 0,
                              BUFFER_OFFSET(sizeof(sphere_points) + sizeof(sphere_normals)));

        GLuint vTexCoord1D = glGetAttribLocation(program, "vTexCoord1D");
        glEnableVertexAttribArray(vTexCoord1D);
        glVertexAttribPointer(vTexCoord1D, 1, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(
                sizeof(sphere_points) + sizeof(sphere_normals) + sizeof(sphere_tex_2d_coordinates)));
    }

    void create_sphere() {
        std::vector<vec4> sphere_vertices;
        std::vector<vec3> tmp_sphere_normals;
        std::vector<vec2> tmp_sphere_tex_2d_coords;
        std::vector<GLfloat> tmp_sphere_tex_1d_coords;

        float x, y, z, xz; // vertex position

        float sectorStep = 2 * M_PI / SECTOR_COUNT;
        float stackStep = M_PI / STACK_COUNT;
        float sectorAngle, stackAngle;

        for (int i = 0; i <= STACK_COUNT; ++i) {

            stackAngle = M_PI / 2 - (float) i * stackStep; // starting from pi/2 to -pi/2
            xz = radius * cosf(stackAngle);      // r * cos(u)
            y = radius * sinf(stackAngle);       // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but
            // different tex coords
            for (int j = 0; j <= SECTOR_COUNT; ++j) {
                sectorAngle = (float) j * sectorStep; // starting from 0 to 2pi

                // vertex position (x, y, z)
                z = xz * cosf(sectorAngle); // r * cos(u) * cos(v)
                x = xz * sinf(sectorAngle); // r * cos(u) * sin(v)
                sphere_vertices.emplace_back(x, y, z, 1.0);
                tmp_sphere_normals.emplace_back(x, y, z);
                tmp_sphere_tex_2d_coords.emplace_back((float) j / SECTOR_COUNT, (float) i / STACK_COUNT);
                tmp_sphere_tex_1d_coords.emplace_back((float) j / SECTOR_COUNT);
            }
        }

        std::vector<int> sphere_indices;
        int k1, k2;
        for (int i = 0; i < STACK_COUNT; ++i) {
            k1 = i * (SECTOR_COUNT + 1); // beginning of current stack
            k2 = k1 + SECTOR_COUNT + 1;  // beginning of next stack

            for (int j = 0; j < SECTOR_COUNT; ++j, ++k1, ++k2) {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1+1
                if (i != 0) {
                    sphere_indices.push_back(k1);
                    sphere_indices.push_back(k2);
                    sphere_indices.push_back(k1 + 1);
                }

                // k1+1 => k2 => k2+1
                if (i != (STACK_COUNT - 1)) {
                    sphere_indices.push_back(k1 + 1);
                    sphere_indices.push_back(k2);
                    sphere_indices.push_back(k2 + 1);
                }
            }
        }

        for (int i = 0; i < sphere_indices.size(); i++) {
            sphere_points[i] = sphere_vertices[sphere_indices[i]];
            sphere_normals[i] = tmp_sphere_normals[sphere_indices[i]];
            sphere_tex_2d_coordinates[i] = tmp_sphere_tex_2d_coords[sphere_indices[i]];
            sphere_tex_1d_coordinates[i] = tmp_sphere_tex_1d_coords[sphere_indices[i]];
        }
    }

    void move() {
        //displacement.y =
        //        displacement.y + velocity.y * dt + 0.5f * y_acceleration * dt * dt;
        //displacement.x = displacement.x + velocity.x * dt;

        // keep the cube moving horizontally
        //velocity.y = velocity.y + y_acceleration * dt;

        //const float floor = -0.5;
        //if (displacement.y < scalar * radius + floor) {
        // displacement.y = (GLfloat)std::tan(29.0 / 2 * DegreesToRadians) *
        //                      start_displacement.z +
        //                  scalar * radius + floor;

        //    displacement.y = scalar * radius + floor;
        //    velocity.y = -velocity.y * 0.75f;
        //    velocity.x = velocity.x * 0.85f;
        //}
        rotate+=1;
        vec3 displacement_inc=vec3();

        if (displacement.x <= -15 + radius) {
            directionX = true;
            displacement_inc.x=0.04;

        } else if (displacement.x >= 15 - radius) {
            directionX = false;
            displacement_inc.x=-0.04;
        }

        if (directionX) {
            displacement.x += 0.04;
        } else {
            displacement.x -= 0.04;
        }

        if (displacement.y < -15 + radius) {
            directionY = true;
            displacement_inc.y=0.04;
        } else if (displacement.y > 15 - radius) {
            directionY = false;
            displacement_inc.y=0.04;
        }

        if (directionY) {
            displacement.y += 0.02;
        } else {
            displacement.y -= 0.02;
        }

        //  Generate the model-view matrix
        modelview = RotateY(0.1) *modelview*Translate(displacement_inc)*RotateY(-1);
    }

};
