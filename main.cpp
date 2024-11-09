
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 21

// FOR MAP GENERATION
#define LEVEL_WIDTH 14
#define LEVEL_HEIGHT 11


#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"
#include <string>

#include <windows.h>

#include "Map.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    //std::vector<Entity> enemies;
    Entity* enemies;


    Map* map;
};

// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH = 640 * 1.5,
WINDOW_HEIGHT = 480 * 1.5;

const float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char SPRITESHEET_FILEPATH[] = "assets/mario.png";
const char PLATFORM_FILEPATH[] = "assets/ground.png";
constexpr char FONTSHEET_FILEPATH[] = "assets/font1.png";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;

const int CD_QUAL_FREQ = 44100,
AUDIO_CHAN_AMT = 2,     // stereo
AUDIO_BUFF_SIZE = 4096;

const char BGM_FILEPATH[] = "assets/goblins.mp3",
SFX_FILEPATH[] = "assets/jump.wav";

const int PLAY_ONCE = 0,    // play once, loop never
NEXT_CHNL = -1,   // next available channel
ALL_SFX_CHNL = -1;

const int num_enemies = 3;


Mix_Music* g_music;
Mix_Chunk* g_jump_sfx;

// ––––– GLOBAL VARIABLES ––––– //  
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

bool left_movement = false;
bool right_movement = false;

constexpr int FONTBANK_SIZE = 16;

GLuint g_font_texture_id;


// DATA FOR MAP
unsigned int LEVEL_DATA[] =
{
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3,
    3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
    3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3
};

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}



void draw_text(ShaderProgram* shader_program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }
    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());

    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());

    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
}

