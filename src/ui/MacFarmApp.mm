#ifdef __APPLE__

#include "farm/ui/MacFarmApp.h"

#include "farm/common/Constants.h"
#include "farm/core/Game.h"
#include "farm/core/UnlockGraph.h"
#include "farm/persistence/SaveManager.h"

#import <Cocoa/Cocoa.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace {

enum class Screen {
    Menu,
    Farm,
    Ranch,
    Workshop,
    Orders,
    Warehouse,
    Shop,
    Fishing,
    Unlock,
    Save,
};

enum ButtonId {
    kNewGame = 1,
    kContinueGame,
    kTick,
    kPause,
    kSpeed1,
    kSpeed2,
    kSpeed4,
    kTabFarm = 20,
    kTabRanch,
    kTabWorkshop,
    kTabOrders,
    kTabWarehouse,
    kTabShop,
    kTabFishing,
    kTabUnlock,
    kTabSave,
    kPlantWheat = 100,
    kPlantCorn,
    kPlantCarrot,
    kPlantTomato,
    kWater,
    kFertilize,
    kHarvest,
    kExpand,
    kFarmModeField,
    kFarmModeGreenhouse,
    kBuildGreenhouse,
    kMakeChickenFeed,
    kMakeChickenFeed3,
    kMakeCowFeed,
    kMakeBread,
    kMakeCheese,
    kMakeJam,
    kClaimProduct,
    kBuildCowBarn,
    kBuildSheepPen,
    kRanchPagePrevious,
    kRanchPageNext,
    kBuyChicken,
    kBuyCow,
    kBuySheep,
    kFeedAll,
    kHarvestAll,
    kFeedSelected,
    kHarvestSelected,
    kWarehouseSellSelected,
    kWarehouseToggleLock,
    kWarehouseUpgrade,
    kWarehousePagePrevious,
    kWarehousePageNext,
    kSaveGame,
    kLoadGame,
    kBuyBait,
    kCastLine,
    kReelIn,
    kUpgradeRod,
    kSellAllFish,
    kPlotBase = 1000,
    kOrderCompleteBase = 2000,
    kOrderAbandonBase = 2100,
    kSellItemBase = 2200,
    kBuyItemBase = 2500,
    kUnlockBase = 2800,
    kWarehouseItemBase = 3100,
    kSelectRanchFacilityBase = 3200,
};

struct MacButton {
    NSRect rect{};
    int id = 0;
    std::string label;
    bool enabled = true;
};

NSString* MacString(const char* text) {
    return [NSString stringWithUTF8String:text == nullptr ? "" : text];
}

NSString* MacString(const std::string& text) {
    return [NSString stringWithUTF8String:text.c_str()];
}

std::string SavePath() {
    return "saves/save01.farm";
}

constexpr int kMacRanchFacilitiesPerPage = 3;
constexpr int kMacWarehouseItemsPerPage = 8;

bool IsPlotButton(int buttonId) {
    return buttonId >= kPlotBase && buttonId < kPlotBase + farm::kMaxPlotCount;
}

bool IsRanchFacilityButton(int buttonId) {
    return buttonId >= kSelectRanchFacilityBase &&
           buttonId < kSelectRanchFacilityBase + kMacRanchFacilitiesPerPage;
}

bool IsWarehouseItemButton(int buttonId) {
    return buttonId >= kWarehouseItemBase &&
           buttonId < kWarehouseItemBase + kMacWarehouseItemsPerPage;
}

bool IsHitOnlyButton(int buttonId) {
    return IsPlotButton(buttonId) || IsRanchFacilityButton(buttonId) || IsWarehouseItemButton(buttonId);
}

std::string ResourcePath(const std::string& relative) {
    namespace fs = std::filesystem;
    const fs::path rel(relative);
    if (fs::exists(rel)) {
        return rel.string();
    }

    NSString* executable = [[NSBundle mainBundle] executablePath];
    if (executable != nil) {
        fs::path base([[executable stringByDeletingLastPathComponent] UTF8String]);
        const fs::path candidates[] = {
            base / rel,
            base.parent_path() / rel,
            base.parent_path().parent_path() / rel,
        };
        for (const fs::path& candidate : candidates) {
            if (fs::exists(candidate)) {
                return candidate.string();
            }
        }
    }
    return relative;
}

NSColor* Rgb(int red, int green, int blue) {
    return [NSColor colorWithCalibratedRed:red / 255.0
                                     green:green / 255.0
                                      blue:blue / 255.0
                                     alpha:1.0];
}

void Fill(NSRect rect, NSColor* color) {
    [color setFill];
    NSRectFill(rect);
}

void FillRounded(NSRect rect, CGFloat radius, NSColor* color) {
    [color setFill];
    [[NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius] fill];
}

void StrokeRounded(NSRect rect, CGFloat radius, NSColor* color, CGFloat width) {
    [color setStroke];
    NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius];
    [path setLineWidth:width];
    [path stroke];
}

void DrawText(const std::string& text, NSRect rect, CGFloat size, NSColor* color, bool bold = false) {
    NSFont* font = bold ? [NSFont boldSystemFontOfSize:size] : [NSFont systemFontOfSize:size];
    NSMutableParagraphStyle* style = [[[NSMutableParagraphStyle alloc] init] autorelease];
    [style setLineBreakMode:NSLineBreakByWordWrapping];
    NSDictionary* attrs = @{
        NSFontAttributeName: font,
        NSForegroundColorAttributeName: color,
        NSParagraphStyleAttributeName: style,
    };
    [MacString(text) drawInRect:rect withAttributes:attrs];
}

const char* WeatherName(farm::WeatherType weather) {
    switch (weather) {
        case farm::WeatherType::Sunny:
            return "晴天";
        case farm::WeatherType::Rainy:
            return "雨天";
        case farm::WeatherType::Cloudy:
            return "多云";
        case farm::WeatherType::Drought:
            return "干旱";
    }
    return "未知";
}

std::string WeatherKey(farm::WeatherType weather) {
    switch (weather) {
        case farm::WeatherType::Sunny:
            return "weather_sunny";
        case farm::WeatherType::Rainy:
            return "weather_rainy";
        case farm::WeatherType::Cloudy:
            return "weather_cloudy";
        case farm::WeatherType::Drought:
            return "weather_storm";
    }
    return "weather_sunny";
}

std::string ScreenTitle(Screen screen) {
    switch (screen) {
        case Screen::Farm:
            return "农田";
        case Screen::Ranch:
            return "牧场";
        case Screen::Workshop:
            return "饲料坊";
        case Screen::Orders:
            return "订单";
        case Screen::Warehouse:
            return "仓库";
        case Screen::Shop:
            return "商店";
        case Screen::Fishing:
            return "钓鱼";
        case Screen::Unlock:
            return "解锁";
        case Screen::Save:
            return "存档";
        case Screen::Menu:
            return "菜单";
    }
    return "农场";
}

std::string BackgroundKey(Screen screen) {
    switch (screen) {
        case Screen::Menu:
            return "menu_background";
        case Screen::Farm:
            return "farm_background";
        case Screen::Ranch:
            return "ranch_background";
        case Screen::Workshop:
            return "workshop_background";
        case Screen::Orders:
            return "orders_background";
        case Screen::Warehouse:
            return "warehouse_background";
        case Screen::Shop:
            return "shop_background";
        case Screen::Fishing:
            return "fishing_background";
        case Screen::Unlock:
            return "unlock_background";
        case Screen::Save:
            return "save_background";
    }
    return "farm_background";
}

std::string CropTextureKey(farm::ItemId crop, int stage) {
    const char* name = "wheat";
    switch (crop) {
        case farm::ItemId::Wheat:
            name = "wheat";
            break;
        case farm::ItemId::Corn:
            name = "corn";
            break;
        case farm::ItemId::Carrot:
            name = "carrot";
            break;
        case farm::ItemId::Tomato:
            name = "tomato";
            break;
        case farm::ItemId::Strawberry:
            name = "strawberry";
            break;
        case farm::ItemId::Pumpkin:
            name = "pumpkin";
            break;
        case farm::ItemId::Mushroom:
            name = "mushroom";
            break;
        default:
            name = "wheat";
            break;
    }
    return std::string("crop_") + name + "_" + std::to_string(std::clamp(stage, 0, 4));
}

int CropStage(const farm::PlotView& plot) {
    if (plot.state == farm::PlotState::Mature) {
        return 4;
    }
    if (plot.state == farm::PlotState::Idle) {
        return 0;
    }
    const farm::ItemInfo& info = farm::GetItemInfo(plot.crop);
    const int total = std::max(1, info.grow_ticks);
    const int elapsed = std::max(0, total - plot.remaining_ticks);
    return std::clamp(1 + elapsed * 3 / total, 1, 3);
}

std::string ResultMessage(const char* action, farm::ErrorCode code) {
    std::string message = action;
    message += "：";
    message += farm::ToString(code);
    return message;
}

std::string ItemIconKey(farm::ItemId item) {
    switch (item) {
        case farm::ItemId::WheatSeed:
            return "seed_wheat";
        case farm::ItemId::CornSeed:
            return "seed_corn";
        case farm::ItemId::CarrotSeed:
            return "seed_carrot";
        case farm::ItemId::TomatoSeed:
            return "seed_tomato";
        case farm::ItemId::StrawberrySeed:
            return "seed_strawberry";
        case farm::ItemId::PumpkinSeed:
            return "seed_pumpkin";
        case farm::ItemId::MushroomSeed:
            return "seed_mushroom";
        case farm::ItemId::Wheat:
            return "wheat";
        case farm::ItemId::Corn:
            return "corn";
        case farm::ItemId::Carrot:
            return "carrot";
        case farm::ItemId::Tomato:
            return "tomato";
        case farm::ItemId::Strawberry:
            return "strawberry";
        case farm::ItemId::Pumpkin:
            return "pumpkin";
        case farm::ItemId::Mushroom:
            return "mushroom";
        case farm::ItemId::ChickenFeed:
        case farm::ItemId::CowFeed:
            return "feed";
        case farm::ItemId::Egg:
            return "egg";
        case farm::ItemId::Milk:
            return "milk";
        case farm::ItemId::Wool:
            return "wool";
        case farm::ItemId::Fertilizer:
            return "fertilizer";
        case farm::ItemId::Bread:
            return "bread";
        case farm::ItemId::Cheese:
            return "cheese";
        case farm::ItemId::Jam:
            return "jam";
        default:
            return "inventory_slot";
    }
}

