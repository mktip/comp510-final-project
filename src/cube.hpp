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

inline void update_shader_lighting_parameters(color4 material_ambient,
                                              color4 material_diffuse,
                                              color4 material_specular,
                                              GLfloat material_shininess,
                                              Light &light, GLuint program) {
  // Initialize shader lighting parameters

  color4 ambient_product = light.get_ambient() * material_ambient;
  color4 diffuse_product = light.get_diffuse() * material_diffuse;
  color4 specular_product = light.get_specular() * material_specular;

  glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1,
               ambient_product);
  glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1,
               diffuse_product);
  glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1,
               specular_product);
  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
               light.get_position());

  glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);
}

class Cube {
public:
  point4 initial_position;
  vector<point4> points;
  vector<vec3> normals;
  vector<point4> vertices;
  vector<color4> materials;
  mat4 model_view;
  GLuint vao;
  GLuint buffer;

  GLuint trig_indices[36] = {1, 0, 3, 1, 3, 2, 2, 3, 7, 2, 7, 6,
                             3, 0, 4, 3, 4, 7, 6, 5, 1, 6, 1, 2,
                             4, 5, 6, 4, 6, 7, 5, 4, 0, 5, 0, 1};
  Cube()
      : initial_position(0.0, 0.0, 0.0, 1.0), points(8), normals(0),
        vertices(0), materials(0), model_view(), vao(), buffer() {}

  Cube(GLfloat edge_length, const point4 &_initial_position,
       vector<color4> face_colors)
      : initial_position(_initial_position), points(8), normals(0), vertices(0),
        materials(0), model_view(), vao(), buffer() {

    // Initialize the main_rubix_cube
    model_view = mat4();
    points[0] = point4(-edge_length, -edge_length, edge_length, 1.0);
    points[1] = point4(-edge_length, edge_length, edge_length, 1.0);
    points[2] = point4(edge_length, edge_length, edge_length, 1.0);
    points[3] = point4(edge_length, -edge_length, edge_length, 1.0);
    points[4] = point4(-edge_length, -edge_length, -edge_length, 1.0);
    points[5] = point4(-edge_length, edge_length, -edge_length, 1.0);
    points[6] = point4(edge_length, edge_length, -edge_length, 1.0);
    points[7] = point4(edge_length, -edge_length, -edge_length, 1.0);

    // Initialize the polygons
    vec3 current_normal = 0;
    for (int i = 35; i > -1; i--) {
      vertices.emplace_back(points[trig_indices[i]]);
    }

    for (unsigned int i = 0; i < 36; i++) {
      if (i % 6 == 0) {
        vec4 u = vertices[i + 1] - vertices[i];
        vec4 v = vertices[i + 2] - vertices[i + 1];
        current_normal = normalize(cross(u, v));
      }

      normals.emplace_back(current_normal);
    }

    mat4 translation =
        Translate(initial_position.x, initial_position.y, initial_position.z);
    model_view = model_view * translation;

    for (size_t i = 0; i < vertices.size() / 6; i++) {
      materials.emplace_back(face_colors[i] * 0.2);
      materials.emplace_back(face_colors[i] * 0.6);
      materials.emplace_back(face_colors[i] * 0.8);
    }
  }

  void load(GLuint program) {

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(point4) +
                     normals.size() * sizeof(vec3),
                 NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(point4),
                    vertices.data());

    glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(point4),
                    normals.size() * sizeof(vec3), normals.data());

    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    GLuint vNormal = glGetAttribLocation(program, "vNormal");
    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(point4) * vertices.size()));
  }

  void draw(GLuint program, Light &light) {

    glBindVertexArray(vao);
    GLuint ModelView = glGetUniformLocation(program, "ModelView");
      model_view = RotateY(0.1)*model_view ;
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    // Draw the main_rubix_cube
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    for (int i = 0; i < 6; i++) {

      update_shader_lighting_parameters(materials[i * 3], materials[i * 3 + 1],
                                        materials[i * 3 + 2], 200.0f, light,
                                        program);
      glDrawArrays(GL_TRIANGLES, i * 6, 6);
    }

    // glDrawArrays(GL_TRIANGLES, 0, 36);

    glFlush();
  }

  void rotateX(GLfloat theta) {
    mat4 rotation = RotateX(theta);
    model_view = rotation * model_view;
  }

  void rotateY(GLfloat theta) {
    mat4 rotation = RotateY(theta);
    model_view = rotation * model_view;
  }

  void rotateZ(GLfloat theta) {
    mat4 rotation = RotateZ(theta);
    model_view = rotation * model_view;
  }

  void translate(GLfloat x, GLfloat y, GLfloat z) {
    // Translate the main_rubix_cube
    mat4 translation = Translate(x, y, z);
    model_view = translation * model_view;
  }
};
