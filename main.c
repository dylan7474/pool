// -----------------------------------------------------------------------------
// A simple 8-Ball Pool Game in C using SDL2
//
// Author: Dylan Jones
// Based on the Linux development environment setup provided.
//
// To compile: Use the provided Makefile with the command `make`
// To run: ./pool_game
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // For drawing text later

// --- Constants ---
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 500
#define TABLE_WIDTH 900
#define TABLE_HEIGHT 450
#define BALL_RADIUS 15
#define BALL_DIAMETER (BALL_RADIUS * 2)
#define NUM_BALLS 16
#define POCKET_RADIUS 30
#define CUSHION_WIDTH 25

// Physics constants
#define FRICTION 0.992f  // Slightly lower friction for smoother ball rolls
#define CUE_POWER_MULTIPLIER 0.15f
#define MIN_VELOCITY 0.1f

// --- Data Structures ---

// A simple 2D vector
typedef struct {
    float x;
    float y;
} Vec2D;

// Represents a single pool ball
typedef struct {
    int id;
    bool isActive;
    Vec2D pos;
    Vec2D vel;
    SDL_Color color;
} Ball;

// Represents the six pockets on the table
typedef struct {
    Vec2D pos;
} Pocket;

// Enum for different game states
typedef enum {
    STATE_AIMING,
    STATE_SIMULATING,
    STATE_GAME_OVER
} GameState;

// --- Global Variables ---
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
Ball gBalls[NUM_BALLS];
Pocket gPockets[6];
GameState gCurrentState = STATE_AIMING;
bool gGameIsRunning = true;

// --- Function Prototypes ---
bool initialize();
void setup_table();
void reset_game();
void game_loop();
void handle_input(SDL_Event* e);
void update();
void render();
void cleanup();
void draw_ball(Ball* ball);
void draw_circle(int centerX, int centerY, int radius, SDL_Color color);


// --- Function Implementations ---

/**
 * @brief Initializes SDL, creates the window and renderer.
 * @return true on success, false on failure.
 */
bool initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    gWindow = SDL_CreateWindow("8-Ball Pool Simulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (gRenderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    // Initialize TTF font library (optional but good practice)
    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        // This is not a critical error for this version, so we don't return false
    }


    reset_game();
    return true;
}

/**
 * @brief Sets the initial positions of the balls in a standard 8-ball rack.
 * Also defines pocket locations.
 */
void setup_table() {
    // Ball colors
    SDL_Color colors[NUM_BALLS] = {
        {255, 255, 255, 255}, // 0: Cue ball
        {255, 215, 0, 255},   // 1: Yellow (Solid)
        {0, 0, 255, 255},     // 2: Blue (Solid)
        {255, 0, 0, 255},     // 3: Red (Solid)
        {75, 0, 130, 255},    // 4: Purple (Solid)
        {255, 165, 0, 255},   // 5: Orange (Solid)
        {0, 128, 0, 255},     // 6: Green (Solid)
        {128, 0, 0, 255},     // 7: Maroon (Solid)
        {0, 0, 0, 255},       // 8: Black
        {255, 215, 0, 255},   // 9: Yellow (Stripe)
        {0, 0, 255, 255},     // 10: Blue (Stripe)
        {255, 0, 0, 255},     // 11: Red (Stripe)
        {75, 0, 130, 255},    // 12: Purple (Stripe)
        {255, 165, 0, 255},   // 13: Orange (Stripe)
        {0, 128, 0, 255},     // 14: Green (Stripe)
        {128, 0, 0, 255}      // 15: Maroon (Stripe)
    };

    // Initialize all balls
    for (int i = 0; i < NUM_BALLS; ++i) {
        gBalls[i].id = i;
        gBalls[i].isActive = true;
        gBalls[i].vel = (Vec2D){0, 0};
        gBalls[i].color = colors[i];
    }

    // --- Position the balls in the rack ---
    float startX = SCREEN_WIDTH * 0.75f;
    float startY = SCREEN_HEIGHT / 2.0f;
    float ball_offset = BALL_DIAMETER * 0.88f; // Vertical distance between rows

    int rackOrder[] = {1, 9, 15, 2, 8, 14, 3, 10, 7, 13, 4, 11, 6, 12, 5};
    int ballIndex = 0;

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col <= row; ++col) {
            gBalls[rackOrder[ballIndex]].pos.x = startX + row * ball_offset;
            gBalls[rackOrder[ballIndex]].pos.y = startY + (col * BALL_DIAMETER) - (row * BALL_RADIUS);
            ballIndex++;
        }
    }

    // Position cue ball
    gBalls[0].pos = (Vec2D){SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT / 2.0f};

    // --- Define pocket locations ---
    float tableX = (SCREEN_WIDTH - TABLE_WIDTH) / 2.0f;
    float tableY = (SCREEN_HEIGHT - TABLE_HEIGHT) / 2.0f;
    gPockets[0] = (Pocket){{tableX, tableY}};
    gPockets[1] = (Pocket){{tableX + TABLE_WIDTH / 2.0f, tableY}};
    gPockets[2] = (Pocket){{tableX + TABLE_WIDTH, tableY}};
    gPockets[3] = (Pocket){{tableX, tableY + TABLE_HEIGHT}};
    gPockets[4] = (Pocket){{tableX + TABLE_WIDTH / 2.0f, tableY + TABLE_HEIGHT}};
    gPockets[5] = (Pocket){{tableX + TABLE_WIDTH, tableY + TABLE_HEIGHT}};
}