std::string FacilityName(farm::RanchFacilityKind kind) {
    switch (kind) {
        case farm::RanchFacilityKind::ChickenCoop:
            return "鸡圈";
        case farm::RanchFacilityKind::CowBarn:
            return "牛棚";
        case farm::RanchFacilityKind::SheepPen:
            return "羊圈";
        case farm::RanchFacilityKind::PigPen:
            return "猪圈";
    }
    return "设施";
}

std::string AnimalName(farm::AnimalKind kind) {
    switch (kind) {
        case farm::AnimalKind::Chicken:
            return "鸡";
        case farm::AnimalKind::Cow:
            return "牛";
        case farm::AnimalKind::Sheep:
            return "羊";
        case farm::AnimalKind::Pig:
            return "猪";
    }
    return "动物";
}

farm::AnimalKind FacilityAnimalKind(farm::RanchFacilityKind kind) {
    switch (kind) {
        case farm::RanchFacilityKind::ChickenCoop:
            return farm::AnimalKind::Chicken;
        case farm::RanchFacilityKind::CowBarn:
            return farm::AnimalKind::Cow;
        case farm::RanchFacilityKind::SheepPen:
            return farm::AnimalKind::Sheep;
        case farm::RanchFacilityKind::PigPen:
            return farm::AnimalKind::Pig;
    }
    return farm::AnimalKind::Chicken;
}

farm::ItemId AnimalFeed(farm::AnimalKind kind) {
    return kind == farm::AnimalKind::Chicken ? farm::ItemId::ChickenFeed : farm::ItemId::CowFeed;
}

farm::ItemId AnimalProduct(farm::AnimalKind kind) {
    switch (kind) {
        case farm::AnimalKind::Chicken:
            return farm::ItemId::Egg;
        case farm::AnimalKind::Cow:
            return farm::ItemId::Milk;
        case farm::AnimalKind::Sheep:
        case farm::AnimalKind::Pig:
            return farm::ItemId::Wool;
    }
    return farm::ItemId::Egg;
}

int AnimalPurchaseCost(farm::AnimalKind kind) {
    switch (kind) {
        case farm::AnimalKind::Chicken:
            return farm::kChickenCost;
        case farm::AnimalKind::Cow:
            return farm::kCowCost;
        case farm::AnimalKind::Sheep:
        case farm::AnimalKind::Pig:
            return farm::kSheepCost;
    }
    return farm::kChickenCost;
}

farm::UnlockId AnimalUnlock(farm::AnimalKind kind) {
    switch (kind) {
        case farm::AnimalKind::Chicken:
            return farm::UnlockId::Chicken;
        case farm::AnimalKind::Cow:
            return farm::UnlockId::Cow;
        case farm::AnimalKind::Sheep:
        case farm::AnimalKind::Pig:
            return farm::UnlockId::Sheep;
    }
    return farm::UnlockId::Chicken;
}

std::string RecipeName(farm::RecipeId recipe) {
    switch (recipe) {
        case farm::RecipeId::ChickenFeed:
            return "鸡饲料";
        case farm::RecipeId::CowFeed:
            return "牛饲料";
        case farm::RecipeId::Bread:
            return "面包";
        case farm::RecipeId::Cheese:
            return "奶酪";
        case farm::RecipeId::Jam:
            return "果酱";
    }
    return "配方";
}

farm::ItemId RecipeProduct(farm::RecipeId recipe) {
    switch (recipe) {
        case farm::RecipeId::ChickenFeed:
            return farm::ItemId::ChickenFeed;
        case farm::RecipeId::CowFeed:
            return farm::ItemId::CowFeed;
        case farm::RecipeId::Bread:
            return farm::ItemId::Bread;
        case farm::RecipeId::Cheese:
            return farm::ItemId::Cheese;
        case farm::RecipeId::Jam:
            return farm::ItemId::Jam;
    }
    return farm::ItemId::ChickenFeed;
}

int RecipeTicks(farm::RecipeId recipe) {
    switch (recipe) {
        case farm::RecipeId::ChickenFeed:
            return farm::kChickenFeedTicks;
        case farm::RecipeId::CowFeed:
            return farm::kCowFeedTicks;
        case farm::RecipeId::Bread:
            return farm::kBreadTicks;
        case farm::RecipeId::Cheese:
            return farm::kCheeseTicks;
        case farm::RecipeId::Jam:
            return farm::kJamTicks;
    }
    return farm::kChickenFeedTicks;
}

std::string CategoryName(farm::UnlockCategory category) {
    switch (category) {
        case farm::UnlockCategory::Seed:
            return "种子";
        case farm::UnlockCategory::Land:
            return "土地";
        case farm::UnlockCategory::Ranch:
            return "牧场";
        case farm::UnlockCategory::Animal:
            return "动物";
    }
    return "内容";
}

std::string FishingStateName(farm::FishingState state) {
    switch (state) {
        case farm::FishingState::Idle:
            return "空闲";
        case farm::FishingState::Waiting:
            return "等待咬钩";
        case farm::FishingState::Biting:
            return "鱼上钩了";
        case farm::FishingState::ReeledIn:
            return "已收竿";
    }
    return "未知";
}

}  // namespace

@interface FarmMacView : NSView {
@private
    farm::Game game_;
    Screen screen_;
    int selected_plot_;
    int selected_greenhouse_plot_;
    bool farm_show_greenhouse_;
    int selected_ranch_facility_id_;
    int ranch_facility_page_;
    farm::ItemId selected_warehouse_item_;
    int warehouse_page_;
    NSTimer* timer_;
    std::string message_;
    bool message_is_error_;
    std::vector<MacButton> buttons_;
    std::map<std::string, NSImage*> images_;
}
@end

@implementation FarmMacView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self != nil) {
        screen_ = Screen::Menu;
        selected_plot_ = 0;
        selected_greenhouse_plot_ = 0;
        farm_show_greenhouse_ = false;
        selected_ranch_facility_id_ = 1;
        ranch_facility_page_ = 0;
        selected_warehouse_item_ = farm::ItemId::WheatSeed;
        warehouse_page_ = 0;
        timer_ = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                  target:self
                                                selector:@selector(onTimer:)
                                                userInfo:nil
                                                 repeats:YES];
        message_ = "准备就绪。";
        message_is_error_ = false;
        [self setWantsLayer:YES];
    }
    return self;
}

- (void)dealloc {
    [timer_ invalidate];
    for (auto& entry : images_) {
        [entry.second release];
    }
    [super dealloc];
}

- (void)onTimer:(NSTimer*)timer {
    (void)timer;
    if (screen_ == Screen::Menu || game_.Time().IsPaused()) {
        return;
    }
    const farm::ErrorCode code = game_.AdvanceBySpeed().code;
    if (code == farm::ErrorCode::Ok) {
        std::filesystem::create_directories("saves");
        (void)game_.AutoSaveIfNeeded(SavePath());
        [self setNeedsDisplay:YES];
    }
}

- (BOOL)isFlipped {
    return YES;
}

- (NSImage*)imageForKey:(const std::string&)key {
    const auto cached = images_.find(key);
    if (cached != images_.end()) {
        return cached->second;
    }

    std::string path;
    if (key == "menu_background") path = "assets/textures/ui/menu_background.png";
    else if (key == "farm_background") path = "assets/textures/terrain/farm_background.png";
    else if (key == "ranch_background") path = "assets/textures/terrain/ranch_background.png";
    else if (key == "workshop_background") path = "assets/textures/terrain/workshop_background.png";
    else if (key == "orders_background") path = "assets/textures/terrain/orders_background.png";
    else if (key == "warehouse_background") path = "assets/textures/terrain/warehouse_background.png";
    else if (key == "shop_background") path = "assets/textures/terrain/shop_background.png";
    else if (key == "fishing_background") path = "assets/textures/terrain/fishing_background.png";
    else if (key == "unlock_background") path = "assets/textures/terrain/unlock_background.png";
    else if (key == "save_background") path = "assets/textures/terrain/save_background.png";
    else if (key == "top_bar") path = "assets/textures/ui/top_bar.png";
    else if (key == "wood_card") path = "assets/textures/ui/wood_card.png";
    else if (key == "side_panel") path = "assets/textures/ui/side_panel.png";
    else if (key == "button_normal") path = "assets/textures/ui/button_normal.png";
    else if (key == "inventory_slot") path = "assets/textures/ui/inventory_slot.png";
    else if (key == "soil") path = "assets/textures/terrain/soil_dry_tile.png";
    else if (key == "soil_wet") path = "assets/textures/terrain/soil_wet_tile.png";
    else if (key == "coin") path = "assets/textures/items/coin.png";
    else if (key == "wheat") path = "assets/textures/items/wheat.png";
    else if (key == "corn") path = "assets/textures/items/corn.png";
    else if (key == "carrot") path = "assets/textures/items/carrot.png";
    else if (key == "tomato") path = "assets/textures/items/tomato.png";
    else if (key == "strawberry") path = "assets/textures/items/strawberry.png";
    else if (key == "pumpkin") path = "assets/textures/items/pumpkin.png";
    else if (key == "feed") path = "assets/textures/items/feed.png";
    else if (key == "egg") path = "assets/textures/items/egg.png";
    else if (key == "milk") path = "assets/textures/items/milk.png";
    else if (key == "wool") path = "assets/textures/items/wool.png";
    else if (key == "fertilizer") path = "assets/textures/items/fertilizer.png";
    else if (key == "bread") path = "assets/textures/items/bread.png";
    else if (key == "cheese") path = "assets/textures/items/cheese.png";
    else if (key == "seed_wheat") path = "assets/textures/seeds/wheat_seed.png";
    else if (key == "seed_corn") path = "assets/textures/seeds/corn_seed.png";
    else if (key == "seed_carrot") path = "assets/textures/seeds/carrot_seed.png";
    else if (key == "seed_tomato") path = "assets/textures/seeds/tomato_seed.png";
    else if (key == "seed_strawberry") path = "assets/textures/seeds/strawberry_seed.png";
    else if (key == "seed_pumpkin") path = "assets/textures/seeds/pumpkin_seed.png";
    else if (key == "weather_sunny") path = "assets/textures/weather/sunny.png";
    else if (key == "weather_rainy") path = "assets/textures/weather/rainy.png";
    else if (key == "weather_cloudy") path = "assets/textures/weather/cloudy.png";
    else if (key == "weather_storm") path = "assets/textures/weather/storm.png";
    else if (key == "building_feed_mill") path = "assets/textures/buildings/feed_mill.png";
    else if (key == "building_warehouse") path = "assets/textures/buildings/warehouse.png";
    else if (key == "building_shop") path = "assets/textures/buildings/shop.png";
    else if (key == "animal_chicken") path = "assets/textures/animals/chicken/chicken_scene.png";
    else if (key == "animal_cow") path = "assets/textures/animals/cow/cow_scene.png";
    else if (key == "animal_sheep") path = "assets/textures/animals/sheep/sheep_scene.png";
    else if (key.rfind("crop_", 0) == 0) {
        std::string rest = key.substr(5);
        const std::size_t split = rest.rfind('_');
        if (split != std::string::npos) {
            path = "assets/textures/crops/" + rest.substr(0, split) + "/" +
                   rest.substr(split + 1) + "_" +
                   (rest.substr(split + 1) == "0" ? "seed" :
                    rest.substr(split + 1) == "1" ? "sprout" :
                    rest.substr(split + 1) == "2" ? "young" :
                    rest.substr(split + 1) == "3" ? "mid" : "mature") +
                   ".png";
        }
    }

    NSImage* image = nil;
    if (!path.empty()) {
        image = [[NSImage alloc] initWithContentsOfFile:MacString(ResourcePath(path))];
    }
    images_[key] = image;
    return image;
}

