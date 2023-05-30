//
//  Display a rotating cube
//

#include "Angel.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

// Window size in pixels
const int xpix = 1024;
const int ypix = 768;
const GLfloat screen_ratio = (GLfloat)xpix / (GLfloat)ypix;

// Sphere constants
#define SECTOR_COUNT 8
#define STACK_COUNT 8

// Object size modifier
const GLfloat radius = 0.1;

// The number of vertices in each object
const int NumSphereVertices = SECTOR_COUNT * STACK_COUNT * 6;

typedef vec4 color4;
typedef vec4 point4;

// point arrays for the objects

point4 sphere_points[NumSphereVertices];
vec3 sphere_normals[NumSphereVertices];


// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
enum { Sphere = 0, NumObjectTypes = 1 };
enum { Color1 = 0, Color2 = 1, Color3 = 2, NumColors = 3 };
enum { Frame = 0, Solid = 1, NumRenderModes = 2 };

int RenderMode = Solid;



GLfloat Theta[NumAxes] = {0.0, 0.0, 0.0};

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

//---------------------------------------------------------------------
//
// init
//

void create_sphere();

GLuint vao;
GLuint program = 0;
GLuint sphere_buffer;

// This function readies the given object for display with the given points and
// colors
void display_object(GLuint object_buffer, point4 *object_points,
                    color4 *object_colors, int num_vertices, GLuint vao) {}

// Have an error callback to make sure to catch any errors
void errorCallback(int error, const char *description) {
  std::cerr << "OpenGL error " << error << ": " << description << std::endl;
}

mat4 projection;
point4 light_position;
vec3 start_displacement(-0.5, 0.5, -2);
void init() {

  // Load shaders and use the resulting shader program

  // Create/load the points for each object
  create_sphere();

  // Create a vertex array object
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &sphere_buffer);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_points) + sizeof(sphere_normals),
               NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphere_points), sphere_points);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphere_points),
                  sizeof(sphere_normals), sphere_normals);

  program = InitShader("shaders/vshader.glsl", "shaders/fshader.glsl");
  glUseProgram(program);

  // set up vertex arrays
  GLuint vPosition = glGetAttribLocation(program, "vPosition");
  glEnableVertexAttribArray(vPosition);
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

  GLuint vNormal = glGetAttribLocation(program, "vNormal");
  glEnableVertexAttribArray(vNormal);
  glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(sphere_points)));

  // Initialize shader lighting parameters
  light_position=vec4(0, 0, -2,1);
  color4 light_ambient(0.2, 0.2, 0.2, 1.0);
  color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
  color4 light_specular(1.0, 1.0, 1.0, 1.0);

  color4 material_ambient(1.0, 0.0, 1.0, 1.0);
  color4 material_diffuse(1.0, 0.8, 0.0, 1.0);
  color4 material_specular(1.0, 0.0, 1.0, 1.0);
  float material_shininess = 5.0;

  color4 ambient_product = light_ambient * material_ambient;
  color4 diffuse_product = light_diffuse * material_diffuse;
  color4 specular_product = light_specular * material_specular;

  glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1,
               ambient_product);
  glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1,
               diffuse_product);
  glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1,
               specular_product);

  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
               light_position);

  glUniform1i(glGetUniformLocation(program, "is_phong"), 0);

  glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);

  // Retrieve transformation uniform variable locations
  ModelView = glGetUniformLocation(program, "ModelView");
  Projection = glGetUniformLocation(program, "Projection");

  // Set projection matrix
  projection = Perspective(29, screen_ratio, 1, 3);
  glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glClearColor(0.2, 0.2, 0.2, 1.0);

}

//---------------------------------------------------------------------
//
// display
//
//

