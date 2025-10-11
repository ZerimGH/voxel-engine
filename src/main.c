#include "game.h"

int main(void) {
  Game *game = create_game();
  if (!game) {
    fprintf(stderr, "(main): Error: create_game() returned NULL.\n");
    return 1;
  }

  while (!game_over(game)) {
    update_game(game);
    render_game(game);
  }

  destroy_game(&game);
  return 0;
}