- (void)drawImage:(const std::string&)key rect:(NSRect)rect fallback:(NSColor*)fallback {
    NSImage* image = [self imageForKey:key];
    if (image != nil) {
        [image drawInRect:rect];
    } else {
        FillRounded(rect, 4, fallback);
    }
}

- (void)addButton:(int)buttonId label:(const std::string&)label rect:(NSRect)rect enabled:(bool)enabled {
    buttons_.push_back(MacButton{rect, buttonId, label, enabled});
}

- (void)drawButton:(const MacButton&)button {
    NSColor* fallback = button.enabled ? Rgb(205, 139, 54) : Rgb(177, 163, 132);
    [self drawImage:"button_normal" rect:button.rect fallback:fallback];
    StrokeRounded(button.rect, 6, button.enabled ? Rgb(105, 65, 30) : Rgb(117, 108, 88), 1.2);
    DrawText(button.label, NSInsetRect(button.rect, 12, 8), 14,
             button.enabled ? Rgb(255, 250, 226) : Rgb(226, 217, 194), true);
}

- (void)drawMenu {
    [self drawImage:"menu_background" rect:self.bounds fallback:Rgb(126, 172, 94)];
    DrawText("田园时光", NSMakeRect(70, 58, 360, 70), 54, Rgb(42, 63, 35), true);
    DrawText("田园时光", NSMakeRect(64, 52, 360, 70), 54, Rgb(255, 248, 218), true);
    DrawText("播种、经营、养殖，打造属于你的农场", NSMakeRect(66, 128, 520, 32), 22,
             Rgb(255, 248, 218), true);

    [self addButton:kNewGame label:"新游戏" rect:NSMakeRect(66, 198, 210, 48) enabled:true];
    [self addButton:kContinueGame label:"继续游戏" rect:NSMakeRect(66, 264, 210, 48) enabled:true];
    DrawText("种植作物 · 加工饲料 · 经营牧场 · 完成订单", NSMakeRect(66, 352, 560, 28), 16,
             Rgb(255, 248, 218), true);
    DrawText(message_, NSMakeRect(66, 390, 640, 28), 15, Rgb(255, 248, 218), false);
}

- (void)drawStatus {
    [self drawImage:"top_bar" rect:NSMakeRect(0, 0, self.bounds.size.width, 66) fallback:Rgb(75, 113, 59)];

    const farm::TimeSnapshot time = game_.Time().Snapshot();
    const farm::WeatherSnapshot weather = game_.Weather().Snapshot();
    const farm::PlayerState& player = game_.Player();

    auto card = ^(NSRect rect) {
        FillRounded(rect, 6, Rgb(255, 241, 205));
        StrokeRounded(rect, 6, Rgb(121, 74, 35), 1.2);
    };
    card(NSMakeRect(18, 8, 170, 50));
    [self drawImage:"coin" rect:NSMakeRect(30, 19, 28, 28) fallback:Rgb(226, 166, 44)];
    DrawText(std::to_string(player.Gold()) + " 金币", NSMakeRect(68, 22, 105, 22), 15,
             Rgb(64, 43, 24), true);

    card(NSMakeRect(198, 8, 266, 50));
    DrawText("等级 " + std::to_string(player.Level()), NSMakeRect(214, 14, 110, 20), 14,
             Rgb(64, 43, 24), true);
    DrawText(std::to_string(player.Experience()) + "/" + std::to_string(player.ExpToNextLevel()),
             NSMakeRect(368, 14, 80, 20), 13, Rgb(64, 43, 24), true);
    FillRounded(NSMakeRect(214, 39, 226, 9), 3, Rgb(186, 157, 104));
    const int expWidth = 226 * std::min(player.Experience(), player.ExpToNextLevel()) /
                         std::max(1, player.ExpToNextLevel());
    FillRounded(NSMakeRect(214, 39, expWidth, 9), 3, Rgb(92, 148, 45));

    card(NSMakeRect(474, 8, 220, 50));
    const std::string minute = time.minute < 10 ? "0" + std::to_string(time.minute)
                                                : std::to_string(time.minute);
    DrawText("第 " + std::to_string(time.day) + " 天   " + std::to_string(time.hour) + ":" + minute,
             NSMakeRect(500, 23, 170, 22), 15, Rgb(64, 43, 24), true);

    card(NSMakeRect(704, 8, 182, 50));
    [self drawImage:WeatherKey(weather.weather) rect:NSMakeRect(718, 14, 40, 40)
           fallback:Rgb(198, 218, 128)];
    DrawText(WeatherName(weather.weather), NSMakeRect(774, 23, 90, 22), 15, Rgb(64, 43, 24), true);

    card(NSMakeRect(896, 8, 184, 50));
    DrawText(time.paused ? "已暂停" : "速度 " + std::to_string(static_cast<int>(time.speed)) + "x",
             NSMakeRect(930, 23, 120, 22), 15, Rgb(64, 43, 24), true);
}

- (void)drawTabs {
    const std::vector<std::pair<Screen, std::string>> tabs = {
        {Screen::Farm, "农田"},       {Screen::Ranch, "牧场"},
        {Screen::Workshop, "饲料坊"}, {Screen::Orders, "订单"},
        {Screen::Warehouse, "仓库"},  {Screen::Shop, "商店"},
        {Screen::Fishing, "钓鱼"},    {Screen::Unlock, "解锁"},
        {Screen::Save, "存档"},
    };
    CGFloat x = 18;
    for (std::size_t i = 0; i < tabs.size(); ++i) {
        const CGFloat width = i == 2 ? 88 : 68;
        NSRect rect = NSMakeRect(x, 74, width, 40);
        const bool selected = screen_ == tabs[i].first;
        FillRounded(rect, 4, selected ? Rgb(255, 249, 226) : Rgb(173, 108, 50));
        StrokeRounded(rect, 4, selected ? Rgb(214, 157, 43) : Rgb(121, 74, 35), 1.2);
        DrawText(tabs[i].second, NSInsetRect(rect, 10, 10), 14,
                 selected ? Rgb(64, 43, 24) : Rgb(255, 247, 218), true);
        [self addButton:kTabFarm + static_cast<int>(i) label:tabs[i].second rect:rect enabled:true];
        x += width + 6;
    }

    [self addButton:kTick label:"+2分" rect:NSMakeRect(720, 74, 70, 38) enabled:true];
    [self addButton:kPause label:"暂停" rect:NSMakeRect(798, 74, 70, 38) enabled:true];
    [self addButton:kSpeed1 label:"1x" rect:NSMakeRect(878, 74, 58, 38) enabled:true];
    [self addButton:kSpeed2 label:"2x" rect:NSMakeRect(942, 74, 58, 38) enabled:true];
    [self addButton:kSpeed4 label:"4x" rect:NSMakeRect(1006, 74, 70, 38) enabled:true];
}

