//
//  Display a rotating cube
//

#include "Angel.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

const int xpix = 1280;
const int ypix = 720;

#define SECTOR_COUNT 10
#define STACK_COUNT 10

typedef vec4 color4;
typedef vec4 point4;

GLuint vao1;
GLuint vao2;
GLuint vao3;

const GLfloat radius = 0.05;
const int NumCubeVertices = 36;
const int NumSphereVertices = SECTOR_COUNT * STACK_COUNT * 6;
const int NumRabbitVertices = 9840 * 3;

const GLfloat screen_ratio = (GLfloat)xpix / (GLfloat)ypix;

GLuint program = 0;

struct Vertex {
  float x, y, z;
  float nx, ny, nz;
};

void create_sphere();
void load_rabbit();

point4 cube_points[NumCubeVertices];
color4 cube_colors[NumCubeVertices];

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[8] = {point4(-radius, -radius, radius, 1.0),
                      point4(-radius, radius, radius, 1.0),
                      point4(radius, radius, radius, 1.0),
                      point4(radius, -radius, radius, 1.0),
                      point4(-radius, -radius, -radius, 1.0),
                      point4(-radius, radius, -radius, 1.0),
                      point4(radius, radius, -radius, 1.0),
                      point4(radius, -radius, -radius, 1.0)};

// RGBA olors
color4 vertex_colors[8] = {
    color4(0.0, 0.0, 0.0, 1.0), // black
    color4(1.0, 0.0, 0.0, 1.0), // red
    color4(1.0, 1.0, 0.0, 1.0), // yellow
    color4(0.0, 1.0, 0.0, 1.0), // green
    color4(0.0, 0.0, 1.0, 1.0), // blue
    color4(1.0, 0.0, 1.0, 1.0), // magenta
    color4(1.0, 1.0, 1.0, 1.0), // white
    color4(0.0, 1.0, 1.0, 1.0)  // cyan
};

point4 sphere_points[NumSphereVertices];
color4 sphere_colors[NumSphereVertices];

point4 rabbit_points[NumRabbitVertices];
color4 rabbit_colors[NumRabbitVertices];

// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
enum { Cube = 0, Sphere = 1, Rabbit = 2, NumObjectTypes = 3 };
enum { Color1 = 0, Color2 = 1, Color3 = 2, NumColors = 3 };
enum { Frame = 0, Solid = 1, NumRenderModes = 2 };

int RenderMode = Solid;
int ObjectType = Rabbit;
int Color = Color1;
int Axis = Yaxis;
GLfloat Theta[NumAxes] = {0.0, 0.0, 0.0};

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

//----------------------------------------------------------------------------

// quad generates two triangles for each face and assigns colors to the vertices
int Index = 0;

void quad(int a, int b, int c, int d) {
  cube_colors[Index] = vertex_colors[a];
  cube_points[Index] = vertices[a];
  Index++;
  cube_colors[Index] = vertex_colors[b];
  cube_points[Index] = vertices[b];
  Index++;
  cube_colors[Index] = vertex_colors[c];
  cube_points[Index] = vertices[c];
  Index++;
  cube_colors[Index] = vertex_colors[a];
  cube_points[Index] = vertices[a];
  Index++;
  cube_colors[Index] = vertex_colors[c];
  cube_points[Index] = vertices[c];
  Index++;
  cube_colors[Index] = vertex_colors[d];
  cube_points[Index] = vertices[d];
  Index++;
}

//----------------------------------------------------------------------------

// generate 12 triangles: 36 vertices and 36 colors

void colorcube() {
  quad(1, 0, 3, 2);
  quad(2, 3, 7, 6);
  quad(3, 0, 4, 7);
  quad(6, 5, 1, 2);
  quad(4, 5, 6, 7);
  quad(5, 4, 0, 1);
}

//---------------------------------------------------------------------
//
// init
//

GLuint cube_buffer;
GLuint sphere_buffer;
GLuint rabbit_buffer;

void display_sphere_buffer() {

  glBindVertexArray(vao1);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_points) + sizeof(sphere_colors),
               NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphere_points), sphere_points);

  switch (Color) {
  case Color1:
    for (int i = 0; i < NumSphereVertices; i++) {
      sphere_colors[i] = color4(1.0, 0.0, 0.0, 1.0);
    }
    break;
  case Color2:
    for (int i = 0; i < NumSphereVertices; i++) {
      sphere_colors[i] = color4(0.0, 1.0, 0.0, 1.0);
    }
    break;
  case Color3:
    for (int i = 0; i < NumSphereVertices; i++) {
      sphere_colors[i] = vertex_colors[i % 8];
    }
    break;
  }

  glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphere_points), sizeof(sphere_colors),
                  sphere_colors);

  // set up vertex arrays
  GLuint vPosition = glGetAttribLocation(program, "vPosition");
  glEnableVertexAttribArray(vPosition);
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

  GLuint vColor = glGetAttribLocation(program, "vColor");
  glEnableVertexAttribArray(vColor);
  glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(sphere_points)));
}

