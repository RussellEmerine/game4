#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <locale>
#include "get_font_textures.hpp"
#include "WriteTextScene.hpp"

// this is actually very limited for my purposes, but i can't do better with the time i have
bool is_ascii_vowel(char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

std::string suffix(const std::string &s) {
    return s + "oś";
}

std::string reduplicate(const std::string &s) {
    size_t v = s.size();
    for (size_t i = 0; i < s.size(); i++) {
        if (is_ascii_vowel(s[i])) {
            v = i;
            break;
        }
    }
    if (v == s.size()) {
        return "";
    } else {
        return s.substr(0, v + 1) + s;
    }
}

std::string infix(const std::string &s) {
    size_t v = s.size();
    for (size_t i = 0; i < s.size(); i++) {
        if (is_ascii_vowel(s[i])) {
            v = i;
            break;
        }
    }
    if (v == s.size()) {
        return "";
    } else {
        return s.substr(0, v) + "al" + s.substr(v);
    }
}

char voice(char c) {
    switch (c) {
        case 'f':
            return 'v';
        case 'k':
            return 'g';
        case 'p':
            return 'b';
        case 's':
            return 'z';
        case 't':
            return 'd';
        default:
            return c;
    }
    // makes less clever compilers happy
    return c;
}

std::string umlaut_and_voice(const std::string &s) {
    std::string res;
    for (char c: s) {
        switch (c) {
            case 'a':
                res += "æ";
                break;
            case 'o':
                res += "œ";
                break;
            case 'u':
                res += "ø";
                break;
            default:
                res += c;
        }
    }
    if (s.empty()) {
        return "";
    }
    // this doesn't fail for non-ascii since it should just replace the wonky character code with itself
    res.back() = voice(res.back());
    return res + "ir";
}

std::array<std::function<std::string(const std::string &)>, PlayMode::LEVEL_COUNT> pluralize = {
        suffix,
        reduplicate,
        infix,
        umlaut_and_voice,
};

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
    MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
    hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
    return ret;
});

Load<WriteGlyphScene> hexapod_scene(LoadTagDefault, []() -> WriteGlyphScene const * {
    return new WriteGlyphScene(
            data_path("hexapod.scene"),
            [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
                Mesh const &mesh = hexapod_meshes->lookup(mesh_name);
                
                scene.drawables.emplace_back(transform);
                Scene::Drawable &drawable = scene.drawables.back();
                
                drawable.pipeline = lit_color_texture_program_pipeline;
                
                drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
                drawable.pipeline.type = mesh.type;
                drawable.pipeline.start = mesh.start;
                drawable.pipeline.count = mesh.count;
            },
            data_path("InknutAntiqua-Regular.ttf"),
            data_path("InknutAntiqua.pnct"),
            data_path("InknutAntiqua.txtr"));
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
    for (Scene::Transform &transform: scene.transforms) {
        if (transform.name == "Cube") {
            cube = &transform;
        } else if (transform.name == "Center Cube") {
            center_cube = &transform;
        } else if (transform.name == "Cone") {
            cone = &transform;
        } else if (transform.name == "Center Cone") {
            center_cone = &transform;
        } else if (transform.name == "Icosphere") {
            icosphere = &transform;
        } else if (transform.name == "Center Icosphere") {
            center_icosphere = &transform;
        } else if (transform.name == "Torus") {
            torus = &transform;
        } else if (transform.name == "Center Torus") {
            center_torus = &transform;
        }
    }
    
    // get pointer to camera for convenience:
    if (scene.cameras.size() != 1)
        throw std::runtime_error(
                "Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
    camera = &scene.cameras.front();
    
    // TODO: rotate stuff
    name_me_line = scene.write_line("Name me!");
    name_me_line->position.z = 10.0f;
    name_me_line->rotation = glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f))
                             * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    name_me_line->parent = torus;
    reload_names();
}

