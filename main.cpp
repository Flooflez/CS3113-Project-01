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
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 640;

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
const float RAINFALL_SPEED = 6.0f;
const float RAINFALL_OFFSET = 10.0f;

const float LEAF_FALL_SPEED = 0.5f;
const float LEAF_ORBIT_SPEED = 1.0f; 
const float LEAF_ROT_SPEED = 1.0f;
const float LEAF_ORBIT_RADIUS = 0.5f;
const float LEAF_CURVE_STEEPNESS = 2.5f;
const float LEAF_START_X = -3.0f;
const float LEAF_START_Y = -1.0f;
const float LEAF_RESPAWN_THRESHOLD = 10.0f;
const float LEAF_ORBIT_START_ANGLE = -135.0f;
const float LEAF_ROT_START_ANGLE = 0.0f;
const float LEAF_BASE_SIZE = 0.4f;
const float LEAF_SCALE_SIZE = 0.1f;

const float WIND_ROT_START_ANGLE = 315.0f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char BG_SPRITE_PATH[] = "assets\\back_drop.png",
            GROUND_SPRITE_PATH[] = "assets\\ground.png",
            LEAF_SPRITE_PATH[] = "assets\\leaf.png",
            TREE_SPRITE_PATH[] = "assets\\tree.png",
            RAIN_SPRITE_PATH[] = "assets\\rain.png",
            WIND_SPRITE_PATH[] = "assets\\wind.png";

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool g_is_growing = true;

ShaderProgram g_shader_program;
glm::mat4 view_matrix,g_projection_matrix,g_trans_matrix;

// Model Matrices
glm::mat4 g_bg_model_matrix, g_ground_model_matrix, g_leaf_model_matrix, g_tree_model_matrix, g_rain_1_model_matrix, g_rain_2_model_matrix, g_wind_model_matrix;

// Leaf Path Reference Point
glm::mat4 g_leaf_reference_matrix;

float g_previous_ticks = 0.0f;

GLuint g_bg_texture_id, g_ground_texture_id, g_leaf_texture_id, g_tree_texture_id, g_rain_1_texture_id, g_rain_2_texture_id, g_wind_texture_id;

float g_rain_1_y_pos = 0.0f,
        g_rain_2_y_pos = RAINFALL_OFFSET;

float g_leaf_ref_x_pos = 0.0f,
        g_leaf_orbit_angle = LEAF_ORBIT_START_ANGLE,
        g_leaf_x_pos, g_leaf_y_pos,
        g_leaf_angle = LEAF_ROT_START_ANGLE;

float g_time_elapsed = 0.0f;

glm::vec3 g_scale_vector = glm::vec3(10.0f, 10.0f, 1.0f);


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    // Initialise video
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("A Rainy Night",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_bg_model_matrix = glm::mat4(1.0f);
    g_ground_model_matrix = glm::mat4(1.0f);
    g_leaf_model_matrix = glm::mat4(1.0f);
    g_rain_1_model_matrix = glm::mat4(1.0f);
    g_rain_2_model_matrix = glm::mat4(1.0f);
    g_tree_model_matrix = glm::mat4(1.0f);
    g_wind_model_matrix = glm::mat4(1.0f);

    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    //g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -1.0f, 1.0f);
    //changed g_projection_matrix from default code since I want a square window

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(view_matrix);
    // Notice we haven't set our model matrix yet!

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_bg_texture_id = load_texture(BG_SPRITE_PATH);
    g_ground_texture_id = load_texture(GROUND_SPRITE_PATH);
    g_leaf_texture_id = load_texture(LEAF_SPRITE_PATH);
    g_tree_texture_id = load_texture(TREE_SPRITE_PATH);
    g_rain_1_texture_id = load_texture(RAIN_SPRITE_PATH);
    g_rain_2_texture_id = load_texture(RAIN_SPRITE_PATH);
    g_wind_texture_id = load_texture(WIND_SPRITE_PATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    //Setup Static Images
    g_bg_model_matrix = glm::scale(g_bg_model_matrix, g_scale_vector);
    g_ground_model_matrix = glm::scale(g_ground_model_matrix, g_scale_vector);
    g_tree_model_matrix = glm::scale(g_tree_model_matrix, g_scale_vector);

    //Move Second Rain to Top
    g_rain_2_model_matrix = glm::translate(g_rain_2_model_matrix, glm::vec3(0.0f, g_rain_2_y_pos, 0.0f));

    
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_WINDOWEVENT_CLOSE:
        case SDL_QUIT:
            g_game_is_running = false;
            break;
        }
    }
}

