//
// TODOS:
// - [X] Create ground (phase-1)
// - [X] Create wall   (phase-1)
// - [X] Create static spheres
// - [X] Cast the shadow of a static object on the wall/ground (phase-2)
// - [X] Cast the shadow of a dynamic object on the wall/ground (phase-2)
// - [X] Create a complex object to cast shadow on (phase-3)
// - [X]dwasdawdsawdas Cast the shadow on the complex object (phase-2)

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

#include "cube.hpp"
#include "sphere.hpp"

using namespace std;

// Window size in pixels
const int xpix = 1280;
const int ypix = 960;
GLfloat far_plane = 60.0f;
const GLfloat screen_ratio = (GLfloat) xpix / (GLfloat) ypix;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
bool follow = false;

// Shadow mapping vars
unsigned int depthCubemap;
unsigned int depthMapFBO;

// Object size modifier
const GLfloat radius = 0.1;
bool shadows = false;

// The number of vertices in each object

typedef vec4 color4;
typedef vec4 point4;

GLuint textures[3];
GLint textureSelect = 0;
std::vector<GLubyte> earth_image;
GLint earth_image_size[2];
std::vector<GLubyte> basketball_image;
GLint basketball_image_size[2];
std::vector<GLubyte> image3;

enum {
    Shading = 0, Wireframe = 1, Texture = 2, NumRenderModes = 3
};

enum {
    NO_SPECULAR = 0, NO_DIFFUSE = 1, NO_AMBIENT = 2, ALL = 3, NumShadingModes = 4
};

enum {
    Metalic = 0, Plastic = 1, NumMaterialTypes = 2
};

GLint renderMode = Shading;
GLint shadingMode = ALL;
GLboolean shadingType = true;
GLint materialType = Metalic;
GLboolean textureType = true;

// Model-view and projection matrices uniform location
GLint ModelView, Projection, renderModeLoc, isBlackLoc, shadingTypeLoc, textureTypeLoc, tex2DSamplerLoc, tex1DSamplerLoc;

GLuint og_program = 0;
GLuint sh_program = 0;

// Have an error callback to make sure to catch any errors
void errorCallback(int error, const char *description) {
    std::cerr << "OpenGL error " << error << ": " << description << std::endl;
}

mat4 projection;
mat4 sh_projection;

color4 colors[7] = {color4(0.0, 0.0, 0.0, 1.0), // black
                    color4(1.0, 0.0, 0.0, 1.0), // red
                    color4(1.0, 1.0, 0.0, 1.0), // yellow
                    color4(0.0, 1.0, 0.0, 1.0), // green
                    color4(0.0, 0.0, 1.0, 1.0), // blue
                    color4(1.0, 1.0, 1.0, 1.0), // white
                    color4(1.0, 0.5, 0.0, 1.0), // orange
};

