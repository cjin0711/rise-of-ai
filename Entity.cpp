#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"
#include <string>

#include <windows.h>

#include "Map.h"

void Entity::ai_activate(Entity *player)
{
    
    switch (m_ai_type)
    {
        case WALKER:
            ai_walk();
            break;
            
        case GUARD:
            ai_guard(player);
            break;

        case JUMPER:
            ai_guard(player);
            break;

        default:
            break;
    }
}

void Entity::ai_walk()
{

    if (m_position.x < 3.0f) {
        m_movement = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else if (m_position.x > 7.0f) {
        m_movement = glm::vec3(-1.0f, 0.0f, 0.0f);
    }
    
}


// primarily for goomba
void Entity::ai_guard(Entity *player)
{

    switch (m_ai_state) {
        case IDLE:
          
            if (glm::distance(m_position.x, player->get_position().x) < 3.0f && fabs(m_position.y - player->get_position().y) < 0.75f) {
                m_ai_state = ATTACKING;
            }

            break;
            
        case JUMPING:
            jump();
            break;

        case ATTACKING:
            // conditional checking if enemy and player are same height
            if (fabs(m_position.y - player->get_position().y) < 0.75f && fabs(m_position.x - player->get_position().x) < 3.0f) {
                if ((m_position.x > player->get_position().x && m_position.x < 7.25f) || m_position.x > 7.25f) {
                    m_movement = glm::vec3(-1.0f, 0.0f, 0.0f);
                }
                else if (m_position.x < player->get_position().x && m_position.x < 7.25f) {
                    m_movement = glm::vec3(1.0f, 0.0f, 0.0f);
                }
            }
            else {
                // still in attack mode, just walking while player not same height level
                if (m_position.x < 3.75f) {
                   
                    m_movement = glm::vec3(1.2f, 0.0f, 0.0f);
                }
                else if (m_position.x > 7.25f) {
                   
                    m_movement = glm::vec3(-1.2f, 0.0f, 0.0f);
                }
            }
            break;
            
        default:
            break;
    }
}
// Default constructor
Entity::Entity()
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(1.5f), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
    m_texture_id(0), m_velocity(0.0f), m_acceleration(0.0f), m_width(0.0f), m_height(0.0f)
{
    // Initialize m_walking with zeros or any default value
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;

    OutputDebugString(L"DEFAULT CONSTRUCTORRRRRRRR\n");
}

// Parameterized constructor
Entity::Entity(GLuint texture_id, float speed, glm::vec3 acceleration, float jump_power, int walking[3][4], float animation_time,
    int animation_frames, int animation_index, int animation_cols,
    int animation_rows, float width, float height, EntityType EntityType)
    : m_position(0.0f, 0.0f, 0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed),m_acceleration(acceleration), m_jumping_power(jump_power), m_animation_cols(animation_cols),
    m_animation_frames(animation_frames), m_animation_index(animation_index),
    m_animation_rows(animation_rows), m_animation_indices(nullptr),
    m_animation_time(animation_time), m_texture_id(texture_id), m_velocity(0.0f),
    m_width(width), m_height(height), m_entity_type(EntityType), kill_count(0.0f)
{
    face_right();
    set_walking(walking);
    set_idle(walking);
    set_jumping(walking);

    OutputDebugString(L"PARAMETERIZED CONSTRUCTORRRRR\n");
}

// Simpler constructor for partial initialization
Entity::Entity(GLuint texture_id, float speed,  float width, float height, EntityType EntityType)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
    m_texture_id(texture_id), m_velocity(0.0f), m_acceleration(0.0f), m_width(width), m_height(height),m_entity_type(EntityType)
{
    // Initialize m_walking with zeros or any default value
    //for (int i = 0; i < SECONDS_PER_FRAME; ++i)
    //    for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;

    OutputDebugString(L"PARTIAL SIMPLE CONSTRUCTOR\n");
}



const char* entityTypeToString(EntityType type) {
    switch (type) {
    case PLAYER: return "PLAYER";
    case ENEMY: return "ENEMY";
        // Add other cases here
    default: return "UNKNOWN";
    }
}



Entity::Entity(GLuint texture_id, float speed, float width, float height, EntityType EntityType, AIType AIType, AIState AIState): m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
m_speed(speed), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
m_texture_id(texture_id), m_velocity(0.0f), m_acceleration(0.0f), m_width(width), m_height(height),m_entity_type(EntityType), m_ai_type(AIType), m_ai_state(AIState), m_idle(), m_is_jumping(false), m_jumping_power(5.0f)
{
// Initialize m_walking with zeros or any default value
//for (int i = 0; i < SECONDS_PER_FRAME; ++i)
//    for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;

OutputDebugString(L"MAKING AN ENEMYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n");

//const char* entityTypeStr = entityTypeToString(m_entity_type);
//std::wstring wideString(entityTypeStr, entityTypeStr + strlen(entityTypeStr));
//OutputDebugString(L"Entity type: ");
//OutputDebugString(wideString.c_str());
//OutputDebugString(L"\n");

std::wstring speeds = std::to_wstring(m_speed);
OutputDebugString(L"ENTITY SPEED ");
OutputDebugString(speeds.c_str());
OutputDebugString(L"\n");

std::wstring widths = std::to_wstring(m_width);
OutputDebugString(L"ENTITY WIDTH ");
OutputDebugString(widths.c_str());
OutputDebugString(L"\n");

std::wstring heights = std::to_wstring(m_height);
OutputDebugString(L"ENTITY HEIGHT ");
OutputDebugString(heights.c_str());
OutputDebugString(L"\n");



OutputDebugString(L"Entity type raw value: ");
std::wstring rawValue = std::to_wstring(static_cast<int>(m_entity_type));
OutputDebugString(rawValue.c_str());
OutputDebugString(L"\n");


OutputDebugString(L"FINISHEEEEEEEEEEEEEEEEEEEEEEEEED\n");

}


Entity::~Entity() { }

void Entity::draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index)
{
    // Step 1: Calculate the UV location of the indexed frame
    float u_coord = (float)(index % m_animation_cols) / (float)m_animation_cols;
    float v_coord = (float)(index / m_animation_cols) / (float)m_animation_rows;

    // Step 2: Calculate its UV size
    float width = 1.0f / (float)m_animation_cols;
    float height = 1.0f / (float)m_animation_rows;

    // Step 3: Just as we have done before, match the texture coordinates to the vertices
    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };

    // Step 4: And render
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

