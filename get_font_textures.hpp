#pragma once

#include <map>
#include <string>
#include "GL.hpp"

std::map<std::string, GLuint> get_font_textures(std::string const &filename);