void createwhiteimage() {
    for (int i = 0; i < 32*32; ++i) {
        image3.emplace_back(255);
        image3.emplace_back(255);
        image3.emplace_back(255);
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

Light light;
Sphere sphere1(2.0f, vec3(5, 2, -10), vector<vec4>({colors[5] * 0.1, colors[5] * 0.7, colors[5] * 0.2}), 50.0f);
Sphere sphere2(4.0f, vec3(-8, -2, -15), vector<vec4>({colors[5] * 0.1, colors[5] * 0.7, colors[5] * 0.2}), 5.0f);
Sphere sphere3(1.0f, vec3(0, 0, -7), vector<vec4>({colors[5] * 0.1, colors[5] * 0.7, colors[5] * 0.2}), 100.0f);


Cube cube(15, vec4(0, 0, -14, 1), vector<vec4>({colors[1], colors[2], colors[3], colors[4], colors[5], colors[6]}));

void init() {

    readPPMImage("assets/earth.ppm", earth_image, earth_image_size);
    readPPMImage("assets/basketball.ppm", basketball_image, basketball_image_size);
    createwhiteimage();

    glGenTextures(1, &depthCubemap);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

    glGenFramebuffers(1, &depthMapFBO);

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(3, textures);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, textures[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR); // try here different alternatives
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR); // try here different alternatives
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, earth_image_size[0], earth_image_size[1], 0,
                 GL_RGB, GL_UNSIGNED_BYTE, earth_image.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR); // try here different alternatives
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR); // try here different alternatives
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, basketball_image_size[0], basketball_image_size[1], 0,
                 GL_RGB, GL_UNSIGNED_BYTE, basketball_image.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR); // try here different alternatives
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR); // try here different alternatives
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image3.data());

    sh_program = InitShaderWithGeo("shaders/sh_vshader.glsl", "shaders/sh_gshader.glsl", "shaders/sh_fshader.glsl");

    glUniform1f(glGetUniformLocation(sh_program, "far_plane"), far_plane);

    og_program = InitShader("shaders/vshader.glsl", "shaders/fshader.glsl");

    sphere1.load(og_program);
    sphere2.load(og_program);
    sphere3.load(og_program);
    cube.load(og_program);

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(og_program, "ModelView");
    Projection = glGetUniformLocation(og_program, "Projection");
    shadingTypeLoc = glGetUniformLocation(og_program, "ShadingType");
    renderModeLoc = glGetUniformLocation(og_program, "RenderMode");
    isBlackLoc = glGetUniformLocation(og_program, "IsBlack");
    textureTypeLoc = glGetUniformLocation(og_program, "textureType");
    tex2DSamplerLoc = glGetUniformLocation(og_program, "tex2D");
    tex1DSamplerLoc = glGetUniformLocation(og_program, "tex1D");

    glUniform1f(glGetUniformLocation(og_program, "far_plane"), far_plane);
    glUniform1i(renderModeLoc, renderMode);
    glUniform1i(shadingTypeLoc, shadingType);
    glUniform1i(textureTypeLoc, textureType);
    glUniform1i(isBlackLoc, 0);
    glUniform1i(tex2DSamplerLoc, 1);
    glUniform1i(glGetUniformLocation(og_program, "depthMap"), 0);
    // Set projection matrix
    projection = Perspective(120, screen_ratio, 0.1, 100);
    sh_projection = Perspective(90, (float) SHADOW_WIDTH / (float) SHADOW_HEIGHT, 0.1, far_plane);

    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void key_callback(__attribute_maybe_unused__ GLFWwindow *window, int key, __attribute_maybe_unused__ int scancode,
                  int action, __attribute_maybe_unused__ int mods) {

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
                std::cout << "Press 'l' to toggle the light (fixed/following)" << std::endl;
                std::cout << "Press 'o' to disable/renable the ambient,diffuse and "
                             "specular properties" << std::endl;
                std::cout << "Press 't' to switch between solid, wireframe, and texture views" << std::endl;
                std::cout << "Press 's' to switch between, Gouraud and Phong shading" << std::endl;
                std::cout << "Press 'i' to switch between, the various textures (earth, "
                             "basketball, 1d)" << std::endl;
                std::cout << "Press 'm' to switch between metalic and plastic materials" << std::endl;
                std::cout << "Press 'w' or 'z' to zoom in or out" << std::endl;
                std::cout << "Press 'x' to toggle shadows (not fully complete)" << std::endl;
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
// This function creates a sphere1 with a given stack and sector count
// (specified by the constants STACK_COUNT and SECTOR_COUNT)
// Adopted from: http://www.songho.ca/opengl/gl_sphere.html

GLfloat dt = 0.01;

void update() {

    light.move();
    sphere1.move();
    sphere2.move();
    sphere3.move();
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

    // Generate Cube depth map texture
    // --------------------------------

    vector<mat4> shadowTransforms;
    shadowTransforms.push_back(sh_projection *
                               LookAt(light.get_position(), light.get_position() + vec3(1.0f, 0.0f, 0.0f),
                                      vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(sh_projection *
                               LookAt(light.get_position(), light.get_position() + vec3(-1.0f, 0.0f, 0.0f),
                                      vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(sh_projection *
                               LookAt(light.get_position(), light.get_position() + vec3(0.0f, 1.0f, 0.0f),
                                      vec3(0.0f, 0.0f, 1.0f)));
    shadowTransforms.push_back(sh_projection *
                               LookAt(light.get_position(), light.get_position() + vec3(0.0f, -1.0f, 0.0f),
                                      vec3(0.0f, 0.0f, -1.0f)));
    shadowTransforms.push_back(sh_projection *
                               LookAt(light.get_position(), light.get_position() + vec3(0.0f, 0.0f, 1.0f),
                                      vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(sh_projection *
                               LookAt(light.get_position(), light.get_position() + vec3(0.0f, 0.0f, -1.0f),
                                      vec3(0.0f, -1.0f, 0.0f)));

    // 1. render scene to depth cubemap
    // --------------------------------

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(sh_program);
    for (unsigned int i = 0; i < 6; ++i) {
        GLuint faceShadowMatrix = glGetUniformLocation(sh_program,
                                                       ("shadowMatrices[" + std::to_string(i) + "]").c_str());

        glUniformMatrix4fv(faceShadowMatrix, 1, GL_TRUE, shadowTransforms[i]);
    }

    glUniform4fv(glGetUniformLocation(sh_program, "LightPosition"), 1, light.get_position());

    cube.draw(sh_program, light);
    sphere1.draw(sh_program, light);
    sphere2.draw(sh_program, light);
    sphere3.draw(sh_program, light);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, xpix, ypix);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(og_program);

    glUniform4fv(glGetUniformLocation(og_program, "LightPosition"), 1, light.get_position());

    glBindTexture(GL_TEXTURE_2D, textures[2]);
    cube.draw(og_program, light);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    sphere1.draw(og_program, light);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    sphere2.draw(og_program, light);
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    sphere3.draw(og_program, light);
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

    GLFWwindow *window = glfwCreateWindow(xpix, ypix, "Dynamic Shadow Demo", nullptr, nullptr);
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

     double frameRate = 60, currentTime, previousTime = 0.0;

    while (!glfwWindowShouldClose(window)) {
         currentTime = glfwGetTime();
         if (currentTime - previousTime >= 1 / frameRate) {
            previousTime = currentTime;
             update();
             glfwSwapBuffers(window);
         }
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