/**
 * @brief Resets the table and game state to their initial values.
 */
void reset_game() {
    setup_table();
    gCurrentState = STATE_AIMING;
}

/**
 * @brief The main game loop. Runs until the user quits.
 */
void game_loop() {
    SDL_Event e;
    while (gGameIsRunning) {
        handle_input(&e);
        update();
        render();
    }
}

/**
 * @brief Handles all user input (mouse and keyboard).
 * @param e Pointer to the SDL_Event structure.
 */
void handle_input(SDL_Event* e) {
    while (SDL_PollEvent(e) != 0) {
        if (e->type == SDL_QUIT) {
            gGameIsRunning = false;
        }

        if (e->type == SDL_KEYDOWN) {
            switch (e->key.keysym.sym) {
                case SDLK_ESCAPE:
                    gGameIsRunning = false;
                    break;
                case SDLK_r:
                    reset_game();
                    break;
            }
        }

        // Handle aiming and shooting
        if (gCurrentState == STATE_AIMING && gBalls[0].isActive) {
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                // Calculate vector from cue ball to mouse
                float dx = mouseX - gBalls[0].pos.x;
                float dy = mouseY - gBalls[0].pos.y;

                // Set velocity proportional to distance (power)
                gBalls[0].vel.x = -dx * CUE_POWER_MULTIPLIER;
                gBalls[0].vel.y = -dy * CUE_POWER_MULTIPLIER;

                gCurrentState = STATE_SIMULATING;
            }
        }
    }
}

/**
 * @brief Updates the game state, including physics simulation.
 */
void update() {
    if (gCurrentState != STATE_SIMULATING) {
        return;
    }

    bool ballsAreMoving = false;

    // --- Physics Simulation Step ---
    for (int i = 0; i < NUM_BALLS; ++i) {
        if (!gBalls[i].isActive) continue;

        // 1. Apply friction
        gBalls[i].vel.x *= FRICTION;
        gBalls[i].vel.y *= FRICTION;

        // 2. Update position
        gBalls[i].pos.x += gBalls[i].vel.x;
        gBalls[i].pos.y += gBalls[i].vel.y;

        // 3. Stop balls with very low velocity
        float speed = sqrt(gBalls[i].vel.x * gBalls[i].vel.x + gBalls[i].vel.y * gBalls[i].vel.y);
        if (speed < MIN_VELOCITY) {
            gBalls[i].vel = (Vec2D){0, 0};
        } else {
            ballsAreMoving = true;
        }

        // 4. Handle collision with cushions
        float tableX1 = (SCREEN_WIDTH - TABLE_WIDTH) / 2.0f + BALL_RADIUS;
        float tableY1 = (SCREEN_HEIGHT - TABLE_HEIGHT) / 2.0f + BALL_RADIUS;
        float tableX2 = tableX1 + TABLE_WIDTH - BALL_DIAMETER;
        float tableY2 = tableY1 + TABLE_HEIGHT - BALL_DIAMETER;

        if (gBalls[i].pos.x < tableX1) { gBalls[i].pos.x = tableX1; gBalls[i].vel.x *= -1; }
        if (gBalls[i].pos.x > tableX2) { gBalls[i].pos.x = tableX2; gBalls[i].vel.x *= -1; }
        if (gBalls[i].pos.y < tableY1) { gBalls[i].pos.y = tableY1; gBalls[i].vel.y *= -1; }
        if (gBalls[i].pos.y > tableY2) { gBalls[i].pos.y = tableY2; gBalls[i].vel.y *= -1; }

        // 5. Handle ball-ball collisions
        for (int j = i + 1; j < NUM_BALLS; ++j) {
            if (!gBalls[j].isActive) continue;

            float dx = gBalls[j].pos.x - gBalls[i].pos.x;
            float dy = gBalls[j].pos.y - gBalls[i].pos.y;
            float distSq = dx * dx + dy * dy;

            if (distSq < BALL_DIAMETER * BALL_DIAMETER) {
                float dist = sqrt(distSq);
                float overlap = (BALL_DIAMETER - dist) / 2.0f;

                // Static resolution (move balls apart)
                gBalls[i].pos.x -= overlap * (dx / dist);
                gBalls[i].pos.y -= overlap * (dy / dist);
                gBalls[j].pos.x += overlap * (dx / dist);
                gBalls[j].pos.y += overlap * (dy / dist);

                // Dynamic resolution (exchange velocity)
                float nx = dx / dist; // Normal vector x
                float ny = dy / dist; // Normal vector y

                // Dot products
                float p1 = gBalls[i].vel.x * nx + gBalls[i].vel.y * ny;
                float p2 = gBalls[j].vel.x * nx + gBalls[j].vel.y * ny;

                // New velocities along the normal
                gBalls[i].vel.x += (p2 - p1) * nx;
                gBalls[i].vel.y += (p2 - p1) * ny;
                gBalls[j].vel.x += (p1 - p2) * nx;
                gBalls[j].vel.y += (p1 - p2) * ny;
            }
        }
        
        // 6. Handle pocketing
        for (int p = 0; p < 6; ++p) {
            float dx = gPockets[p].pos.x - gBalls[i].pos.x;
            float dy = gPockets[p].pos.y - gBalls[i].pos.y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist < POCKET_RADIUS) {
                gBalls[i].isActive = false;
                // Simple game over logic
                if (gBalls[i].id == 8) {
                    gCurrentState = STATE_GAME_OVER;
                }
            }
        }
    }

    // If no balls are moving, switch back to aiming state
    if (!ballsAreMoving) {
        gCurrentState = STATE_AIMING;
    }
}


