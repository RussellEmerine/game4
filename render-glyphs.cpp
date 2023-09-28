/*
 * This file takes a .tff file, renders its glyphs using FreeType, and writes them to assets the program
 * can use. One output is a .pnct file that can be converted into a MeshBuffer, just like Blender exports.
 * There is one rectangle (two triangles really) for each glyph. The other file is a .txtr file, which is
 * a chunk-based format very similar to MeshBuffer, and is processed to produce a texture for each glyph.
 */

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <fstream>
#include <cassert>
#include <glm/glm.hpp>

#include "data_path.hpp"
#include "read_write_chunk.hpp"

// TODO: put constants in a common file
constexpr size_t PIXEL_COUNT = 200;
constexpr float PIXEL_SCALE = 0.01f;

int main(int argc, char **argv) {
    FT_Library ft_library;
    FT_Face face;
    
    if (FT_Init_FreeType(&ft_library)) {
        assert(false && "Problem initializing FreeType");
    }
    // TODO: copy fonts into dist
    if (FT_New_Face(ft_library,
                    data_path("fonts/Inknut_Antiqua/InknutAntiqua-Regular.ttf").c_str(),
                    0, &face)) {
        assert(false && "Problem initializing font");
    }
    // the unit is 1/64 pixel, so this is the right count of pixels.
    // 0 for char_height assumes the same as char_width.
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
        
        // Constructs a very plain very boring vertex
        explicit Vertex(glm::vec3 position, glm::vec2 tex_coord) :
                Position(position),
                Normal(0.0, 0.0, 1.0),
                Color(0x00, 0x00, 0x00, 0xff),
                TexCoord(tex_coord) {}
    };
    static_assert(sizeof(Vertex) == 3 * 4 + 3 * 4 + 4 * 1 + 2 * 4, "Vertex is packed.");
    
    struct IndexEntry {
        uint32_t name_begin, name_end;
        uint32_t vertex_begin, vertex_end;
    };
    static_assert(sizeof(IndexEntry) == 16, "IndexEntry should be packed");
    
    struct TexIndexEntry {
        uint32_t name_begin, name_end;
        uint32_t tex_begin, tex_end;
        uint32_t width, height;
    };
    static_assert(sizeof(TexIndexEntry) == 24, "TexIndexEntry should be packed");
    
    std::vector<Vertex> vertices;
    std::vector<uint8_t> texture_colors;
    std::vector<char> strings;
    std::vector<IndexEntry> vertex_indices;
    std::vector<TexIndexEntry> texture_indices;
    
    size_t glyph_count = 0;
    // Loop through all the glyphs and render them in .pnct and .txtr formats.
    // I couldn't find what glyph indices are officially valid, but it seems like this works for the font I'm using.
    for (FT_Long glyph_index = 0; glyph_index < face->num_glyphs; glyph_index++) {
        IndexEntry index_entry{};
        TexIndexEntry tex_index_entry{};
        { // Find the name and add it
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
                std::cerr << "Problem getting glyph name for index_entry " << glyph_index << "\n";
                continue;
            }
            std::string name(buf);
            // std::cout << "Found name \"" << name << "\" for index_entry " << glyph_index << "\n";
            
            index_entry.name_begin = (uint32_t) strings.size();
            tex_index_entry.name_begin = (uint32_t) strings.size();
            strings.insert(strings.end(), name.begin(), name.end());
            index_entry.name_end = (uint32_t) strings.size();
            tex_index_entry.name_end = (uint32_t) strings.size();
        }
        
        /*
         * FT_LOAD_RENDER ensures the bitmap is filled. The default is using FT_RENDER_MODE_NORMAL,
         * which means every byte is a pixel's alpha value.
         *
         * There are other ways to do the drawing using a callback system, but as far as I can tell that only works
         * on outlines. This approach relies on FreeType's shipped renderer and should work for all fonts.
         */
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER)) {
            std::cerr << "Warning: problem getting glyph with index_entry " << glyph_index << " (out of "
                      << face->num_glyphs
                      << "glyphs)\n";
        } else {
            glyph_count++;
        }
        
        { // Make the rectangle
            index_entry.vertex_begin = (uint32_t) vertices.size();
            // all of these are in pixels
            FT_Int top = face->glyph->bitmap_top;
            FT_Int left = face->glyph->bitmap_left;
            FT_Int bottom = top - (FT_Int) face->glyph->bitmap.rows;
            FT_Int right = left + (FT_Int) face->glyph->bitmap.width;
            
            // It's a little weird that I'm going top then bottom but it helps with the pixel loop later
            
            // draw the first triangle
            vertices.emplace_back(glm::vec3((float) left * PIXEL_SCALE,
                                            (float) top * PIXEL_SCALE,
                                            0),
                                  glm::vec2(0.0, 0.0));
            vertices.emplace_back(glm::vec3((float) right * PIXEL_SCALE,
                                            (float) top * PIXEL_SCALE,
                                            0),
                                  glm::vec2(1.0, 0.0));
            vertices.emplace_back(glm::vec3((float) right * PIXEL_SCALE,
                                            (float) bottom * PIXEL_SCALE,
                                            0),
                                  glm::vec2(1.0, 1.0));
            vertices.emplace_back(glm::vec3((float) left * PIXEL_SCALE,
                                            (float) top * PIXEL_SCALE,
                                            0),
                                  glm::vec2(0.0, 0.0));
            vertices.emplace_back(glm::vec3((float) right * PIXEL_SCALE,
                                            (float) bottom * PIXEL_SCALE,
                                            0),
                                  glm::vec2(1.0, 1.0));
            vertices.emplace_back(glm::vec3((float) left * PIXEL_SCALE,
                                            (float) bottom * PIXEL_SCALE,
                                            0),
                                  glm::vec2(0.0, 1.0));
            index_entry.vertex_end = (uint32_t) vertices.size();
        }
        
        { // Fill the texture
            FT_Bitmap &bitmap = face->glyph->bitmap;
            tex_index_entry.height = bitmap.rows;
            tex_index_entry.width = bitmap.width;
            
            tex_index_entry.tex_begin = (uint32_t) texture_colors.size();
            unsigned int r = 0;
            // filled in row by row, top-down
            for (uint8_t *row = bitmap.buffer; r < bitmap.rows; row = &row[bitmap.pitch], r++) {
                unsigned int c = 0;
                for (uint8_t *col = row; c < bitmap.width; col++, c++) {
                    texture_colors.push_back(*col);
                }
            }
            
            tex_index_entry.tex_end = (uint32_t) texture_colors.size();
        }
        
        texture_indices.push_back(tex_index_entry);
        vertex_indices.push_back(index_entry);
    }
    
    { // write rectangles to .pnct file
        std::ofstream out(data_path("dist/Inknut_Antiqua.pnct"), std::ios::binary);
        write_chunk("pnct", vertices, &out);
        write_chunk("str0", strings, &out);
        write_chunk("idx0", vertex_indices, &out);
    }
    
    { // write textures to texture file
        std::ofstream out(data_path("dist/Inknut_Antiqua.txtr"), std::ios::binary);
        write_chunk("txtr", texture_colors, &out);
        write_chunk("str0", strings, &out);
        write_chunk("idx1", texture_indices, &out);
    }
    
    std::cout << glyph_count << " glyphs recognized for vertex_indices under " << face->num_glyphs << "\n";
    
    if (FT_Done_Face(face)) {
        assert(false && "Problem destroying face");
    }
    if (FT_Done_FreeType(ft_library)) {
        assert(false && "Problem destroying library");
    }
}