void update()
{
    //delta time stuff
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    //reset
    g_rain_1_model_matrix = glm::mat4(1.0f);
    g_rain_2_model_matrix = glm::mat4(1.0f);

    g_leaf_reference_matrix = glm::mat4(1.0f);
    g_leaf_model_matrix = glm::mat4(1.0f);

    g_wind_model_matrix = glm::mat4(1.0f);

    //Rainfall Movement
    g_rain_1_y_pos -= RAINFALL_SPEED * delta_time;
    if (g_rain_1_y_pos < -RAINFALL_OFFSET) {
        g_rain_1_y_pos = RAINFALL_OFFSET;
    }
    g_rain_2_y_pos -= RAINFALL_SPEED * delta_time;
    if (g_rain_2_y_pos < -RAINFALL_OFFSET) {
        g_rain_2_y_pos = RAINFALL_OFFSET;
    }

    g_rain_1_model_matrix = glm::translate(g_rain_1_model_matrix, glm::vec3(0.0f, g_rain_1_y_pos, 0.0f));
    g_rain_2_model_matrix = glm::translate(g_rain_2_model_matrix, glm::vec3(0.0f, g_rain_2_y_pos, 0.0f));

    g_rain_1_model_matrix = glm::scale(g_rain_1_model_matrix, g_scale_vector);
    g_rain_2_model_matrix = glm::scale(g_rain_2_model_matrix, g_scale_vector);

    //Leaf and Wind Movement
    float leaf_ref_y_pos = LEAF_CURVE_STEEPNESS * glm::pow(glm::e<float>(), -g_leaf_ref_x_pos) - LEAF_CURVE_STEEPNESS; //calculate y position based on y = a*e^(-x) - a

    g_leaf_ref_x_pos += LEAF_FALL_SPEED * delta_time;
    if (g_leaf_ref_x_pos > LEAF_RESPAWN_THRESHOLD) {
        g_leaf_ref_x_pos = 0.0f;
        g_leaf_orbit_angle = LEAF_ORBIT_START_ANGLE;
        g_leaf_angle = LEAF_ROT_START_ANGLE;
        g_time_elapsed = 0.0f;
    }
    
    g_time_elapsed += delta_time;

    g_leaf_orbit_angle += LEAF_ORBIT_SPEED * delta_time;
    g_leaf_x_pos = LEAF_ORBIT_RADIUS * glm::cos(g_leaf_orbit_angle);
    g_leaf_y_pos = LEAF_ORBIT_RADIUS * glm::sin(g_leaf_orbit_angle);

    g_leaf_angle += LEAF_ROT_SPEED * delta_time;

    float wind_angle = glm::atan( -LEAF_CURVE_STEEPNESS * glm::pow(glm::e<float>(), -g_leaf_ref_x_pos)) + WIND_ROT_START_ANGLE;

    g_leaf_reference_matrix = glm::translate(g_leaf_reference_matrix, glm::vec3(g_leaf_ref_x_pos + LEAF_START_X, leaf_ref_y_pos + LEAF_START_Y, 0.0f));
    g_leaf_model_matrix = glm::translate(g_leaf_reference_matrix, glm::vec3(g_leaf_x_pos, g_leaf_y_pos, 0.0f));

    g_leaf_model_matrix = glm::rotate(g_leaf_model_matrix, g_leaf_angle, glm::vec3(0.0f, 0.0f, 1.0f));

    float leaf_size = LEAF_SCALE_SIZE * glm::sin(g_time_elapsed) + LEAF_BASE_SIZE;
    g_leaf_model_matrix = glm::scale(g_leaf_model_matrix, glm::vec3(leaf_size, leaf_size, 1.0f));

    g_wind_model_matrix = glm::translate(g_wind_model_matrix, glm::vec3(g_leaf_ref_x_pos + LEAF_START_X, leaf_ref_y_pos + LEAF_START_Y, 0.0f));
    g_wind_model_matrix = glm::rotate(g_wind_model_matrix, wind_angle, glm::vec3(0.0f, 0.0f, 1.0f));
}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind textures
    draw_object(g_bg_model_matrix, g_bg_texture_id);

    draw_object(g_wind_model_matrix, g_wind_texture_id);
    draw_object(g_leaf_model_matrix, g_leaf_texture_id);
    draw_object(g_tree_model_matrix, g_tree_texture_id);
    
    draw_object(g_rain_1_model_matrix, g_rain_1_texture_id);
    draw_object(g_rain_2_model_matrix, g_rain_2_texture_id);

    draw_object(g_ground_model_matrix, g_ground_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{

    SDL_Quit();
}

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
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