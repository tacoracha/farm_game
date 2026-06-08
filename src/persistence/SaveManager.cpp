#include "farm/persistence/SaveManager.h"

#include "farm/common/Constants.h"
#include "farm/core/Game.h"

#include <algorithm>
#include <cstdio>
#include <chrono>
#include <fstream>
#include <sstream>

namespace farm {
namespace {

int ItemToInt(ItemId item) { return static_cast<int>(item); }
ItemId IntToItem(int value) { return static_cast<ItemId>(value); }
int StateToInt(PlotState state) { return static_cast<int>(state); }
PlotState IntToPlotState(int value) { return static_cast<PlotState>(value); }
int WeatherToInt(WeatherType weather) { return static_cast<int>(weather); }
WeatherType IntToWeather(int value) { return static_cast<WeatherType>(value); }

long long CurrentUnixSeconds() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

bool ReadTag(std::istream& in, const std::string& expected) {
    std::string tag;
    in >> tag;
    return tag == expected;
}

}  // namespace

bool SaveManager::Exists(const std::string& path) {
    std::ifstream in(path);
    return in.good();
}

Result<void> SaveManager::Save(const Game& game, const std::string& path) {
    const std::string temp_path = path + ".tmp";
    std::ofstream out(temp_path, std::ios::trunc);
    if (!out) {
        return Result<void>::failure(ErrorCode::SaveOpenFailed);
    }

    const TimeSnapshot time = game.Time().Snapshot();
    const WeatherSnapshot weather = game.Weather().Snapshot();
    out << "FARM_SAVE " << kSaveVersion << "\n";
    out << "TIME " << time.tick << " " << static_cast<int>(time.speed) << " "
        << game.LastAutoSaveTick() << "\n";
    out << "WEATHER " << WeatherToInt(weather.weather) << " " << weather.remaining_ticks << "\n";
    out << "EVENT " << game.LastRandomEventTick() << "\n";
    out << "PLAYER " << game.Player().Gold() << " " << game.Player().Level() << " "
        << game.Player().Experience() << " " << game.Player().WarehouseCapacity() << "\n";
    for (ItemId item : AllItems()) {
        out << "ITEM " << ItemToInt(item) << " " << game.Player().ItemCount(item) << " "
            << (game.Player().IsItemLocked(item) ? 1 : 0) << " "
            << (game.Player().IsSeedUnlocked(item) ? 1 : 0) << "\n";
    }
    out << "UNLOCKS " << game.Player().UnlockedContentForSave().size() << "\n";
    for (UnlockId id : game.Player().UnlockedContentForSave()) {
        out << "UNLOCK " << static_cast<int>(id) << "\n";
    }
    out << "PLOTS " << game.Planting().Plots().size() << "\n";
    for (const PlotData& plot : game.Planting().Plots()) {
        out << "PLOT " << StateToInt(plot.state) << " " << static_cast<int>(plot.water) << " "
            << ItemToInt(plot.crop) << " " << plot.planted_tick << " " << plot.mature_tick << " "
            << (plot.fertilized ? 1 : 0) << " " << plot.growth_remainder << "\n";
    }
    out << "RANCH " << game.Ranch().NextAnimalIdForSave() << " "
        << game.Ranch().Facilities().size() << "\n";
    for (const RanchFacilityData& facility : game.Ranch().Facilities()) {
        out << "FACILITY " << facility.id << " " << static_cast<int>(facility.kind) << " "
            << facility.level << " " << facility.capacity << " " << facility.animals.size()
            << "\n";
        for (const AnimalData& animal : facility.animals) {
            out << "ANIMAL " << animal.id << " " << static_cast<int>(animal.kind) << " "
                << static_cast<int>(animal.quality) << " " << static_cast<int>(animal.state)
                << " " << animal.finish_tick << "\n";
        }
    }
    out << "WORKSHOP " << game.Workshop().ShelfChickenFeed() << " "
        << game.Workshop().ShelfCowFeed() << " "
        << game.Workshop().Queue().size() << "\n";
    for (const ProductionJob& job : game.Workshop().Queue()) {
        out << "JOB " << static_cast<int>(job.recipe) << " " << job.remaining_ticks << "\n";
    }
    out << "ORDERS " << game.Orders().NextOrderIdForSave() << " "
        << game.Orders().SequenceForSave() << " " << game.Orders().Orders().size() << "\n";
    for (const OrderData& order : game.Orders().Orders()) {
        out << "ORDER " << order.id << " " << static_cast<int>(order.state) << " "
            << ItemToInt(order.item) << " " << order.quantity << " " << order.reward_gold << " "
            << order.reward_exp << " " << order.cooldown_until_tick << " "
            << (order.locked ? 1 : 0) << "\n";
    }
    out << "REALTIME " << CurrentUnixSeconds() << "\n";
    out << "END\n";
    out.close();
    if (!out) {
        return Result<void>::failure(ErrorCode::SaveWriteFailed);
    }
    std::remove(path.c_str());
    if (std::rename(temp_path.c_str(), path.c_str()) != 0) {
        return Result<void>::failure(ErrorCode::SaveWriteFailed);
    }
    return Result<void>::success();
}

Result<void> SaveManager::Load(const std::string& path, Game& game) {
    std::ifstream in(path);
    if (!in) {
        return Result<void>::failure(ErrorCode::SaveOpenFailed);
    }

    std::string magic;
    int version = 0;
    in >> magic >> version;
    if (magic != "FARM_SAVE") {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    if (version != kSaveVersion) {
        return Result<void>::failure(ErrorCode::SaveVersionMismatch);
    }

    Game loaded = Game::NewGame();

    int tick = 0;
    int speed = 1;
    int auto_tick = 0;
    if (!ReadTag(in, "TIME") || !(in >> tick >> speed >> auto_tick)) {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    loaded.Time().SetTickForLoad(tick);
    loaded.Time().SetSpeed(static_cast<GameSpeed>(speed));
    loaded.SetLastAutoSaveTickForLoad(auto_tick);

    int weather = 0;
    int weather_left = 0;
    if (!ReadTag(in, "WEATHER") || !(in >> weather >> weather_left)) {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    loaded.Weather().SetForLoad(IntToWeather(weather), weather_left);

    std::string tag;
    in >> tag;
    if (tag == "EVENT") {
        int last_event_tick = 0;
        if (!(in >> last_event_tick)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        loaded.SetLastRandomEventTickForLoad(last_event_tick);
    } else {
        in.putback('\n');
        for (auto it = tag.rbegin(); it != tag.rend(); ++it) {
            in.putback(*it);
        }
    }

    int gold = 0;
    int level = 1;
    int exp = 0;
    int cap = 0;
    if (!ReadTag(in, "PLAYER") || !(in >> gold >> level >> exp >> cap)) {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    loaded.Player().ClearForLoad();
    loaded.Player().SetGoldForLoad(gold);
    loaded.Player().SetLevelForLoad(level);
    loaded.Player().SetExperienceForLoad(exp);
    loaded.Player().SetWarehouseCapacityForLoad(cap);

    for (int i = 0; i < static_cast<int>(AllItems().size()); ++i) {
        int item = 0;
        int quantity = 0;
        int locked = 0;
        int unlocked = 0;
        if (!ReadTag(in, "ITEM") || !(in >> item >> quantity >> locked >> unlocked)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        loaded.Player().SetItemForLoad(IntToItem(item), quantity);
        loaded.Player().SetItemLocked(IntToItem(item), locked != 0);
        if (IsSeed(IntToItem(item))) {
            loaded.Player().SetSeedUnlockedForLoad(IntToItem(item), unlocked != 0);
        }
    }

    std::size_t unlock_count = 0;
    in >> tag;
    if (tag == "UNLOCKS") {
        if (!(in >> unlock_count)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        for (std::size_t i = 0; i < unlock_count; ++i) {
            int unlock_id = 0;
            if (!ReadTag(in, "UNLOCK") || !(in >> unlock_id)) {
                return Result<void>::failure(ErrorCode::SaveCorrupted);
            }
            loaded.Player().SetUnlockedForLoad(static_cast<UnlockId>(unlock_id), true);
        }
    } else {
        in.putback('\n');
        for (auto it = tag.rbegin(); it != tag.rend(); ++it) {
            in.putback(*it);
        }
    }

    std::size_t plot_count = 0;
    if (!ReadTag(in, "PLOTS") || !(in >> plot_count)) {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    std::vector<PlotData> plots;
    for (std::size_t i = 0; i < plot_count; ++i) {
        int state = 0;
        int water = 0;
        int crop = 0;
        int fertilized = 0;
        PlotData plot;
        if (!ReadTag(in, "PLOT") ||
            !(in >> state >> water >> crop >> plot.planted_tick >> plot.mature_tick >>
              fertilized >> plot.growth_remainder)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        plot.state = IntToPlotState(state);
        plot.water = static_cast<PlotWaterState>(water);
        plot.crop = IntToItem(crop);
        plot.fertilized = fertilized != 0;
        plots.push_back(plot);
    }
    loaded.Planting().SetPlotsForLoad(plots);

    int next_animal = 1;
    std::size_t facility_count = 0;
    if (!ReadTag(in, "RANCH") || !(in >> next_animal >> facility_count)) {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    std::vector<RanchFacilityData> facilities;
    for (std::size_t i = 0; i < facility_count; ++i) {
        RanchFacilityData facility;
        int kind = 0;
        std::size_t animal_count = 0;
        if (!ReadTag(in, "FACILITY") ||
            !(in >> facility.id >> kind >> facility.level >> facility.capacity >> animal_count)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        facility.kind = static_cast<RanchFacilityKind>(kind);
        for (std::size_t j = 0; j < animal_count; ++j) {
            AnimalData animal;
            int animal_kind = 0;
            int quality = 0;
            int state = 0;
            if (!ReadTag(in, "ANIMAL") ||
                !(in >> animal.id >> animal_kind >> quality >> state >> animal.finish_tick)) {
                return Result<void>::failure(ErrorCode::SaveCorrupted);
            }
            animal.kind = static_cast<AnimalKind>(animal_kind);
            animal.quality = static_cast<AnimalQuality>(quality);
            animal.state = static_cast<AnimalState>(state);
            facility.animals.push_back(animal);
        }
        facilities.push_back(facility);
    }
    loaded.Ranch().SetFacilitiesForLoad(facilities, next_animal);

    int shelf = 0;
    int cow_shelf = 0;
    std::size_t queue_count = 0;
    if (!ReadTag(in, "WORKSHOP") || !(in >> shelf >> cow_shelf >> queue_count)) {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    std::vector<ProductionJob> queue;
    for (std::size_t i = 0; i < queue_count; ++i) {
        int recipe = 0;
        ProductionJob job;
        if (!ReadTag(in, "JOB") || !(in >> recipe >> job.remaining_ticks)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        job.recipe = static_cast<RecipeId>(recipe);
        queue.push_back(job);
    }
    loaded.Workshop().SetForLoad(queue, shelf, cow_shelf);

    int next_order = 1;
    int sequence = 0;
    std::size_t order_count = 0;
    if (!ReadTag(in, "ORDERS") || !(in >> next_order >> sequence >> order_count)) {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    std::vector<OrderData> orders;
    for (std::size_t i = 0; i < order_count; ++i) {
        OrderData order;
        int state = 0;
        int item = 0;
        int locked = 0;
        if (!ReadTag(in, "ORDER") ||
            !(in >> order.id >> state >> item >> order.quantity >> order.reward_gold >>
              order.reward_exp >> order.cooldown_until_tick >> locked)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        order.state = static_cast<OrderState>(state);
        order.item = IntToItem(item);
        order.locked = locked != 0;
        orders.push_back(order);
    }
    loaded.Orders().SetForLoad(orders, next_order, sequence);

    long long saved_real_time = 0;
    in >> tag;
    if (tag == "REALTIME") {
        if (!(in >> saved_real_time)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        if (!ReadTag(in, "END")) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
    } else if (tag != "END") {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    if (saved_real_time > 0) {
        const long long offline_seconds = CurrentUnixSeconds() - saved_real_time;
        loaded.ProcessOfflineSeconds(static_cast<int>(std::max(0LL, offline_seconds)));
    }
    game = loaded;
    return Result<void>::success();
}

}  // namespace farm
