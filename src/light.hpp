#ifndef LIGHT_HPP
#define LIGHT_HPP

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

typedef vec4 color4;
typedef vec4 point4;

// Sphere constants

class Light {
  vec4 light_position = vec4(0, 0, 0, 1);
  color4 light_ambient = color4(0.2, 0.2, 0.2, 1.0);
  color4 light_diffuse = color4(1.0, 1.0, 1.0, 1.0);
  color4 light_specular = color4(1.0, 1.0, 1.0, 1.0);
  bool direction = false;

public:
  Light() {}
  Light(vec4 position, color4 ambient, color4 diffuse, color4 specular)
      : light_position(position), light_ambient(ambient),
        light_diffuse(diffuse), light_specular(specular) {}

  vec4 get_position() { return light_position; }
  color4 get_ambient() { return light_ambient; }
  color4 get_diffuse() { return light_diffuse; }
  color4 get_specular() { return light_specular; }

  void move() {
    if (light_position.x <= -14) {
      direction = true;
    } else if (light_position.x >= 14) {
      direction = false;
    }

    if (direction) {
      light_position.x += 0.1;
    }else{
        light_position.x -= 0.1;
    }


      light_position=RotateY(0.1)*light_position;
  }

};

#endif
