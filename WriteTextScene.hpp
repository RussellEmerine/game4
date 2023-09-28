#pragma once

#include "WriteGlyphScene.hpp"

struct TextLine {
    Scene::Transform base;
    std::list<Scene::Transform> transforms;
};

struct WriteTextScene : WriteGlyphScene {
    hb_font_t *font;
    std::list<TextLine> lines;
    
    explicit WriteTextScene(const WriteGlyphScene &scene);
    
    ~WriteTextScene();
    
    // Returns a Transform* that refers to base of the TextLine, which can be used to identify what to erase later.
    Transform *write_line(std::string const &s);
    
    // Erases the TextLine
    void erase_line(Transform *transform);
};