/**
 * @brief Renders all game objects to the screen.
 */
void render() {
    // --- Clear screen (brown background) ---
    SDL_SetRenderDrawColor(gRenderer, 50, 25, 0, 255);
    SDL_RenderClear(gRenderer);

    // --- Draw table ---
    // Felt
    SDL_Rect tableRect = {
        (SCREEN_WIDTH - TABLE_WIDTH) / 2,
        (SCREEN_HEIGHT - TABLE_HEIGHT) / 2,
        TABLE_WIDTH,
        TABLE_HEIGHT
    };
    SDL_SetRenderDrawColor(gRenderer, 0, 85, 0, 255);
    SDL_RenderFillRect(gRenderer, &tableRect);

    // --- Draw pockets ---
    for (int i = 0; i < 6; ++i) {
        draw_circle(gPockets[i].pos.x, gPockets[i].pos.y, POCKET_RADIUS, (SDL_Color){0, 0, 0, 255});
    }

    // --- Draw balls ---
    for (int i = 0; i < NUM_BALLS; ++i) {
        if (gBalls[i].isActive) {
            draw_ball(&gBalls[i]);
        }
    }

    // --- Draw cue stick when aiming ---
    if (gCurrentState == STATE_AIMING && gBalls[0].isActive) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_SetRenderDrawColor(gRenderer, 200, 150, 100, 255);
        SDL_RenderDrawLine(gRenderer, gBalls[0].pos.x, gBalls[0].pos.y, mouseX, mouseY);
    }
    
    // --- Draw Game Over text ---
    if (gCurrentState == STATE_GAME_OVER) {
        // This part requires a font, which we haven't loaded,
        // so we'll just change the background color to indicate game over.
        SDL_SetRenderDrawColor(gRenderer, 128, 0, 0, 255);
        SDL_RenderClear(gRenderer);
        // In a full game, you'd render "Game Over" text here.
    }


    // --- Update screen ---
    SDL_RenderPresent(gRenderer);
}

/**
 * @brief Cleans up SDL resources.
 */
void cleanup() {
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    gRenderer = NULL;

    TTF_Quit();
    SDL_Quit();
}

/**
 * @brief Draws a pool ball with an outline and optional stripe.
 * @param ball Pointer to the ball to render.
 */
void draw_ball(Ball* ball) {
    int cx = (int)ball->pos.x;
    int cy = (int)ball->pos.y;

    // Outline for better visibility
    draw_circle(cx, cy, BALL_RADIUS + 2, (SDL_Color){0, 0, 0, 255});

    for (int w = -BALL_RADIUS; w <= BALL_RADIUS; ++w) {
        for (int h = -BALL_RADIUS; h <= BALL_RADIUS; ++h) {
            if (w * w + h * h <= BALL_RADIUS * BALL_RADIUS) {
                SDL_Color color = ball->color;
                if (ball->id > 8 && fabs(h) < BALL_RADIUS * 0.3f) {
                    color = (SDL_Color){255, 255, 255, 255};
                }

                SDL_SetRenderDrawColor(gRenderer, color.r, color.g, color.b, color.a);
                SDL_RenderDrawPoint(gRenderer, cx + w, cy + h);
            }
        }
    }
}


/**
 * @brief A helper function to draw a filled circle.
 * @param centerX The x-coordinate of the circle's center.
 * @param centerY The y-coordinate of the circle's center.
 * @param radius The radius of the circle.
 * @param color The color of the circle.
 */
void draw_circle(int centerX, int centerY, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(gRenderer, color.r, color.g, color.b, color.a);
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(gRenderer, centerX + dx, centerY + dy);
            }
        }
    }
}


// --- Main Entry Point ---
int main(int argc, char* args[]) {
    if (!initialize()) {
        printf("Failed to initialize!\n");
    } else {
        game_loop();
    }

    cleanup();
    return 0;
}