void initialise()
{
    srand(time(0));
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Hello, Physics (again)!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ––––– BGM ––––– //
    Mix_OpenAudio(CD_QUAL_FREQ, MIX_DEFAULT_FORMAT, AUDIO_CHAN_AMT, AUDIO_BUFF_SIZE);

    // STEP 1: Have openGL generate a pointer to your music file
    g_music = Mix_LoadMUS(BGM_FILEPATH); // works only with mp3 files

    // STEP 2: Play music
    Mix_PlayMusic(
        g_music,  // music file
        -1        // -1 means loop forever; 0 means play once, look never
    );

    // STEP 3: Set initial volume
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2.0);

    // ––––– SFX ––––– //
    g_jump_sfx = Mix_LoadWAV(SFX_FILEPATH);

    // ––––– MAP ––––– //
    GLuint map_texture_id = load_texture("assets/new2_tileset.png");
    g_state.map = new Map(LEVEL_WIDTH, LEVEL_HEIGHT, LEVEL_DATA, map_texture_id, 1.0f, 4, 1);

    // ----- FONT ----- //
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH);


    // ––––– PLAYER (GEORGE) ––––– //
    // Existing
    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);

    int player_walking_animation[3][4] =
    {
        { 4, 5, 6, 7 },  // for George to move to the left,
        { 0, 1, 2, 3 }, // for George to move to the right,
        { 8, 9, 10, 11}
        //{ 2, 6, 10, 14 }, // for George to move upwards,
        //{ 0, 4, 8, 12 }   // for George to move downwards
    };

    glm::vec3 acceleration = glm::vec3(0.0f, -18.0f, 0.0f);

    g_state.player = new Entity(
        player_texture_id,         // texture id
        5.0f,                      // speed
        acceleration,              // acceleration
        3.5f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        3,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4,                         // animation row amount
        0.75f,                      // width
        1.0f,                       // height
        PLAYER
    );

    // Jumping
    g_state.player->set_jumping_power(10.0f);
    g_state.player->set_position(glm::vec3(1.0f, -5.0f, 0.0f));


  // ----- ENEMIES ----- //

    GLuint goomba_texture_id = load_texture("assets/goomba.png");
    GLuint shell_texture_id = load_texture("assets/shell.png");
    GLuint koopa_texture_id = load_texture("assets/koopa.png");


    g_state.enemies = new Entity[num_enemies]{
        Entity(goomba_texture_id, 1.0f, 0.8f, 0.8f, ENEMY, WALKER, IDLE),
        Entity(shell_texture_id, 1.0f, 0.8f, 0.8f, ENEMY, WALKER, IDLE),
        Entity(koopa_texture_id, 1.0f, 0.8f, 0.8f, ENEMY, WALKER, IDLE),
    };

    //std::vector<Entity> enemies;

    g_state.enemies[0] = Entity(
        goomba_texture_id,    // texture id
        1.0f,               // speed
        0.75f,              // width
        0.75f,               // height
        ENEMY,              // entity type
        GUARD,              // AI type
        IDLE                // initial AI state
    );

    g_state.enemies[0].set_position(glm::vec3(3.74f, -2.0f, 0.0f));
    g_state.enemies[0].set_acceleration(glm::vec3(0.0f, -5.0f, 0.0f));


    // SHELL
    g_state.enemies[1] = Entity(
        shell_texture_id,    // texture id
        1.5f,               // speed
        0.75f,              // width
        0.75f,               // height
        ENEMY,              // entity type
        WALKER,              // AI type
        IDLE                // initial AI state
    );

    g_state.enemies[1].set_position(glm::vec3(2.99f, -6.0f, 0.0f));
    g_state.enemies[1].set_acceleration(glm::vec3(0.0f, -5.0f, 0.0f));

    // KOOPA

    g_state.enemies[2] = Entity(
        koopa_texture_id,    // texture id
        1.0f,               // speed
        0.75f,              // width
        0.75f,               // height
        ENEMY,              // entity type
        JUMPER,              // AI type
        JUMPING               // initial AI state
    );

    g_state.enemies[2].set_position(glm::vec3(10.0f, -3.0f, 0.0f));
    g_state.enemies[2].set_jumping_power(5.0f);
    g_state.enemies[2].set_acceleration(glm::vec3(0.0f, -15.0f, 0.0f));


    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));


    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_SPACE:
                // Jump
                if (g_state.player->get_touched()) {
                    break;
                }
                if (g_state.player->get_collided_bottom())
                {

                    g_state.player->jump();

                    Mix_PlayChannel(NEXT_CHNL, g_jump_sfx, 0);

                    /**
                     Mix_FadeInChannel(channel_id, sound_chunk, loops, fade_in_time);


                     */
                }
                break;

            case SDLK_h:
                // Stop music
                Mix_HaltMusic();
                break;

            case SDLK_p:
                Mix_PlayMusic(g_music, -1);

            default:
                break;
            }

        default:
            break;
        }
    }
    if (g_state.player->get_touched()) {
        return;
    }

    if (key_state[SDL_SCANCODE_LEFT]) {
        g_state.player->move_left();
        right_movement = false;
        left_movement = true;
    }
    else if (key_state[SDL_SCANCODE_RIGHT]) {
        g_state.player->move_right();
        left_movement = false;
        right_movement = true;
    }
    else {
        if (left_movement) {
            g_state.player->idle_left();
        }
        else if (right_movement) {
            g_state.player->idle_right();
        }
    }

    if (glm::length(g_state.player->get_movement()) > 1.0f)
    {
        g_state.player->normalise_movement();
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }


    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, g_state.player, g_state.enemies, 3, g_state.map);

        for (int i = 0; i < 3; i++) {
            //if (!g_state.enemies[i].get_is_active()) {
            //    delete &g_state.enemies[i];
            //}
            //else {
                g_state.enemies[i].update(FIXED_TIMESTEP, g_state.player, g_state.player, 1, g_state.map);
            //}
           
        }


        //g_state.goomba->update(FIXED_TIMESTEP, g_state.player, g_state.player, 1, g_state.map);

        //g_state.shell->update(FIXED_TIMESTEP, g_state.player, g_state.player, 1, g_state.map);

        //g_state.koopa->update(FIXED_TIMESTEP, g_state.player, g_state.player, 1, g_state.map);

        delta_time -= FIXED_TIMESTEP;
    }


    // CAMERA FOLLOWING PLAYER
    //g_view_matrix = glm::mat4(1.0f);
    //g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_state.player->get_position().x, 0.0f, 0.0f));

    g_view_matrix = glm::mat4(1.0f);

    //// Camera Follows the player
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_state.player->get_position().x, -g_state.player->get_position().y, 0.0f));


    //if player colliding with platform to deact: platforms[platformtodeact].deactivate()


    g_accumulator = delta_time;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_program.set_view_matrix(g_view_matrix);


    if (g_state.player->get_kill_count() == 3) {
        draw_text(&g_program, g_font_texture_id, "YOU WIN!", 0.5f, 0.05f,
            glm::vec3(g_state.player->get_position().x - 2.0f, g_state.player->get_position().y + 2.0f, 0.0f));
    }
    else if (g_state.player->get_kill_count() < 3 && g_state.player->get_touched()) {
        draw_text(&g_program, g_font_texture_id, "YOU LOSE!", 0.5f, 0.05f,
            glm::vec3(g_state.player->get_position().x - 2.0f, g_state.player->get_position().y + 2.0f, 0.0f));
    }

    g_state.player->render(&g_program);

    for (int i = 0; i < 3; i++) {
        g_state.enemies[i].render(&g_program);
    }


    //g_state.goomba->render(&g_program);

    //g_state.shell->render(&g_program);

    //g_state.koopa->render(&g_program);

    g_state.map->render(&g_program);

    //for (int i = 0; i < PLATFORM_COUNT; i++) {
    //    //if (g_state.platforms[i].get_entity_type() != DISAPPEARING)
    //        g_state.platforms[i].render(&g_program);
    //}

    //for (int i = 0; i < g_state.enemy_count; i++) {
    //    g_state.enemies[i].render(&g_program);
    //}

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete g_state.enemies;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();


    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }


    shutdown();
    return 0;
}