PlayMode::~PlayMode() = default;

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    if (evt.type == SDL_KEYDOWN) {
        if (evt.key.keysym.sym == SDLK_ESCAPE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            return true;
        } else if (evt.key.keysym.sym == SDLK_LEFT) {
            left.downs += 1;
            left.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_RIGHT) {
            right.downs += 1;
            right.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_UP) {
            up.downs += 1;
            up.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_DOWN) {
            down.downs += 1;
            down.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_BACKSPACE && !done) {
            if (!torus_name.empty()) {
                torus_name.pop_back();
            }
            reload_torus_name();
        } else {
            std::string keyname(SDL_GetKeyName(evt.key.keysym.sym));
            std::locale loc("C");
            if (keyname.size() == 1 && std::isalpha(std::tolower(keyname[0], loc), loc) && !done) {
                char c = std::tolower(keyname[0], loc);
                torus_name.push_back(c);
                if (torus_name == goal_name[level]) {
                    if (level + 1 < LEVEL_COUNT) {
                        level++;
                        reload_names();
                    } else {
                        reload_torus_name();
                        done = true;
                        Scene::Transform *t = scene.write_line("You won!");
                        t->position = glm::vec3(-10.0f, -20.0f, 10.0f);
                        t->rotation = glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f))
                                      * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
                        t->scale = glm::vec3(10.0, 10.0, 10.0);
                        t->parent = center_torus;
                    }
                } else {
                    reload_torus_name();
                }
            }
        }
    } else if (evt.type == SDL_KEYUP) {
        if (evt.key.keysym.sym == SDLK_LEFT) {
            left.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_RIGHT) {
            right.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_UP) {
            up.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_DOWN) {
            down.pressed = false;
            return true;
        }
    } else if (evt.type == SDL_MOUSEBUTTONDOWN) {
        if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            return true;
        }
    } else if (evt.type == SDL_MOUSEMOTION) {
        // copied from my game2
        if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
            glm::vec2 motion = glm::vec2(
                    float(evt.motion.xrel) / float(window_size.y),
                    -float(evt.motion.yrel) / float(window_size.y)
            );
            
            // rotation speed and fov is a little uncomfortable but whatever
            
            /*
             * NOTE: every documentation source I've found, including glm's source internal function names, says pitch,
             * yaw, roll. However, this is what works, and it seems to be pitch, roll, yaw, unless I misunderstand what
             * those words mean. No idea why, but it works so it works.
             *
             * Also, as of Sep 12 2023, https://glm.g-truc.net/0.9.0/api/a00184.html totally spells it "eular".
             */
            // gives Euler angles in radians: pitch, roll, yaw, or "pitch, yaw, roll" as they say
            glm::vec3 angles = glm::eulerAngles(camera->transform->rotation);
            // change and bound the pitch
            angles.x = std::clamp(angles.x + motion.y, glm::pi<float>() * 1 / 12, glm::pi<float>() * 11 / 12);
            // change the yaw
            angles.z -= motion.x;
            // roll is always zero
            // TODO: When a scene is loaded in, the camera may not always have roll zero. Find and correct.
            // The fix is needed because wasd will misbehave before the first mouse movement.
            angles.y = 0;
            // make a quaternion from the new camera direction
            // this constructor uses the Euler angles
            camera->transform->rotation = glm::qua(angles);
            return true;
        }
    }
    
    return false;
}