- (void)drawFarm {
    DrawText(farm_show_greenhouse_ ? "温室  点选温室地块后在右侧操作"
                                   : "农田  点选地块后在右侧操作",
             NSMakeRect(42, 132, 430, 28), 18,
             Rgb(64, 43, 24), true);
    [self addButton:kFarmModeField label:"农田" rect:NSMakeRect(420, 128, 70, 34)
            enabled:farm_show_greenhouse_];
    [self addButton:kFarmModeGreenhouse label:"温室" rect:NSMakeRect(500, 128, 70, 34)
            enabled:!farm_show_greenhouse_];

    const auto plots = farm_show_greenhouse_
                           ? game_.Planting().GreenhouseView(game_.Time().CurrentTick())
                           : game_.Planting().View(game_.Time().CurrentTick());
    const int maxPlots = farm_show_greenhouse_ ? farm::kGreenhouseMaxPlots : farm::kMaxPlotCount;
    const int selectedIndex = farm_show_greenhouse_ ? selected_greenhouse_plot_ : selected_plot_;
    constexpr CGFloat plotSize = 65;
    constexpr CGFloat plotStep = 80;
    constexpr CGFloat startX = 42;
    constexpr CGFloat startY = 166;

    for (int i = 0; i < maxPlots; ++i) {
        const int row = i / 6;
        const int col = i % 6;
        NSRect rect = NSMakeRect(startX + col * plotStep, startY + row * plotStep, plotSize, plotSize);
        if (i >= static_cast<int>(plots.size())) {
            FillRounded(rect, 4, Rgb(104, 148, 78));
            DrawText(farm_show_greenhouse_ ? "未建" : "锁定", NSInsetRect(rect, 12, 24), 13,
                     Rgb(238, 232, 196), true);
            continue;
        }

        const farm::PlotView& plot = plots[static_cast<std::size_t>(i)];
        [self drawImage:plot.water == farm::PlotWaterState::Watered ? "soil_wet" : "soil"
                   rect:rect
               fallback:Rgb(139, 94, 52)];
        if (plot.state != farm::PlotState::Idle) {
            [self drawImage:CropTextureKey(plot.crop, CropStage(plot))
                       rect:NSInsetRect(rect, 7, 5)
                   fallback:plot.state == farm::PlotState::Mature ? Rgb(237, 190, 67)
                                                                   : Rgb(136, 170, 69)];
        }
        StrokeRounded(rect, 4, selectedIndex == i ? Rgb(255, 222, 72) : Rgb(82, 52, 28),
                      selectedIndex == i ? 3.0 : 1.0);
        FillRounded(NSMakeRect(rect.origin.x + 4, rect.origin.y + 4, 20, 18), 3,
                    selectedIndex == i ? Rgb(255, 224, 94) : Rgb(250, 242, 214));
        DrawText(std::to_string(i + 1), NSMakeRect(rect.origin.x + 10, rect.origin.y + 5, 16, 16),
                 12, Rgb(64, 43, 24), true);
        if (plot.state == farm::PlotState::Mature) {
            FillRounded(NSMakeRect(rect.origin.x + 7, rect.origin.y + 44, 51, 17), 3,
                        Rgb(255, 224, 94));
            DrawText("成熟", NSMakeRect(rect.origin.x + 18, rect.origin.y + 45, 36, 15), 12,
                     Rgb(64, 43, 24), true);
        } else if (plot.state == farm::PlotState::Growing) {
            FillRounded(NSMakeRect(rect.origin.x + 5, rect.origin.y + 44, 55, 17), 3,
                        Rgb(250, 242, 214));
            DrawText(std::to_string(plot.remaining_ticks * farm::kGameMinutesPerTick) + "分",
                     NSMakeRect(rect.origin.x + 14, rect.origin.y + 45, 45, 15), 12,
                     Rgb(64, 43, 24), true);
        }
        [self addButton:kPlotBase + i label:"plot" rect:rect enabled:true];
    }

    NSRect panel = NSMakeRect(548, 126, 390, 420);
    [self drawImage:"wood_card" rect:panel fallback:Rgb(255, 241, 205)];
    DrawText("土地管理", NSMakeRect(576, 154, 180, 28), 20, Rgb(64, 43, 24), true);

    const farm::PlotView* selected = nullptr;
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(plots.size())) {
        selected = &plots[static_cast<std::size_t>(selectedIndex)];
    }
    std::string selectedText = farm_show_greenhouse_ ? "请选择已建温室地块" : "请选择已解锁土地";
    if (selected != nullptr) {
        selectedText = "第 " + std::to_string(selectedIndex + 1) + " 块  ";
        if (selected->state == farm::PlotState::Idle) {
            selectedText += "空闲，可播种";
        } else if (selected->state == farm::PlotState::Mature) {
            selectedText += std::string(farm::ToString(selected->crop)) + " 成熟";
        } else {
            selectedText += std::string(farm::ToString(selected->crop)) + " 生长中";
        }
    }
    FillRounded(NSMakeRect(570, 186, 344, 34), 4, Rgb(255, 248, 224));
    StrokeRounded(NSMakeRect(570, 186, 344, 34), 4, Rgb(197, 151, 84), 1.0);
    DrawText(selectedText, NSMakeRect(584, 194, 316, 20), 14, Rgb(64, 43, 24), true);

    const bool canPlant = selected != nullptr && selected->state == farm::PlotState::Idle;
    const bool canTend = selected != nullptr && selected->state == farm::PlotState::Growing;
    const bool canHarvest = selected != nullptr && selected->state == farm::PlotState::Mature;

    [self addButton:kPlantWheat label:"小麦" rect:NSMakeRect(570, 250, 82, 40) enabled:canPlant];
    [self addButton:kPlantCorn label:"玉米" rect:NSMakeRect(660, 250, 82, 40) enabled:canPlant];
    [self addButton:kPlantCarrot label:"胡萝卜" rect:NSMakeRect(750, 250, 82, 40) enabled:canPlant];
    [self addButton:kPlantTomato label:"番茄" rect:NSMakeRect(840, 250, 82, 40) enabled:canPlant];
    [self addButton:kWater label:"浇水" rect:NSMakeRect(570, 320, 166, 42) enabled:canTend && selected->water == farm::PlotWaterState::Dry];
    [self addButton:kFertilize label:farm_show_greenhouse_ ? "温室不施肥" : "施肥"
                rect:NSMakeRect(756, 320, 166, 42)
             enabled:!farm_show_greenhouse_ && canTend && !selected->fertilized];
    [self addButton:kHarvest label:"收割当前作物" rect:NSMakeRect(570, 384, 352, 44) enabled:canHarvest];
    if (farm_show_greenhouse_) {
        [self addButton:kBuildGreenhouse label:"建造温室地块"
                    rect:NSMakeRect(700, 478, 222, 40)
                 enabled:static_cast<int>(plots.size()) < farm::kGreenhouseMaxPlots];
    } else {
        [self addButton:kExpand label:"购买下一块土地" rect:NSMakeRect(700, 478, 222, 40)
                enabled:static_cast<int>(plots.size()) < farm::kMaxPlotCount];
    }

    DrawText("库存", NSMakeRect(970, 132, 80, 24), 18, Rgb(64, 43, 24), true);
    const std::vector<std::pair<std::string, std::string>> inventory = {
        {"seed_wheat", "小麦种 " + std::to_string(game_.Player().ItemCount(farm::ItemId::WheatSeed))},
        {"wheat", "小麦 " + std::to_string(game_.Player().ItemCount(farm::ItemId::Wheat))},
        {"feed", "饲料 " + std::to_string(game_.Player().ItemCount(farm::ItemId::ChickenFeed))},
        {"egg", "鸡蛋 " + std::to_string(game_.Player().ItemCount(farm::ItemId::Egg))},
        {"fertilizer", "肥料 " + std::to_string(game_.Player().ItemCount(farm::ItemId::Fertilizer))},
    };
    for (std::size_t i = 0; i < inventory.size(); ++i) {
        const CGFloat y = 172 + static_cast<CGFloat>(i) * 54;
        [self drawImage:"inventory_slot" rect:NSMakeRect(970, y, 118, 42) fallback:Rgb(255, 241, 205)];
        [self drawImage:inventory[i].first rect:NSMakeRect(980, y + 7, 28, 28) fallback:Rgb(236, 192, 91)];
        DrawText(inventory[i].second, NSMakeRect(1018, y + 12, 68, 18), 13, Rgb(64, 43, 24), true);
    }
}

- (void)drawRanch {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText("牧场", NSMakeRect(70, 154, 180, 32), 22, Rgb(64, 43, 24), true);

    const auto facilities = game_.Ranch().FacilityViews();
    if (!facilities.empty()) {
        bool selectedExists = false;
        for (const farm::RanchFacilityView& facility : facilities) {
            if (facility.id == selected_ranch_facility_id_) {
                selectedExists = true;
                break;
            }
        }
        if (!selectedExists) {
            selected_ranch_facility_id_ = facilities.front().id;
        }
    }

    const int pageCount = std::max(1, (static_cast<int>(facilities.size()) + kMacRanchFacilitiesPerPage - 1) /
                                         kMacRanchFacilitiesPerPage);
    ranch_facility_page_ = std::clamp(ranch_facility_page_, 0, pageCount - 1);
    [self addButton:kRanchPagePrevious label:"<" rect:NSMakeRect(300, 152, 42, 32)
            enabled:ranch_facility_page_ > 0];
    DrawText("设施 " + std::to_string(ranch_facility_page_ + 1) + "/" + std::to_string(pageCount),
             NSMakeRect(352, 160, 120, 24), 14, Rgb(116, 86, 54), true);
    [self addButton:kRanchPageNext label:">" rect:NSMakeRect(455, 152, 42, 32)
            enabled:ranch_facility_page_ + 1 < pageCount];

    const int first = ranch_facility_page_ * kMacRanchFacilitiesPerPage;
    for (int slot = 0; slot < kMacRanchFacilitiesPerPage; ++slot) {
        const int index = first + slot;
        const CGFloat x = 72 + static_cast<CGFloat>(slot) * 300;
        NSRect card = NSMakeRect(x, 205, 270, 145);
        FillRounded(card, 6, Rgb(255, 248, 224));
        StrokeRounded(card, 6, Rgb(143, 96, 48), 1.0);
        if (index >= static_cast<int>(facilities.size())) {
            DrawText("空设施位", NSMakeRect(x + 84, 260, 120, 24), 15, Rgb(116, 86, 54), true);
            continue;
        }

        const farm::RanchFacilityView& facility = facilities[static_cast<std::size_t>(index)];
        if (facility.id == selected_ranch_facility_id_) {
            StrokeRounded(card, 6, Rgb(255, 222, 72), 3.0);
        }
        [self addButton:kSelectRanchFacilityBase + slot label:"facility" rect:card enabled:true];
        const farm::AnimalKind kind = FacilityAnimalKind(facility.kind);
        [self drawImage:kind == farm::AnimalKind::Chicken ? "animal_chicken" :
                        kind == farm::AnimalKind::Cow ? "animal_cow" : "animal_sheep"
                   rect:NSMakeRect(x + 14, 238, 88, 74)
               fallback:Rgb(164, 197, 78)];
        DrawText(FacilityName(facility.kind) + " #" + std::to_string(facility.id),
                 NSMakeRect(x + 114, 226, 130, 22), 15, Rgb(64, 43, 24), true);
        DrawText(std::to_string(facility.animal_count) + "/" + std::to_string(facility.capacity) +
                     "  待喂 " + std::to_string(facility.idle_count),
                 NSMakeRect(x + 114, 256, 140, 20), 13, Rgb(116, 86, 54), false);
        DrawText("生产 " + std::to_string(facility.producing_count) +
                     "  可收 " + std::to_string(facility.ready_count),
                 NSMakeRect(x + 114, 282, 140, 20), 13, Rgb(116, 86, 54), false);
    }

    const farm::RanchFacilityView* selected = nullptr;
    for (const farm::RanchFacilityView& facility : facilities) {
        if (facility.id == selected_ranch_facility_id_) {
            selected = &facility;
            break;
        }
    }

    NSRect controls = NSMakeRect(72, 382, 900, 138);
    FillRounded(controls, 6, Rgb(255, 248, 224));
    StrokeRounded(controls, 6, Rgb(143, 96, 48), 1.0);
    [self addButton:kBuildCowBarn label:"建牛棚" rect:NSMakeRect(92, 462, 118, 38)
            enabled:game_.Player().IsUnlocked(farm::UnlockId::CowBarn)];
    [self addButton:kBuildSheepPen label:"建羊圈" rect:NSMakeRect(224, 462, 118, 38)
            enabled:game_.Player().IsUnlocked(farm::UnlockId::SheepPen)];
    [self addButton:kFeedAll label:"全牧场喂食" rect:NSMakeRect(356, 462, 130, 38) enabled:true];
    [self addButton:kHarvestAll label:"全牧场收获" rect:NSMakeRect(500, 462, 130, 38) enabled:true];

    if (selected != nullptr) {
        const farm::AnimalKind kind = FacilityAnimalKind(selected->kind);
        const int buyButton = kind == farm::AnimalKind::Chicken ? kBuyChicken :
                              kind == farm::AnimalKind::Cow ? kBuyCow : kBuySheep;
        DrawText("当前：" + FacilityName(selected->kind) +
                     "  饲料 " + std::to_string(game_.Player().ItemCount(AnimalFeed(kind))) +
                     "  产物 " + std::string(farm::ToString(AnimalProduct(kind))),
                 NSMakeRect(92, 402, 520, 22), 14, Rgb(64, 43, 24), true);
        [self addButton:buyButton label:"购买" + AnimalName(kind)
                    rect:NSMakeRect(92, 424, 118, 34)
                 enabled:game_.Player().IsUnlocked(AnimalUnlock(kind)) &&
                         selected->animal_count < selected->capacity &&
                         game_.Player().Gold() >= AnimalPurchaseCost(kind)];
        [self addButton:kFeedSelected label:"喂当前设施" rect:NSMakeRect(224, 424, 130, 34)
                enabled:selected->idle_count > 0];
        [self addButton:kHarvestSelected label:"收当前设施" rect:NSMakeRect(368, 424, 130, 34)
                enabled:selected->ready_count > 0];
    }
}