bool const Entity::check_collision(Entity* other) const
{
    if (other->get_is_active()) {
        float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
        float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);

        return x_distance < 0.0f && y_distance < 0.0f;
    }
    return false;
}

void const Entity::check_collision_y(Entity *collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity *collidable_entity = &collidable_entities[i];
        
        
        if (check_collision(collidable_entity))
        {
/*----------------- IF THE PLAYER HITS A DISAPPEARING BLOCK, DEACTIVATE IT ----------*/
            if (m_entity_type == PLAYER) {
                collidable_entity->deactivate();
                set_kill_count(get_kill_count() + 1);
            }
    /*-----------DON'T DO COLLISIONS IF THE BLOCK ISN'T ACTIVE-----------*/
            if (!collidable_entity->m_is_active) {
                break;
            }
            float y_distance = fabs(m_position.y - collidable_entity->m_position.y);
            float y_overlap = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));
            if (m_velocity.y > 0)
            {
                m_position.y -= y_overlap;
                m_velocity.y = 0;

                // Collision!
                m_collided_top = true;
            }
            else if (m_velocity.y < 0)
            {
                m_position.y += y_overlap;
                m_velocity.y = 0;

                // Collision!
                m_collided_bottom = true;
            }
        }
    }
}

void const Entity::check_collision_x(Entity *collidable_entities, int collidable_entity_count, Entity* player)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity))
        {
/*-----------DON'T DO COLLISIONS IF THE BLOCK ISN'T ACTIVE-----------*/
            if (!collidable_entity->m_is_active) {
                break;
            }
            float x_distance = fabs(m_position.x - collidable_entity->m_position.x);
            float x_overlap = fabs(x_distance - (m_width / 2.0f) - (collidable_entity->m_width / 2.0f));
            if (m_velocity.x > 0)
            {
                m_position.x     -= x_overlap;
                m_velocity.x      = 0;

                // Collision!
                m_collided_right  = true;
                
            } else if (m_velocity.x < 0)
            {
                m_position.x    += x_overlap;
                m_velocity.x     = 0;
 
                // Collision!
                m_collided_left  = true;
            }
            if (m_collided_right || m_collided_left) {
                if (m_entity_type == FIREBALL) {
                    this->deactivate();
                    collidable_entity->deactivate();
                    player->set_kill_count(player->get_kill_count() + 1);
                }
                else {
                    set_touched();
                }
                
            }
        }
    }
}