vec3 displacement(start_displacement);
vec3 start_velocity(0.5, 0.0, 0.0);
vec3 velocity(start_velocity);
const GLfloat y_acceleration = -9.81;

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //  Generate the model-view matrix

  mat4 model_view =
      (Translate(displacement) * Scale(1.0, 1.0, 1.0) * RotateX(Theta[Xaxis]) *
       RotateY(Theta[Yaxis]) *
       RotateZ(Theta[Zaxis]));
  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
                 light_position);
  glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
  glDrawArrays(GL_TRIANGLES, 0, NumSphereVertices);

  glFlush();
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {

  if (action != GLFW_PRESS) {
    return;
  }

  switch (key) {
  case GLFW_KEY_ESCAPE:
  case GLFW_KEY_Q:
    exit(EXIT_SUCCESS);
    break;
  case GLFW_KEY_I:
    displacement = start_displacement;
    velocity = start_velocity;
    break;
  case GLFW_KEY_C:
    // toggle the color (between green, red, and our custom colors)

    break;
  case GLFW_KEY_H:
    std::cout << "Help:" << std::endl;
    std::cout << "Press 'q' or 'esc' to quit" << std::endl;
    std::cout << "Press 'i' to reset the object" << std::endl;
    std::cout << "Press 'c' to toggle the color" << std::endl;
    std::cout << "Click the left mouse button to toggle the render mode "
                 "(solid, wireframe)"
              << std::endl;
    std::cout << "Click the right mouse button to toggle the object type "
                 "(sphere)"
              << std::endl;
    std::cout << "Press 'h' to display this help message" << std::endl;
  }
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  if (action == GLFW_PRESS) {
    switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT:
      break;
    case GLFW_MOUSE_BUTTON_LEFT:
      RenderMode = (RenderMode + 1) % NumRenderModes;
      break;
    }
  }
}

// create_sphere
// This function creates a sphere with a given stack and sector count
// (specified by the constants STACK_COUNT and SECTOR_COUNT)
// Adopted from: http://www.songho.ca/opengl/gl_sphere.html
void create_sphere() {

  std::vector<vec4> sphere_vertices;

  float x, y, z, xz; // vertex position

  float sectorStep = 2 * M_PI / SECTOR_COUNT;
  float stackStep = M_PI / STACK_COUNT;
  float sectorAngle, stackAngle;

  for (int i = 0; i <= STACK_COUNT; ++i) {

    stackAngle = M_PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
    xz = radius * cosf(stackAngle);        // r * cos(u)
    y = radius * sinf(stackAngle);         // r * sin(u)

    // add (sectorCount+1) vertices per stack
    // the first and last vertices have same position and normal, but different
    // tex coords
    for (int j = SECTOR_COUNT; j >= 0; --j) {
      sectorAngle = j * sectorStep; // starting from 0 to 2pi

      // vertex position (x, y, z)
      x = xz * cosf(sectorAngle); // r * cos(u) * cos(v)
      z = xz * sinf(sectorAngle); // r * cos(u) * sin(v)
      sphere_vertices.push_back(vec4(x, y, z, 1.0));
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
    sphere_normals[i] = vec3(sphere_vertices[sphere_indices[i]][0],
                             sphere_vertices[sphere_indices[i]][1],
                             sphere_vertices[sphere_indices[i]][2]);
  }
}

GLfloat dt = 0.01;
GLfloat zero = 0.01;

void update(void) {

  switch (RenderMode) {
  case Solid:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case Frame:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  }

  // compute the effect of gravity on the cube
  displacement.y =
      displacement.y + velocity.y * dt + 0.5f * y_acceleration * dt * dt;
  displacement.x = displacement.x + velocity.x * dt;

  // keep the cube moving horizontally
  velocity.y = velocity.y + y_acceleration * dt;

  // bounce the object off the floor (while reducing its speed overall on each
  // bounce)

  if (displacement.y <
      (std::tan(29.0 / 2 * DegreesToRadians) * start_displacement.z + radius)) {
    displacement.y =
        (std::tan(29.0 / 2 * DegreesToRadians) * start_displacement.z + radius);
    velocity.y = -velocity.y * 0.75;
    velocity.x = velocity.x * 0.85;
  }
}

int main() {

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  GLFWwindow *window = glfwCreateWindow(xpix, ypix, "Homework 3", NULL, NULL);
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

  double frameRate = 120, currentTime, previousTime = 0.0;

  while (!glfwWindowShouldClose(window)) {
    currentTime = glfwGetTime();
    if (currentTime - previousTime >= 1 / frameRate) {
      previousTime = currentTime;
      update();
    }

    display();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