- (void)drawWorkshop {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText("饲料坊", NSMakeRect(70, 154, 180, 32), 22, Rgb(64, 43, 24), true);
    [self drawImage:"building_feed_mill" rect:NSMakeRect(78, 210, 190, 170) fallback:Rgb(207, 139, 54)];
    const farm::WorkshopView view = game_.Workshop().View();
    DrawText("队列 " + std::to_string(view.queue_count) + "/" + std::to_string(view.queue_capacity) +
                 "    货架 " + std::to_string(view.shelf_count) + "/" + std::to_string(view.shelf_capacity),
             NSMakeRect(310, 218, 360, 26), 16, Rgb(64, 43, 24), true);

    struct RecipeButton {
        farm::RecipeId recipe;
        int button;
        int min_level;
        std::string input;
    };
    const std::vector<RecipeButton> recipes = {
        {farm::RecipeId::ChickenFeed, kMakeChickenFeed, 1, "小麦 x2"},
        {farm::RecipeId::CowFeed, kMakeCowFeed, 1, "玉米 x2 + 胡萝卜 x1"},
        {farm::RecipeId::Bread, kMakeBread, 2, "小麦 x3"},
        {farm::RecipeId::Cheese, kMakeCheese, 3, "牛奶 x2"},
        {farm::RecipeId::Jam, kMakeJam, 4, "番茄 x3"},
    };
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        const RecipeButton& recipe = recipes[i];
        const CGFloat x = 310 + static_cast<CGFloat>(i % 2) * 255;
        const CGFloat y = 270 + static_cast<CGFloat>(i / 2) * 72;
        FillRounded(NSMakeRect(x, y, 232, 58), 6, Rgb(255, 248, 224));
        StrokeRounded(NSMakeRect(x, y, 232, 58), 6, Rgb(143, 96, 48), 1.0);
        [self drawImage:ItemIconKey(RecipeProduct(recipe.recipe))
                   rect:NSMakeRect(x + 10, y + 13, 32, 32)
               fallback:Rgb(236, 192, 91)];
        DrawText(RecipeName(recipe.recipe), NSMakeRect(x + 52, y + 9, 92, 20), 14,
                 Rgb(64, 43, 24), true);
        DrawText(recipe.input, NSMakeRect(x + 52, y + 31, 120, 18), 12,
                 Rgb(116, 86, 54), false);
        const bool enabled = view.queue_count < view.queue_capacity &&
                             game_.Player().Level() >= recipe.min_level;
        [self addButton:recipe.button
                  label:enabled ? "制作" : "Lv." + std::to_string(recipe.min_level)
                   rect:NSMakeRect(x + 154, y + 14, 64, 30)
                enabled:enabled];
        if (recipe.recipe == farm::RecipeId::ChickenFeed) {
            [self addButton:kMakeChickenFeed3 label:"x3"
                       rect:NSMakeRect(x + 154, y + 38, 64, 18)
                    enabled:enabled && view.queue_count + 3 <= view.queue_capacity];
        }
    }

    DrawText("货架：鸡料 " + std::to_string(view.chicken_feed_shelf) +
                 "  牛料 " + std::to_string(view.cow_feed_shelf) +
                 "  面包 " + std::to_string(view.bread_shelf) +
                 "  奶酪 " + std::to_string(view.cheese_shelf) +
                 "  果酱 " + std::to_string(view.jam_shelf),
             NSMakeRect(310, 492, 520, 24), 13, Rgb(116, 86, 54), true);
    [self addButton:kClaimProduct label:"领取成品" rect:NSMakeRect(850, 488, 130, 38)
            enabled:view.shelf_count > 0];
}

- (void)drawOrders {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText("订单板", NSMakeRect(70, 154, 180, 32), 22, Rgb(64, 43, 24), true);
    const auto& orders = game_.Orders().Orders();
    for (std::size_t i = 0; i < orders.size(); ++i) {
        const farm::OrderData& order = orders[i];
        const CGFloat x = 70 + static_cast<CGFloat>(i % 2) * 480;
        const CGFloat y = 202 + static_cast<CGFloat>(i / 2) * 170;
        NSRect card = NSMakeRect(x, y, 430, 142);
        [self drawImage:"wood_card" rect:card fallback:Rgb(255, 248, 224)];
        DrawText(order.label.empty() ? "农场订单" : order.label, NSMakeRect(x + 18, y + 16, 220, 24),
                 17, Rgb(64, 43, 24), true);
        DrawText("奖励 " + std::to_string(order.reward_gold) + " 金币 / " +
                     std::to_string(order.reward_exp) + " 经验",
                 NSMakeRect(x + 248, y + 18, 160, 22), 13, Rgb(92, 78, 45), true);
        std::string reqText;
        for (const farm::OrderRequirement& req : order.requirements) {
            if (!reqText.empty()) {
                reqText += "   ";
            }
            reqText += farm::ToString(req.item);
            reqText += " x";
            reqText += std::to_string(req.quantity);
            reqText += " (有 ";
            reqText += std::to_string(game_.Player().ItemCount(req.item));
            reqText += ")";
        }
        DrawText(reqText, NSMakeRect(x + 18, y + 50, 380, 42), 14, Rgb(64, 43, 24), false);
        const bool canDeliver = game_.Orders().CanDeliver(game_.Player(), static_cast<int>(i));
        [self addButton:kOrderCompleteBase + static_cast<int>(i) label:"交付"
                   rect:NSMakeRect(x + 18, y + 98, 112, 34) enabled:canDeliver];
        [self addButton:kOrderAbandonBase + static_cast<int>(i) label:"放弃"
                   rect:NSMakeRect(x + 144, y + 98, 112, 34) enabled:true];
        std::string state = order.locked ? "已锁定"
                            : order.state == farm::OrderState::CoolingDown ? "冷却中"
                                                                            : "可接单";
        DrawText(state, NSMakeRect(x + 280, y + 104, 110, 22), 13,
                 canDeliver ? Rgb(83, 145, 49) : Rgb(116, 86, 54), true);
    }
}

- (void)drawWarehouse {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText("仓库", NSMakeRect(70, 154, 180, 32), 22, Rgb(64, 43, 24), true);
    const int used = game_.Player().WarehouseUsed();
    const int cap = game_.Player().WarehouseCapacity();
    DrawText("容量 " + std::to_string(used) + "/" + std::to_string(cap),
             NSMakeRect(185, 160, 180, 24), 15, Rgb(116, 86, 54), true);
    [self addButton:kWarehouseUpgrade label:"扩容" rect:NSMakeRect(350, 152, 80, 32)
            enabled:game_.Player().WarehouseLevel() < farm::kWarehouseMaxLevel];
    const auto inventory = game_.Player().InventoryView();
    if (inventory.empty()) {
        DrawText("仓库里还没有物品。", NSMakeRect(72, 220, 300, 28), 16, Rgb(64, 43, 24), false);
        return;
    }

    bool selectedExists = false;
    for (const farm::InventoryItemView& item : inventory) {
        if (item.item == selected_warehouse_item_) {
            selectedExists = true;
            break;
        }
    }
    if (!selectedExists) {
        selected_warehouse_item_ = inventory.front().item;
    }
    const int pageCount = std::max(1, (static_cast<int>(inventory.size()) + kMacWarehouseItemsPerPage - 1) /
                                         kMacWarehouseItemsPerPage);
    warehouse_page_ = std::clamp(warehouse_page_, 0, pageCount - 1);
    [self addButton:kWarehousePagePrevious label:"<" rect:NSMakeRect(455, 152, 42, 32)
            enabled:warehouse_page_ > 0];
    DrawText(std::to_string(warehouse_page_ + 1) + "/" + std::to_string(pageCount),
             NSMakeRect(507, 160, 60, 20), 13, Rgb(64, 43, 24), true);
    [self addButton:kWarehousePageNext label:">" rect:NSMakeRect(565, 152, 42, 32)
            enabled:warehouse_page_ + 1 < pageCount];

    const int first = warehouse_page_ * kMacWarehouseItemsPerPage;
    for (int slot = 0; slot < kMacWarehouseItemsPerPage; ++slot) {
        const int index = first + slot;
        if (index >= static_cast<int>(inventory.size())) {
            continue;
        }
        const farm::InventoryItemView& item = inventory[static_cast<std::size_t>(index)];
        const CGFloat x = 72 + static_cast<CGFloat>(slot % 4) * 220;
        const CGFloat y = 210 + static_cast<CGFloat>(slot / 4) * 86;
        NSRect card = NSMakeRect(x, y, 198, 62);
        [self drawImage:"inventory_slot" rect:card fallback:Rgb(255, 248, 224)];
        if (item.item == selected_warehouse_item_) {
            StrokeRounded(card, 6, Rgb(255, 222, 72), 3.0);
        }
        [self drawImage:ItemIconKey(item.item) rect:NSMakeRect(x + 12, y + 14, 34, 34)
               fallback:Rgb(236, 192, 91)];
        DrawText(std::string(farm::ToString(item.item)) + " x" + std::to_string(item.quantity),
                 NSMakeRect(x + 56, y + 10, 118, 22), 14, Rgb(64, 43, 24), true);
        const int sellPrice = farm::GetItemInfo(item.item).sell_price;
        DrawText(item.locked ? "已保护" : (sellPrice > 0 ? "售价 " + std::to_string(sellPrice) : "不可出售"),
                 NSMakeRect(x + 56, y + 34, 90, 20), 12, Rgb(116, 86, 54), false);
        [self addButton:kSellItemBase + static_cast<int>(item.item) label:"卖"
                   rect:NSMakeRect(x + 150, y + 30, 36, 26) enabled:sellPrice > 0 && !item.locked];
        [self addButton:kWarehouseItemBase + slot label:"item" rect:card enabled:true];
    }

    const farm::InventoryItemView* selected = nullptr;
    for (const farm::InventoryItemView& item : inventory) {
        if (item.item == selected_warehouse_item_) {
            selected = &item;
            break;
        }
    }
    if (selected != nullptr) {
        const farm::ItemInfo& info = farm::GetItemInfo(selected->item);
        FillRounded(NSMakeRect(72, 408, 630, 76), 6, Rgb(255, 248, 224));
        DrawText(std::string(farm::ToString(selected->item)) + "  x" + std::to_string(selected->quantity) +
                     "  " + farm::ToString(info.category),
                 NSMakeRect(94, 424, 330, 22), 14, Rgb(64, 43, 24), true);
        DrawText(info.sell_price > 0 ? "售价 " + std::to_string(info.sell_price) + " 金币" : "该物品不可出售",
                 NSMakeRect(94, 452, 260, 20), 13, Rgb(116, 86, 54), false);
        [self addButton:kWarehouseSellSelected label:"出售 1 个"
                    rect:NSMakeRect(420, 428, 100, 34)
                 enabled:info.sell_price > 0 && !selected->locked];
        [self addButton:kWarehouseToggleLock label:selected->locked ? "解除保护" : "保护物品"
                    rect:NSMakeRect(535, 428, 112, 34) enabled:true];
    }
}