void PlayMode::update(float elapsed) {
    // copied from my game2
    //move camera:
    {
        //combine inputs into a move:
        constexpr float PlayerSpeed = 5.0f;
        auto move = glm::vec3(0.0f);
        // using z here because camera points in negative z
        if (down.pressed && !up.pressed) move.z = 1.0f;
        if (!down.pressed && up.pressed) move.z = -1.0f;
        // using x here for horizontal from the camera's perspective
        if (left.pressed && !right.pressed) move.x = -1.0f;
        if (!left.pressed && right.pressed) move.x = 1.0f;
        
        // rotate the movement towards where the camera points
        move = camera->transform->rotation * move;
        // and make it in the horizontal plane after rotation
        move.z = 0.0f;
        // scale it for constant speed
        if (move != glm::vec3(0.0f, 0.0f, 0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;
        // and actually move
        camera->transform->position += move;
        // but also don't move out of bounds
        camera->transform->position.x = std::clamp(camera->transform->position.x, -10.0f, 10.0f);
        camera->transform->position.y = std::clamp(camera->transform->position.y, -10.0f, 10.0f);
    }
    
    //reset button press counters:
    left.downs = 0;
    right.downs = 0;
    up.downs = 0;
    down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
    //update camera aspect ratio for drawable:
    camera->aspect = float(drawable_size.x) / float(drawable_size.y);
    
    //set up light type and position for lit_color_texture_program:
    // TODO: consider using the Light(s) in the scene to do this
    glUseProgram(lit_color_texture_program->program);
    glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
    glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
    glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
    glUseProgram(0);
    
    glClearColor(1.0f, 0.9f, 0.9f, 1.0f);
    glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.
    
    scene.draw(*camera);
    
    { //use DrawLines to overlay some text:
        glDisable(GL_DEPTH_TEST);
        float aspect = float(drawable_size.x) / float(drawable_size.y);
        DrawLines lines(glm::mat4(
                1.0f / aspect, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
        ));
        
        constexpr float H = 0.09f;
        lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
                        glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
                        glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                        glm::u8vec4(0x00, 0x00, 0x00, 0x00));
        float ofs = 2.0f / drawable_size.y;
        lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
                        glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
                        glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                        glm::u8vec4(0xff, 0xff, 0xff, 0x00));
    }
    GL_ERRORS();
}

void PlayMode::reload_torus_name() {
    scene.erase_line(torus_name_line);
    
    torus_name_line = scene.write_line(torus_name);
    torus_name_line->position.z = 5.0f;
    torus_name_line->rotation = glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f))
                                * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    torus_name_line->parent = torus;
}

void PlayMode::reload_names() {
    scene.erase_line(cube_name_line);
    scene.erase_line(cube_name_plural_line);
    scene.erase_line(cone_name_line);
    scene.erase_line(cone_name_plural_line);
    scene.erase_line(icosphere_name_line);
    scene.erase_line(icosphere_name_plural_line);
    scene.erase_line(goal_line);
    
    
    // TODO: rotate stuff
    cube_name_line = scene.write_line(cube_name[level]);
    cube_name_line->position.z = 5.0f;
    cube_name_line->rotation = glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f))
                               * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    cube_name_line->parent = cube;
    cube_name_plural_line = scene.write_line(pluralize[level](cube_name[level]));
    cube_name_plural_line->position.z = 5.0f;
    cube_name_plural_line->rotation = glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f))
                                      * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    cube_name_plural_line->parent = center_cube;
    
    cone_name_line = scene.write_line(cone_name[level]);
    cone_name_line->position.z = 5.0f;
    cone_name_line->rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f))
                               * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    cone_name_line->parent = cone;
    cone_name_plural_line = scene.write_line(pluralize[level](cone_name[level]));
    cone_name_plural_line->position.z = 5.0f;
    cone_name_plural_line->rotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f))
                                      * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    cone_name_plural_line->parent = center_cone;
    
    icosphere_name_line = scene.write_line(icosphere_name[level]);
    icosphere_name_line->position.z = 5.0f;
    icosphere_name_line->rotation = glm::angleAxis(3.0f * glm::pi<float>() / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f))
                                    * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    icosphere_name_line->parent = icosphere;
    icosphere_name_plural_line = scene.write_line(pluralize[level](icosphere_name[level]));
    icosphere_name_plural_line->position.z = 5.0f;
    icosphere_name_plural_line->rotation = glm::angleAxis(3.0f * glm::pi<float>() / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f))
                                           * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    icosphere_name_plural_line->parent = center_icosphere;
    
    torus_name = "";
    reload_torus_name();
    
    goal_line = scene.write_line("Goal: " + pluralize[level](goal_name[level]));
    goal_line->position.z = 5.0f;
    goal_line->rotation = glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f))
                          * glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    goal_line->parent = center_torus;
}
