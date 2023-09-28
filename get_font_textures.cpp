#include "get_font_textures.hpp"
#include "read_write_chunk.hpp"
#include "gl_errors.hpp"
#include <fstream>

// modeled after Mesh.cpp

std::map<std::string, GLuint> get_font_textures(std::string const &filename) {
    std::map<std::string, GLuint> textures;
    
    std::ifstream file(filename, std::ios::binary);
    
    GLuint total = 0;
    
    std::vector<uint8_t> data;
    // I copy all the alpha values to here because OpenGL understands RGBA but not just A
    std::vector<uint8_t> colors;
    
    if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".txtr") {
        read_chunk(file, "txtr", &data);
        
        for (uint8_t alpha: data) {
            colors.push_back(0);
            colors.push_back(0);
            colors.push_back(0);
            colors.push_back(alpha);
        }
        
        total = GLuint(data.size());
    }
    
    std::vector<char> strings;
    read_chunk(file, "str0", &strings);
    
    {
        struct TexIndexEntry {
            uint32_t name_begin, name_end;
            uint32_t tex_begin, tex_end;
            uint32_t width, height;
        };
        static_assert(sizeof(TexIndexEntry) == 24, "TexIndexEntry should be packed");
        
        std::vector<TexIndexEntry> index;
        read_chunk(file, "idx1", &index);
        
        for (auto const &entry: index) {
            if (!(entry.name_begin <= entry.name_end && entry.name_end <= strings.size())) {
                throw std::runtime_error("index entry has out-of-range name begin/end");
            }
            if (!(entry.tex_begin <= entry.tex_end && entry.tex_end <= total)) {
                throw std::runtime_error("index entry has out-of-range tex start/count");
            }
            std::string name(&strings[0] + entry.name_begin, &strings[0] + entry.name_end);
            
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) entry.width, (GLsizei) entry.height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, &colors[4 * entry.tex_begin]);
            // took me waaay too long to figure out i needed the parameteri things
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            textures[name] = texture;
            glBindTexture(GL_TEXTURE_2D, 0);
            
            GL_ERRORS();
        }
    }
    
    return textures;
}