// ---------- OVERLOADING COLLISION FUNCTIONS ------------- //
void const Entity::check_collision_y(Map* map)
{
    // Probes for tiles above
    glm::vec3 top = glm::vec3(m_position.x, m_position.y + (m_height / 2), m_position.z);
    glm::vec3 top_left = glm::vec3(m_position.x - (m_width / 2), m_position.y + (m_height / 2), m_position.z);
    glm::vec3 top_right = glm::vec3(m_position.x + (m_width / 2), m_position.y + (m_height / 2), m_position.z);

    // Probes for tiles below
    glm::vec3 bottom = glm::vec3(m_position.x, m_position.y - (m_height / 2), m_position.z);
    glm::vec3 bottom_left = glm::vec3(m_position.x - (m_width / 2), m_position.y - (m_height / 2), m_position.z);
    glm::vec3 bottom_right = glm::vec3(m_position.x + (m_width / 2), m_position.y - (m_height / 2), m_position.z);

    float penetration_x = 0;
    float penetration_y = 0;

    // If the map is solid, check the top three points
    if (map->is_solid(top, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    else if (map->is_solid(top_left, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    else if (map->is_solid(top_right, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }

    // And the bottom three points
    if (map->is_solid(bottom, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
    }
    else if (map->is_solid(bottom_left, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
    }
    else if (map->is_solid(bottom_right, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;

    }
}

void const Entity::check_collision_x(Map* map)
{
    // Probes for tiles; the x-checking is much simpler
    glm::vec3 left = glm::vec3(m_position.x - (m_width / 2), m_position.y, m_position.z);
    glm::vec3 right = glm::vec3(m_position.x + (m_width / 2), m_position.y, m_position.z);

    float penetration_x = 0;
    float penetration_y = 0;

    if (map->is_solid(left, &penetration_x, &penetration_y) && m_velocity.x < 0)
    {
        m_position.x += penetration_x;
        m_velocity.x = 0;
        m_collided_left = true;
        if (m_entity_type == FIREBALL) {
            this->deactivate();
        }
        
    }
    if (map->is_solid(right, &penetration_x, &penetration_y) && m_velocity.x > 0)
    {
        m_position.x -= penetration_x;
        m_velocity.x = 0;
        m_collided_right = true;
        if (m_entity_type == FIREBALL) {
            this->deactivate();
        }
    }
}






void Entity::update(float delta_time, Entity *player, Entity *collidable_entities, int collidable_entity_count , Map *map)
{
    //OutputDebugString(L"Updating entity\n");

    if (!m_is_active) return;
 
    m_collided_top    = false;
    m_collided_bottom = false;
    m_collided_left   = false;
    m_collided_right  = false;
    
    if (m_entity_type == ENEMY) {
        ai_activate(player);
    }

    if (m_entity_type == FIREBALL) {
        if (!m_is_active) {
            return;
        }
    }

    //if (m_entity_type == PLAYER) {
    //    // Update playerOne position
    //    if (m_position.x < -4.8f)
    //    {
    //        m_position.x = -4.8f;
    //    }
    //    else if (m_position.x > 4.8f)
    //    {
    //        m_position.x = 4.8f;
    //    }
    //}

    if (m_animation_indices != NULL)
    {
        if (glm::length(m_movement) != 0)
        {
            m_animation_time += delta_time;
            float frames_per_second = (float) 1 / SECONDS_PER_FRAME;
            
            if (m_animation_time >= frames_per_second)
            {
                m_animation_time = 0.0f;
                m_animation_index++;

                if (m_animation_index >= m_animation_frames)
                {
                    m_animation_index = 0;
                }
                
            }
        }
    }
    
    m_velocity.x = m_movement.x * m_speed;
    m_velocity += m_acceleration * delta_time;
    
    m_position.y += m_velocity.y * delta_time;
    check_collision_y(collidable_entities, collidable_entity_count);
    check_collision_y(map);
    
    m_position.x += m_velocity.x * delta_time;
    check_collision_x(collidable_entities, collidable_entity_count, player);
    check_collision_x(map);
    
    if (m_is_jumping && m_collided_bottom)
    {
        m_is_jumping = false;
        m_velocity.y += m_jumping_power;
    }

    if (m_shooting) {
        m_shooting = false;

    }
    
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);

    // Add this line to apply scaling:
    m_model_matrix = glm::scale(m_model_matrix, glm::vec3(m_width, m_height, 1.0f));
}


void Entity::render(ShaderProgram* program)
{
    if (!m_is_active) {
        return;
    }
  
    program->set_model_matrix(m_model_matrix);


    if (m_animation_indices != NULL)
    {
        draw_sprite_from_texture_atlas(program, m_texture_id, m_animation_indices[m_animation_index]);
        return;
    }

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = { 0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };

    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}