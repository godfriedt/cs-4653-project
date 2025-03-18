#include "cards.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// These specific values are not valid for a card, but correspond
// to the correct place on the spritesheet
const Card BACKFACE = 14 | Diamond;
const Card JOKER_BLACK = 14 | Club;
const Card JOKER_RED = 14 | Spade;

Card new_card(Face face, Suite suite) { return face | suite; }

Card face_values[CARD_COUNT] = {};

Face get_face(Card card) { return (card & 0xff); }

Suite get_suite(Card card) { return (card & 0xff00); }

void print_card(Card card) {
  switch (get_face(card)) {
  case Ace:
    printf("A");
    break;
  case Two:
    printf("2");
    break;
  case Three:
    printf("3");
    break;
  case Four:
    printf("4");
    break;
  case Five:
    printf("5");
    break;
  case Six:
    printf("6");
    break;
  case Seven:
    printf("7");
    break;
  case Eight:
    printf("8");
    break;
  case Nine:
    printf("9");
    break;
  case Ten:
    printf("10");
    break;
  case Jack:
    printf("J");
    break;
  case Queen:
    printf("Q");
    break;
  case King:
    printf("K");
    break;
  }
  switch (get_suite(card)) {
  case Heart:
    printf("H");
    break;
  case Club:
    printf("C");
    break;
  case Diamond:
    printf("D");
    break;
  case Spade:
    printf("S");
    break;
  }
  printf("\n");
}
// Sorted from ace of hearts to king of spades
void init_face_values() {
  const int FIRST_SUITE = Club;
  const int LAST_SUITE = Heart;
  const int FIRST_CARD = Ace;
  const int LAST_CARD = King;

  Suite suite = FIRST_SUITE;
  Face face = FIRST_CARD;
  for (int i = 0; i < 52; i++) {
    face_values[i] = new_card(face, suite);
    suite += (1 << 8);
    if (suite > LAST_SUITE) {
      face += 1;
      suite = FIRST_SUITE;
    };
    if (face > LAST_CARD)
      face = FIRST_CARD;
  }
}
// Fisher-Yates shuffle
void shuffle_face_values() {
  for (int i = 0; i < 52; i++) {
    Card temp = face_values[i];
    size_t random_card = rand() % 52;
    face_values[i] = face_values[random_card];
    face_values[random_card] = temp;
  }
}
void print_deck(Deck deck) {
  for (int i = 0; i < 52; i++) {
    print_card(deck[i]);
  }
}

void sort(uint16_t *array) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 5 - i - 1; j++) {
      if (array[j] > array[j + 1]) {
        uint16_t temp = array[j];
        array[j] = array[j + 1];
        array[j + 1] = temp;
      }
    }
  }
}

HandValue evaluate_5_cards(Card hand[5]) {
  // Is all the same suite
  int is_flush = 1;
  // Is a straight if aces are high
  int is_straight_high = 1;
  // Is a straight if aces are low
  int is_straight_low = 1;
  int has_two_pair = 0;
  int has_two_kind = 0;
  int has_three_kind = 0;
  int has_four_kind = 0;
  // The highest card (aces high)
  uint16_t high_card = 0;
  uint16_t faces_ace_low[5] = {};
  uint16_t faces_ace_high[5] = {};
  Suite suites[5] = {};
  for (int i = 0; i < 5; i++) {
    uint16_t value = get_face(hand[i]);
    faces_ace_low[i] = value;
    if (value == Ace)
      value = King + 1;
    faces_ace_high[i] = value;
    suites[i] = get_suite(hand[i]);
    if (i > 0)
      is_flush &= suites[i] == suites[i - 1];
    if (high_card < value)
      high_card = value;
  }
  sort(faces_ace_low);
  sort(faces_ace_high);
  int consecutive_counter = 0;
  for (int i = 0; i < 5; i++) {
    if (i > 0) {
      is_straight_low &= faces_ace_low[i] == faces_ace_low[i + 1] + 1;
      is_straight_high &= faces_ace_high[i] == faces_ace_high[i + 1] + 1;
      if (faces_ace_low[i] == faces_ace_low[i - 1]) {
        consecutive_counter += 1;
      } else {
        if (consecutive_counter == 1 && has_two_kind == 0)
          has_two_kind = 1;
        else if (consecutive_counter == 1)
          has_two_pair = 1;
        else if (consecutive_counter == 2)
          has_three_kind = 1;
        else if (consecutive_counter == 3)
          has_four_kind = 1;
        consecutive_counter = 0;
      }
    }
  }
  int is_straight = is_straight_low | is_straight_high;
  // printf("two_kind=%d, three_kind=%d\n", has_two_kind, has_three_kind);

  if (is_flush && is_straight_high)
    return RoyalFlush | Ace;
  else if (is_flush && is_straight_low)
    return StraightFlush | high_card;
  else if (has_four_kind)
    return FourKind | high_card;
  else if (has_three_kind && has_two_kind)
    return FullHouse | high_card;
  else if (is_flush)
    return Flush | high_card;
  else if (is_straight)
    return Straight | high_card;
  else if (has_three_kind)
    return ThreeKind | high_card;
  else if (has_two_pair)
    return TwoPair | high_card;
  else if (has_two_kind == 1)
    return TwoKind | high_card;
  else
    return HighCard | high_card;
};

// QuickPerm algorithm
// https://www.quickperm.org/
HandValue evaluate_hand(Card hand[2], Card board[5]) {
  Card total[7] = {hand[0],  hand[1],  board[0], board[1],
                   board[2], board[3], board[4]};
  int count = 0;
  HandValue top_value = 0;
  int n = 7;
  int p[8] = {};
  for (int i = 0; i < 8; i++) {
    p[i] = i;
  }
  int i = 1;
  while (i < n) {
    count += 1;
    HandValue new_value = evaluate_5_cards(
        (Card[5]){total[0], total[1], total[2], total[3], total[4]});
    if (top_value < new_value) {
      top_value = new_value;
    }
    p[i]--;
    int j = i % 2 * p[i];
    Card temp = total[j];
    total[j] = total[i];
    total[i] = temp;
    i = 1;
    while (p[i] == 0) {
      p[i] = i;
      i++;
    }
  }
  return top_value;
}

char *hand_value_string(HandValue val) {
  switch (val & 0xff00) {
  case HighCard: {
    return "High card";
  }
  case TwoKind: {
    return "Two of a kind";
  }
  case TwoPair: {
    return "Two pairs";
  }
  case ThreeKind: {
    return "Three of a kind";
  }
  case Straight: {
    return "Straight";
  }
  case Flush: {
    return "Flush";
  }
  case FullHouse: {
    return "Full house";
  }
  case FourKind: {
    return "Four of a kind";
  }
  case StraightFlush: {
    return "Straight flush";
  }
  case RoyalFlush: {
    return "Royal flush";
  }
  default: {
    return "Invalid hand value";
  }
  }
}
