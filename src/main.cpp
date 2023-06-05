//
// TODOS:
// - [X] Create ground (phase-1)
// - [X] Create wall   (phase-1)
// - [ ] Cast the shadow of a static object on the wall/ground (phase-2)
// - [ ] Cast the shadow of a dynamic object on the wall/ground (phase-2)
// - [ ] Create a complex object to cast shadow on (phase-3)
// - [ ] Cast the shadow on the complex object (phase-2)

//  Display a rotating cube

#include "Angel.h"
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

// Window size in pixels
const int xpix = 1024;
const int ypix = 768;
const GLfloat screen_ratio = (GLfloat)xpix / (GLfloat)ypix;
bool follow = false;

// Sphere constants
#define SECTOR_COUNT 30
#define STACK_COUNT 30

// Object size modifier
const GLfloat radius = 0.1;
bool shadows = false;

// The number of vertices in each object
const int NumSphereVertices = SECTOR_COUNT * STACK_COUNT * 6;

typedef vec4 color4;
typedef vec4 point4;

GLuint textures[3];
GLint textureSelect = 0;
std::vector<GLubyte> image1;
std::vector<GLubyte> image2;
GLint image1size[2];
GLint image2size[2];
std::vector<GLubyte> image3;

// point arrays for the object
// vec2 sphere_tex_2d_coordinates[NumSphereVertices];
// GLfloat sphere_tex_1d_coordinates[NumSphereVertices];

enum { Shading = 0, Wireframe = 1, Texture = 2, NumRenderModes = 3 };

enum {
  NO_SPECULAR = 0,
  NO_DIFFUSE = 1,
  NO_AMBIENT = 2,
  ALL = 3,
  NumShadingModes = 4
};

enum { Metalic = 0, Plastic = 1, NumMaterialTypes = 2 };

GLint renderMode = Shading;
GLint shadingMode = ALL;
GLboolean shadingType = false;
GLint materialType = Metalic;
GLboolean textureType = true;

// Model-view and projection matrices uniform location
GLint ModelView, Projection, renderModeLoc, isBlackLoc, shadingTypeLoc,
    textureTypeLoc, tex2DSamplerLoc, tex1DSamplerLoc;

void update_shader_lighting_parameters(color4 material_ambient,
                                       color4 material_diffuse,
                                       color4 material_specular,
                                       GLfloat material_shininess);

GLuint program = 0;

// Have an error callback to make sure to catch any errors
void errorCallback(int error, const char *description) {
  std::cerr << "OpenGL error " << error << ": " << description << std::endl;
}

mat4 projection;
point4 light_position;