- (void)drawShop {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText("商店", NSMakeRect(70, 154, 180, 32), 22, Rgb(64, 43, 24), true);
    const auto items = game_.Shop().Items(game_.Player());
    for (std::size_t i = 0; i < items.size(); ++i) {
        const farm::ShopItemView& item = items[i];
        const CGFloat x = 72 + static_cast<CGFloat>(i % 3) * 315;
        const CGFloat y = 218 + static_cast<CGFloat>(i / 3) * 112;
        [self drawImage:"shop_card" rect:NSMakeRect(x, y, 280, 82) fallback:Rgb(255, 248, 224)];
        [self drawImage:ItemIconKey(item.item) rect:NSMakeRect(x + 14, y + 18, 42, 42)
               fallback:Rgb(236, 192, 91)];
        DrawText(farm::ToString(item.item), NSMakeRect(x + 68, y + 15, 130, 24), 15,
                 Rgb(64, 43, 24), true);
        DrawText("单价 " + std::to_string(item.unit_price),
                 NSMakeRect(x + 68, y + 42, 100, 20), 13, Rgb(116, 86, 54), false);
        [self addButton:kBuyItemBase + static_cast<int>(item.item) label:item.unlocked ? "购买" : "未解锁"
                   rect:NSMakeRect(x + 190, y + 25, 72, 34) enabled:item.unlocked];
    }
}

- (void)drawUnlock {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText("解锁路线", NSMakeRect(70, 154, 180, 32), 22, Rgb(64, 43, 24), true);
    const auto& nodes = farm::UnlockGraph::Nodes();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const farm::UnlockNode& node = nodes[i];
        const CGFloat x = 72 + static_cast<CGFloat>(i % 3) * 315;
        const CGFloat y = 206 + static_cast<CGFloat>(i / 3) * 74;
        NSRect card = NSMakeRect(x, y, 280, 58);
        FillRounded(card, 6, game_.Player().IsUnlocked(node.id) ? Rgb(232, 246, 207)
                                                               : Rgb(255, 248, 224));
        StrokeRounded(card, 6, Rgb(143, 96, 48), 1.0);
        DrawText(node.name, NSMakeRect(x + 12, y + 9, 120, 20), 14, Rgb(64, 43, 24), true);
        DrawText(CategoryName(node.category) + "  Lv." + std::to_string(node.required_level) +
                     "  " + std::to_string(node.gold_cost) + "金",
                 NSMakeRect(x + 12, y + 31, 152, 18), 12, Rgb(116, 86, 54), false);
        const bool canUnlock = game_.Player().CanUnlock(node.id);
        [self addButton:kUnlockBase + static_cast<int>(node.id)
                  label:game_.Player().IsUnlocked(node.id) ? "已解锁" : "解锁"
                   rect:NSMakeRect(x + 188, y + 14, 74, 30) enabled:canUnlock];
    }
}

- (void)drawSave {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText("存档", NSMakeRect(70, 154, 180, 32), 22, Rgb(64, 43, 24), true);
    DrawText("Mac 与 Windows 共用项目目录下 saves/save01.farm。",
             NSMakeRect(72, 220, 760, 32), 15, Rgb(64, 43, 24), false);
    [self addButton:kSaveGame label:"手动保存" rect:NSMakeRect(72, 282, 160, 42) enabled:true];
    [self addButton:kLoadGame label:"读取存档" rect:NSMakeRect(252, 282, 160, 42) enabled:true];
    DrawText("当前 tick：" + std::to_string(game_.Time().CurrentTick()) +
                 "    自动保存 tick：" + std::to_string(game_.LastAutoSaveTick()),
             NSMakeRect(72, 360, 420, 24), 14, Rgb(116, 86, 54), true);
}

- (void)drawFishing {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText("钓鱼", NSMakeRect(70, 154, 180, 32), 22, Rgb(64, 43, 24), true);
    const farm::FishingSystem& fishing = game_.Fishing();
    DrawText("状态：" + FishingStateName(fishing.State()) +
                 "    鱼饵 " + std::to_string(fishing.BaitCount()) +
                 "    鱼竿 Lv." + std::to_string(fishing.RodLevel()) +
                 "    收藏价值 " + std::to_string(fishing.CollectionValue()),
             NSMakeRect(72, 204, 620, 24), 15, Rgb(64, 43, 24), true);
    const int pos = fishing.GetReelPos();
    const int greenStart = fishing.GetReelGreenStart();
    const int greenEnd = fishing.GetReelGreenEnd();
    FillRounded(NSMakeRect(72, 260, 520, 26), 4, Rgb(107, 78, 46));
    FillRounded(NSMakeRect(72 + greenStart * 520 / 100, 260, (greenEnd - greenStart) * 520 / 100, 26),
                4, Rgb(83, 145, 49));
    FillRounded(NSMakeRect(72 + pos * 520 / 100, 254, 8, 38), 3, Rgb(255, 224, 94));
    const farm::FishDef* pending = fishing.GetPendingFish();
    if (pending != nullptr) {
        DrawText(std::string("目标鱼：") + pending->name, NSMakeRect(72, 304, 260, 24), 14,
                 Rgb(64, 43, 24), true);
    }
    [self addButton:kBuyBait label:"购买鱼饵 x5" rect:NSMakeRect(72, 370, 150, 40) enabled:true];
    [self addButton:kCastLine label:"抛竿" rect:NSMakeRect(240, 370, 110, 40) enabled:fishing.CanFish()];
    [self addButton:kReelIn label:"收竿" rect:NSMakeRect(368, 370, 110, 40)
            enabled:fishing.State() == farm::FishingState::Biting];
    [self addButton:kUpgradeRod label:"升级鱼竿" rect:NSMakeRect(496, 370, 130, 40) enabled:true];
    [self addButton:kSellAllFish label:"卖出全部鱼" rect:NSMakeRect(646, 370, 150, 40) enabled:true];

    CGFloat y = 448;
    DrawText("图鉴", NSMakeRect(72, y, 100, 22), 16, Rgb(64, 43, 24), true);
    int shown = 0;
    for (const farm::FishRecord& record : fishing.Collection()) {
        const farm::FishDef* fish = fishing.GetFishDef(record.fish_id);
        if (fish == nullptr) {
            continue;
        }
        DrawText(std::string(fish->name) + " x" + std::to_string(record.count),
                 NSMakeRect(150 + shown * 130, y, 120, 22), 13, Rgb(64, 43, 24), false);
        ++shown;
        if (shown >= 6) {
            break;
        }
    }
}

- (void)drawPlaceholderPage {
    [self drawImage:"wood_card" rect:NSMakeRect(42, 132, 1010, 430) fallback:Rgb(255, 241, 205)];
    DrawText(ScreenTitle(screen_), NSMakeRect(70, 154, 280, 36), 24, Rgb(64, 43, 24), true);
    DrawText("Mac 兼容界面已接入同一套游戏核心和素材背景。\n这个页面会继续按 Windows 版布局补齐，当前可先从农田、牧场和饲料坊验证主要流程。",
             NSMakeRect(72, 220, 760, 120), 16, Rgb(64, 43, 24), false);
}

- (void)drawToast {
    FillRounded(NSMakeRect(28, 622, 680, 42), 6, Rgb(255, 246, 216));
    Fill(NSMakeRect(28, 622, 6, 42), message_is_error_ ? Rgb(180, 72, 52) : Rgb(83, 145, 49));
    StrokeRounded(NSMakeRect(28, 622, 680, 42), 6, Rgb(143, 96, 48), 1.0);
    DrawText(message_, NSMakeRect(50, 633, 630, 20), 14, Rgb(66, 46, 27), true);
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    buttons_.clear();

    if (screen_ == Screen::Menu) {
        [self drawMenu];
    } else {
        [self drawImage:BackgroundKey(screen_) rect:self.bounds fallback:Rgb(126, 172, 94)];
        [self drawStatus];
        [self drawTabs];
        if (screen_ == Screen::Farm) {
            [self drawFarm];
        } else if (screen_ == Screen::Ranch) {
            [self drawRanch];
        } else if (screen_ == Screen::Workshop) {
            [self drawWorkshop];
        } else if (screen_ == Screen::Orders) {
            [self drawOrders];
        } else if (screen_ == Screen::Warehouse) {
            [self drawWarehouse];
        } else if (screen_ == Screen::Shop) {
            [self drawShop];
        } else if (screen_ == Screen::Fishing) {
            [self drawFishing];
        } else if (screen_ == Screen::Unlock) {
            [self drawUnlock];
        } else if (screen_ == Screen::Save) {
            [self drawSave];
        } else {
            [self drawPlaceholderPage];
        }
        [self drawToast];
    }

    for (const MacButton& button : buttons_) {
        if (IsHitOnlyButton(button.id)) {
            continue;
        }
        [self drawButton:button];
    }
}

