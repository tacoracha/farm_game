#pragma once

#include "farm/common/Types.h"

#include <string>

namespace farm {

class Game;

class SaveManager {
public:
    static Result<void> Save(const Game& game, const std::string& path);
    static Result<void> Load(const std::string& path, Game& game);
    static bool Exists(const std::string& path);
};

}  // namespace farm

