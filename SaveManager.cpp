#include "farm/persistence/SaveManager.h"

#include "farm/achievement/AchievementSystem.h"
#include "farm/common/Constants.h"
#include "farm/core/Game.h"
#include "farm/dailytask/DailyTaskSystem.h"
#include "farm/fishing/FishingSystem.h"
#include "farm/merchant/TravelingMerchantSystem.h"

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
        << game.Player().Experience() << " " << game.Player().WarehouseCapacity() << " "
        << game.Player().WarehouseLevel() << "\n";
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
    out << "GREENHOUSE " << game.Planting().GreenhousePlots().size() << "\n";
    for (const PlotData& plot : game.Planting().GreenhousePlots()) {
        out << "GHPLOT " << StateToInt(plot.state) << " " << static_cast<int>(plot.water) << " "
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
                << " " << animal.finish_tick << " " << animal.mood << " "
                << animal.last_fed_tick << " " << animal.age_ticks << " "
                << (animal.is_baby ? 1 : 0) << "\n";
        }
    }
    out << "WORKSHOP " << game.Workshop().ShelfChickenFeed() << " "
        << game.Workshop().ShelfCowFeed() << " "
        << game.Workshop().ShelfBread() << " "
        << game.Workshop().ShelfCheese() << " "
        << game.Workshop().ShelfJam() << " "
        << game.Workshop().Queue().size() << "\n";
    for (const ProductionJob& job : game.Workshop().Queue()) {
        out << "JOB " << static_cast<int>(job.recipe) << " " << job.remaining_ticks << "\n";
    }
    out << "ORDERS " << game.Orders().NextOrderIdForSave() << " "
        << game.Orders().SequenceForSave() << " " << game.Orders().Orders().size() << "\n";
    for (const OrderData& order : game.Orders().Orders()) {
        out << "ORDER " << order.id << " " << static_cast<int>(order.state) << " "
            << order.requirements.size() << " " << order.reward_gold << " "
            << order.reward_exp << " " << order.cooldown_until_tick << " "
            << (order.locked ? 1 : 0) << " " << order.label << "\n";
        for (const OrderRequirement& req : order.requirements) {
            out << "REQ " << ItemToInt(req.item) << " " << req.quantity << "\n";
        }
    }
    out << "SEASON " << static_cast<int>(game.Season().Current())
        << " " << game.Season().SeasonStartTickForSave() << "\n";
    out << "REALTIME " << CurrentUnixSeconds() << "\n";
    out << "END\n";
    // Achievement data after END (optional, won't break old loaders)
    auto& ach = AchievementSystem::Instance();
    out << "ACHIEVE " << ach.TotalHarvests() << " " << ach.WheatHarvests() << " "
        << ach.TotalProcessed() << " " << ach.TotalOrders() << " "
        << ach.TotalGoldEarned() << " " << ach.UnlockCount() << " "
        << ach.SeasonHarvestCount() << " " << ach.SeedsUnlocked() << " "
        << ach.BuildingsUnlocked() << " " << ach.RecipesKnown() << " "
        << ach.SeenSeasons().size();
    for (Season s : ach.SeenSeasons()) out << " " << static_cast<int>(s);
    out << " " << ach.HarvestedCropsForSave().size();
    for (ItemId c : ach.HarvestedCropsForSave()) out << " " << static_cast<int>(c);
    out << " " << ach.OwnedAnimalsForSave().size();
    for (AnimalKind a : ach.OwnedAnimalsForSave()) out << " " << static_cast<int>(a);
    int done_count = 0;
    for (const Achievement& a : ach.All()) if (a.completed) ++done_count;
    out << " " << done_count;
    for (const Achievement& a : ach.All()) if (a.completed) out << " " << static_cast<int>(a.id);
    for (const Achievement& a : ach.All()) out << " " << a.current;
    out << "\n";
    // Daily task save
    auto& dts = DailyTaskSystem::Instance();
    out << "DAILYTASK " << dts.LastRefreshDay() << " " << dts.TodayTasks().size() << "\n";
    for (const DailyTask& t : dts.TodayTasks()) {
        out << "DTASK " << static_cast<int>(t.id) << " " << t.current << " "
            << (t.completed ? 1 : 0) << "\n";
    }
    // Merchant save
    auto& mer = TravelingMerchantSystem::Instance();
    out << "MERCHANT " << (mer.PresentForSave() ? 1 : 0) << " " << mer.AppearTickForSave()
        << " " << mer.ItemsForSave().size() << " " << mer.OffersForSave().size() << "\n";
    for (const MerchantItem& mi : mer.ItemsForSave())
        out << "MITEM " << static_cast<int>(mi.item) << " " << mi.price << " " << mi.stock << " " << (mi.sold_out ? 1 : 0) << "\n";
    for (const MerchantOffer& mo : mer.OffersForSave())
        out << "MOFFER " << static_cast<int>(mo.item) << " " << mo.price << " " << mo.max_buy << " " << mo.bought << "\n";
    // Fishing save
    auto& fish = FishingSystem::Instance();
    out << "FISHING " << fish.BaitForSave() << " " << fish.RodForSave() << " "
        << fish.CatchesForSave() << " " << fish.CollectionForSave().size() << "\n";
    for (const FishRecord& fr : fish.CollectionForSave())
        out << "FISHREC " << fr.fish_id << " " << fr.count << " " << fr.max_size << "\n";

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
    int wh_level = 1;
    if (!ReadTag(in, "PLAYER") || !(in >> gold >> level >> exp >> cap >> wh_level)) {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }
    loaded.Player().ClearForLoad();
    loaded.Player().SetGoldForLoad(gold);
    loaded.Player().SetLevelForLoad(level);
    loaded.Player().SetExperienceForLoad(exp);
    loaded.Player().SetWarehouseLevelForLoad(wh_level);
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

    // Read greenhouse plots (optional, may not exist in old saves)
    std::size_t gh_count = 0;
    in >> tag;
    if (tag == "GREENHOUSE") {
        if (!(in >> gh_count)) return Result<void>::failure(ErrorCode::SaveCorrupted);
        std::vector<PlotData> gh_plots;
        for (std::size_t i = 0; i < gh_count; ++i) {
            PlotData plot;
            int state = 0, water = 0, crop = 0, fertilized = 0;
            if (!ReadTag(in, "GHPLOT") || !(in >> state >> water >> crop >> plot.planted_tick >>
                  plot.mature_tick >> fertilized >> plot.growth_remainder))
                return Result<void>::failure(ErrorCode::SaveCorrupted);
            plot.state = static_cast<PlotState>(state);
            plot.water = static_cast<PlotWaterState>(water);
            plot.crop = IntToItem(crop);
            plot.fertilized = fertilized != 0;
            gh_plots.push_back(plot);
        }
        loaded.Planting().SetGreenhouseForLoad(gh_plots);
        // Read next tag for RANCH
        if (!ReadTag(in, "RANCH")) return Result<void>::failure(ErrorCode::SaveCorrupted);
    } else if (tag != "RANCH") {
        return Result<void>::failure(ErrorCode::SaveCorrupted);
    }

    int next_animal = 1;
    std::size_t facility_count = 0;
    if (!(in >> next_animal >> facility_count)) {
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
            int baby = 0;
            if (!ReadTag(in, "ANIMAL") ||
                !(in >> animal.id >> animal_kind >> quality >> state >> animal.finish_tick >>
                  animal.mood >> animal.last_fed_tick >> animal.age_ticks >> baby)) {
                return Result<void>::failure(ErrorCode::SaveCorrupted);
            }
            animal.is_baby = baby != 0;
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
    int bread_shelf = 0;
    int cheese_shelf = 0;
    int jam_shelf = 0;
    std::size_t queue_count = 0;
    if (!ReadTag(in, "WORKSHOP") ||
        !(in >> shelf >> cow_shelf >> bread_shelf >> cheese_shelf >> jam_shelf >> queue_count)) {
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
    loaded.Workshop().SetForLoad(queue, shelf, cow_shelf, bread_shelf, cheese_shelf, jam_shelf);

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
        std::size_t req_count = 0;
        int locked = 0;
        if (!ReadTag(in, "ORDER") ||
            !(in >> order.id >> state >> req_count >> order.reward_gold >>
              order.reward_exp >> order.cooldown_until_tick >> locked >> order.label)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        order.state = static_cast<OrderState>(state);
        order.locked = locked != 0;
        for (std::size_t j = 0; j < req_count; ++j) {
            int item = 0;
            int qty = 0;
            if (!ReadTag(in, "REQ") || !(in >> item >> qty)) {
                return Result<void>::failure(ErrorCode::SaveCorrupted);
            }
            order.requirements.push_back({IntToItem(item), qty});
        }
        orders.push_back(order);
    }
    loaded.Orders().SetForLoad(orders, next_order, sequence);

    long long saved_real_time = 0;
    in >> tag;
    if (tag == "SEASON") {
        int season_val = 0;
        int s_tick = 0;
        if (!(in >> season_val >> s_tick)) {
            return Result<void>::failure(ErrorCode::SaveCorrupted);
        }
        loaded.Season().SetForLoad(static_cast<Season>(season_val), s_tick);
        in >> tag;
    }
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
    // Optional achievement data
    std::string ach_tag;
    if (in >> ach_tag && ach_tag == "ACHIEVE") {
        int th = 0, wh = 0, tp = 0, to = 0, tg = 0, uc = 0, sh = 0, su = 0, bu = 0, rk = 0;
        if (in >> th >> wh >> tp >> to >> tg >> uc >> sh >> su >> bu >> rk) {
            auto& ach = AchievementSystem::Instance();
            ach.ClearForLoad();
            ach.SetTotalHarvests(th); ach.SetWheatHarvests(wh);
            ach.SetTotalProcessed(tp); ach.SetTotalOrders(to);
            ach.SetTotalGoldEarned(tg); ach.SetUnlockCount(uc);
            ach.SetSeasonHarvestCount(sh); ach.SetSeedsUnlocked(su);
            ach.SetBuildingsUnlocked(bu); ach.SetRecipesKnown(rk);
            int cnt = 0, val = 0;
            if (in >> cnt) for (int i = 0; i < cnt; ++i) { if (in >> val) ach.SetSeenSeason(val); }
            if (in >> cnt) for (int i = 0; i < cnt; ++i) { if (in >> val) ach.SetHarvestedCrop(val); }
            if (in >> cnt) for (int i = 0; i < cnt; ++i) { if (in >> val) ach.SetOwnedAnimal(val); }
            if (in >> cnt) for (int i = 0; i < cnt; ++i) {
                if (in >> val) ach.SetProgressForLoad(static_cast<AchievementID>(val), 999, true);
            }
            for (int i = 0; i < static_cast<int>(AchievementID::COUNT); ++i) {
                if (in >> val) ach.SetProgressIfNotDone(static_cast<AchievementID>(i), val);
            }
        }
    }
    // Optional daily task data
    std::string dt_tag;
    if (in >> dt_tag && dt_tag == "DAILYTASK") {
        int day = 0; std::size_t dt_count = 0;
        if (in >> day >> dt_count) {
            DailyTaskSystem::Instance().Init();
            std::vector<DailyTask> dts;
            for (std::size_t i = 0; i < dt_count; ++i) {
                int tid = 0, cur = 0, comp = 0;
                in >> dt_tag >> tid >> cur >> comp;
                DailyTask t;
                t.id = static_cast<DailyTaskID>(tid);
                t.current = cur;
                t.completed = comp != 0;
                dts.push_back(t);
            }
            DailyTaskSystem::Instance().SetForLoad(day, dts);
        }
    }
    // Optional merchant data
    if (in >> dt_tag && dt_tag == "MERCHANT") {
        int mp = 0, mat = 0; std::size_t mic = 0, moc = 0;
        if (in >> mp >> mat >> mic >> moc) {
            TravelingMerchantSystem::Instance().ClearForLoad();
            std::vector<MerchantItem> items;
            for (std::size_t i = 0; i < mic; ++i) {
                int it = 0, pr = 0, st = 0, so = 0;
                in >> dt_tag >> it >> pr >> st >> so;
                items.push_back({static_cast<ItemId>(it), pr, st, so != 0});
            }
            std::vector<MerchantOffer> offers;
            for (std::size_t i = 0; i < moc; ++i) {
                int it = 0, pr = 0, mb = 0, bo = 0;
                in >> dt_tag >> it >> pr >> mb >> bo;
                offers.push_back({static_cast<ItemId>(it), pr, mb, bo});
            }
            TravelingMerchantSystem::Instance().SetForLoad(mp != 0, mat, items, offers);
        }
    }
    // Optional fishing data
    if (in >> dt_tag && dt_tag == "FISHING") {
        int bait = 0, rod = 1, catches = 0; std::size_t fc = 0;
        if (in >> bait >> rod >> catches >> fc) {
            FishingSystem::Instance().ClearForLoad();
            std::vector<FishRecord> col;
            for (std::size_t i = 0; i < fc; ++i) {
                int fid = 0, cnt = 0, mx = 0;
                in >> dt_tag >> fid >> cnt >> mx;
                col.push_back({fid, cnt, mx});
            }
            FishingSystem::Instance().SetForLoad(bait, rod, catches, col);
        }
    }
    game = loaded;
    return Result<void>::success();
}

}  // namespace farm