- (void)setMessage:(const std::string&)message isError:(bool)isError {
    message_ = message;
    message_is_error_ = isError;
}

- (void)handleGameButton:(int)buttonId {
    const int tick = game_.Time().CurrentTick();
    const farm::Season season = game_.Season().Current();
    farm::ErrorCode code = farm::ErrorCode::Ok;

    if (IsWarehouseItemButton(buttonId)) {
        const int slot = buttonId - kWarehouseItemBase;
        const int index = warehouse_page_ * kMacWarehouseItemsPerPage + slot;
        const auto inventory = game_.Player().InventoryView();
        if (index >= 0 && index < static_cast<int>(inventory.size())) {
            selected_warehouse_item_ = inventory[static_cast<std::size_t>(index)].item;
        }
        return;
    }
    if (IsRanchFacilityButton(buttonId)) {
        const int slot = buttonId - kSelectRanchFacilityBase;
        const int index = ranch_facility_page_ * kMacRanchFacilitiesPerPage + slot;
        const auto facilities = game_.Ranch().FacilityViews();
        if (index >= 0 && index < static_cast<int>(facilities.size())) {
            selected_ranch_facility_id_ = facilities[static_cast<std::size_t>(index)].id;
        }
        return;
    }
    if (buttonId >= kOrderCompleteBase && buttonId < kOrderCompleteBase + 100) {
        const int slot = buttonId - kOrderCompleteBase;
        code = game_.Orders().CompleteOrder(game_.Player(), slot, tick).code;
        [self setMessage:ResultMessage("交付订单", code) isError:code != farm::ErrorCode::Ok];
        return;
    }
    if (buttonId >= kOrderAbandonBase && buttonId < kOrderAbandonBase + 100) {
        const int slot = buttonId - kOrderAbandonBase;
        code = game_.Orders().AbandonOrder(slot, tick).code;
        [self setMessage:ResultMessage("放弃订单", code) isError:code != farm::ErrorCode::Ok];
        return;
    }
    if (buttonId >= kSellItemBase && buttonId < kSellItemBase + 256) {
        const farm::ItemId item = static_cast<farm::ItemId>(buttonId - kSellItemBase);
        code = game_.Player().TrySellItem(item, 1).code;
        [self setMessage:ResultMessage("出售物品", code) isError:code != farm::ErrorCode::Ok];
        return;
    }
    if (buttonId >= kBuyItemBase && buttonId < kBuyItemBase + 256) {
        const farm::ItemId item = static_cast<farm::ItemId>(buttonId - kBuyItemBase);
        code = game_.Shop().BuyItem(game_.Player(), item, 1).code;
        [self setMessage:ResultMessage("购买物品", code) isError:code != farm::ErrorCode::Ok];
        return;
    }
    if (buttonId >= kUnlockBase && buttonId < kUnlockBase + 256) {
        const farm::UnlockId id = static_cast<farm::UnlockId>(buttonId - kUnlockBase);
        code = game_.Player().UnlockContent(id).code;
        [self setMessage:ResultMessage("解锁内容", code) isError:code != farm::ErrorCode::Ok];
        return;
    }

    auto selectedFacility = [&](farm::RanchFacilityKind kind) {
        int first = -1;
        for (const farm::RanchFacilityView& facility : game_.Ranch().FacilityViews()) {
            if (facility.kind != kind) {
                continue;
            }
            if (first < 0) {
                first = facility.id;
            }
            if (facility.id == selected_ranch_facility_id_) {
                return facility.id;
            }
        }
        return first;
    };

    switch (buttonId) {
        case kTick:
            game_.AdvanceTicks(1);
            [self setMessage:"时间推进 2 分钟。" isError:false];
            return;
        case kPause:
            if (game_.Time().IsPaused()) game_.Time().Resume();
            else game_.Time().Pause();
            [self setMessage:"速度状态已切换。" isError:false];
            return;
        case kSpeed1:
            game_.Time().SetSpeed(farm::GameSpeed::Normal);
            [self setMessage:"速度 1x。" isError:false];
            return;
        case kSpeed2:
            game_.Time().SetSpeed(farm::GameSpeed::Fast);
            [self setMessage:"速度 2x。" isError:false];
            return;
        case kSpeed4:
            game_.Time().SetSpeed(farm::GameSpeed::VeryFast);
            [self setMessage:"速度 4x。" isError:false];
            return;
        case kFarmModeField:
            farm_show_greenhouse_ = false;
            [self setMessage:"已切换到农田。" isError:false];
            return;
        case kFarmModeGreenhouse:
            farm_show_greenhouse_ = true;
            [self setMessage:"已切换到温室。" isError:false];
            return;
        case kPlantWheat:
            code = farm_show_greenhouse_
                       ? game_.Planting().TryPlantGreenhouseAt(game_.Player(), selected_greenhouse_plot_,
                                                               farm::ItemId::WheatSeed, tick).code
                       : game_.Planting().TryPlantAt(game_.Player(), selected_plot_,
                                                     farm::ItemId::WheatSeed, tick, season).code;
            [self setMessage:ResultMessage("播种小麦", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kPlantCorn:
            code = farm_show_greenhouse_
                       ? game_.Planting().TryPlantGreenhouseAt(game_.Player(), selected_greenhouse_plot_,
                                                               farm::ItemId::CornSeed, tick).code
                       : game_.Planting().TryPlantAt(game_.Player(), selected_plot_,
                                                     farm::ItemId::CornSeed, tick, season).code;
            [self setMessage:ResultMessage("播种玉米", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kPlantCarrot:
            code = farm_show_greenhouse_
                       ? game_.Planting().TryPlantGreenhouseAt(game_.Player(), selected_greenhouse_plot_,
                                                               farm::ItemId::CarrotSeed, tick).code
                       : game_.Planting().TryPlantAt(game_.Player(), selected_plot_,
                                                     farm::ItemId::CarrotSeed, tick, season).code;
            [self setMessage:ResultMessage("播种胡萝卜", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kPlantTomato:
            code = farm_show_greenhouse_
                       ? game_.Planting().TryPlantGreenhouseAt(game_.Player(), selected_greenhouse_plot_,
                                                               farm::ItemId::TomatoSeed, tick).code
                       : game_.Planting().TryPlantAt(game_.Player(), selected_plot_,
                                                     farm::ItemId::TomatoSeed, tick, season).code;
            [self setMessage:ResultMessage("播种番茄", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kWater:
            code = farm_show_greenhouse_
                       ? game_.Planting().WaterGreenhousePlot(selected_greenhouse_plot_, tick).code
                       : game_.Planting().WaterPlot(selected_plot_, tick).code;
            [self setMessage:ResultMessage("浇水", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kFertilize:
            code = game_.Planting().ApplyFertilizer(game_.Player(), selected_plot_, tick).code;
            [self setMessage:ResultMessage("施肥", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kHarvest:
            code = farm_show_greenhouse_
                       ? game_.Planting().HarvestGreenhouse(game_.Player(), selected_greenhouse_plot_).code
                       : game_.Planting().Harvest(game_.Player(), selected_plot_).code;
            [self setMessage:ResultMessage("收割", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kExpand: {
            auto expanded = game_.Planting().Expand(game_.Player());
            [self setMessage:expanded.ok() ? "土地已扩建。" : ResultMessage("扩建土地", expanded.code)
                    isError:!expanded.ok()];
            return;
        }
        case kBuildGreenhouse: {
            auto built = game_.Planting().BuildGreenhouse(game_.Player());
            if (built.ok()) {
                selected_greenhouse_plot_ = built.value;
            }
            [self setMessage:built.ok() ? "温室地块已建造。" : ResultMessage("建造温室", built.code)
                    isError:!built.ok()];
            return;
        }
        case kMakeChickenFeed:
            code = game_.Workshop()
                       .StartProduction(game_.Player(), farm::RecipeId::ChickenFeed, 1,
                                        game_.Player().Level())
                       .code;
            [self setMessage:ResultMessage("制作鸡饲料", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kMakeChickenFeed3:
            code = game_.Workshop()
                       .StartProduction(game_.Player(), farm::RecipeId::ChickenFeed, 3,
                                        game_.Player().Level())
                       .code;
            [self setMessage:ResultMessage("制作鸡饲料 x3", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kMakeCowFeed:
            code = game_.Workshop()
                       .StartProduction(game_.Player(), farm::RecipeId::CowFeed, 1,
                                        game_.Player().Level())
                       .code;
            [self setMessage:ResultMessage("制作牛饲料", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kMakeBread:
            code = game_.Workshop()
                       .StartProduction(game_.Player(), farm::RecipeId::Bread, 1,
                                        game_.Player().Level())
                       .code;
            [self setMessage:ResultMessage("制作面包", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kMakeCheese:
            code = game_.Workshop()
                       .StartProduction(game_.Player(), farm::RecipeId::Cheese, 1,
                                        game_.Player().Level())
                       .code;
            [self setMessage:ResultMessage("制作奶酪", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kMakeJam:
            code = game_.Workshop()
                       .StartProduction(game_.Player(), farm::RecipeId::Jam, 1,
                                        game_.Player().Level())
                       .code;
            [self setMessage:ResultMessage("制作果酱", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kClaimProduct:
            code = game_.Workshop().ClaimProduct(game_.Player()).code;
            [self setMessage:ResultMessage("领取成品", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kBuildCowBarn: {
            auto built = game_.Ranch().BuildFacility(game_.Player(), farm::RanchFacilityKind::CowBarn);
            if (built.ok()) {
                selected_ranch_facility_id_ = built.value;
            }
            [self setMessage:built.ok() ? "牛棚已建造。" : ResultMessage("建造牛棚", built.code)
                    isError:!built.ok()];
            return;
        }
        case kBuildSheepPen: {
            auto built = game_.Ranch().BuildFacility(game_.Player(), farm::RanchFacilityKind::SheepPen);
            if (built.ok()) {
                selected_ranch_facility_id_ = built.value;
            }
            [self setMessage:built.ok() ? "羊圈已建造。" : ResultMessage("建造羊圈", built.code)
                    isError:!built.ok()];
            return;
        }
        case kRanchPagePrevious:
            ranch_facility_page_ = std::max(0, ranch_facility_page_ - 1);
            return;
        case kRanchPageNext: {
            const int facilityCount = static_cast<int>(game_.Ranch().FacilityViews().size());
            const int pageCount = std::max(1, (facilityCount + kMacRanchFacilitiesPerPage - 1) /
                                              kMacRanchFacilitiesPerPage);
            ranch_facility_page_ = std::min(pageCount - 1, ranch_facility_page_ + 1);
            return;
        }
        case kBuyChicken: {
            auto bought = game_.Ranch().BuyAnimal(game_.Player(),
                                                  selectedFacility(farm::RanchFacilityKind::ChickenCoop),
                                                  farm::AnimalKind::Chicken);
            [self setMessage:bought.ok() ? "购买鸡：Ok" : ResultMessage("购买鸡", bought.code)
                    isError:!bought.ok()];
            return;
        }
        case kBuyCow: {
            auto bought = game_.Ranch().BuyAnimal(game_.Player(),
                                                  selectedFacility(farm::RanchFacilityKind::CowBarn),
                                                  farm::AnimalKind::Cow);
            [self setMessage:bought.ok() ? "购买牛：Ok" : ResultMessage("购买牛", bought.code)
                    isError:!bought.ok()];
            return;
        }
        case kBuySheep: {
            auto bought = game_.Ranch().BuyAnimal(game_.Player(),
                                                  selectedFacility(farm::RanchFacilityKind::SheepPen),
                                                  farm::AnimalKind::Sheep);
            [self setMessage:bought.ok() ? "购买羊：Ok" : ResultMessage("购买羊", bought.code)
                    isError:!bought.ok()];
            return;
        }
        case kFeedSelected: {
            auto fed = game_.Ranch().BatchFeed(game_.Player(), selected_ranch_facility_id_, tick);
            [self setMessage:fed.ok() ? "当前设施已喂食 " + std::to_string(fed.value) + " 只动物。"
                                      : ResultMessage("喂当前设施", fed.code)
                    isError:!fed.ok()];
            return;
        }
        case kHarvestSelected: {
            auto harvested = game_.Ranch().BatchHarvest(game_.Player(), selected_ranch_facility_id_);
            [self setMessage:harvested.ok() ? "当前设施已收获 " + std::to_string(harvested.value) + " 个产品。"
                                            : ResultMessage("收当前设施", harvested.code)
                    isError:!harvested.ok()];
            return;
        }
        case kFeedAll: {
            int total = 0;
            for (const farm::RanchFacilityView& facility : game_.Ranch().FacilityViews()) {
                auto fed = game_.Ranch().BatchFeed(game_.Player(), facility.id, tick);
                if (fed.ok()) {
                    total += fed.value;
                }
            }
            [self setMessage:"已喂食 " + std::to_string(total) + " 只动物。" isError:false];
            return;
        }
        case kHarvestAll: {
            int total = 0;
            for (const farm::RanchFacilityView& facility : game_.Ranch().FacilityViews()) {
                auto harvested = game_.Ranch().BatchHarvest(game_.Player(), facility.id);
                if (harvested.ok()) {
                    total += harvested.value;
                }
            }
            [self setMessage:"已收获 " + std::to_string(total) + " 个产品。" isError:false];
            return;
        }
        case kWarehouseSellSelected:
            code = game_.Player().TrySellItem(selected_warehouse_item_, 1).code;
            [self setMessage:ResultMessage("出售物品", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kWarehouseToggleLock: {
            const bool locked = game_.Player().IsItemLocked(selected_warehouse_item_);
            game_.Player().SetItemLocked(selected_warehouse_item_, !locked);
            [self setMessage:locked ? "已解除物品保护。" : "已保护物品。" isError:false];
            return;
        }
        case kWarehouseUpgrade:
            code = game_.Player().UpgradeWarehouse().code;
            [self setMessage:ResultMessage("仓库扩容", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kWarehousePagePrevious:
            warehouse_page_ = std::max(0, warehouse_page_ - 1);
            return;
        case kWarehousePageNext: {
            const int itemCount = static_cast<int>(game_.Player().InventoryView().size());
            const int pageCount = std::max(1, (itemCount + kMacWarehouseItemsPerPage - 1) /
                                              kMacWarehouseItemsPerPage);
            warehouse_page_ = std::min(pageCount - 1, warehouse_page_ + 1);
            return;
        }
        case kSaveGame:
            std::filesystem::create_directories("saves");
            code = game_.ManualSave(SavePath()).code;
            [self setMessage:ResultMessage("保存", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kLoadGame:
            code = game_.Load(SavePath()).code;
            [self setMessage:ResultMessage("读取", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kBuyBait:
            code = game_.Fishing().BuyBait(game_.Player(), 5).code;
            [self setMessage:ResultMessage("购买鱼饵", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kCastLine:
            if (game_.Fishing().CanFish()) {
                game_.Fishing().CastLine(tick);
                [self setMessage:"已抛竿，推进时间等待咬钩。" isError:false];
            } else {
                [self setMessage:"鱼饵不足或当前不能抛竿。" isError:true];
            }
            return;
        case kReelIn:
            code = game_.Fishing().TryReelIn(tick).code;
            [self setMessage:ResultMessage("收竿", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kUpgradeRod:
            code = game_.Fishing().UpgradeRod(game_.Player()).code;
            [self setMessage:ResultMessage("升级鱼竿", code) isError:code != farm::ErrorCode::Ok];
            return;
        case kSellAllFish: {
            auto sold = game_.Fishing().SellAllFish(game_.Player());
            [self setMessage:sold.ok() ? "卖鱼获得 " + std::to_string(sold.value) + " 金币。"
                                       : ResultMessage("卖鱼", sold.code)
                    isError:!sold.ok()];
            return;
        }
        default:
            return;
    }
}

- (void)mouseDown:(NSEvent*)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    for (const MacButton& button : buttons_) {
        if (!button.enabled || !NSPointInRect(point, button.rect)) {
            continue;
        }
        if (IsPlotButton(button.id)) {
            if (farm_show_greenhouse_) {
                selected_greenhouse_plot_ = button.id - kPlotBase;
            } else {
                selected_plot_ = button.id - kPlotBase;
            }
            const int selectedIndex = farm_show_greenhouse_ ? selected_greenhouse_plot_ : selected_plot_;
            [self setMessage:"已选择第 " + std::to_string(selectedIndex + 1) + " 块土地。"
                    isError:false];
            [self setNeedsDisplay:YES];
            return;
        }
        if (button.id == kNewGame) {
            game_ = farm::Game::NewGame();
            screen_ = Screen::Farm;
            selected_plot_ = 0;
            selected_greenhouse_plot_ = 0;
            farm_show_greenhouse_ = false;
            selected_ranch_facility_id_ = 1;
            ranch_facility_page_ = 0;
            selected_warehouse_item_ = farm::ItemId::WheatSeed;
            warehouse_page_ = 0;
            [self setMessage:"新游戏开始。" isError:false];
            [self setNeedsDisplay:YES];
            return;
        }
        if (button.id == kContinueGame) {
            if (farm::SaveManager::Exists(SavePath())) {
                const farm::ErrorCode loaded = game_.Load(SavePath()).code;
                if (loaded == farm::ErrorCode::Ok) {
                    screen_ = Screen::Farm;
                    selected_plot_ = 0;
                    selected_greenhouse_plot_ = 0;
                    farm_show_greenhouse_ = false;
                    selected_ranch_facility_id_ = 1;
                    ranch_facility_page_ = 0;
                    selected_warehouse_item_ = farm::ItemId::WheatSeed;
                    warehouse_page_ = 0;
                }
                [self setMessage:loaded == farm::ErrorCode::Ok ? "继续游戏。" : ResultMessage("读取", loaded)
                        isError:loaded != farm::ErrorCode::Ok];
            } else {
                game_ = farm::Game::NewGame();
                screen_ = Screen::Farm;
                selected_plot_ = 0;
                selected_greenhouse_plot_ = 0;
                farm_show_greenhouse_ = false;
                selected_ranch_facility_id_ = 1;
                ranch_facility_page_ = 0;
                selected_warehouse_item_ = farm::ItemId::WheatSeed;
                warehouse_page_ = 0;
                [self setMessage:"没有找到存档，已开始新游戏。" isError:false];
            }
            [self setNeedsDisplay:YES];
            return;
        }
        if (button.id >= kTabFarm && button.id <= kTabSave) {
            screen_ = static_cast<Screen>(static_cast<int>(Screen::Farm) + (button.id - kTabFarm));
            [self setMessage:"已切换到" + ScreenTitle(screen_) + "。" isError:false];
            [self setNeedsDisplay:YES];
            return;
        }
        [self handleGameButton:button.id];
        [self setNeedsDisplay:YES];
        return;
    }
}

@end

@interface FarmMacAppDelegate : NSObject <NSApplicationDelegate> {
@private
    NSWindow* window_;
}
@end

@implementation FarmMacAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    NSRect frame = NSMakeRect(0, 0, 1120, 700);
    window_ = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:(NSWindowStyleMaskTitled |
                                                     NSWindowStyleMaskClosable |
                                                     NSWindowStyleMaskMiniaturizable |
                                                     NSWindowStyleMaskResizable)
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [window_ setTitle:@"田园时光"];
    [window_ center];
    [window_ setMinSize:NSMakeSize(1040, 680)];
    FarmMacView* view = [[FarmMacView alloc] initWithFrame:frame];
    [window_ setContentView:view];
    [view release];
    [window_ makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    (void)sender;
    return YES;
}

- (void)dealloc {
    [window_ release];
    [super dealloc];
}

@end

namespace farm::ui {

int RunMacFarmApp() {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        FarmMacAppDelegate* delegate = [[FarmMacAppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
        [delegate release];
    }
    return 0;
}

}  // namespace farm::ui

#endif