color4 colors[7] = {
    color4(0.0, 0.0, 0.0, 1.0), // black
    color4(1.0, 0.0, 0.0, 1.0), // red
    color4(1.0, 1.0, 0.0, 1.0), // yellow
    color4(0.0, 1.0, 0.0, 1.0), // green
    color4(0.0, 0.0, 1.0, 1.0), // blue
    color4(1.0, 1.0, 1.0, 1.0), // white
    color4(1.0, 0.5, 0.0, 1.0), // orange
};

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
      materials.emplace_back(face_colors[i] * 0.6);
      materials.emplace_back(face_colors[i] * 0.8);
      materials.emplace_back(face_colors[i] * 0.2);
    }
  }

  void load() {

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

  void draw() const {
    glBindVertexArray(vao);
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    // Draw the main_rubix_cube
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    for (int i = 0; i < 6; i++) {

      update_shader_lighting_parameters(materials[i * 3], materials[i * 3 + 1],
                                        materials[i * 3 + 2], 100.0f);
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

class Sphere {
  point4 sphere_points[NumSphereVertices];
  vec3 sphere_normals[NumSphereVertices];
  vec2 sphere_tex_2d_coordinates[NumSphereVertices];
  GLfloat sphere_tex_1d_coordinates[NumSphereVertices];
  GLuint vao;
  GLuint sphere_buffer;
  vec3 displacement = vec3(-0.5, 0.5, -3);
  GLfloat scalar = 1.0f;
  GLfloat rotate = 0.0f;
  vec3 velocity = vec3(0.5, 0.0, 0.0);
  vec3 start_velocity = vec3(0.5, 0.0, 0.0);
  vec3 start_displacement = vec3(-0.5, 0.5, -3);
  GLfloat y_acceleration = -9.8f;
  GLfloat dt = 0.01f;

public:
  Sphere() { create_sphere(); }

  void draw() {
    // compute the effect of gravity on the cube
    displacement.y =
        displacement.y + velocity.y * dt + 0.5f * y_acceleration * dt * dt;
    displacement.x = displacement.x + velocity.x * dt;

    // keep the cube moving horizontally
    velocity.y = velocity.y + y_acceleration * dt;

    // bounce the object off the floor (while reducing its speed overall on each
    // bounce)

    // displacement.y <
    // (std::tan(29.0 / 2 * DegreesToRadians) * start_displacement.z +
    //  scalar * radius) +
    //     floor) {
    //
    const float floor = -0.5;
    if (displacement.y < scalar * radius + floor) {
      // displacement.y = (GLfloat)std::tan(29.0 / 2 * DegreesToRadians) *
      //                      start_displacement.z +
      //                  scalar * radius + floor;

      displacement.y = scalar * radius + floor;
      velocity.y = -velocity.y * 0.75f;
      velocity.x = velocity.x * 0.85f;
    }

    glBindVertexArray(vao);
    //  Generate the model-view matrix
    mat4 model_view = Translate(displacement) *
                      Scale(vec3(scalar, scalar, scalar)) * RotateY(rotate);

    rotate += 0.5;
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
    glPolygonMode(GL_FRONT, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, NumSphereVertices);
  }

  void load() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &sphere_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(sphere_points) + sizeof(sphere_normals) +
                     sizeof(sphere_tex_2d_coordinates) +
                     sizeof(sphere_tex_1d_coordinates),
                 nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphere_points), sphere_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphere_points),
                    sizeof(sphere_normals), sphere_normals);
    glBufferSubData(
        GL_ARRAY_BUFFER, sizeof(sphere_points) + sizeof(sphere_normals),
        sizeof(sphere_tex_2d_coordinates), sphere_tex_2d_coordinates);
    glBufferSubData(GL_ARRAY_BUFFER,
                    sizeof(sphere_points) + sizeof(sphere_normals) +
                        sizeof(sphere_tex_2d_coordinates),
                    sizeof(sphere_tex_1d_coordinates),
                    sphere_tex_1d_coordinates);

    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    GLuint vNormal = glGetAttribLocation(program, "vNormal");
    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(sphere_points)));

    GLuint vTexCoord2D = glGetAttribLocation(program, "vTexCoord2D");
    glEnableVertexAttribArray(vTexCoord2D);
    glVertexAttribPointer(
        vTexCoord2D, 2, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(sphere_points) + sizeof(sphere_normals)));

    GLuint vTexCoord1D = glGetAttribLocation(program, "vTexCoord1D");
    glEnableVertexAttribArray(vTexCoord1D);
    glVertexAttribPointer(vTexCoord1D, 1, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(sphere_points) +
                                        sizeof(sphere_normals) +
                                        sizeof(sphere_tex_2d_coordinates)));
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

      stackAngle =
          M_PI / 2 - (float)i * stackStep; // starting from pi/2 to -pi/2
      xz = radius * cosf(stackAngle);      // r * cos(u)
      y = radius * sinf(stackAngle);       // r * sin(u)

      // add (sectorCount+1) vertices per stack
      // the first and last vertices have same position and normal, but
      // different tex coords
      for (int j = 0; j <= SECTOR_COUNT; ++j) {
        sectorAngle = (float)j * sectorStep; // starting from 0 to 2pi

        // vertex position (x, y, z)
        z = xz * cosf(sectorAngle); // r * cos(u) * cos(v)
        x = xz * sinf(sectorAngle); // r * cos(u) * sin(v)
        sphere_vertices.emplace_back(x, y, z, 1.0);
        tmp_sphere_normals.emplace_back(x, y, z);
        tmp_sphere_tex_2d_coords.emplace_back((float)j / SECTOR_COUNT,
                                              (float)i / STACK_COUNT);
        tmp_sphere_tex_1d_coords.emplace_back((float)j / SECTOR_COUNT);
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
      sphere_tex_2d_coordinates[i] =
          tmp_sphere_tex_2d_coords[sphere_indices[i]];
      sphere_tex_1d_coordinates[i] =
          tmp_sphere_tex_1d_coords[sphere_indices[i]];
    }
  }
};

void create1DImage() {
  for (int i = 0; i < 1024; ++i) {
    if (i % 96 < 32) {
      image3.emplace_back(255);
      image3.emplace_back(0);
      image3.emplace_back(0);
    } else if (i % 96 > 64) {
      image3.emplace_back(0);
      image3.emplace_back(0);
      image3.emplace_back(255);
    } else {
      image3.emplace_back(0);
      image3.emplace_back(255);
      image3.emplace_back(0);
    }
  }
}

