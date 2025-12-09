#include "Game.hpp"

Game *game = nullptr;

int main(int argc, char *argv[]) {
  game = new Game();

  if (game->init("Zombie Flood", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                 800, 600, false)) {
    while (game->running()) {
      game->handleEvents();
      game->update();
      game->render();
    }
  } else {
    std::cerr << "Game Init Failed" << std::endl;
    return -1;
  }

  game->clean();
  delete game;

  return 0;
}
