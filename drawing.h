#ifndef DRAWING_H
#define DRAWING_H
#include "cards.h"
#include <raylib.h>
#include <stddef.h>

// Pixel art resolution to upscale from (px)
const extern int WORLD_WIDTH;
const extern int WORLD_HEIGHT;
const extern float CARD_HEIGHT;
const extern float CARD_WIDTH;
const extern Vector2 DECK_POSITION;
const extern Vector2 HAND_POSITIONS[4][2];

Vector2 screen_to_world(Vector2 screen_position);
void init_drawing();
void draw_card(Card, Vector2 position, float rotation, float flip);
void reset_board();
void queue_anim_move(size_t card, Vector2 position, float rotation, float flip);
void queue_anim_wait(float time);
void draw_text(char *text, Vector2 position, int font_size, Color color);
void draw();

#endif
