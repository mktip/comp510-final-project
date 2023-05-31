//
//  Display a rotating cube
//

#include "Angel.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
// Window size in pixels
const int xpix = 1024;
const int ypix = 768;
const GLfloat screen_ratio = (GLfloat)xpix / (GLfloat)ypix;

// Sphere constants
#define SECTOR_COUNT 30
#define STACK_COUNT 30

// Object size modifier
const GLfloat radius = 0.1;

// The number of vertices in each object
const int NumSphereVertices = SECTOR_COUNT * STACK_COUNT * 6;

typedef vec4 color4;
typedef vec4 point4;

GLuint textures[2];
GLint textureSelect=0;
std::vector<GLubyte> image1;
std::vector<GLubyte> image2;
GLint image1size[2];
GLint image2size[2];
GLubyte *image3;

// point arrays for the object
point4 sphere_points[NumSphereVertices];
vec3 sphere_normals[NumSphereVertices];
vec2 sphere_tex_coordinates[NumSphereVertices];


enum { Shading = 0,Wireframe = 1, Texture = 2, NumRenderModes = 3 };
enum { None = 0,Specular = 1, Diffuse = 2, Ambient = 3,  NumShadingModes = 4 };

GLint renderMode = Shading;
GLint shadingMode = None;
GLboolean shadingType = false;

// Model-view and projection matrices uniform location
GLint ModelView, Projection,renderModeLoc,shadingTypeLoc;

//---------------------------------------------------------------------
//
// init
//

void create_sphere();

GLuint vao;
GLuint program = 0;
GLuint sphere_buffer;

// Have an error callback to make sure to catch any errors
void errorCallback(int error, const char *description) {
  std::cerr << "OpenGL error " << error << ": " << description << std::endl;
}

mat4 projection;
point4 light_position;
vec3 start_displacement(-0.5, 0.5, -2);