void display_cube_buffer() {
  glBindVertexArray(vao2);
  glBindBuffer(GL_ARRAY_BUFFER, cube_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cube_points) + sizeof(cube_colors), NULL,
               GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cube_points), cube_points);

  switch (Color) {
  case Color1:
    for (int i = 0; i < NumCubeVertices; i++) {
      cube_colors[i] = color4(1.0, 0.0, 0.0, 1.0);
    }
    break;
  case Color2:
    for (int i = 0; i < NumCubeVertices; i++) {
      cube_colors[i] = color4(0.0, 1.0, 0.0, 1.0);
    }
    break;
  case Color3:
    for (int i = 0; i < NumCubeVertices; i++) {
      cube_colors[i] = vertex_colors[i % 8];
    }
    break;
  }

  glBufferSubData(GL_ARRAY_BUFFER, sizeof(cube_points), sizeof(cube_colors),
                  cube_colors);

  // set up vertex arrays
  GLuint vPosition = glGetAttribLocation(program, "vPosition");
  glEnableVertexAttribArray(vPosition);
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

  GLuint vColor = glGetAttribLocation(program, "vColor");
  glEnableVertexAttribArray(vColor);
  glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(cube_points)));
}

void display_rabbit_buffer() {
  glBindVertexArray(vao3);
  glBindBuffer(GL_ARRAY_BUFFER, rabbit_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(rabbit_points) + sizeof(rabbit_colors),
               NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rabbit_points), rabbit_points);

  switch (Color) {
  case Color1:
    for (int i = 0; i < NumRabbitVertices; i++) {
      rabbit_colors[i] = color4(1.0, 0.0, 0.0, 1.0);
    }
    break;
  case Color2:
    for (int i = 0; i < NumRabbitVertices; i++) {
      rabbit_colors[i] = color4(0.0, 1.0, 0.0, 1.0);
    }
    break;
  case Color3:
    for (int i = 0; i < NumRabbitVertices; i++) {
      rabbit_colors[i] = vertex_colors[i % 8];
    }
    break;
  }

  glBufferSubData(GL_ARRAY_BUFFER, sizeof(rabbit_points), sizeof(rabbit_colors),
                  rabbit_colors);

  // set up vertex arrays
  GLuint vPosition = glGetAttribLocation(program, "vPosition");
  glEnableVertexAttribArray(vPosition);
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

  GLuint vColor = glGetAttribLocation(program, "vColor");
  glEnableVertexAttribArray(vColor);
  glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(rabbit_points)));
}

void errorCallback(int error, const char *description) {
  std::cerr << "OpenGL error " << error << ": " << description << std::endl;
}

void init() {

  // Load shaders and use the resulting shader program
  program = InitShader("shaders/vshader.glsl", "shaders/fshader.glsl");
  glUseProgram(program);

  colorcube(); // create the cube in terms of 6 faces each of which is made of
               // two triangles
  create_sphere();
  load_rabbit();

  // Create a vertex array object
  glGenVertexArrays(1, &vao1);
  glGenVertexArrays(1, &vao2);
  glGenVertexArrays(1, &vao3);

  glGenBuffers(1, &sphere_buffer);
  glGenBuffers(1, &cube_buffer);
  glGenBuffers(1, &rabbit_buffer);

  display_rabbit_buffer();

  // Retrieve transformation uniform variable locations
  ModelView = glGetUniformLocation(program, "ModelView");
  Projection = glGetUniformLocation(program, "Projection");

  // Set projection matrix
  mat4 projection;
  projection = Ortho(-1.0 * (screen_ratio), 1.0 * (screen_ratio), -1.0, 1.0,
                     -1.0, 1.0); // Ortho(): user-defined function in mat.h
  glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

  glEnable(GL_DEPTH_TEST);
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

//---------------------------------------------------------------------
//
// display
//

vec3 start_displacement(-1.0 * screen_ratio + radius, 1.0 - radius, 0.0);
vec3 displacement(start_displacement);
vec3 start_velocity(1.0, 0.0, 0.0);
vec3 velocity(start_velocity);
const GLfloat y_acceleration = -9.81;

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //  Generate the model-view matrix

  mat4 model_view =
      (Translate(displacement) * Scale(1.0, 1.0, 1.0) * RotateX(Theta[Xaxis]) *
       RotateY(Theta[Yaxis]) *
       RotateZ(Theta[Zaxis])); // Scale(), Translate(), RotateX(), RotateY(),
                               // RotateZ(): user-defined functions in mat.h

  glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
  // TODO: change the num vertices when switching between sphere and cube
  switch (ObjectType) {
  case Sphere:
    glDrawArrays(GL_TRIANGLES, 0, NumSphereVertices);
    break;
  case Rabbit:
    glDrawArrays(GL_TRIANGLES, 0, NumRabbitVertices);
    break;
  case Cube:
    glDrawArrays(GL_TRIANGLES, 0, NumCubeVertices);
    break;
  }

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
    Color = (Color + 1) % NumColors;
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
                 "(rabbit, sphere, cube)"
              << std::endl;
    std::cout << "Press 'h' to display this help message" << std::endl;
  }
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  if (action == GLFW_PRESS) {
    switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT:
      ObjectType = (ObjectType + 1) % NumObjectTypes;
      break;
    case GLFW_MOUSE_BUTTON_LEFT:
      RenderMode = (RenderMode + 1) % NumRenderModes;
      break;
    }
  }
}

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
    for (int j = 0; j <= SECTOR_COUNT; ++j) {
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
    sphere_colors[i] = vertex_colors[i % 8];
  }
}

