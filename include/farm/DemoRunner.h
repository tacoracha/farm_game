#pragma once

namespace farm {

class Game;

// Thin orchestration for local runs; rules live in systems (not main).
void RunFarmDemo(Game& game);

}  // namespace farm
