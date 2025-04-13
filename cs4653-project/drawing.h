#ifndef DRAWING_H
#define DRAWING_H
#include "cards.h"
#include <raylib.h>
#include <stddef.h>

// Pixel art resolution to upscale from (px)
#define WORLD_WIDTH 640
#define WORLD_HEIGHT 360
#define CARD_HEIGHT 77.0
#define CARD_WIDTH 52.0
const extern Vector2 DECK_POSITION;
const extern Vector2 HAND_POSITIONS[4][2];
extern HandValue display_hand;
typedef enum {
  NoButton,
  BetButton,
  CallButton,
  FoldButton,
} ButtonState;
extern ButtonState button_choice;
extern float bet_spinner_value;

void init_drawing();
void draw_card(Card, Vector2 position, float rotation, float flip);
void queue_anim_move(size_t card, Vector2 position, float rotation, float flip);
void queue_anim_money(size_t wallet, uint16_t new_amount);
void queue_anim_wait(float time);
int is_animations_finished();
void flip_card(size_t card);
void draw_text(char *text, Vector2 position, int font_size, Color color);
void draw();

#endif