GLfloat dt = 0.01;
GLfloat zero = 0.01;

void update(void) {

  // Rotate the cube by 1 degree
  // Theta[Zaxis] = (GLfloat) (((int) Theta[Zaxis] + 359) % 360);

  switch (RenderMode) {
  // TODO: maybe recheck if it can be done only for the front
  case Solid:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case Frame:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  }

  switch (ObjectType) {
  case Cube:
    display_cube_buffer();
    break;
  case Sphere:
    display_sphere_buffer();
    break;
  case Rabbit:
    display_rabbit_buffer();
    break;
  }

  if (velocity.y < zero && velocity.x < zero) {
    switch (ObjectType) {
    case Cube:
    case Sphere:
      displacement.y = -1.0 + radius;
      break;
    case Rabbit:
      displacement.y = -1.0;
      break;
    }
    return;
  }

  // compute the effect of gravity on the cube
  displacement.y =
      displacement.y + velocity.y * dt + 0.5 * y_acceleration * dt * dt;
  displacement.x = displacement.x + velocity.x * dt;

  // keep the cube moving horizontally
  velocity.y = velocity.y + y_acceleration * dt;

  switch (ObjectType) {
  case Cube:
  case Sphere:
    // bounce the cube/sphere off the floor
    if (displacement.y < -1.0 + radius) {
      displacement.y = -1.0 + radius;
      velocity.y = -velocity.y * 0.75;
      velocity.x = velocity.x * 0.85;
    }
    break;
  case Rabbit:
    if (displacement.y < -1.0) {
      displacement.y = -1.0;
      velocity.y = -velocity.y * 0.75;
      velocity.x = velocity.x * 0.85;
    }
    break;
  }
}

void load_rabbit() {

  std::ifstream file("models/bunny.off");
  std::string line;
  std::getline(file, line);
  std::getline(file, line);

  std::istringstream iss(line);
  int num_vertices, num_faces, num_edges;
  iss >> num_vertices >> num_faces >> num_edges;
  std::vector<vec4> rabbit_vertices;
  for (int i = 0; i < num_vertices; i++) {
    std::getline(file, line);
    std::istringstream iss(line);
    float x, y, z;
    iss >> y >> x >> y;
    x *= -1;
    x *= 0.5 * radius;
    y *= 0.5 * radius;
    z *= 0.5 * radius;

    rabbit_vertices.push_back(vec4(x, y, z, 1.0));
  }

  std::vector<int> rabbit_indices;
  for (int i = 0; i < num_faces; i++) {
    std::getline(file, line);
    std::istringstream iss(line);
    int num_vertices, v1, v2, v3;
    iss >> num_vertices >> v1 >> v2 >> v3;
    rabbit_indices.push_back(v1);
    rabbit_indices.push_back(v2);
    rabbit_indices.push_back(v3);
  }

  for (int i = 0; i < rabbit_indices.size(); i++) {
    rabbit_points[i] = rabbit_vertices[rabbit_indices[i]];
    rabbit_colors[i] = vertex_colors[0];
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

  GLFWwindow *window = glfwCreateWindow(xpix, ypix, "Spin Cube", NULL, NULL);
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
