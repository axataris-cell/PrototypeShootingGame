// Shooter 3D goc nhin thu nhat - C++17 / OpenGL 3.3 Core / GLAD 2 / GLFW / GLM / Assimp / miniaudio
//
// WASD: di chuyen | Shift: di bo | Ctrl: ngoi | Space: nhay | Chuot: ngam
// Chuot trai: ban | R: nap dan | 1 / 2 hoac con lan chuot: doi sung | ESC: ve menu

#include "Game.hpp"

int main() {
    Game game;
    if (!game.init()) return -1;
    game.run();
    game.shutdown();
    return 0;
}
