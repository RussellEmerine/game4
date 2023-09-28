#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "WriteTextScene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <array>
#include <algorithm>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

struct PlayMode : Mode {
    PlayMode();
    
    ~PlayMode() override;
    
    //functions called by main loop:
    bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    
    void update(float elapsed) override;
    
    void draw(glm::uvec2 const &drawable_size) override;
    
    //----- game state -----
    
    //input tracking:
    struct Button {
        uint8_t downs = 0;
        uint8_t pressed = 0;
    } left, right, down, up;
    
    //local copy of the game scene (so code can change it during gameplay):
    WriteTextScene scene;
    
    //camera:
    Scene::Camera *camera = nullptr;
    
    Scene::Transform
            *cube,
            *center_cube,
            *cone,
            *center_cone,
            *icosphere,
            *center_icosphere,
            *torus,
            *center_torus,
            *cube_name_line,
            *cube_name_plural_line,
            *cone_name_line,
            *cone_name_plural_line,
            *icosphere_name_line,
            *icosphere_name_plural_line,
            *torus_name_line,
            *name_me_line,
            *goal_line;
    
    size_t level = 0;
    bool done = false;
    std::string torus_name;
    
    static constexpr size_t LEVEL_COUNT = 4;
    const std::array<std::string, LEVEL_COUNT> cube_name = {
            "ėnevað",
            "bana",
            "emilan",
            "konop"
    };
    const std::array<std::string, LEVEL_COUNT> cone_name = {
            "raðan",
            "ndirino",
            "kiphilon",
            "hungildaf"
    };
    const std::array<std::string, LEVEL_COUNT> icosphere_name = {
            "jamėk",
            "anudu",
            "tantu",
            "alfinostak"
    };
    const std::array<std::string, LEVEL_COUNT> goal_name = {
            "laano",
            "sisini",
            "bulanaph",
            "stroumet"
    };
    
    void reload_names();
    
    void reload_torus_name();
};