void readPPMImage(const char *fn, std::vector<GLubyte> &image, GLint *imgSize) {

  std::string line;
  std::ifstream ppmFile(fn);

  if (ppmFile.is_open()) {
    std::string header;
    std::getline(ppmFile, header);

    if (header == "P3") {
      int height, width, maxValue;

      ppmFile >> width >> height >> maxValue;

      imgSize[0] = width;
      imgSize[1] = height;

      int numPixels = height * width;

      image.resize(3 * numPixels);

      for (int idx = numPixels; idx > 0; idx--) {
        int R, G, B;

        ppmFile >> R >> G >> B;
        image[3 * numPixels - 3 * idx] = R;
        image[3 * numPixels - 3 * idx + 1] = G;
        image[3 * numPixels - 3 * idx + 2] = B;
      }
    } else {
      std::cout << "File is not a PPM file" << std::endl;
    }
  }
}

Sphere sphere;

Cube cube(15, vec4(0, 0, 0, 1),
          vector<vec4>({colors[1], colors[2], colors[3], colors[4], colors[5],
                        colors[6]}));
void init() {

  // Load shaders and use the resulting shader program

  // Create/load the points for each object
  readPPMImage("assets/earth.ppm", image1, image1size);
  readPPMImage("assets/basketball.ppm", image2, image2size);
  create1DImage();
  // Initialize texture objects
  glGenTextures(3, textures);

  glActiveTexture(GL_TEXTURE0 + textureSelect);
  glBindTexture(GL_TEXTURE_2D, textures[textureSelect]);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR); // try here different alternatives
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR); // try here different alternatives
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image1size[0], image1size[1], 0,
               GL_RGB, GL_UNSIGNED_BYTE, image1.data());
  glGenerateMipmap(GL_TEXTURE_2D);

  textureSelect++;
  glActiveTexture(GL_TEXTURE0 + textureSelect);
  glBindTexture(GL_TEXTURE_2D, textures[textureSelect]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image2size[0], image2size[1], 0,
               GL_RGB, GL_UNSIGNED_BYTE, image2.data());
  glGenerateMipmap(GL_TEXTURE_2D);

  textureSelect++;
  glActiveTexture(GL_TEXTURE0 + textureSelect);
  glBindTexture(GL_TEXTURE_1D, textures[textureSelect]);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE,
               image3.data());
  glGenerateMipmap(GL_TEXTURE_1D);

  textureSelect = 0;

  // Create a vertex array object

  program = InitShader("shaders/vshader.glsl", "shaders/fshader.glsl");
  glUseProgram(program);

  // sphere.load();

  cube.load();

  // update_shader_lighting_parameters();
  // Retrieve transformation uniform variable locations
  ModelView = glGetUniformLocation(program, "ModelView");
  Projection = glGetUniformLocation(program, "Projection");
  shadingTypeLoc = glGetUniformLocation(program, "ShadingType");
  renderModeLoc = glGetUniformLocation(program, "RenderMode");
  isBlackLoc = glGetUniformLocation(program, "IsBlack");
  textureTypeLoc = glGetUniformLocation(program, "textureType");
  tex2DSamplerLoc = glGetUniformLocation(program, "tex2D");
  tex1DSamplerLoc = glGetUniformLocation(program, "tex1D");

  glUniform1i(renderModeLoc, renderMode);
  glUniform1i(shadingTypeLoc, shadingType);
  glUniform1i(textureTypeLoc, textureType);
  glUniform1i(isBlackLoc, 0);
  glUniform1i(tex2DSamplerLoc, textureSelect);
  glUniform1i(tex1DSamplerLoc, 2);

  // Set projection matrix
  projection = Perspective(120, screen_ratio, 0.1, 100);
  glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

  glEnable(GL_DEPTH_TEST);
  // glEnable(GL_CULL_FACE);
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

void update_shader_lighting_parameters(color4 material_ambient,
                                       color4 material_diffuse,
                                       color4 material_specular,
                                       GLfloat material_shininess) {
  // Initialize shader lighting parameters
  light_position = vec4(-0, 0, -10, 1);
  color4 light_ambient(0.2, 0.2, 0.2, 1.0);
  color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
  color4 light_specular(1.0, 1.0, 1.0, 1.0);

  color4 ambient_product = light_ambient * material_ambient;
  color4 diffuse_product = light_diffuse * material_diffuse;
  color4 specular_product = light_specular * material_specular;

  switch (shadingMode) {
  case NO_AMBIENT:
    ambient_product = color4(0, 0, 0, 1);
    specular_product = color4(0, 0, 0, 1);
    diffuse_product = color4(0, 0, 0, 1);
    break;
  case NO_DIFFUSE:
    specular_product = color4(0, 0, 0, 1);
    diffuse_product = color4(0, 0, 0, 1);
    break;
  case NO_SPECULAR:
    specular_product = color4(0, 0, 0, 1);
    break;
  case ALL:
    break;
  }

  glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1,
               ambient_product);
  glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1,
               diffuse_product);
  glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1,
               specular_product);

  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
               light_position);

  glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);
}

