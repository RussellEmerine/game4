#include "WriteGlyphScene.hpp"
#include "data_path.hpp"
#include "util.hpp"
#include "LitColorTextureProgram.hpp"

#include <iostream>

WriteGlyphScene::WriteGlyphScene(
        std::string const &filename,
        std::function<void(Scene &, Transform *, std::string const &)> const &on_drawable,
        std::string const &font_ttf,
        std::string const &font_pnct,
        std::string const &font_txtr
) : Scene(filename, on_drawable),
    font_meshes(font_pnct),
    font_program(font_meshes.make_vao_for_program(lit_color_texture_program->program)),
    textures(get_font_textures(font_txtr)) {
    // wouldn't it be nice if there was a standardized autoformatter and i didn't ever have to think about formatting ever again? we tend to say the same thing about package managers
    if (FT_Init_FreeType(&library)) {
        assert(false && "Problem initializing FreeType");
    }
    if (FT_New_Face(library, font_ttf.c_str(), 0, &face)) {
        assert(false && "Problem initializing font");
    }
    if (FT_Set_Char_Size(face, PIXEL_COUNT * 64, 0, 0, 0)) {
        assert(false && "Problem setting character size");
    }
}

WriteGlyphScene::~WriteGlyphScene() {
    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

/*
 * Draws a glyph at a transform. (The transform need not be in the scene's transforms list.)
 */
void WriteGlyphScene::write_glyph_at(Transform *transform, std::string const &glyph_name) {
    Mesh mesh = font_meshes.lookup(glyph_name);
    assert(textures.count(glyph_name));
    GLuint texture = textures[glyph_name];

//    std::cerr << "Writing glyph " << glyph_name << " at " << transform->position.x << ", " << transform->position.y << "\n";
    
    drawables.emplace_back(transform);
    Drawable &drawable = drawables.back();
    
    drawable.pipeline = lit_color_texture_program_pipeline;
    drawable.pipeline.vao = font_program;
    drawable.pipeline.textures[0].texture = texture;
    drawable.pipeline.type = mesh.type;
    drawable.pipeline.start = mesh.start;
    drawable.pipeline.count = mesh.count;
}

/*
 * Erases a glyph based on its transform. (Technically can erase any drawable, oopsie)
 */
void WriteGlyphScene::erase_glyph_at(Scene::Transform *transform) {
    drawables.remove_if([transform](Drawable &drawable) { return drawable.transform == transform; });
}