void readPPMImage(const char *fn ,std::vector<GLubyte> &image, GLint* imgSize) {

    std::string line;
    std::ifstream ppmFile(fn);

    if (ppmFile.is_open())
    {
        std::string header;
        std::getline(ppmFile, header);

        if (header=="P3")
        {
            int height, width, maxValue;

            ppmFile >> width >> height >> maxValue;

            imgSize[0] = width;
            imgSize[1] = height;

            int numPixels = height * width;

            image.resize(3 * numPixels);

            for (int idx = numPixels; idx > 0; idx--)
            {
                int R, G, B;

                ppmFile >> R >> G >> B;
                image[3 * numPixels - 3 * idx] = R;
                image[3 * numPixels - 3 * idx + 1] = G;
                image[3 * numPixels - 3 * idx + 2] = B;
            }
        }
        else
        {
            std::cout << "File is not a PPM file" << std::endl;
        }
    }
    /*int tmpint;
    char str[100];
    FILE* inFile = fopen (fn,"r");

    if (inFile == NULL)
    {
        printf ("Can't open input file %s. Exiting.\n",fn);
        exit (1);
    }

    fscanf (inFile,"P%d\n", &tmpint);

    if (tmpint != 3)
    {
        printf ("Input file is not ppm. Exiting.\n");
        exit (1);
    }
    fgets (str,100,inFile);
    sscanf (str,"%d %d %d\n",&imgSize[0], &imgSize[1],&tmpint);

    if (tmpint != 255)
        printf("Warning: maxvalue is not 255 in ppm file\n");


    *imgArr = (GLubyte*) malloc (3 * imgSize[0] *  imgSize[1] * sizeof (GLubyte));

    if (*imgArr == nullptr)
    {
        printf ("Can't allocate image of size %dx%d. Exiting\n", imgSize[0], imgSize[1]);
        exit (1);
    }
    else
        printf("Reading image %s of size %dx%d\n", fn, imgSize[0], imgSize[1]);


    fread (*imgArr, sizeof (GLubyte), 3 * imgSize[0] * imgSize[1], inFile);

    fclose (inFile);*/
}
void init() {

  // Load shaders and use the resulting shader program

  // Create/load the points for each object
    create_sphere();
    readPPMImage("assets/earth.ppm",image1,image1size);
    readPPMImage("assets/basketball.ppm",image2,image2size);

    // Initialize texture objects
    glGenTextures( 2, textures );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_2D, textures[textureSelect] );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR); //try here different alternatives
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //try here different alternatives
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, image1size[0], image1size[1], 0,
                  GL_RGB, GL_UNSIGNED_BYTE, image1.data() );
    glGenerateMipmap(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    textureSelect++;
    glBindTexture( GL_TEXTURE_2D, textures[textureSelect] );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, image2size[0], image2size[1], 0,
                  GL_RGB, GL_UNSIGNED_BYTE, image2.data() );
    glGenerateMipmap(GL_TEXTURE_2D); // try also activating mipmaps for the second texture object
    textureSelect=0;
    glBindTexture( GL_TEXTURE_2D, textures[textureSelect] ); //set current texture

  // Create a vertex array object
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &sphere_buffer);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_points) + sizeof(sphere_normals) + sizeof(sphere_tex_coordinates),
               nullptr, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphere_points), sphere_points);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphere_points),
                  sizeof(sphere_normals), sphere_normals);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphere_points) + sizeof(sphere_normals),
                    sizeof(sphere_tex_coordinates), sphere_tex_coordinates);

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
  GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
  glEnableVertexAttribArray(vTexCoord);
  glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(sphere_points) + sizeof(sphere_normals)));

  // Initialize shader lighting parameters
  light_position=vec4(0, 0, 0,1);
  color4 light_ambient(0.2, 0.2, 0.2, 1.0);
  color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
  color4 light_specular(1.0, 1.0, 1.0, 1.0);

  color4 material_ambient(1.0, 0.0, 0.0, 1.0);
  color4 material_diffuse(0.5, 0.2, 0.0, 1.0);
  color4 material_specular(1.0, 0.0, 1.0, 1.0);
  GLfloat material_shininess = 5.0f;

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

  glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);

  // Retrieve transformation uniform variable locations
  ModelView = glGetUniformLocation(program, "ModelView");
  Projection = glGetUniformLocation(program, "Projection");
    shadingTypeLoc = glGetUniformLocation(program, "ShadingType");
  renderModeLoc = glGetUniformLocation(program, "RenderMode");

  glUniform1i( renderModeLoc, renderMode );
  glUniform1i(shadingTypeLoc, shadingType);

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
GLfloat scalar=1.0;
GLboolean follow=0;
const GLfloat y_acceleration = -9.81;
GLfloat rotate =0;
void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //  Generate the model-view matrix
  mat4 model_view = Translate(displacement)* Scale(vec3(scalar,scalar,scalar))* RotateY(rotate);
    rotate+=0.5;
  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
                     light_position);

  glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

  glDrawArrays(GL_TRIANGLES, 0, NumSphereVertices);

  glFlush();
}

