//
//  Display a rotating cube
//

#include "Angel.h"

#include <vector>

typedef vec4  color4;
typedef vec4  point4;

GLuint vao1;
GLuint vao2;

const GLfloat radius = 0.05;
const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)
const int xpix = 1280;
const int ypix = 720;

const GLfloat screen_ratio = (GLfloat) xpix / (GLfloat) ypix;

GLuint program = 0;

void create_sphere();

point4 cube_points[NumVertices];
color4 cube_colors[NumVertices];

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[8] = {
  point4( -radius, -radius,  radius, 1.0 ),
  point4( -radius,  radius,  radius, 1.0 ),
  point4(  radius,  radius,  radius, 1.0 ),
  point4(  radius, -radius,  radius, 1.0 ),
  point4( -radius, -radius, -radius, 1.0 ),
  point4( -radius,  radius, -radius, 1.0 ),
  point4(  radius,  radius, -radius, 1.0 ),
  point4(  radius, -radius, -radius, 1.0 )
};

// RGBA olors
color4 vertex_colors[8] = {
  color4( 0.0, 0.0, 0.0, 1.0 ),  // black
  color4( 1.0, 0.0, 0.0, 1.0 ),  // red
  color4( 1.0, 1.0, 0.0, 1.0 ),  // yellow
  color4( 0.0, 1.0, 0.0, 1.0 ),  // green
  color4( 0.0, 0.0, 1.0, 1.0 ),  // blue
  color4( 1.0, 0.0, 1.0, 1.0 ),  // magenta
  color4( 1.0, 1.0, 1.0, 1.0 ),  // white
  color4( 0.0, 1.0, 1.0, 1.0 )   // cyan
};

std::vector<color4> sphere_colors;
std::vector<vec4>   sphere_points;



// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
enum { Cube = 0, Sphere = 1, Rabbit = 2, NumObjectTypes};
enum { Frame = 0, Solid = 1, NumRenderModes = 2};


int      RenderMode = Solid;
int      ObjectType = Sphere;
int      Axis = Xaxis;
GLfloat  Theta[NumAxes] = { 0.0, 0.0, 0.0 };

// Model-view and projection matrices uniform location
GLuint  ModelView, Projection;

//----------------------------------------------------------------------------

// quad generates two triangles for each face and assigns colors to the vertices
int Index = 0;

void quad( int a, int b, int c, int d )
{
  cube_colors[Index] = vertex_colors[a]; cube_points[Index] = vertices[a]; Index++;
  cube_colors[Index] = vertex_colors[b]; cube_points[Index] = vertices[b]; Index++;
  cube_colors[Index] = vertex_colors[c]; cube_points[Index] = vertices[c]; Index++;
  cube_colors[Index] = vertex_colors[a]; cube_points[Index] = vertices[a]; Index++;
  cube_colors[Index] = vertex_colors[c]; cube_points[Index] = vertices[c]; Index++;
  cube_colors[Index] = vertex_colors[d]; cube_points[Index] = vertices[d]; Index++;
}

//----------------------------------------------------------------------------

// generate 12 triangles: 36 vertices and 36 colors

void colorcube()
{
  quad( 1, 0, 3, 2 );
  quad( 2, 3, 7, 6 );
  quad( 3, 0, 4, 7 );
  quad( 6, 5, 1, 2 );
  quad( 4, 5, 6, 7 );
  quad( 5, 4, 0, 1 );
}

//---------------------------------------------------------------------
//
// init
//

GLuint cube_buffer;
GLuint sphere_buffer;


void display_sphere_buffer() {
  glBindVertexArray( vao1 );
  glBindBuffer( GL_ARRAY_BUFFER, sphere_buffer );
  glBufferData( GL_ARRAY_BUFFER, sizeof(sphere_points) + sizeof(sphere_colors), NULL, GL_STATIC_DRAW );
  glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(sphere_points), sphere_points.data() );
  glBufferSubData( GL_ARRAY_BUFFER, sizeof(sphere_points), sizeof(sphere_colors), sphere_colors.data() );

  // set up vertex arrays
  GLuint vPosition = glGetAttribLocation( program, "vPosition" );
  glEnableVertexAttribArray( vPosition );
  glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

  GLuint vColor = glGetAttribLocation( program, "vColor" );
  glEnableVertexAttribArray( vColor );
  glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(sphere_points)) );
}

void display_cube_buffer() {
  glBindVertexArray( vao2 );
  glBindBuffer( GL_ARRAY_BUFFER, cube_buffer );
  glBufferData( GL_ARRAY_BUFFER, sizeof(cube_points) + sizeof(cube_colors), NULL, GL_STATIC_DRAW );
  glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(cube_points), cube_points );
  glBufferSubData( GL_ARRAY_BUFFER, sizeof(cube_points), sizeof(cube_colors), cube_colors );

  // set up vertex arrays
  GLuint vPosition = glGetAttribLocation( program, "vPosition" );
  glEnableVertexAttribArray( vPosition );
  glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

  GLuint vColor = glGetAttribLocation( program, "vColor" );
  glEnableVertexAttribArray( vColor );
  glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(cube_points)) );
}


void errorCallback(int error, const char* description) {
  std::cerr << "OpenGL error " << error << ": " << description << std::endl;
}



