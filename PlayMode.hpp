#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "WriteGlyphScene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

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
    WriteGlyphScene scene;
    
    //hexapod leg to wobble:
    Scene::Transform *hip = nullptr;
    Scene::Transform *upper_leg = nullptr;
    Scene::Transform *lower_leg = nullptr;
    glm::quat hip_base_rotation;
    glm::quat upper_leg_base_rotation;
    glm::quat lower_leg_base_rotation;
    float wobble = 0.0f;
    
    glm::vec3 get_leg_tip_position();
    
    //music coming from the tip of the leg (as a demonstration):
    std::shared_ptr<Sound::PlayingSample> leg_tip_loop;
    
    //camera:
    Scene::Camera *camera = nullptr;
};