void key_callback(__attribute_maybe_unused__ GLFWwindow *window, int key, __attribute_maybe_unused__ int scancode, int action,
                  __attribute_maybe_unused__ int mods) {

  if (action == GLFW_PRESS) {
      switch (key) {
          case GLFW_KEY_S:
              shadingType=!shadingType;
              glUniform1i(shadingTypeLoc, shadingType);
              break;
          case GLFW_KEY_O:
              shadingMode = (shadingMode + 1) % NumRenderModes;
              glUniform1i( renderModeLoc, shadingMode );
              break;
          case GLFW_KEY_L:
              follow=!follow;
              break;
          case GLFW_KEY_M:
              break;
          case GLFW_KEY_Z:
              scalar -= 0.1;
              break;
          case GLFW_KEY_W:
              scalar += 0.1;
              break;
          case GLFW_KEY_I:
              textureSelect= (textureSelect + 1) % 2;
              glBindTexture(GL_TEXTURE_2D, textures[textureSelect]);
              break;
          case GLFW_KEY_T:
              renderMode = (renderMode + 1) % NumRenderModes;
              glUniform1i( renderModeLoc, renderMode );
              break;
          case GLFW_KEY_ESCAPE:
          case GLFW_KEY_Q:
              exit(EXIT_SUCCESS);
          case GLFW_KEY_R:
              displacement = start_displacement;
              velocity = start_velocity;
              break;
          case GLFW_KEY_H:
              std::cout << "Help:" << std::endl;
              std::cout << "Press 'q' or 'esc' to quit" << std::endl;
              std::cout << "Press 'r' to reset the object" << std::endl;
              std::cout << "Press 'c' to toggle the color" << std::endl;
              std::cout << "Click the left mouse button to toggle the render mode (solid, wireframe)" << std::endl;
              std::cout << "Click the right mouse button to toggle the object type (sphere)" << std::endl;
              std::cout << "Press 'h' to display this help message" << std::endl;
              break;
          default:
              break;
      }
  }
}

void mouse_button_callback(__attribute_maybe_unused__ GLFWwindow *window, int button, int action,
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
void create_sphere() {

  std::vector<vec4> sphere_vertices;
  std::vector<vec3> tmp_sphere_normals;
  std::vector<vec2> tmp_sphere_tex_coords;

  float x, y, z, xz; // vertex position

  float sectorStep = 2 * M_PI / SECTOR_COUNT;
  float stackStep = M_PI / STACK_COUNT;
  float sectorAngle, stackAngle;

  for (int i = 0; i <= STACK_COUNT; ++i) {

    stackAngle = M_PI / 2 - (float) i * stackStep; // starting from pi/2 to -pi/2
    xz = radius * cosf(stackAngle);        // r * cos(u)
    y = radius * sinf(stackAngle);         // r * sin(u)

    // add (sectorCount+1) vertices per stack
    // the first and last vertices have same position and normal, but different
    // tex coords
    for (int j = 0; j <= SECTOR_COUNT; ++j) {
      sectorAngle = (float) j * sectorStep; // starting from 0 to 2pi

      // vertex position (x, y, z)
      z = xz * cosf(sectorAngle); // r * cos(u) * cos(v)
      x = xz * sinf(sectorAngle); // r * cos(u) * sin(v)
      sphere_vertices.emplace_back(x, y, z, 1.0);
      tmp_sphere_normals.emplace_back(x,y,z);
        float u;
        float v;
        u = 0.5 + atan2(z, x) / (2 * M_PI);
        v = 0.5 - asin(y) / M_PI;
      tmp_sphere_tex_coords.emplace_back((float)j/SECTOR_COUNT,(float)i/STACK_COUNT);

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
    sphere_tex_coordinates[i] = tmp_sphere_tex_coords[sphere_indices[i]];
  }
}

GLfloat dt = 0.01;
GLfloat zero = 0.01;

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

  // compute the effect of gravity on the cube
  displacement.y =
      displacement.y + velocity.y * dt + 0.5f * y_acceleration * dt * dt;
  displacement.x = displacement.x + velocity.x * dt;

  // keep the cube moving horizontally
  velocity.y = velocity.y + y_acceleration * dt;

  // bounce the object off the floor (while reducing its speed overall on each
  // bounce)

  if (displacement.y <
      (std::tan(29.0 / 2 * DegreesToRadians) * start_displacement.z + scalar*radius)) {
    displacement.y =
            (GLfloat) std::tan(29.0 / 2 * DegreesToRadians) * start_displacement.z + scalar*radius;
    velocity.y = -velocity.y * 0.75f;
    velocity.x = velocity.x * 0.85f;
  }

    if (follow){
        light_position.x=displacement.x;
        light_position.y=displacement.y;
    }else{
        light_position=vec4(0, 0, 0,1);
    }
}

int main() {

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  GLFWwindow *window = glfwCreateWindow(xpix, ypix, "Homework 3", nullptr, nullptr);
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
