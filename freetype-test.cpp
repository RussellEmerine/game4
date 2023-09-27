#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>
#include <fstream>
#include <cassert>
#include <glm/glm.hpp>

#include "data_path.hpp"
#include "read_write_chunk.hpp"

// TODO: put constants in a common file
constexpr size_t PIXEL_COUNT = 36;
constexpr float PIXEL_SCALE = 0.1;

int main(int argc, char **argv) {
    FT_Library ft_library;
    FT_Face face;
    
    if (FT_Init_FreeType(&ft_library)) {
        assert(false && "Problem initializing FreeType");
    };
    // TODO: copy fonts into dist
    if (FT_New_Face(ft_library,
                    data_path("fonts/Inknut_Antiqua/InknutAntiqua-Regular.ttf").c_str(),
                    0, &face)) {
        assert(false && "Problem initializing font");
    }
    // TODO: fix magic numbers
    if (FT_Set_Char_Size(face, PIXEL_COUNT * 64, 0, 0, 0)) {
        assert(false && "Problem setting character size");
    }
    
    std::cout << "Inknut Antiqua provides " << face->num_glyphs << " glyphs.\n";
    
    /*
     * This next part realizes the glyph in .pnct format.
     * The process is based on the MeshBuffer constructor, inverted.
     * The structures are taken directly from Mesh.cpp.
     */
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::u8vec4 Color;
        glm::vec2 TexCoord;
        
        /*
         * Constructs a very plain very boring vertex
         *
         * (Not even sure what TexCoord is exactly but I'll assume position is fine there)
         */
        explicit Vertex(glm::vec3 position) :
                Position(position),
                Normal(0.0, 0.0, 1.0),
                Color(0x00, 0x00, 0x00, 0xff),
                TexCoord(position) {}
    };
    static_assert(sizeof(Vertex) == 3 * 4 + 3 * 4 + 4 * 1 + 2 * 4, "Vertex is packed.");
    
    struct IndexEntry {
        uint32_t name_begin, name_end;
        uint32_t vertex_begin, vertex_end;
    };
    static_assert(sizeof(IndexEntry) == 16, "Index entry should be packed");
    
    std::vector<Vertex> data;
    std::vector<char> strings;
    std::vector<IndexEntry> index;
    
    size_t glyph_count = 0;
    // Loop through all the glyphs and render them in .pnct format.
    // I couldn't find what glyph indices are officially valid, but it seems like this works for the font I'm using.
    for (FT_Long glyph_index = 0; glyph_index < face->num_glyphs; glyph_index++) {
        /*
         * FT_LOAD_RENDER ensures the bitmap is filled.
         * There are other ways to do the drawing using a callback system, but as far as I can tell that only works
         * on outlines. This approach relies on FreeType's shipped renderer and should work for everything.
         *
         * I make sure to load monochrome because I don't have a great way to convert grayness into triangles.
         * So, I should used the highest-contrast setting instead. I can always make the shape bigger for more accuracy.
         */
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO)) {
            std::cerr << "Warning: problem getting glyph with index " << glyph_index << " (out of "
                      << face->num_glyphs
                      << "glyphs)\n";
        } else {
            glyph_count++;
        }
        
        /*
         * There's probably not an official limit anywhere since anyone can make a font, but
         * - https://github.com/adobe-type-tools/agl-specification
         * - https://learn.microsoft.com/en-us/typography/opentype/spec/post
         * - http://adobe-type-tools.github.io/afdko/OpenTypeFeatureFileSpecification.html#2fi-glyph-name
         * agree that it's maximum 63 characters. So, 64 for the 63 and a null terminator.
         */
        char buf[64];
        /*
         * From the FreeType documentation https://freetype.org/freetype2/docs/reference/ft2-information_retrieval.html#ft_get_glyph_name:
         *
         * This function has limited capabilities if the config macro FT_CONFIG_OPTION_POSTSCRIPT_NAMES is not defined in ftoption.h:
         * It then works only for fonts that actually embed glyph names (which many recent OpenType fonts do not).
         *
         * This flag is defined on the MacOS distribution of nest-libs. It should be checked on other versions.
         */
        if (FT_Get_Glyph_Name(face, glyph_index, buf, 64)) {
            assert(false && "Problem getting glyph name");
        }
        std::string name(buf);
        std::cout << "Found name \"" << name << "\" for index " << glyph_index << "\n";
        
        uint32_t name_begin = strings.size();
        strings.insert(strings.end(), name.begin(), name.end());
        uint32_t name_end = strings.size();
        
        uint32_t vertex_begin = data.size();
        // loop through every pixel
        {
            int r = 0;
            FT_Bitmap &bitmap = face->glyph->bitmap;
            for (unsigned char *row = bitmap.buffer; r < bitmap.rows; row = &row[bitmap.pitch], r++) {
                // remember, it was rendered in mono so every pixel is one bit
                for (int c = 0; c < bitmap.width; c++) {
                    //   the byte's...    nth bit
                    if ((row[c / 8] >> (c % 8)) & 1) {
                        // draw the first triangle
                        data.emplace_back(glm::vec3((float) (face->glyph->bitmap_top + r) / PIXEL_SCALE,
                                                    (float) (face->glyph->bitmap_left + c) / PIXEL_SCALE,
                                                    0));
                        data.emplace_back(glm::vec3((float) (face->glyph->bitmap_top + r) / PIXEL_SCALE + PIXEL_SCALE,
                                                    (float) (face->glyph->bitmap_left + c) / PIXEL_SCALE,
                                                    0));
                        data.emplace_back(glm::vec3((float) (face->glyph->bitmap_top + r) / PIXEL_SCALE + PIXEL_SCALE,
                                                    (float) (face->glyph->bitmap_left + c) / PIXEL_SCALE + PIXEL_SCALE,
                                                    0));
                        // draw the second triangle
                        data.emplace_back(glm::vec3((float) (face->glyph->bitmap_top + r) / PIXEL_SCALE,
                                                    (float) (face->glyph->bitmap_left + c) / PIXEL_SCALE,
                                                    0));
                        data.emplace_back(glm::vec3((float) (face->glyph->bitmap_top + r) / PIXEL_SCALE + PIXEL_SCALE,
                                                    (float) (face->glyph->bitmap_left + c) / PIXEL_SCALE + PIXEL_SCALE,
                                                    0));
                        data.emplace_back(glm::vec3((float) (face->glyph->bitmap_top + r) / PIXEL_SCALE,
                                                    (float) (face->glyph->bitmap_left + c) / PIXEL_SCALE + PIXEL_SCALE,
                                                    0));
                    }
                }
            }
        }
        uint32_t vertex_end = data.size();
        
        index.push_back({name_begin, name_end, vertex_begin, vertex_end});
    }
    
    std::cout << "Number of vertices is " << data.size() << "\n";
    
    std::ofstream out(data_path("dist/Inknut_Antiqua.pnct"), std::ios::binary);
    write_chunk("pnct", data, &out);
    write_chunk("str0", strings, &out);
    write_chunk("idx0", index, &out);
    
    std::cout << glyph_count << " glyphs recognized for index under " << face->num_glyphs << "\n";
    
    if (FT_Done_Face(face)) {
        assert(false && "Problem destroying face");
    }
    if (FT_Done_FreeType(ft_library)) {
        assert(false && "Problem destroying library");
    }
}
