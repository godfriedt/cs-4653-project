#ifndef CARDS_H
#define CARDS_H
#include <stdint.h>
#define CARD_COUNT 52

typedef enum {
  Ace = 1,
  Two = 2,
  Three = 3,
  Four = 4,
  Five = 5,
  Six = 6,
  Seven = 7,
  Eight = 8,
  Nine = 9,
  Ten = 10,
  Jack = 11,
  Queen = 12,
  King = 13,
} Face;
typedef enum {
  Club = 1 << 8,
  Spade = 2 << 8,
  Diamond = 3 << 8,
  Heart = 4 << 8,
} Suite;
// Cards are nullable, 0 does not represent a valid card
typedef int16_t Card;
typedef Card *Deck;

extern Card deck[CARD_COUNT];

const extern Card BACKFACE;
const extern Card JOKER_BLACK;
const extern Card JOKER_RED;

Card new_card(Face, Suite);
Face get_face(Card);
Suite get_suite(Card);
void print_card(Card);
void init_deck(Card *);
void shuffle_deck(Deck);
void print_deck(Deck);

typedef enum {
  HighCard = 0 << 8,
  TwoKind = 1 << 8,
  TwoPair = 2 << 8,
  ThreeKind = 3 << 8,
  Straight = 4 << 8,
  Flush = 5 << 8,
  FullHouse = 6 << 8,
  FourKind = 7 << 8,
  StraightFlush = 8 << 8,
  RoyalFlush = 9 << 8,
} HandRank;
typedef uint16_t HandValue;
HandValue evaluate_hand(Card hand[2], Card board[5]);
char *hand_value_string(HandValue val);
#endif