void key_callback(__attribute_maybe_unused__ GLFWwindow *window, int key,
                  __attribute_maybe_unused__ int scancode, int action,
                  __attribute_maybe_unused__ int mods) {

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    switch (key) {
    case GLFW_KEY_S:
      shadingType = !shadingType;
      glUniform1i(shadingTypeLoc, shadingType);
      break;
    case GLFW_KEY_X:
      shadows = !shadows;
      break;
    case GLFW_KEY_O:
      shadingMode = (shadingMode + 1) % NumShadingModes;
      break;
    case GLFW_KEY_L:
      follow = !follow;
      break;
    case GLFW_KEY_M:
      materialType = (materialType + 1) % NumMaterialTypes;
      break;
    case GLFW_KEY_Z:
      break;
    case GLFW_KEY_W:
      break;
    case GLFW_KEY_I:
      textureSelect = (textureSelect + 1) % 3;
      if (textureSelect == 2) {
        textureType = !textureType;
        glUniform1i(textureTypeLoc, textureType);
        glUniform1i(tex1DSamplerLoc, textureSelect);
      } else {
        if (!textureType) {
          textureType = !textureType;
        }
        glUniform1i(textureTypeLoc, textureType);
        glUniform1i(tex2DSamplerLoc, textureSelect);
      }
      break;
    case GLFW_KEY_T:
      renderMode = (renderMode + 1) % NumRenderModes;
      glUniform1i(renderModeLoc, renderMode);
      break;
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
      exit(EXIT_SUCCESS);
    case GLFW_KEY_R:
      // displacement = start_displacement;
      // velocity = start_velocity;
      break;
    case GLFW_KEY_H:
      std::cout << "Help:" << std::endl;
      std::cout << "Press 'q' or 'esc' to quit" << std::endl;
      std::cout << "Press 'r' to reset the object" << std::endl;
      std::cout << "Press 'l' to toggle the light (fixed/following)"
                << std::endl;
      std::cout << "Press 'o' to disable/renable the ambient,diffuse and "
                   "specular properties"
                << std::endl;
      std::cout
          << "Press 't' to switch between solid, wireframe, and texture views"
          << std::endl;
      std::cout << "Press 's' to switch between, Gouraud and Phong shading"
                << std::endl;
      std::cout << "Press 'i' to switch between, the various textures (earth, "
                   "basketball, 1d)"
                << std::endl;
      std::cout << "Press 'm' to switch between metalic and plastic materials"
                << std::endl;
      std::cout << "Press 'w' or 'z' to zoom in or out" << std::endl;
      std::cout << "Press 'x' to toggle shadows (not fully complete)"
                << std::endl;
      std::cout << "Press 'h' to display this help message" << std::endl;
      break;
    default:
      break;
    }
  }
}

void mouse_button_callback(__attribute_maybe_unused__ GLFWwindow *window,
                           int button, int action,
                           __attribute_maybe_unused__ int mods) {
  if (action == GLFW_PRESS) {
    switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT:
      break;
    case GLFW_MOUSE_BUTTON_LEFT:
      break;
    default:
      break;
    }
  }
}

// create_sphere
// This function creates a sphere with a given stack and sector count
// (specified by the constants STACK_COUNT and SECTOR_COUNT)
// Adopted from: http://www.songho.ca/opengl/gl_sphere.html

GLfloat dt = 0.01;

void update() {

  switch (renderMode) {
  case Shading:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case Wireframe:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  case Texture:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  default:
    break;
  }

  // update_shader_lighting_parameters();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //  Generate the model-view matrix

  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
               light_position);

  // sphere.draw();
  cube.rotateY(1);
  cube.rotateX(1);
  cube.draw();

  // mat4 model_view = Translate(displacement) *
  //                   Scale(vec3(scalar, scalar, scalar)); // *
  //                   RotateY(rotate);
  //
  // rotate += 0.5;
  //
  // glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
  // glUniform1i(isBlackLoc, 0);
  // glDrawArrays(GL_TRIANGLES, 0, NumSphereVertices);
  glFlush();
}

int main() {

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  GLFWwindow *window =
      glfwCreateWindow(xpix, ypix, "Homework 3", nullptr, nullptr);
  glfwMakeContextCurrent(window);

  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetErrorCallback(errorCallback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);

  glewExperimental = GL_TRUE;
  glewInit();
  init();

  // double frameRate = 60, currentTime, previousTime = 0.0;

  while (!glfwWindowShouldClose(window)) {
    // currentTime = glfwGetTime();
    // if (currentTime - previousTime >= 1 / frameRate) {
    // previousTime = currentTime;
    update();
    // }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
