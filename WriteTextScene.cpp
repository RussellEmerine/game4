#include "WriteTextScene.hpp"
#include "util.hpp"
#include <iostream>

WriteTextScene::WriteTextScene(const WriteGlyphScene &scene) :
        WriteGlyphScene(scene) {
    font = hb_ft_font_create_referenced(face);
}

WriteTextScene::~WriteTextScene() {
    hb_font_destroy(font);
}

Scene::Transform *WriteTextScene::write_line(const std::string &s) {
    // based on https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
    hb_buffer_t *buffer;
    buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, s.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    
    hb_shape(font, buffer, nullptr, 0);
    
    unsigned int len = hb_buffer_get_length(buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buffer, nullptr);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buffer, nullptr);
    
    lines.emplace_back();
    TextLine &line = lines.back();
    line.transforms.resize(len);
    
    {
        float x = 0, y = 0;
        auto transform = line.transforms.begin();
        for (unsigned int i = 0; i < len; i++, transform++) {
            hb_codepoint_t gid = info[i].codepoint;
            char buf[64];
            if (FT_Get_Glyph_Name(face, gid, buf, 64)) {
                std::cerr << "Warning: problem getting glyph name for " << gid << "\n";
            }
            std::string name(buf);
            
            transform->parent = &line.base;
            transform->position.x = x + PIXEL_SCALE * (float) pos[i].x_offset / 64.0f;
            transform->position.y = y + PIXEL_SCALE * (float) pos[i].y_offset / 64.0f;
            // gotta love the &* operator
            write_glyph_at(&*transform, name);
            
            x += PIXEL_SCALE * (float) pos[i].x_advance / 64.0f;
            y += PIXEL_SCALE * (float) pos[i].y_advance / 64.0f;
        }
    }
    
    hb_buffer_destroy(buffer);
    
    return &line.base;
}

void WriteTextScene::erase_line(Scene::Transform *transform) {
    for (TextLine &line: lines) {
        if (&line.base == transform) {
            for (Transform &t: line.transforms) {
                erase_glyph_at(&t);
            }
        }
    }
}
