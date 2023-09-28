#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include "Scene.hpp"
#include "get_font_textures.hpp"
#include "Mesh.hpp"

struct WriteGlyphScene : Scene {
    FT_Library library{};
    FT_Face face{};
    
    MeshBuffer font_meshes;
    GLuint font_program;
    std::map<std::string, GLuint> textures;
    
    WriteGlyphScene(std::string const &filename,
                    std::function<void(Scene &, Transform *, std::string const &)> const &on_drawable,
                    std::string const &font_ttf,
                    std::string const &font_pnct,
                    std::string const &font_txtr);
    
    ~WriteGlyphScene();
    
    void write_glyph_at(Transform *transform, std::string const &glyph_name);
    
    void erase_glyph_at(Transform *transform);
};