void init()
{

  // Load shaders and use the resulting shader program
  program = InitShader( "shaders/vshader.glsl", "shaders/fshader.glsl" );
  glUseProgram( program );

  colorcube(); // create the cube in terms of 6 faces each of which is made of two triangles
  create_sphere();

  // Create a vertex array object
  glGenVertexArrays( 1, &vao1 );


  glGenBuffers( 1, &sphere_buffer );

  glGenVertexArrays( 1, &vao2 );


  // Create and initialize a buffer object
  glGenBuffers( 1, &cube_buffer );

  display_sphere_buffer();

  // Retrieve transformation uniform variable locations
  ModelView = glGetUniformLocation( program, "ModelView" );
  Projection = glGetUniformLocation( program, "Projection" );

  // Set projection matrix
  mat4  projection;
  projection = Ortho(-1.0 * (screen_ratio), 1.0 * (screen_ratio), -1.0, 1.0, -1.0, 1.0); // Ortho(): user-defined function in mat.h
  glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );

  glEnable( GL_DEPTH_TEST );
  glClearColor( 1.0, 1.0, 1.0, 1.0 );
}

//---------------------------------------------------------------------
//
// display
//

vec3 start_displacement( -1.0 * screen_ratio + radius, 1.0 - radius, 0.0 );
vec3 displacement(start_displacement);
vec3 start_velocity(1.0, 0.0, 0.0);
vec3 velocity(start_velocity);
const GLfloat y_acceleration = -9.81;

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //  Generate the model-view matrix

  mat4  model_view = (  Translate( displacement ) * Scale(1.0, 1.0, 1.0) *
      RotateX( Theta[Xaxis] ) *
      RotateY( Theta[Yaxis] ) *
      RotateZ( Theta[Zaxis] ) );  // Scale(), Translate(), RotateX(), RotateY(), RotateZ(): user-defined functions in mat.h

  glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view );
  // TODO: change the num vertices when switching between sphere and cube
  switch ( ObjectType ) {
    case Sphere: case Rabbit: glDrawArrays( GL_TRIANGLES, 0, 660.0 / 3 ); ;break;
    case Cube: glDrawArrays( GL_TRIANGLES, 0, NumVertices );;break;
  }

  glFlush();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  switch( key ) {
    case GLFW_KEY_ESCAPE: case GLFW_KEY_Q:
      exit( EXIT_SUCCESS );
      break;
    case GLFW_KEY_I:
      displacement = start_displacement;
      velocity = start_velocity;
  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  if ( action == GLFW_PRESS ) {
    switch( button ) {
      case GLFW_MOUSE_BUTTON_RIGHT: ObjectType = (ObjectType + 1) % NumObjectTypes;  break;
      case GLFW_MOUSE_BUTTON_LEFT:  RenderMode = (RenderMode + 1) % NumRenderModes;  break;
    }
  }
}


void create_sphere() {
  int sectorCount = 10;
  int stackCount = 10;

  std::vector<vec4> sphere_vertices;
  std::vector<int>  sphere_indices;

  // clear memory of prev arrays
  std::vector<vec4>().swap(sphere_vertices);

  float x, y, z, xy;                              // vertex position

  float sectorStep = 2 * M_PI / sectorCount;
  float stackStep = M_PI / stackCount;
  float sectorAngle, stackAngle;
  int k1, k2;

  for(int i = 0; i <= stackCount; ++i)
  {

    stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
    xy = radius * cosf(stackAngle);             // r * cos(u)
    z = radius * sinf(stackAngle);              // r * sin(u)
                                                //
    k1 = i * (sectorCount + 1);     // beginning of current stack
    k2 = k1 + sectorCount + 1;      // beginning of next stack

    // add (sectorCount+1) vertices per stack
    // the first and last vertices have same position and normal, but different tex coords
    for(int j = 0; j <= sectorCount; ++j)
    {
      sectorAngle = j * sectorStep;           // starting from 0 to 2pi

      // vertex position (x, y, z)
      x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
      y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
      sphere_vertices.push_back(vec4(x, y, z, 1.0));

      if(i != 0)
      {
        sphere_indices.push_back(k1);
        sphere_indices.push_back(k2);
        sphere_indices.push_back(k1 + 1);
      }

      if(i != (stackCount-1))
      {
        sphere_indices.push_back(k1 + 1);
        sphere_indices.push_back(k2);
        sphere_indices.push_back(k2 + 1);
      }
    }
  }

  for (auto i : sphere_indices) {
    sphere_points.push_back(sphere_vertices[i]);
    sphere_colors.push_back(vertex_colors[0]);
  }
}

GLfloat dt = 0.01;

void update( void )
{

  switch( RenderMode ) {
    // TODO: maybe recheck if it can be done only for the front
    case Solid: glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); break;
    case Frame: glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); break;
  }

  switch( ObjectType ) {
    case Cube:   display_cube_buffer(); break;
    case Sphere: case Rabbit: display_sphere_buffer(); break;
  }

  // compute the effect of gravity on the cube
  displacement.y = displacement.y + velocity.y * dt + 0.5 * y_acceleration * dt * dt;
  displacement.x = displacement.x + velocity.x * dt;

  // keep the cube moving horizontally
  velocity.y = velocity.y + y_acceleration * dt;

  // bounce the cube off the floor
  if ( displacement.y <  -1.0 + radius) {
    displacement.y = -1.0 + radius;
    velocity.y = -velocity.y * 0.75;
  }
}

int main()
{
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  GLFWwindow* window = glfwCreateWindow(xpix, ypix, "Spin Cube", NULL, NULL);
  glfwMakeContextCurrent(window);

  if (!window)
  {
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


  while (!glfwWindowShouldClose(window))
  {
    currentTime = glfwGetTime();
    if (currentTime - previousTime >= 1/frameRate){
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
