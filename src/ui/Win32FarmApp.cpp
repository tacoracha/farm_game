#ifdef _WIN32

#include "farm/ui/Win32FarmApp.h"

#include "farm/achievement/AchievementSystem.h"
#include "farm/dailytask/DailyTaskSystem.h"
#include "farm/fishing/FishingSystem.h"
#include "farm/merchant/TravelingMerchantSystem.h"
#include "farm/core/Game.h"
#include "farm/core/UnlockGraph.h"
#include "farm/persistence/SaveManager.h"
#include "farm/ui/TextureManager.h"

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace farm::ui {
namespace {

enum class Screen {
    Menu,
    Farm,
    Ranch,
    Workshop,
    Orders,
    Warehouse,
    Shop,
    Unlock,
    Achieve,
    DailyTask,
    Merchant,
    Fishing,
    Save,
};

struct UiButton {
    RECT rect{};
    int id = 0;
};

constexpr int kBtnNew = 1;
constexpr int kBtnContinue = 2;
constexpr int kTabFarm = 10;
constexpr int kTabRanch = 11;
constexpr int kTabWorkshop = 12;
constexpr int kTabOrders = 13;
constexpr int kTabWarehouse = 14;
constexpr int kTabShop = 15;
constexpr int kTabUnlock = 16;
constexpr int kTabSave = 17;
constexpr int kTabMerchant = 96;
constexpr int kTabFishing = 97;
constexpr int kCastLine = 970;
constexpr int kReelIn = 971;
constexpr int kFishUpgradeRod = 972;
constexpr int kFishCollection = 973;
constexpr int kMerchantBuyBase = 950;
constexpr int kMerchantSellBase = 960;
constexpr int kTabDailyTask = 19;
constexpr int kTabAchieve = 18;
constexpr int kTick = 20;
constexpr int kPause = 21;
constexpr int kSpeed1 = 22;
constexpr int kSpeed2 = 23;
constexpr int kSpeed4 = 24;

constexpr int kPlantWheat = 100;
constexpr int kPlantCorn = 101;
constexpr int kPlantCarrot = 102;
constexpr int kPlantTomato = 103;
constexpr int kWater = 104;
constexpr int kFertilize = 105;
constexpr int kHarvest = 106;
constexpr int kExpand = 107;

constexpr int kBuildCowBarn = 200;
constexpr int kBuildSheepPen = 201;
constexpr int kBuyChicken = 202;
constexpr int kBuyCow = 203;
constexpr int kBuySheep = 204;
constexpr int kFeedAll = 205;
constexpr int kHarvestAll = 206;
constexpr int kBatchPlant = 107;
constexpr int kBatchWater = 108;
constexpr int kBatchFertilize = 109;
constexpr int kBatchHarvest = 110;
constexpr int kTabGreenhouse = 111;
constexpr int kBuildGreenhouse = 112;
constexpr int kPopupCancel = 998;
constexpr int kPopupGo = 999;

constexpr int kMakeChickenFeed = 300;
constexpr int kMakeChickenFeed3 = 301;
constexpr int kMakeCowFeed = 302;
constexpr int kClaimFeed = 303;
constexpr int kMakeBread = 304;
constexpr int kMakeCheese = 305;
constexpr int kMakeJam = 306;

constexpr int kCompleteOrder = 400;
constexpr int kAbandonOrder = 401;
constexpr int kBatchCompleteOrder = 402;
constexpr int kOrderBase = 420;

constexpr int kBuyWheatSeed = 500;
constexpr int kBuyCornSeed = 501;
constexpr int kBuyCarrotSeed = 502;
constexpr int kBuyTomatoSeed = 503;
constexpr int kBuyFertilizer = 504;
constexpr int kBuyBait = 505;
constexpr int kBaitQtyBase = 506;
constexpr int kShopQtyBase = 530;  // moved up to avoid overlap with bait (506-508)

constexpr int kSellWheat = 600;
constexpr int kSellCorn = 601;
constexpr int kSellCarrot = 602;
constexpr int kSellTomato = 603;
constexpr int kSellEgg = 604;
constexpr int kSellMilk = 605;
constexpr int kSellWool = 606;
constexpr int kBatchSellAll = 607;
constexpr int kUpgradeWarehouse = 608;
constexpr int kLockItemBase = 700;

constexpr int kSave = 700;
constexpr int kLoad = 701;
constexpr int kNewGame = 702;
constexpr int kUnlockBase = 900;

std::string Narrow(const std::wstring& text) {
    return std::string(text.begin(), text.end());
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return std::wstring(text.begin(), text.end());
    }
    std::wstring wide(static_cast<std::size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), size);
    return wide;
}

const wchar_t* WeatherName(WeatherType weather) {
    switch (weather) {
        case WeatherType::Sunny:
            return L"晴天";
        case WeatherType::Rainy:
            return L"雨天";
        case WeatherType::Cloudy:
            return L"多云";
        case WeatherType::Drought:
            return L"干旱";
    }
    return L"未知";
}

const wchar_t* SpeedName(GameSpeed speed) {
    switch (speed) {
        case GameSpeed::Paused:
            return L"暂停";
        case GameSpeed::Normal:
            return L"1x";
        case GameSpeed::Fast:
            return L"2x";
        case GameSpeed::VeryFast:
            return L"4x";
    }
    return L"1x";
}

const wchar_t* ItemName(ItemId item) {
    switch (item) {
        case ItemId::WheatSeed:
            return L"小麦种子";
        case ItemId::CornSeed:
            return L"玉米种子";
        case ItemId::CarrotSeed:
            return L"胡萝卜种子";
        case ItemId::TomatoSeed:
            return L"番茄种子";
        case ItemId::Wheat:
            return L"小麦";
        case ItemId::Corn:
            return L"玉米";
        case ItemId::Carrot:
            return L"胡萝卜";
        case ItemId::Tomato:
            return L"番茄";
        case ItemId::ChickenFeed:
            return L"鸡饲料";
        case ItemId::CowFeed:
            return L"牛饲料";
        case ItemId::Egg:
            return L"鸡蛋";
        case ItemId::Milk:
            return L"牛奶";
        case ItemId::Wool:
            return L"羊毛";
        case ItemId::Fertilizer:
            return L"肥料";
    }
    return L"未知物品";
}

const wchar_t* FacilityName(RanchFacilityKind kind) {
    switch (kind) {
        case RanchFacilityKind::ChickenCoop:
            return L"鸡圈";
        case RanchFacilityKind::CowBarn:
            return L"牛棚";
        case RanchFacilityKind::SheepPen:
            return L"羊圈";
        case RanchFacilityKind::PigPen:
            return L"猪圈";
    }
    return L"设施";
}

std::wstring CropTextureKey(ItemId crop, int stage) {
    if (stage < 0) {
        stage = 0;
    }
    if (stage > 4) {
        stage = 4;
    }
    const wchar_t* crop_name = L"wheat";
    switch (crop) {
        case ItemId::Wheat:
            crop_name = L"wheat";
            break;
        case ItemId::Corn:
            crop_name = L"corn";
            break;
        case ItemId::Carrot:
            crop_name = L"carrot";
            break;
        case ItemId::Tomato:
            crop_name = L"tomato";
            break;
        default:
            crop_name = L"wheat";
            break;
    }
    return L"crop_" + std::wstring(crop_name) + L"_" + std::to_wstring(stage);
}

int CropStage(const PlotView& plot) {
    if (plot.state == PlotState::Idle) {
        return 0;
    }
    if (plot.state == PlotState::Mature) {
        return 4;
    }
    const int total = GetItemInfo(plot.crop).grow_ticks;
    if (total <= 0) {
        return 1;
    }
    const int done = total - plot.remaining_ticks;
    if (done <= 0) {
        return 0;
    }
    if (done * 4 >= total * 3) {
        return 3;
    }
    if (done * 4 >= total * 2) {
        return 2;
    }
    return 1;
}

std::wstring ItemTextureKey(ItemId item) {
    switch (item) {
        case ItemId::WheatSeed:
            return L"seed_wheat";
        case ItemId::CornSeed:
            return L"seed_corn";
        case ItemId::CarrotSeed:
            return L"seed_carrot";
        case ItemId::TomatoSeed:
            return L"seed_tomato";
        case ItemId::Wheat:
            return L"item_wheat";
        case ItemId::Corn:
            return L"item_corn";
        case ItemId::Carrot:
            return L"item_carrot";
        case ItemId::Tomato:
            return L"item_tomato";
        case ItemId::ChickenFeed:
        case ItemId::CowFeed:
            return L"item_feed";
        case ItemId::Egg:
            return L"item_egg";
        case ItemId::Milk:
            return L"item_milk";
        case ItemId::Wool:
            return L"item_wool";
        case ItemId::Fertilizer:
            return L"item_fertilizer";
    }
    return L"item_unknown";
}

std::wstring AnimalTextureKey(AnimalKind kind, AnimalState state) {
    switch (kind) {
        case AnimalKind::Chicken:
            if (state == AnimalState::Ready) {
                return L"animal_chicken_ready";
            }
            if (state == AnimalState::Producing) {
                return L"animal_chicken_fed";
            }
            return L"animal_chicken_idle";
        case AnimalKind::Cow:
            return L"animal_cow_idle";
        case AnimalKind::Sheep:
            return L"animal_sheep_idle";
        case AnimalKind::Pig:
            return L"animal_pig_idle";
    }
    return L"animal_unknown";
}

std::wstring FacilityTextureKey(RanchFacilityKind kind) {
    switch (kind) {
        case RanchFacilityKind::ChickenCoop:
            return L"building_chicken_coop";
        case RanchFacilityKind::CowBarn:
            return L"building_cow_barn";
        case RanchFacilityKind::SheepPen:
            return L"building_sheep_pen";
        case RanchFacilityKind::PigPen:
            return L"building_pig_pen";
    }
    return L"building_unknown";
}

std::wstring WeatherTextureKey(WeatherType weather) {
    switch (weather) {
        case WeatherType::Sunny:
            return L"weather_sunny";
        case WeatherType::Rainy:
            return L"weather_rainy";
        case WeatherType::Cloudy:
            return L"weather_cloudy";
        case WeatherType::Drought:
            return L"weather_sunny";
    }
    return L"weather_sunny";
}

const wchar_t* ErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::Ok:
            return L"操作成功。";
        case ErrorCode::InvalidItem:
            return L"物品无效。";
        case ErrorCode::InvalidQuantity:
            return L"数量无效。";
        case ErrorCode::InsufficientGold:
            return L"金币不足。";
        case ErrorCode::InsufficientItem:
            return L"物品数量不足。";
        case ErrorCode::WarehouseFull:
            return L"仓库已满。";
        case ErrorCode::ProtectedItem:
            return L"该物品已锁定。";
        case ErrorCode::CannotSell:
            return L"该物品不能出售。";
        case ErrorCode::NotASeed:
            return L"这不是种子。";
        case ErrorCode::NotFertilizer:
            return L"这不是肥料。";
        case ErrorCode::SeedNotUnlocked:
            return L"种子尚未解锁。";
        case ErrorCode::ContentLocked:
            return L"内容尚未解锁，请先到解锁页完成前置节点。";
        case ErrorCode::PlotOutOfRange:
            return L"地块不存在。";
        case ErrorCode::PlotNotIdle:
            return L"地块已经被占用。";
        case ErrorCode::PlotNotGrowing:
            return L"地块上没有正在生长的作物。";
        case ErrorCode::PlotAlreadyWatered:
            return L"这块地已经浇过水。";
        case ErrorCode::PlotAlreadyFertilized:
            return L"这块地已经施过肥。";
        case ErrorCode::PlotNotMature:
            return L"作物还没有成熟。";
        case ErrorCode::NoIdlePlot:
            return L"没有空闲地块。";
        case ErrorCode::FacilityOutOfRange:
            return L"设施不存在或动物类型不匹配。";
        case ErrorCode::FacilityFull:
            return L"设施容量已满。";
        case ErrorCode::AnimalOutOfRange:
            return L"动物不存在。";
        case ErrorCode::AnimalNotIdle:
            return L"动物正在忙碌。";
        case ErrorCode::AnimalNotReady:
            return L"动物产品还没有准备好。";
        case ErrorCode::RecipeUnavailable:
            return L"配方不可用。";
        case ErrorCode::ProductionQueueFull:
            return L"生产队列已满。";
        case ErrorCode::ProductNotReady:
            return L"没有可领取的产品。";
        case ErrorCode::ShelfFull:
            return L"货架已满。";
        case ErrorCode::OrderSlotOutOfRange:
            return L"订单槽不存在。";
        case ErrorCode::OrderCoolingDown:
            return L"订单正在冷却。";
        case ErrorCode::OrderLocked:
            return L"订单已锁定。";
        case ErrorCode::SaveOpenFailed:
            return L"无法打开存档文件。";
        case ErrorCode::SaveWriteFailed:
            return L"写入存档失败。";
        case ErrorCode::SaveReadFailed:
            return L"读取存档失败。";
        case ErrorCode::SaveVersionMismatch:
            return L"存档版本不兼容。";
        case ErrorCode::SaveCorrupted:
            return L"存档已损坏。";
        case ErrorCode::GamePaused:
            return L"游戏已暂停。";
    }
    return L"未知错误。";
}

class FarmWindow {
public:
    int Run();
    ~FarmWindow();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    void PaintBuffered(HDC hdc);
    void Paint(HDC hdc);
    void DrawMenu(HDC hdc);
    void DrawGame(HDC hdc);
    void DrawStatus(HDC hdc);
    void DrawTabs(HDC hdc);
    void DrawFarm(HDC hdc);
    void DrawRanch(HDC hdc);
    void DrawWorkshop(HDC hdc);
    void DrawOrders(HDC hdc);
    void DrawWarehouse(HDC hdc);
    void DrawShop(HDC hdc);
    void DrawUnlock(HDC hdc);
    void DrawSave(HDC hdc);
    void DrawAchievements(HDC hdc);
    void DrawDailyTasks(HDC hdc);
    void DrawMerchant(HDC hdc);
    void DrawFishing(HDC hdc);
    void DrawToasts(HDC hdc);
    void DrawPanelFrame(HDC hdc, const wchar_t* title, int x, int y, int w, int h);
    void DrawMaterialPopup(HDC hdc);
    void ShowMaterialPopup(ItemId missing_item);
    Screen BestScreenForItem(ItemId item) const;
    void DrawTextureOrFill(HDC hdc, const std::wstring& key, RECT rect, COLORREF fallback);
    void Fill(HDC hdc, RECT rect, COLORREF color);
    void Text(HDC hdc, int x, int y, const std::wstring& text);
    void Button(HDC hdc, int id, RECT rect, const std::wstring& text);
    void OnButton(int id);
    void SetMessage(ErrorCode code);
    void SetMessage(const std::wstring& text);
    std::wstring ItemLine(ItemId item, int quantity) const;
    std::wstring SavePath() const;
    bool TryContinue();
    int FirstFacility(RanchFacilityKind kind) const;

    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr;
    TextureManager textures_;
    Game game_ = Game::NewGame();
    Screen screen_ = Screen::Menu;
    std::wstring message_ = L"准备就绪。";
    std::vector<UiButton> buttons_;
    int selected_plot_ = 0;
    ItemId selected_seed_ = ItemId::WheatSeed;
    bool show_greenhouse_ = false;
    bool show_fishpedia_ = false;
    int selected_shop_qty_ = 1;
    int shop_qtys_[8] = {1,1,1,1,1,1,1,1};  // per-item qty tracking
    bool popup_visible_ = false;
    ItemId popup_item_ = ItemId::Wheat;
    ItemId error_item_ = ItemId::Wheat;
    std::wstring popup_title_ = L"材料不足";

    // Item tooltip hover
    struct HoverEntry { ItemId item; RECT rect; };
    std::vector<HoverEntry> hover_rects_;
    ItemId hovered_item_ = ItemId::Wheat;
    bool hover_active_ = false;
    int hover_mx_ = 0, hover_my_ = 0;
    void RegisterItemRect(ItemId item, RECT rect);
    void DrawItemTooltip(HDC hdc);
    std::set<int> selected_orders_;
};

namespace { constexpr int kDesignW = 1120, kDesignH = 720, kPanelX = 28, kPanelY = 130, kPanelW = 820, kPanelH = 490, kB = 30, kBtnH = 36; }

int FarmWindow::Run() {
    const wchar_t kClassName[] = L"FarmGameWindow";
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = FarmWindow::WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (RegisterClassExW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"农场游戏无法注册窗口类。", L"农场游戏", MB_ICONERROR | MB_OK);
        return 1;
    }

    hwnd_ = CreateWindowExW(0, kClassName, L"农场游戏", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                            CW_USEDEFAULT, 1120, 720, nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr) {
        std::ofstream log("farm_game_startup.log", std::ios::trunc);
        log << "CreateWindowExW failed: " << GetLastError() << "\n";
        MessageBoxW(nullptr, L"农场游戏无法创建主窗口。", L"农场游戏", MB_ICONERROR | MB_OK);
        return 1;
    }

    textures_.Load(L"grass", L"assets/textures/terrain/grass_tile.png", L"assets/textures/grass.ppm");
    textures_.Load(L"soil", L"assets/textures/terrain/soil_dry_tile.png", L"assets/textures/soil.ppm");
    textures_.Load(L"soil_wet", L"assets/textures/terrain/soil_wet_tile.png", L"assets/textures/soil.ppm");
    textures_.Load(L"wood", L"assets/textures/ui/button_normal.png", L"assets/textures/wood.ppm");
    textures_.Load(L"top_bar", L"assets/textures/ui/top_bar.png", L"");
    textures_.Load(L"wood_card", L"assets/textures/ui/wood_card.png", L"assets/textures/wood.ppm");
    textures_.Load(L"order_card", L"assets/textures/ui/order_card.png", L"assets/textures/wood.ppm");
    textures_.Load(L"shop_card", L"assets/textures/ui/shop_card.png", L"assets/textures/wood.ppm");
    textures_.Load(L"inventory_slot", L"assets/textures/ui/inventory_slot.png", L"assets/textures/wood.ppm");
    textures_.Load(L"crop_wheat_0", L"assets/textures/crops/wheat/0_seed.png", L"");
    textures_.Load(L"crop_wheat_1", L"assets/textures/crops/wheat/1_sprout.png", L"");
    textures_.Load(L"crop_wheat_2", L"assets/textures/crops/wheat/2_young.png", L"");
    textures_.Load(L"crop_wheat_3", L"assets/textures/crops/wheat/3_mid.png", L"");
    textures_.Load(L"crop_wheat_4", L"assets/textures/crops/wheat/4_mature.png", L"");
    textures_.Load(L"crop_corn_0", L"assets/textures/crops/corn/0_seed.png", L"");
    textures_.Load(L"crop_corn_1", L"assets/textures/crops/corn/1_sprout.png", L"");
    textures_.Load(L"crop_corn_2", L"assets/textures/crops/corn/2_young.png", L"");
    textures_.Load(L"crop_corn_3", L"assets/textures/crops/corn/3_mid.png", L"");
    textures_.Load(L"crop_corn_4", L"assets/textures/crops/corn/4_mature.png", L"");
    textures_.Load(L"crop_carrot_0", L"assets/textures/crops/carrot/0_seed.png", L"");
    textures_.Load(L"crop_carrot_1", L"assets/textures/crops/carrot/1_sprout.png", L"");
    textures_.Load(L"crop_carrot_2", L"assets/textures/crops/carrot/2_young.png", L"");
    textures_.Load(L"crop_carrot_3", L"assets/textures/crops/carrot/3_mid.png", L"");
    textures_.Load(L"crop_carrot_4", L"assets/textures/crops/carrot/4_mature.png", L"");
    textures_.Load(L"crop_tomato_0", L"assets/textures/crops/tomato/0_seed.png", L"");
    textures_.Load(L"crop_tomato_1", L"assets/textures/crops/tomato/1_sprout.png", L"");
    textures_.Load(L"crop_tomato_2", L"assets/textures/crops/tomato/2_young.png", L"");
    textures_.Load(L"crop_tomato_3", L"assets/textures/crops/tomato/3_mid.png", L"");
    textures_.Load(L"crop_tomato_4", L"assets/textures/crops/tomato/4_mature.png", L"");
    textures_.Load(L"animal_chicken_idle", L"assets/textures/animals/chicken/chicken_idle.png", L"");
    textures_.Load(L"animal_chicken_fed", L"assets/textures/animals/chicken/chicken_fed.png", L"");
    textures_.Load(L"animal_chicken_ready", L"assets/textures/animals/chicken/chicken_ready.png", L"");
    textures_.Load(L"animal_cow_idle", L"assets/textures/animals/cow/cow_idle.png", L"");
    textures_.Load(L"animal_sheep_idle", L"assets/textures/animals/sheep/sheep_idle.png", L"");
    textures_.Load(L"animal_pig_idle", L"assets/textures/animals/pig/pig_idle.png", L"");
    textures_.Load(L"animal_unknown", L"assets/textures/placeholders/animal_unknown.png", L"");
    textures_.Load(L"building_chicken_coop", L"assets/textures/buildings/chicken_coop.png", L"");
    textures_.Load(L"building_cow_barn", L"assets/textures/buildings/cow_barn.png", L"");
    textures_.Load(L"building_sheep_pen", L"assets/textures/buildings/sheep_pen.png", L"");
    textures_.Load(L"building_pig_pen", L"assets/textures/buildings/pig_pen.png", L"");
    textures_.Load(L"building_feed_mill", L"assets/textures/buildings/feed_mill.png", L"");
    textures_.Load(L"building_shop", L"assets/textures/buildings/shop.png", L"");
    textures_.Load(L"building_warehouse", L"assets/textures/buildings/warehouse.png", L"");
    textures_.Load(L"building_unknown", L"assets/textures/placeholders/building_unknown.png", L"");
    textures_.Load(L"seed_wheat", L"assets/textures/seeds/wheat_seed.png", L"");
    textures_.Load(L"seed_corn", L"assets/textures/seeds/corn_seed.png", L"");
    textures_.Load(L"seed_carrot", L"assets/textures/seeds/carrot_seed.png", L"");
    textures_.Load(L"seed_tomato", L"assets/textures/seeds/tomato_seed.png", L"");
    textures_.Load(L"item_wheat", L"assets/textures/items/wheat.png", L"");
    textures_.Load(L"item_corn", L"assets/textures/items/corn.png", L"");
    textures_.Load(L"item_carrot", L"assets/textures/items/carrot.png", L"");
    textures_.Load(L"item_tomato", L"assets/textures/items/tomato.png", L"");
    textures_.Load(L"item_feed", L"assets/textures/items/feed.png", L"");
    textures_.Load(L"item_egg", L"assets/textures/items/egg.png", L"assets/icons/egg.ppm");
    textures_.Load(L"item_milk", L"assets/textures/items/milk.png", L"");
    textures_.Load(L"item_wool", L"assets/textures/items/wool.png", L"");
    textures_.Load(L"item_fertilizer", L"assets/textures/items/fertilizer.png", L"");
    textures_.Load(L"item_coin", L"assets/textures/items/coin.png", L"");
    textures_.Load(L"item_unknown", L"assets/textures/placeholders/item_unknown.png", L"");
    textures_.Load(L"weather_sunny", L"assets/textures/weather/sunny.png", L"");
    textures_.Load(L"weather_rainy", L"assets/textures/weather/rainy.png", L"");
    textures_.Load(L"weather_cloudy", L"assets/textures/weather/cloudy.png", L"");
    font_ = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");

    ShowWindow(hwnd_, SW_SHOW);
    SetTimer(hwnd_, 1, 1000, nullptr);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

FarmWindow::~FarmWindow() {
    if (font_ != nullptr) {
        DeleteObject(font_);
        font_ = nullptr;
    }
}

LRESULT CALLBACK FarmWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    FarmWindow* app = reinterpret_cast<FarmWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCT*>(lparam);
        app = reinterpret_cast<FarmWindow*>(create->lpCreateParams);
        app->hwnd_ = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        return TRUE;
    }
    return app == nullptr ? DefWindowProc(hwnd, msg, wparam, lparam)
                          : app->HandleMessage(msg, wparam, lparam);
}

LRESULT FarmWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd_, &ps);
            PaintBuffered(hdc);
            EndPaint(hwnd_, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_MOUSEMOVE: {
            RECT ca; GetClientRect(hwnd_, &ca);
            float sx = ca.right > 0 ? static_cast<float>(kDesignW) / ca.right : 1.0f;
            float sy = ca.bottom > 0 ? static_cast<float>(kDesignH) / ca.bottom : 1.0f;
            hover_mx_ = static_cast<int>(LOWORD(lparam) * sx);
            hover_my_ = static_cast<int>(HIWORD(lparam) * sy);
            hover_active_ = false;
            for (const HoverEntry& he : hover_rects_) {
                if (hover_mx_ >= he.rect.left && hover_mx_ <= he.rect.right &&
                    hover_my_ >= he.rect.top && hover_my_ <= he.rect.bottom) {
                    hovered_item_ = he.item; hover_active_ = true; break;
                }
            }
            return 0;
        }
        case WM_SIZE:
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        case WM_LBUTTONDOWN: {
            // Scale mouse coords from physical → design (1120×720)
            RECT ca; GetClientRect(hwnd_, &ca);
            float sx = ca.right > 0 ? static_cast<float>(kDesignW) / ca.right : 1.0f;
            float sy = ca.bottom > 0 ? static_cast<float>(kDesignH) / ca.bottom : 1.0f;
            const int x = static_cast<int>(LOWORD(lparam) * sx);
            const int y = static_cast<int>(HIWORD(lparam) * sy);
            for (const UiButton& button : buttons_) {
                if (x >= button.rect.left && x <= button.rect.right && y >= button.rect.top &&
                    y <= button.rect.bottom) {
                    OnButton(button.id);
                    InvalidateRect(hwnd_, nullptr, FALSE);
                    return 0;
                }
            }
            if (screen_ == Screen::Farm && y >= 165 && y < 405 && x >= 40 && x < 520) {
                selected_plot_ = ((y - 165) / 80) * 6 + (x - 40) / 80;
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            return 0;
        }
        case WM_TIMER:
            if (screen_ != Screen::Menu) {
                game_.AdvanceBySpeed();
                game_.PruneToasts(game_.Time().CurrentTick());
                if (!game_.LastEventMessage().empty()) {
                    message_ = Utf8ToWide(game_.LastEventMessage());
                    game_.ClearLastEventMessage();
                }
                game_.AutoSaveIfNeeded(Narrow(SavePath()));
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd_, msg, wparam, lparam);
}

void FarmWindow::PaintBuffered(HDC target) {
    RECT area;
    GetClientRect(hwnd_, &area);
    const int width = area.right - area.left;
    const int height = area.bottom - area.top;
    if (width <= 0 || height <= 0) return;

    // Back buffer at design resolution — always render at 1120×720
    HDC memory = CreateCompatibleDC(target);
    HBITMAP bitmap = CreateCompatibleBitmap(target, kDesignW, kDesignH);
    HGDIOBJ old = SelectObject(memory, bitmap);
    Paint(memory);

    // Scale the back buffer to fill the actual client area
    SetStretchBltMode(target, HALFTONE);
    StretchBlt(target, 0, 0, width, height, memory, 0, 0, kDesignW, kDesignH, SRCCOPY);
    SelectObject(memory, old);
    DeleteObject(bitmap);
    DeleteDC(memory);
}

void FarmWindow::Paint(HDC hdc) {
    buttons_.clear();
    hover_rects_.clear();
    if (font_ != nullptr) {
        SelectObject(hdc, font_);
    }
    // Background fill at design resolution
    RECT design_area{0, 0, kDesignW, kDesignH};
    COLORREF bg_colors[] = { RGB(228, 245, 210), RGB(210, 235, 175), RGB(245, 225, 170), RGB(225, 238, 248) };
    COLORREF bg = bg_colors[static_cast<int>(game_.Season().Current())];
    DrawTextureOrFill(hdc, L"grass", design_area, bg);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(58, 54, 39));
    if (screen_ == Screen::Menu) {
        DrawMenu(hdc);
    } else {
        DrawGame(hdc);
    }
}

// Unified panel helper — draws a centered card background + bold title + dark border
void FarmWindow::DrawPanelFrame(HDC hdc, const wchar_t* title, int x, int y, int w, int h) {
    RECT card{x, y, x + w, y + h};
    DrawTextureOrFill(hdc, L"order_card", card, RGB(252, 248, 235));
    // Dark border
    FrameRect(hdc, &card, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    // Inner border
    RECT inner{x + 2, y + 2, x + w - 2, y + h - 2};
    FrameRect(hdc, &inner, reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
    // Bold title
    HFONT tf = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
    HGDIOBJ old = SelectObject(hdc, tf);
    SetTextColor(hdc, RGB(58, 54, 39));
    Text(hdc, x + 18, y + 12, title);
    SelectObject(hdc, old);
    DeleteObject(tf);
    // Title separator
    RECT sep{x + 18, y + 46, x + w - 18, y + 48};
    Fill(hdc, sep, RGB(180, 160, 120));
}

void FarmWindow::DrawMenu(HDC hdc) {
    DrawPanelFrame(hdc, L"农场游戏", kPanelX, kPanelY, 580, 300);
    SetTextColor(hdc, RGB(58, 54, 39));
    Text(hdc, kPanelX + 30, kPanelY + 68, L"1 秒现实时间 = 2 分钟游戏时间。");
    Text(hdc, kPanelX + 30, kPanelY + 95, L"通过 DAG 解锁种子、土地和动物。");
    Button(hdc, kBtnNew, RECT{kPanelX + 30, kPanelY + 135, kPanelX + 180, kPanelY + 175}, L"新游戏");
    Button(hdc, kBtnContinue, RECT{kPanelX + 30, kPanelY + 195, kPanelX + 180, kPanelY + 235}, L"继续游戏");
    Text(hdc, kPanelX + 30, kPanelY + 265, message_);
}

void FarmWindow::DrawGame(HDC hdc) {
    DrawStatus(hdc);
    DrawTabs(hdc);
    switch (screen_) {
        case Screen::Farm:
            DrawFarm(hdc);
            break;
        case Screen::Ranch:
            DrawRanch(hdc);
            break;
        case Screen::Workshop:
            DrawWorkshop(hdc);
            break;
        case Screen::Orders:
            DrawOrders(hdc);
            break;
        case Screen::Warehouse:
            DrawWarehouse(hdc);
            break;
        case Screen::Shop:
            DrawShop(hdc);
            break;
        case Screen::Unlock:
            DrawUnlock(hdc);
            break;
        case Screen::Achieve:
            DrawAchievements(hdc);
            break;
        case Screen::DailyTask:
            DrawDailyTasks(hdc);
            break;
        case Screen::Merchant:
            DrawMerchant(hdc);
            break;
        case Screen::Fishing:
            DrawFishing(hdc);
            break;
        case Screen::Save:
            DrawSave(hdc);
            break;
        case Screen::Menu:
            break;
    }
    // Toast notifications
    DrawToasts(hdc);
    // Item tooltip
    DrawItemTooltip(hdc);
    // Material shortage popup (overlays everything)
    if (popup_visible_) DrawMaterialPopup(hdc);
    // Bottom message bar — centered, smaller font
    if (!message_.empty()) {
        HFONT msg_font = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
        HGDIOBJ old_msg = SelectObject(hdc, msg_font);
        SIZE ms; GetTextExtentPoint32W(hdc, message_.c_str(), static_cast<int>(message_.size()), &ms);
        int mw = ms.cx + 30, mx = (kDesignW - mw) / 2;
        COLORREF msg_colors[] = { RGB(240, 250, 225), RGB(230, 245, 210), RGB(252, 238, 195), RGB(225, 240, 250) };
        Fill(hdc, RECT{mx, 654, mx + mw, 678}, msg_colors[static_cast<int>(game_.Season().Current())]);
        Text(hdc, mx + 15, 658, message_);
        SelectObject(hdc, old_msg);
        DeleteObject(msg_font);
    }
}

void FarmWindow::DrawStatus(HDC hdc) {
    COLORREF bar_colors[] = { RGB(106, 165, 87), RGB(80, 140, 50), RGB(190, 140, 55), RGB(85, 140, 185) };
    DrawTextureOrFill(hdc, L"top_bar", RECT{0, 0, 1120, 62},
                      bar_colors[static_cast<int>(game_.Season().Current())]);
    SetTextColor(hdc, RGB(255, 250, 230));
    const TimeSnapshot time = game_.Time().Snapshot();
    const WeatherSnapshot weather = game_.Weather().Snapshot();
    DrawTextureOrFill(hdc, L"item_coin", RECT{22, 14, 46, 38}, RGB(237, 190, 67));
    DrawTextureOrFill(hdc, WeatherTextureKey(weather.weather), RECT{560, 12, 600, 52},
                      RGB(198, 218, 128));
    // Season display with color
    const SeasonSnapshot ssnap = game_.Season().Snapshot();
    COLORREF season_color = RGB(100,180,100);  // Spring green
    switch (ssnap.season) {
        case Season::Spring: season_color = RGB(100,200,100); break;
        case Season::Summer: season_color = RGB(220,120,80); break;
        case Season::Autumn: season_color = RGB(220,170,60); break;
        case Season::Winter: season_color = RGB(130,180,220); break;
    }
    SetTextColor(hdc, season_color);
    int season_day = ssnap.ticks_elapsed / kTicksPerDay + 1;
    int season_remain_day = std::max(1, ssnap.ticks_remaining / kTicksPerDay);
    std::wstringstream sstxt;
    sstxt << Utf8ToWide(SeasonName(ssnap.season)) << L" 剩余" << season_remain_day << L"天";
    Text(hdc, 610, 20, sstxt.str());
    SetTextColor(hdc, RGB(255, 250, 230));
    std::wstringstream ss;
    ss << L"金币 " << game_.Player().Gold() << L"   等级 " << game_.Player().Level()
       << L"   经验 " << game_.Player().Experience() << L"/"
       << game_.Player().ExpToNextLevel() << L"   第 " << time.day << L" 天 "
       << std::setw(2) << std::setfill(L'0') << time.hour << L":" << std::setw(2)
       << time.minute << std::setfill(L' ') << L"   天气 " << WeatherName(weather.weather)
       << L"   速度 " << SpeedName(time.speed);
    Text(hdc, 52, 20, ss.str());
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::DrawTabs(HDC hdc) {
    int x = 22;
    auto tab = [&](int id, const wchar_t* label, int w = 64) {
        Button(hdc, id, RECT{x, 75, x + w, 110}, label); x += w + 4;
    };
    tab(kTabFarm, L"农田");
    tab(kTabRanch, L"牧场");
    tab(kTabWorkshop, L"加工");
    tab(kTabOrders, L"订单");
    tab(kTabWarehouse, L"仓库");
    tab(kTabShop, L"商店");
    tab(kTabUnlock, L"解锁", 56);
    tab(kTabDailyTask, game_.DailyTasks().HasNewComplete() ? L"任务●" : L"任务", 56);
    tab(kTabAchieve, game_.Achievements().HasNew() ? L"成就●" : L"成就", 56);
    tab(kTabFishing, L"钓鱼", 56);
    if (game_.Merchant().IsPresent())
        tab(kTabMerchant, L"商人!", 56);
    tab(kTabSave, L"存档", 56);
    // Control buttons on the right
    int rx = 1000;
    Button(hdc, kTick, RECT{rx, 75, rx + 50, 110}, L"+2"); rx += 54;
    Button(hdc, kPause, RECT{rx, 75, rx + 50, 110}, L"⏸"); rx += 54;
    Button(hdc, kSpeed1, RECT{rx, 75, rx + 30, 110}, L"1x"); rx += 34;
    Button(hdc, kSpeed2, RECT{rx, 75, rx + 30, 110}, L"2x"); rx += 34;
    Button(hdc, kSpeed4, RECT{rx, 75, rx + 30, 110}, L"4x");
}

void FarmWindow::DrawFarm(HDC hdc) {
    // Greenhouse tab toggle
    bool gh_unlocked = game_.Player().IsUnlocked(UnlockId::GreenhouseTech);
    if (gh_unlocked) {
        if (show_greenhouse_) {
            Button(hdc, kTabGreenhouse, RECT{40, 130, 180, 165}, L"◀ 温室地块");
        } else {
            Button(hdc, kTabGreenhouse, RECT{40, 130, 180, 165}, L"普通地块 ▶");
        }
    }
    const bool is_gh = show_greenhouse_ && gh_unlocked;
    const auto plots = is_gh ? game_.Planting().GreenhouseView(game_.Time().CurrentTick())
                              : game_.Planting().View(game_.Time().CurrentTick());
    Text(hdc, 200, 142, is_gh ? L"温室 (不受季节/天气影响)" : L"农田地块");
    for (std::size_t i = 0; i < plots.size(); ++i) {
        const int row = static_cast<int>(i) / 6;
        const int col = static_cast<int>(i) % 6;
        RECT r{40 + col * 80, 165 + row * 80, 105 + col * 80, 230 + row * 80};
        const std::wstring ground =
            plots[i].water == PlotWaterState::Watered ? L"soil_wet" : L"soil";
        DrawTextureOrFill(hdc, ground, r, RGB(139, 94, 52));
        if (plots[i].state != PlotState::Idle) {
            RECT crop_rect{r.left + 9, r.top + 7, r.right - 9, r.bottom - 14};
            DrawTextureOrFill(hdc, CropTextureKey(plots[i].crop, CropStage(plots[i])), crop_rect,
                              plots[i].state == PlotState::Mature ? RGB(237, 190, 67)
                                                                  : RGB(136, 170, 69));
        }
        if (static_cast<int>(i) == selected_plot_) {
            HBRUSH border = CreateSolidBrush(RGB(40, 40, 40));
            FrameRect(hdc, &r, border);
            DeleteObject(border);
        }
        std::wstringstream ss;
        ss << i;
        if (plots[i].state != PlotState::Idle) {
            ss << L" " << ItemName(plots[i].crop);
        }
        Text(hdc, r.left + 5, r.top + 5, ss.str());
        if (plots[i].remaining_ticks > 0) {
            std::wstringstream rt;
            rt << plots[i].remaining_ticks * kGameMinutesPerTick << L"分";
            Text(hdc, r.left + 13, r.bottom - 22, rt.str());
        }
    }
    // --- Batch operation bar at top ---
    Text(hdc, 570, 130, L"批量操作");
    Button(hdc, kBatchPlant, RECT{570, 148, 730, 183}, L"一键种植");
    Button(hdc, kBatchWater, RECT{740, 148, 900, 183}, L"一键浇水");
    Button(hdc, kBatchFertilize, RECT{570, 191, 730, 226}, L"一键施肥");
    Button(hdc, kBatchHarvest, RECT{740, 191, 900, 226}, L"一键收割");
    Text(hdc, 570, 245, L"选中地块操作");
    auto btnPlant = [&](int id, ItemId seed, const wchar_t* label, int x, int y) {
        bool can = PlantingSystem::CanPlantInSeason(seed, game_.Season().Current());
        if (!can) SetTextColor(hdc, RGB(160,160,160));
        std::wstring txt = can ? label : (std::wstring(label) + L"(不可种植)");
        Button(hdc, id, RECT{x, y, x + 150, y + 38}, txt.c_str());
        if (!can) SetTextColor(hdc, RGB(58,54,39));
    };
    btnPlant(kPlantWheat, ItemId::WheatSeed, L"种小麦", 570, 265);
    btnPlant(kPlantCorn, ItemId::CornSeed, L"种玉米", 570, 311);
    btnPlant(kPlantCarrot, ItemId::CarrotSeed, L"种胡萝卜", 570, 357);
    btnPlant(kPlantTomato, ItemId::TomatoSeed, L"种番茄", 570, 403);
    Button(hdc, kWater, RECT{750, 265, 900, 303}, L"浇水");
    Button(hdc, kFertilize, RECT{750, 311, 900, 349}, L"施肥");
    Button(hdc, kHarvest, RECT{750, 357, 900, 395}, L"收割");
    Button(hdc, kExpand, RECT{750, 403, 900, 441}, L"购买土地");
    if (game_.Player().IsUnlocked(UnlockId::GreenhouseTech))
        Button(hdc, kBuildGreenhouse, RECT{570, 449, 720, 487}, L"建温室地块");
}

void FarmWindow::DrawRanch(HDC hdc) {
    Text(hdc, 40, 135, L"牧场设施");
    int y = 175;
    for (const RanchFacilityView& facility : game_.Ranch().FacilityViews()) {
        RECT icon{45, y - 8, 93, y + 40};
        DrawTextureOrFill(hdc, FacilityTextureKey(facility.kind), icon, RGB(190, 134, 84));
        std::wstringstream ss;
        ss << FacilityName(facility.kind) << L" #" << facility.id << L"  动物 "
           << facility.animal_count << L"/" << facility.capacity << L"  空闲 "
           << facility.idle_count << L"  生产中 " << facility.producing_count << L"  可收 "
           << facility.ready_count;
        Text(hdc, 105, y, ss.str());
        const auto animals = game_.Ranch().AnimalViews(facility.id, game_.Time().CurrentTick());
        int animal_x = 105;
        for (const AnimalView& animal : animals) {
            RECT animal_rect{animal_x, y + 28, animal_x + 44, y + 72};
            DrawTextureOrFill(hdc, AnimalTextureKey(animal.kind, animal.state), animal_rect,
                              RGB(226, 196, 126));
            std::wstringstream detail;
            if (animal.is_baby) {
                detail << L"幼崽 " << animal.age_ticks << L"/" << kBabyGrowTicks;
                SetTextColor(hdc, RGB(100, 180, 220));
            } else {
                // Mood indicator
                if (animal.mood >= 80) detail << L"😊";
                else if (animal.mood >= 30) detail << L"😐";
                else detail << L"😞";
                detail << L" " << animal.age_ticks << L"/" << animal.max_age;
                if (animal.state == AnimalState::Ready) detail << L" ★";
            }
            Text(hdc, animal_x + 2, y + 46, detail.str());
            SetTextColor(hdc, RGB(58, 54, 39));
            animal_x += 50;
            if (animal_x > 470) {
                break;
            }
        }
        y += 94;
    }
    Text(hdc, 600, 145, L"牧场操作");
    Button(hdc, kBuildCowBarn, RECT{600, 185, 740, 223}, L"建牛棚");
    Button(hdc, kBuildSheepPen, RECT{755, 185, 895, 223}, L"建羊圈");
    Button(hdc, kBuyChicken, RECT{600, 240, 740, 278}, L"购买鸡");
    Button(hdc, kBuyCow, RECT{755, 240, 895, 278}, L"购买牛");
    Button(hdc, kBuySheep, RECT{910, 240, 1050, 278}, L"购买羊");
    Button(hdc, kFeedAll, RECT{600, 295, 740, 333}, L"全部喂食");
    Button(hdc, kHarvestAll, RECT{755, 295, 895, 333}, L"全部收获");
    Text(hdc, 600, 365, L"饲料：鸡吃鸡饲料；牛和羊吃牛饲料。");
    Text(hdc, 600, 400, L"产物：鸡蛋、牛奶、羊毛可出售或进入订单。");
}

void FarmWindow::DrawWorkshop(HDC hdc) {
    DrawPanelFrame(hdc, L"加工坊", kPanelX, kPanelY, kPanelW, kPanelH);
    const WorkshopView view = game_.Workshop().View();
    int bx = kPanelX + kB, by = kPanelY + 58;
    std::wstringstream ss;
    ss << L"队列 " << view.queue_count << L"/" << view.queue_capacity << L"  货架 "
       << view.shelf_count << L"/" << view.shelf_capacity << L"  鸡饲料 " << view.chicken_feed_shelf
       << L"  牛饲料 " << view.cow_feed_shelf << L"  面包 " << view.bread_shelf
       << L"  奶酪 " << view.cheese_shelf << L"  果酱 " << view.jam_shelf;
    if (view.queue_count > 0) ss << L"  剩余 " << view.active_remaining_ticks * kGameMinutesPerTick << L"分";
    Text(hdc, bx, by, ss.str()); by += 26;
    // Recipes
    SetTextColor(hdc, RGB(100,100,100)); Text(hdc, bx, by, L"配方:"); SetTextColor(hdc, RGB(58,54,39)); by += 22;
    Text(hdc, bx + 12, by, L"鸡饲料: 小麦×2→4金 [Lv1]  牛饲料: 玉米×2+胡萝卜×1→7金 [Lv1]  面包: 小麦×3→18金 [Lv2]"); by += 20;
    Text(hdc, bx + 12, by, L"奶酪: 牛奶×2→35金 [Lv3]  果酱: 番茄×3→40金 [Lv4]"); by += 28;
    // Buttons
    Button(hdc, kMakeChickenFeed, RECT{bx, by, bx + 150, by + kBtnH}, L"鸡饲料 x1");
    Button(hdc, kMakeChickenFeed3, RECT{bx + 158, by, bx + 308, by + kBtnH}, L"鸡饲料 x3");
    Button(hdc, kMakeCowFeed, RECT{bx + 316, by, bx + 466, by + kBtnH}, L"牛饲料 x1");
    by += kBtnH + 8;
    Button(hdc, kMakeBread, RECT{bx, by, bx + 150, by + kBtnH}, L"面包 x1");
    Button(hdc, kMakeCheese, RECT{bx + 158, by, bx + 308, by + kBtnH}, L"奶酪 x1");
    Button(hdc, kMakeJam, RECT{bx + 316, by, bx + 466, by + kBtnH}, L"果酱 x1");
    Button(hdc, kClaimFeed, RECT{bx + 474, by - kBtnH - 8, bx + 630, by + kBtnH}, L"领取产品");
}

void FarmWindow::DrawOrders(HDC hdc) {
    DrawPanelFrame(hdc, L"订单", kPanelX, kPanelY, kPanelW, kPanelH);
    const auto& orders = game_.Orders().Orders();
    int y = kPanelY + 58;
    for (std::size_t i = 0; i < orders.size(); ++i) {
        const OrderData& order = orders[i];
        RECT card{kPanelX + kB, y, kPanelX + kPanelW - kB, y + 56};
        DrawTextureOrFill(hdc, L"order_card", card, selected_orders_.count(static_cast<int>(i)) ? RGB(255, 230, 160) : RGB(250, 242, 214));
        if (!order.requirements.empty())
            DrawTextureOrFill(hdc, ItemTextureKey(order.requirements[0].item), RECT{kPanelX + kB + 14, y + 8, kPanelX + kB + 50, y + 44}, RGB(226, 196, 126));
        std::wstringstream ss;
        if (!order.label.empty()) ss << L"[" << Utf8ToWide(order.label) << L"] ";
        for (std::size_t r = 0; r < order.requirements.size(); ++r) {
            if (r > 0) ss << L" + ";
            ss << ItemName(order.requirements[r].item) << L" x" << order.requirements[r].quantity;
        }
        ss << L" → " << order.reward_gold << L"金 +" << order.reward_exp << L"经验";
        if (order.state == OrderState::CoolingDown) ss << L" 冷却中";
        Text(hdc, kPanelX + kB + 62, y + 12, ss.str());
        Button(hdc, kOrderBase + static_cast<int>(i), RECT{kPanelX + kPanelW - kB - 90, y + 8, kPanelX + kPanelW - kB, y + 44}, L"选择");
        y += 64;
    }
    Button(hdc, kBatchCompleteOrder, RECT{kPanelX + kB, kPanelY + kPanelH - 56, kPanelX + kB + 280, kPanelY + kPanelH - 20}, L"交付所有可完成订单");
    Button(hdc, kCompleteOrder, RECT{kPanelX + kB + 290, kPanelY + kPanelH - 56, kPanelX + kB + 440, kPanelY + kPanelH - 20}, L"交付选中");
    Button(hdc, kAbandonOrder, RECT{kPanelX + kB + 450, kPanelY + kPanelH - 56, kPanelX + kB + 600, kPanelY + kPanelH - 20}, L"放弃选中");
}

void FarmWindow::DrawWarehouse(HDC hdc) {
    const int wh_level = game_.Player().WarehouseLevel(), wh_cap = game_.Player().WarehouseCapacity();
    std::wstringstream cap; cap << L"仓库 Lv" << wh_level << L"  " << game_.Player().WarehouseUsed() << L"/" << wh_cap;
    DrawPanelFrame(hdc, cap.str().c_str(), kPanelX, kPanelY, kPanelW, kPanelH);
    int y = kPanelY + 58, bx = kPanelX + kB;
    const auto items = game_.Player().InventoryView();
    for (std::size_t idx = 0; idx < items.size(); ++idx) {
        const InventoryItemView& item = items[idx];
        bool locked = game_.Player().IsItemLocked(item.item);
        RECT slot{bx, y, bx + 500, y + 30};
        DrawTextureOrFill(hdc, L"inventory_slot", slot, locked ? RGB(240, 200, 200) : RGB(250, 242, 214));
        DrawTextureOrFill(hdc, ItemTextureKey(item.item), RECT{bx + 4, y + 2, bx + 30, y + 28}, RGB(226, 196, 126));
        RegisterItemRect(item.item, RECT{bx, y, bx + 380, y + 28});  // tooltip hover
        std::wstringstream ss; ss << ItemName(item.item) << L" x" << item.quantity;
        if (locked) ss << L" (锁定)";
        Text(hdc, bx + 38, y + 5, ss.str());
        Button(hdc, kLockItemBase + static_cast<int>(idx), RECT{bx + 420, y + 2, bx + 472, y + 28}, locked ? L"解锁" : L"锁定");
        y += 33;
        if (y > kPanelY + kPanelH - 100) break;
    }
    int by2 = kPanelY + kPanelH - 58;
    Button(hdc, kBatchSellAll, RECT{bx, by2, bx + 180, by2 + kBtnH}, L"批量出售全部");
    if (wh_level < kWarehouseMaxLevel) {
        std::wstringstream upg; upg << L"扩容→Lv" << (wh_level+1) << L"(" << kWarehouseCapacities[wh_level] << L"格)" << kWarehouseUpgradeCosts[wh_level] << L"金";
        Button(hdc, kUpgradeWarehouse, RECT{bx + 190, by2, bx + 400, by2 + kBtnH}, upg.str());
    } else { Text(hdc, bx + 200, by2 + 8, L"仓库已达最大等级"); }
}

void FarmWindow::DrawShop(HDC hdc) {
    DrawPanelFrame(hdc, L"商店", kPanelX, kPanelY, 500, kPanelH);
    int bx = kPanelX + kB, y = kPanelY + 58;
    struct ShopEntry { int buy_id; ItemId item; const wchar_t* desc; };
    const std::vector<ShopEntry> goods = {
        {kBuyWheatSeed, ItemId::WheatSeed, L"种植获得小麦"},
        {kBuyCornSeed, ItemId::CornSeed, L"种植获得玉米"},
        {kBuyCarrotSeed, ItemId::CarrotSeed, L"种植获得胡萝卜"},
        {kBuyTomatoSeed, ItemId::TomatoSeed, L"夏季种植获得番茄"},
        {kBuyFertilizer, ItemId::Fertilizer, L"加速作物生长"},
    };
    // Quantity selector helper: uses base_id, base_id+1, base_id+2 for 1,5,10
    auto drawQtyBtns = [&](int base_id, int qx_off, int cur_qty) {
        int qi = 0; for (int q : {1, 5, 10}) {
            bool sel = cur_qty == q;
            RECT qr{bx + qx_off, y + 12, bx + qx_off + 34, y + 34};
            DrawTextureOrFill(hdc, L"wood", qr, sel ? RGB(120, 200, 120) : RGB(180, 140, 100));
            FrameRect(hdc, &qr, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            std::wstring txt = std::to_wstring(q);
            Text(hdc, bx + qx_off + 8, y + 16, txt);
            buttons_.push_back({qr, base_id + qi});
            qx_off += 38; ++qi;
        }
    };
    // Standard goods
    int item_idx = 0;
    for (const auto& g : goods) {
        const ItemInfo& info = GetItemInfo(g.item);
        bool unlocked = !IsSeed(g.item) || game_.Player().IsSeedUnlocked(g.item);
        int have = game_.Player().ItemCount(g.item);
        int qty = shop_qtys_[item_idx];
        RECT card{bx, y, bx + 400, y + 48};
        DrawTextureOrFill(hdc, L"shop_card", card, unlocked ? RGB(252, 248, 235) : RGB(235, 225, 225));
        DrawTextureOrFill(hdc, ItemTextureKey(g.item), RECT{bx + 10, y + 6, bx + 44, y + 40}, RGB(226, 196, 126));
        RegisterItemRect(g.item, RECT{bx, y, bx + 400, y + 48});
        SetTextColor(hdc, unlocked ? RGB(58,54,39) : RGB(160,160,160));
        std::wstringstream lbl;
        lbl << ItemName(g.item) << L"  " << (qty * info.buy_price) << L"金  (拥有:" << have << L")";
        Text(hdc, bx + 52, y + 8, lbl.str());
        std::string price_str = std::to_string(info.buy_price) + "金/个  ";
        Text(hdc, bx + 52, y + 28, Utf8ToWide(price_str) + g.desc);
        SetTextColor(hdc, RGB(58,54,39));
        if (unlocked) {
            Button(hdc, g.buy_id, RECT{bx + 220, y + 6, bx + 280, y + 42}, L"购买");
            drawQtyBtns(kShopQtyBase + item_idx * 3, 292, qty);
        } else {
            Text(hdc, bx + 230, y + 14, L"未解锁");
        }
        y += 56; ++item_idx;
    }
    // Bait row
    y += 4;
    int bqty = shop_qtys_[6];
    RECT bait_card{bx, y, bx + 400, y + 48};
    DrawTextureOrFill(hdc, L"shop_card", bait_card, RGB(252, 248, 235));
    DrawTextureOrFill(hdc, ItemTextureKey(ItemId::Fertilizer), RECT{bx + 10, y + 6, bx + 44, y + 40}, RGB(226, 196, 126));
    std::wstringstream bl;
    bl << L"鱼饵  " << (bqty * 3) << L"金  (拥有:" << game_.Fishing().BaitCount() << L")";
    Text(hdc, bx + 52, y + 8, bl.str());
    Text(hdc, bx + 52, y + 28, L"3金/个  钓鱼消耗品");
    Button(hdc, kBuyBait, RECT{bx + 220, y + 6, bx + 280, y + 42}, L"购买");
    drawQtyBtns(kBaitQtyBase, 292, bqty);
}

void FarmWindow::DrawUnlock(HDC hdc) {
    DrawPanelFrame(hdc, L"解锁技术", kPanelX, kPanelY, kPanelW, kPanelH);
    int y = kPanelY + 58, bx = kPanelX + kB;
    int index = 0;
    for (const UnlockNode& node : UnlockGraph::Nodes()) {
        std::wstringstream ss;
        ss << Utf8ToWide(node.name) << L"  Lv" << node.required_level << L"  " << node.gold_cost << L"金";
        if (game_.Player().IsUnlocked(node.id)) { ss << L"  [已解锁]"; SetTextColor(hdc, RGB(80,160,80)); }
        else if (game_.Player().CanUnlock(node.id)) { ss << L"  [可解锁]"; SetTextColor(hdc, RGB(160,140,40)); }
        else { ss << L"  [锁定]"; SetTextColor(hdc, RGB(160,160,160)); }
        Text(hdc, bx, y + 4, ss.str()); SetTextColor(hdc, RGB(58,54,39));
        Button(hdc, kUnlockBase + index, RECT{bx + 420, y, bx + 520, y + kBtnH}, L"解锁");
        y += kBtnH + 8; ++index;
        if (y > kPanelY + kPanelH - 30) break;
    }
}

void FarmWindow::DrawSave(HDC hdc) {
    DrawPanelFrame(hdc, L"存档管理", kPanelX, kPanelY, 500, 220);
    int bx = kPanelX + kB;
    Button(hdc, kSave, RECT{bx, kPanelY + 60, bx + 160, kPanelY + 100}, L"手动保存");
    Button(hdc, kLoad, RECT{bx + 180, kPanelY + 60, bx + 340, kPanelY + 100}, L"读取存档");
    Button(hdc, kNewGame, RECT{bx, kPanelY + 120, bx + 160, kPanelY + 160}, L"新游戏");
    Text(hdc, bx, kPanelY + 185, L"存档文件：saves/save01.farm  每12刻自动保存。");
}

void FarmWindow::DrawTextureOrFill(HDC hdc, const std::wstring& key, RECT rect,
                                   COLORREF fallback) {
    if (!textures_.Draw(hdc, key, rect)) {
        Fill(hdc, rect, fallback);
    }
}

void FarmWindow::Fill(HDC hdc, RECT rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void FarmWindow::Text(HDC hdc, int x, int y, const std::wstring& text) {
    TextOutW(hdc, x, y, text.c_str(), static_cast<int>(text.size()));
}

void FarmWindow::Button(HDC hdc, int id, RECT rect, const std::wstring& text) {
    buttons_.push_back(UiButton{rect, id});
    COLORREF wood_colors[] = { RGB(160, 130, 70), RGB(130, 160, 70), RGB(195, 140, 60), RGB(120, 145, 175) };
    DrawTextureOrFill(hdc, L"wood", rect, wood_colors[static_cast<int>(game_.Season().Current())]);
    FrameRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetTextColor(hdc, RGB(255, 250, 230));
    Text(hdc, rect.left + 10, rect.top + 9, text);
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::OnButton(int id) {
    if (id == kPopupCancel) {
        popup_visible_ = false;
        return;
    }
    if (id == kPopupGo) {
        popup_visible_ = false;
        screen_ = BestScreenForItem(popup_item_);
        SetMessage(L"已跳转到 " + std::wstring(ItemName(popup_item_)) + L" 的获取途径。");
        return;
    }
    if (id == kBtnNew) {
        game_ = Game::NewGame();
        screen_ = Screen::Farm;
        SetMessage(L"新游戏已开始。");
        return;
    }
    if (id == kBtnContinue) {
        if (TryContinue()) {
            screen_ = Screen::Farm;
        }
        return;
    }
    if (id == kTabFishing) { screen_ = Screen::Fishing; return; }
    if (id == kTabMerchant) {
        screen_ = Screen::Merchant;
        return;
    }
    if (id == kTabDailyTask) {
        screen_ = Screen::DailyTask;
        game_.DailyTasks().ClearNewFlag();
        return;
    }
    if (id == kTabAchieve) {
        screen_ = Screen::Achieve;
        game_.Achievements().ClearNewFlag();
        return;
    }
    if (id >= kTabFarm && id <= kTabSave) {
        screen_ = static_cast<Screen>(static_cast<int>(Screen::Farm) + (id - kTabFarm));
        return;
    }
    if (id == kTick) {
        game_.AdvanceTicks(1);
        SetMessage(L"时间推进 2 分钟。");
        return;
    }
    if (id == kPause) {
        game_.Time().IsPaused() ? game_.Time().Resume() : game_.Time().Pause();
        return;
    }
    if (id == kSpeed1) {
        game_.Time().SetSpeed(GameSpeed::Normal);
        return;
    }
    if (id == kSpeed2) {
        game_.Time().SetSpeed(GameSpeed::Fast);
        return;
    }
    if (id == kSpeed4) {
        game_.Time().SetSpeed(GameSpeed::VeryFast);
        return;
    }

    Result<void> result = Result<void>::success();
    if (id == kPlantWheat) {
        selected_seed_ = ItemId::WheatSeed; error_item_ = ItemId::WheatSeed;
        result = game_.Planting().TryPlantAt(game_.Player(), selected_plot_, ItemId::WheatSeed,
                                             game_.Time().CurrentTick(), game_.Season().Current());
    } else if (id == kPlantCorn) {
        selected_seed_ = ItemId::CornSeed; error_item_ = ItemId::CornSeed;
        result = game_.Planting().TryPlantAt(game_.Player(), selected_plot_, ItemId::CornSeed,
                                             game_.Time().CurrentTick(), game_.Season().Current());
    } else if (id == kPlantCarrot) {
        selected_seed_ = ItemId::CarrotSeed; error_item_ = ItemId::CarrotSeed;
        result = game_.Planting().TryPlantAt(game_.Player(), selected_plot_, ItemId::CarrotSeed,
                                             game_.Time().CurrentTick(), game_.Season().Current());
    } else if (id == kPlantTomato) {
        selected_seed_ = ItemId::TomatoSeed; error_item_ = ItemId::TomatoSeed;
        result = game_.Planting().TryPlantAt(game_.Player(), selected_plot_, ItemId::TomatoSeed,
                                             game_.Time().CurrentTick(), game_.Season().Current());
    } else if (id == kWater) {
        result = game_.Planting().WaterPlot(selected_plot_, game_.Time().CurrentTick());
    } else if (id == kFertilize) {
        result = game_.Planting().ApplyFertilizer(game_.Player(), selected_plot_,
                                                  game_.Time().CurrentTick());
    } else if (id == kHarvest) {
        result = game_.Planting().Harvest(game_.Player(), selected_plot_);
    } else if (id == kBatchPlant) {
        error_item_ = selected_seed_;
        auto batch = game_.Planting().BatchPlant(game_.Player(), selected_seed_,
                                                  game_.Time().CurrentTick(), game_.Season().Current());
        if (batch.ok() && batch.value > 0) {
            SetMessage(L"成功种植 " + std::to_wstring(batch.value) + L" 块地。");
            return;
        }
        if (batch.ok()) {
            SetMessage(L"没有空闲地块或种子不足。");
            return;
        }
        result = Result<void>::failure(batch.code);
    } else if (id == kBatchWater) {
        auto batch = game_.Planting().BatchWater(game_.Time().CurrentTick());
        if (batch.ok()) {
            SetMessage(L"成功浇水 " + std::to_wstring(batch.value) + L" 块地。");
            return;
        }
        result = Result<void>::failure(batch.code);
    } else if (id == kBatchFertilize) {
        auto batch = game_.Planting().BatchFertilize(game_.Player(), game_.Time().CurrentTick());
        if (batch.ok()) {
            SetMessage(L"成功施肥 " + std::to_wstring(batch.value) + L" 块地。");
            return;
        }
        result = Result<void>::failure(batch.code);
    } else if (id == kBatchHarvest) {
        auto batch = show_greenhouse_ ? game_.Planting().BatchHarvestGreenhouse(game_.Player())
                                      : game_.Planting().BatchHarvest(game_.Player());
        if (batch.ok()) { SetMessage(L"成功收割 " + std::to_wstring(batch.value) + L" 块地。"); return; }
        result = Result<void>::failure(batch.code);
    } else if (id == kTabGreenhouse) {
        show_greenhouse_ = !show_greenhouse_;
        return;
    } else if (id == kBuildGreenhouse) {
        auto built = game_.Planting().BuildGreenhouse(game_.Player());
        if (built.ok()) { SetMessage(L"建造了一块温室地块。"); return; }
        result = Result<void>::failure(built.code);
    } else if (id == kExpand) {
        auto expanded = game_.Planting().Expand(game_.Player());
        result = expanded.ok() ? Result<void>::success() : Result<void>::failure(expanded.code);
    } else if (id == kBuildCowBarn) {
        auto built = game_.Ranch().BuildFacility(game_.Player(), RanchFacilityKind::CowBarn);
        result = built.ok() ? Result<void>::success() : Result<void>::failure(built.code);
    } else if (id == kBuildSheepPen) {
        auto built = game_.Ranch().BuildFacility(game_.Player(), RanchFacilityKind::SheepPen);
        result = built.ok() ? Result<void>::success() : Result<void>::failure(built.code);
    } else if (id == kBuyChicken) {
        auto bought = game_.Ranch().BuyAnimal(game_.Player(), FirstFacility(RanchFacilityKind::ChickenCoop),
                                              AnimalKind::Chicken);
        result = bought.ok() ? Result<void>::success() : Result<void>::failure(bought.code);
    } else if (id == kBuyCow) {
        auto bought = game_.Ranch().BuyAnimal(game_.Player(), FirstFacility(RanchFacilityKind::CowBarn),
                                              AnimalKind::Cow);
        result = bought.ok() ? Result<void>::success() : Result<void>::failure(bought.code);
    } else if (id == kBuySheep) {
        auto bought = game_.Ranch().BuyAnimal(game_.Player(), FirstFacility(RanchFacilityKind::SheepPen),
                                              AnimalKind::Sheep);
        result = bought.ok() ? Result<void>::success() : Result<void>::failure(bought.code);
    } else if (id == kFeedAll) {
        int fed = 0;
        for (const RanchFacilityView& facility : game_.Ranch().FacilityViews()) {
            auto one = game_.Ranch().BatchFeed(game_.Player(), facility.id, game_.Time().CurrentTick());
            if (one.ok()) {
                fed += one.value;
            }
        }
        SetMessage(L"已喂食 " + std::to_wstring(fed) + L" 只动物。");
        return;
    } else if (id == kHarvestAll) {
        int got = 0;
        for (const RanchFacilityView& facility : game_.Ranch().FacilityViews()) {
            auto one = game_.Ranch().BatchHarvest(game_.Player(), facility.id);
            if (one.ok()) {
                got += one.value;
            }
        }
        SetMessage(L"已收获 " + std::to_wstring(got) + L" 个产品。");
        return;
    } else if (id == kMakeChickenFeed) {
        error_item_ = ItemId::Wheat;
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::ChickenFeed, 1,
                                                   game_.Player().Level());
    } else if (id == kMakeChickenFeed3) {
        error_item_ = ItemId::Wheat;
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::ChickenFeed, 3,
                                                   game_.Player().Level());
    } else if (id == kMakeCowFeed) {
        error_item_ = ItemId::Corn;
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::CowFeed, 1,
                                                   game_.Player().Level());
    } else if (id == kMakeBread) {
        error_item_ = ItemId::Wheat;
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::Bread, 1,
                                                   game_.Player().Level());
    } else if (id == kMakeCheese) {
        error_item_ = ItemId::Milk;
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::Cheese, 1,
                                                   game_.Player().Level());
    } else if (id == kMakeJam) {
        error_item_ = ItemId::Tomato;
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::Jam, 1,
                                                   game_.Player().Level());
    } else if (id == kClaimFeed) {
        result = game_.Workshop().ClaimProduct(game_.Player());
    } else if (id >= kOrderBase && id < kOrderBase + 10) {
        int slot = id - kOrderBase;
        if (selected_orders_.count(slot)) {
            selected_orders_.erase(slot);
        } else {
            selected_orders_.insert(slot);
        }
    } else if (id == kCompleteOrder) {
        // Complete the first selected order, or slot 0 if none selected
        int target = selected_orders_.empty() ? 0 : *selected_orders_.begin();
        result = game_.Orders().CompleteOrder(game_.Player(), target,
                                              game_.Time().CurrentTick());
        if (result.ok()) {
            selected_orders_.erase(target);
        }
    } else if (id == kAbandonOrder) {
        if (selected_orders_.empty()) {
            SetMessage(L"请先选择要放弃的订单。");
            return;
        }
        std::vector<int> slots(selected_orders_.begin(), selected_orders_.end());
        int count = game_.Orders().AbandonOrders(slots, game_.Time().CurrentTick());
        selected_orders_.clear();
        if (count > 0) {
            SetMessage(L"已放弃 " + std::to_wstring(count) + L" 个订单。");
            return;
        }
    } else if (id == kBatchCompleteOrder) {
        auto batch_result = game_.Orders().BatchComplete(game_.Player(), game_.Time().CurrentTick());
        int count = batch_result.ok() ? batch_result.value.completed : 0;
        int gold = batch_result.ok() ? batch_result.value.gold_earned : 0;
        int exp = batch_result.ok() ? batch_result.value.exp_earned : 0;
        if (count > 0) {
            SetMessage(L"成功交付 " + std::to_wstring(count) + L" 个订单，获得 " +
                       std::to_wstring(gold) + L" 金币和 " + std::to_wstring(exp) + L" 经验！");
            selected_orders_.clear();
            return;
        }
        SetMessage(L"没有可以交付的订单。");
        return;
    } else if (id == kBuyWheatSeed) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::WheatSeed, shop_qtys_[0]);
        if (result.ok()) SetMessage(L"购买成功！已获得小麦种子 x" + std::to_wstring(shop_qtys_[0]));
        return;
    } else if (id == kBuyCornSeed) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::CornSeed, shop_qtys_[1]);
        if (result.ok()) SetMessage(L"购买成功！已获得玉米种子 x" + std::to_wstring(shop_qtys_[1]));
        return;
    } else if (id == kBuyCarrotSeed) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::CarrotSeed, shop_qtys_[2]);
        if (result.ok()) SetMessage(L"购买成功！已获得胡萝卜种子 x" + std::to_wstring(shop_qtys_[2]));
        return;
    } else if (id == kBuyTomatoSeed) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::TomatoSeed, shop_qtys_[3]);
        if (result.ok()) SetMessage(L"购买成功！已获得番茄种子 x" + std::to_wstring(shop_qtys_[3]));
        return;
    } else if (id == kBuyFertilizer) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::Fertilizer, shop_qtys_[4]);
        if (result.ok()) SetMessage(L"购买成功！已获得肥料 x" + std::to_wstring(shop_qtys_[4]));
        return;
    } else if (id >= kBaitQtyBase && id < kBaitQtyBase + 10) {
        int off = id - kBaitQtyBase;
        shop_qtys_[6] = (off == 0) ? 1 : (off == 1) ? 5 : 10;  // bait at index 6
        return;
    } else if (id >= kShopQtyBase && id < kShopQtyBase + 50) {
        int raw = id - kShopQtyBase;
        int idx = raw / 3;  // item index
        int off = raw % 3;
        if (idx >= 0 && idx < 6) shop_qtys_[idx] = (off == 0) ? 1 : (off == 1) ? 5 : 10;
        return;
    } else if (id == kBuyBait) {
        int cost = 3 * shop_qtys_[6];
        if (game_.Player().Gold() < cost) { SetMessage(L"金币不足，无法购买！"); return; }
        auto spent = game_.Player().TrySpendGold(cost);
        if (spent.ok()) {
            game_.Fishing().AddBait(shop_qtys_[6]);
            SetMessage(L"购买成功！已获得鱼饵 x" + std::to_wstring(shop_qtys_[6]));
        }
        return;
    } else if (id == kSellWheat) {
        result = game_.Player().TrySellItem(ItemId::Wheat, 1);
    } else if (id == kSellCorn) {
        result = game_.Player().TrySellItem(ItemId::Corn, 1);
    } else if (id == kSellCarrot) {
        result = game_.Player().TrySellItem(ItemId::Carrot, 1);
    } else if (id == kSellTomato) {
        result = game_.Player().TrySellItem(ItemId::Tomato, 1);
    } else if (id == kSellEgg) {
        result = game_.Player().TrySellItem(ItemId::Egg, 1);
    } else if (id == kSellMilk) {
        result = game_.Player().TrySellItem(ItemId::Milk, 1);
    } else if (id == kSellWool) {
        result = game_.Player().TrySellItem(ItemId::Wool, 1);
    } else if (id == kBatchSellAll) {
        int sold = game_.Player().BatchSellAll();
        if (sold > 0) {
            SetMessage(L"批量出售了 " + std::to_wstring(sold) + L" 个物品。");
        } else {
            SetMessage(L"没有可以出售的物品。");
        }
        return;
    } else if (id == kUpgradeWarehouse) {
        result = game_.Player().UpgradeWarehouse();
        if (result.ok()) {
            SetMessage(L"仓库已扩容至 Lv" + std::to_wstring(game_.Player().WarehouseLevel()));
            return;
        }
    } else if (id >= kLockItemBase && id < kLockItemBase + 50) {
        int idx = id - kLockItemBase;
        auto items = game_.Player().InventoryView();
        if (idx >= 0 && idx < static_cast<int>(items.size())) {
            ItemId item = items[static_cast<std::size_t>(idx)].item;
            bool locked = game_.Player().IsItemLocked(item);
            game_.Player().SetItemLocked(item, !locked);
            return;
        }
    } else if (id >= kUnlockBase && id < kUnlockBase + 100) {
        const int index = id - kUnlockBase;
        const auto& nodes = UnlockGraph::Nodes();
        if (index >= 0 && index < static_cast<int>(nodes.size())) {
            result = game_.Player().UnlockContent(nodes[static_cast<std::size_t>(index)].id);
        }
    } else if (id == kSave) {
        std::filesystem::create_directories("saves");
        result = game_.ManualSave(Narrow(SavePath()));
    } else if (id == kLoad) {
        result = game_.Load(Narrow(SavePath()));
    } else if (id == kCastLine) {
        auto& fs2 = game_.Fishing();
        if (fs2.BaitCount() <= 0) { SetMessage(L"鱼饵不足，无法抛竿！"); return; }
        if (fs2.State() != FishingState::Idle) { SetMessage(L"请先完成上一次钓鱼。"); return; }
        fs2.CastLine(game_.Time().CurrentTick());
        SetMessage(L"抛竿！等待鱼咬钩...");
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    } else if (id == kReelIn) {
        auto& fs3 = game_.Fishing();
        if (fs3.State() != FishingState::Biting) return;
        auto caught = fs3.TryReelIn(game_.Time().CurrentTick());
        if (caught.ok()) {
            int fish_id = fs3.LastCaughtFish();
            const FishDef* f = fs3.GetFishDef(fish_id);
            if (f) {
                game_.Player().TryAddItem(ItemId::Egg, 1);
                std::string rl = f->rarity == FishRarity::Legendary ? "传说" : f->rarity == FishRarity::Rare ? "稀有" : "普通";
                int tc = f->rarity == FishRarity::Legendary ? 2 : f->rarity == FishRarity::Rare ? 2 : 0;
                game_.EnqueueToast("【" + rl + "】" + std::string(f->name) + " (" + std::to_string(f->sell_price) + "金)", tc, game_.Time().CurrentTick() + 70);
                SetMessage(std::wstring(L"钓到了 ") + Utf8ToWide(f->name) + L"！");
            }
        } else {
            SetMessage(L"鱼跑掉了...下次在绿色区域点击提竿！");
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    } else if (id == kFishUpgradeRod) {
        auto& fs4 = game_.Fishing();
        int cost = fs4.RodUpgradeCost();
        if (cost == 0) { SetMessage(L"鱼竿已达最高等级 Lv5！"); return; }
        auto spent = game_.Player().TrySpendGold(cost);
        if (spent.ok()) {
            fs4.UpgradeRod();
            SetMessage(L"鱼竿已升级至 Lv" + std::to_wstring(fs4.RodLevel()) + L"！");
            return;
        }
        result = spent;
    } else if (id == kFishCollection) {
        show_fishpedia_ = !show_fishpedia_;
        return;
    } else if (id >= kMerchantBuyBase && id < kMerchantBuyBase + 10) {
        result = game_.Merchant().BuyFromMerchant(game_.Player(), id - kMerchantBuyBase);
    } else if (id >= kMerchantSellBase && id < kMerchantSellBase + 10) {
        result = game_.Merchant().SellToMerchant(game_.Player(), id - kMerchantSellBase);
    } else if (id == kNewGame) {
        game_ = Game::NewGame();
        SetMessage(L"新游戏已开始。");
        return;
    }
    SetMessage(result.code);
    if (result.code == ErrorCode::InsufficientItem) {
        ShowMaterialPopup(error_item_);
    }
}

void FarmWindow::DrawToasts(HDC hdc) {
    const auto& toasts = game_.Toasts();
    if (toasts.empty()) return;

    // Compact toasts in top-right corner — smaller font
    HFONT toast_font = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
    HGDIOBJ old_font = SelectObject(hdc, toast_font);
    int y = 118;
    int count = std::min(static_cast<int>(toasts.size()), 3);
    for (int i = 0; i < count; ++i) {
        const ToastMessage& t = toasts[i];
        COLORREF bg = (t.color == 0) ? RGB(70, 170, 70) :
                      (t.color == 1) ? RGB(200, 55, 55) : RGB(225, 145, 35);
        std::wstring wtext = Utf8ToWide(t.text);
        SIZE ts;
        GetTextExtentPoint32W(hdc, wtext.c_str(), static_cast<int>(wtext.size()), &ts);
        int w = (std::min)(static_cast<int>(ts.cx) + 24, 450);
        int x = kDesignW - w - 16;
        RECT r{x, y, x + w, y + 22};
        Fill(hdc, r, bg);
        SetTextColor(hdc, RGB(255, 255, 255));
        Text(hdc, x + 10, y + 3, wtext);
        SetTextColor(hdc, RGB(58, 54, 39));
        y += 26;
    }
    SelectObject(hdc, old_font);
    DeleteObject(toast_font);
}

void FarmWindow::DrawAchievements(HDC hdc) {
    auto& ach = game_.Achievements();
    const auto& all = ach.All();
    if (all.empty()) { Text(hdc, 60, 175, L"成就系统未初始化。"); return; }

    // --- Panel background ---
    const int panel_x = 25, panel_w = 830, panel_y = 130, panel_h = 500;
    RECT panel{panel_x, panel_y, panel_x + panel_w, panel_y + panel_h};
    DrawTextureOrFill(hdc, L"order_card", panel, RGB(250, 242, 214));
    FrameRect(hdc, &panel, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    // --- Title ---
    SetTextColor(hdc, RGB(58, 54, 39));
    HFONT title_font = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
    HGDIOBJ old_font = SelectObject(hdc, title_font);
    Text(hdc, panel_x + 20, panel_y + 12, L"成就系统");
    SelectObject(hdc, old_font);
    DeleteObject(title_font);

    // --- Horizontal separator ---
    RECT sep{panel_x + 20, panel_y + 50, panel_x + panel_w - 20, panel_y + 52};
    Fill(hdc, sep, RGB(180, 160, 120));

    // --- Achievement list ---
    int y = panel_y + 60;
    AchievementCategory cur_cat = AchievementCategory::Plant;
    COLORREF cat_colors[] = {
        RGB(100, 170, 90),   // Plant - green
        RGB(180, 140, 60),   // Ranch - brown
        RGB(90, 140, 200),   // Process - blue
        RGB(200, 140, 60),   // Order - orange
        RGB(140, 100, 190),  // Explore - purple
        RGB(200, 170, 40),   // General - gold
    };

    for (std::size_t idx = 0; idx < all.size(); ++idx) {
        const Achievement& a = all[idx];

        // --- Category header ---
        if (a.category != cur_cat) {
            cur_cat = a.category;
            y += 8;
            // Category separator line
            RECT cat_sep{panel_x + 40, y - 2, panel_x + panel_w - 40, y};
            Fill(hdc, cat_sep, RGB(210, 200, 175));
            y += 10;
        }

        int cat_idx = static_cast<int>(a.category);
        COLORREF cat_color = cat_colors[cat_idx % 6];

        // Category header on first item of each category
        if (idx == 0 || a.category != all[idx - 1].category) {
            std::wstring cat_name = Utf8ToWide(CategoryName(a.category));
            SetTextColor(hdc, cat_color);
            HFONT cat_font = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
            HGDIOBJ old_cat = SelectObject(hdc, cat_font);
            Text(hdc, panel_x + 40, y, L"■ " + cat_name);
            SelectObject(hdc, old_cat);
            DeleteObject(cat_font);
            y += 28;
        }

        // --- Achievement row ---
        // Category color square icon
        RECT icon{panel_x + 50, y + 2, panel_x + 66, y + 18};
        Fill(hdc, icon, a.completed ? RGB(80, 180, 80) : cat_color);

        // Name (left) + progress (right) on same line with proper spacing
        std::wstring name = Utf8ToWide(a.name);
        std::wstringstream prog_ss;
        prog_ss << a.current << L"/" << a.target;
        std::wstring progress = prog_ss.str();

        COLORREF name_color = a.completed ? RGB(60, 160, 60) : RGB(58, 54, 39);
        COLORREF prog_color = a.completed ? RGB(60, 160, 60) : RGB(140, 130, 110);

        // Name left-aligned
        SetTextColor(hdc, name_color);
        Text(hdc, panel_x + 75, y, name);

        // Progress right-aligned
        SIZE ts;
        GetTextExtentPoint32W(hdc, progress.c_str(), static_cast<int>(progress.size()), &ts);
        SetTextColor(hdc, prog_color);
        Text(hdc, panel_x + panel_w - 40 - ts.cx, y, progress);

        // Completion checkmark
        if (a.completed) {
            SetTextColor(hdc, RGB(60, 180, 60));
            Text(hdc, panel_x + panel_w - 30, y, L"✓");
        }

        SetTextColor(hdc, RGB(58, 54, 39));
        y += 22;

        if (y > panel_y + panel_h - 16) break;
    }

    // --- Wood frame border ---
    RECT outer{panel_x - 4, panel_y - 4, panel_x + panel_w + 4, panel_y + panel_h + 4};
    FrameRect(hdc, &outer, reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));

    if (ach.HasNew()) ach.ClearNewFlag();
}

void FarmWindow::DrawFishing(HDC hdc) {
    auto& fs = game_.Fishing();
    // Fishpedia overlay
    if (show_fishpedia_) {
        DrawPanelFrame(hdc, L"鱼类图鉴", kPanelX + 100, kPanelY, 620, kPanelH);
        int fy = kPanelY + 58;
        Button(hdc, kFishCollection, RECT{kPanelX + kPanelW - 160, kPanelY + 10, kPanelX + kPanelW - 40, kPanelY + 44}, L"关闭");
        for (const FishDef& f : FishingSystem::FishList()) {
            bool caught = false; int count = 0;
            for (const FishRecord& r : fs.Collection()) { if (r.fish_id == f.id) { caught = true; count = r.count; break; } }
            SetTextColor(hdc, caught ? RGB(58,54,39) : RGB(170,170,170));
            std::wstring name = caught ? Utf8ToWide(f.name) : L"???";
            const wchar_t* rarity = f.rarity == FishRarity::Legendary ? L"传说" : f.rarity == FishRarity::Rare ? L"稀有" : L"普通";
            std::wstringstream ss; ss << name << L"  [" << rarity << L"]  " << f.sell_price << L"金";
            if (caught) ss << L"  x" << count;
            Text(hdc, kPanelX + 130, fy, ss.str());
            fy += 22; if (fy > kPanelY + kPanelH - 16) break;
        }
        SetTextColor(hdc, RGB(58,54,39));
        return;
    }

    DrawPanelFrame(hdc, L"农场池塘", kPanelX, kPanelY, kPanelW, kPanelH);
    int bx = kPanelX + kB, y = kPanelY + 58;
    int rx = bx + 520;
    int bait = fs.BaitCount();
    int rod = fs.RodLevel();
    FishingState st = fs.State();

    // --- Status bar ---
    std::wstringstream status;
    status << L"鱼竿 Lv" << rod << L"  鱼饵: " << bait << L"  累计垂钓: " << fs.TotalCatches() << L"次";
    SetTextColor(hdc, RGB(58,54,39));
    Text(hdc, bx, y, status.str());
    COLORREF st_col = RGB(60,170,60);
    const wchar_t* st_txt = L"[准备就绪]";
    if (st == FishingState::Waiting) { st_txt = L"[等待中...]"; st_col = RGB(200,160,30); }
    else if (st == FishingState::Biting) { st_txt = L"[咬钩了！]"; st_col = RGB(220,60,60); }
    else if (bait <= 0) { st_txt = L"[鱼饵不足]"; st_col = RGB(200,60,60); }
    SetTextColor(hdc, st_col);
    Text(hdc, bx + 460, y, st_txt);
    SetTextColor(hdc, RGB(58,54,39));
    y += 32;

    // --- Pond ---
    RECT pond{bx, y, bx + 500, y + 170};
    DrawTextureOrFill(hdc, L"soil_wet", pond, RGB(70, 130, 200));
    for (int i = 0; i < 8; ++i) {
        int rx2 = bx + 30 + (game_.Time().CurrentTick() * 3 + i * 60) % 440;
        int ry2 = y + 20 + (i * 31) % 130;
        RECT ripple{rx2, ry2, rx2 + 40, ry2 + 3};
        Fill(hdc, ripple, RGB(120, 180, 240));
    }
    Text(hdc, bx + 180, y + 75, L"~ 农场池塘 ~");

    // Last catch with rarity color
    int last_id = fs.LastCaughtFish();
    if (last_id >= 0) {
        const FishDef* f = fs.GetFishDef(last_id);
        if (f) {
            const wchar_t* rlabel = L"普通"; COLORREF rcol = RGB(60,150,60);
            if (f->rarity == FishRarity::Rare) { rlabel = L"稀有"; rcol = RGB(60,100,220); }
            else if (f->rarity == FishRarity::Legendary) { rlabel = L"传说"; rcol = RGB(220,150,30); }
            std::wstringstream r; r << L"上次: 【" << rlabel << L"】" << Utf8ToWide(f->name) << L" " << f->sell_price << L"金";
            SetTextColor(hdc, rcol);
            Text(hdc, bx, y + 180, r.str());
            SetTextColor(hdc, RGB(58,54,39));
        }
    }

    // --- Right column: action buttons ---
    int ry = kPanelY + 58;
    if (st == FishingState::Idle) {
        if (bait <= 0) SetTextColor(hdc, RGB(160,160,160));
        Button(hdc, kCastLine, RECT{rx, ry, rx + 200, ry + kBtnH}, L"抛竿钓鱼 (1鱼饵)");
        if (bait <= 0) { SetTextColor(hdc, RGB(200,60,60)); Text(hdc, rx, ry + kBtnH + 4, L"鱼饵不足！"); SetTextColor(hdc, RGB(58,54,39)); }
    } else if (st == FishingState::Waiting) {
        SetTextColor(hdc, RGB(160,160,160)); Text(hdc, rx, ry + 8, L"等待咬钩中..."); SetTextColor(hdc, RGB(58,54,39));
    } else if (st == FishingState::Biting) {
        Button(hdc, kReelIn, RECT{rx, ry, rx + 200, ry + kBtnH}, L"提竿！");
        RECT bar{rx, ry + kBtnH + 8, rx + 200, ry + kBtnH + 22};
        Fill(hdc, bar, RGB(60, 60, 60));
        int gs = fs.GetReelGreenStart(), ge = fs.GetReelGreenEnd();
        Fill(hdc, RECT{rx + gs * 2, ry + kBtnH + 8, rx + ge * 2, ry + kBtnH + 22}, RGB(80, 200, 80));
        int pos = fs.GetReelPos();
        Fill(hdc, RECT{rx + pos * 2 - 3, ry + kBtnH + 6, rx + pos * 2 + 3, ry + kBtnH + 24}, RGB(255, 255, 255));
        Text(hdc, rx, ry + kBtnH + 26, L"指针在绿色区点击提竿！");
    }
    ry += 70;

    int cost = fs.RodUpgradeCost();
    std::wstringstream upg;
    if (cost > 0) upg << L"升级鱼竿 Lv" << (rod+1) << L" (" << cost << L"金)";
    else upg << L"鱼竿已满级 Lv5";
    if (cost > 0) Button(hdc, kFishUpgradeRod, RECT{rx, ry, rx + 200, ry + kBtnH}, upg.str());
    else Text(hdc, rx, ry + 8, upg.str());
    ry += 46;

    Button(hdc, kFishCollection, RECT{rx, ry, rx + 200, ry + kBtnH}, L"鱼类图鉴");
}

void FarmWindow::DrawMerchant(HDC hdc) {
    auto& mer = game_.Merchant();
    if (!mer.IsPresent()) { screen_ = Screen::Farm; return; }
    DrawPanelFrame(hdc, L"旅行商人", kPanelX, kPanelY, kPanelW, kPanelH);
    int bx = kPanelX + kB, y = kPanelY + 58;
    // --- Sale items (player buys) ---
    Text(hdc, bx, y, L"━ 商人出售 ━"); y += 24;
    const auto& items = mer.SaleItems();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const MerchantItem& mi = items[i];
        RECT row{bx, y, bx + 380, y + 42};
        DrawTextureOrFill(hdc, L"shop_card", row, mi.sold_out ? RGB(240,220,220) : RGB(250,242,214));
        DrawTextureOrFill(hdc, ItemTextureKey(mi.item), RECT{bx + 6, y + 4, bx + 38, y + 38}, RGB(226,196,126));
        std::wstringstream ss;
        ss << ItemName(mi.item) << L"  " << mi.price << L"金";
        if (mi.sold_out) ss << L" (售罄)"; else ss << L" 库存:" << mi.stock;
        Text(hdc, bx + 48, y + 8, ss.str());
        if (!mi.sold_out)
            Button(hdc, kMerchantBuyBase + static_cast<int>(i), RECT{bx + 300, y + 6, bx + 370, y + 36}, L"购买");
        y += 48;
    }

    // --- Buy offers (player sells to merchant) ---
    y += 8;
    Text(hdc, bx, y, L"━ 商人高价收购 ━"); y += 24;
    const auto& offers = mer.BuyOffers();
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const MerchantOffer& mo = offers[i];
        RECT row{bx, y, bx + 450, y + 42};
        bool full = mo.bought >= mo.max_buy;
        DrawTextureOrFill(hdc, L"shop_card", row, full ? RGB(240,220,220) : RGB(255,245,225));
        DrawTextureOrFill(hdc, ItemTextureKey(mo.item), RECT{bx + 6, y + 4, bx + 38, y + 38}, RGB(226,196,126));
        std::wstringstream ss;
        ss << ItemName(mo.item) << L"  收购价 " << mo.price << L"金  进度 "
           << mo.bought << L"/" << mo.max_buy;
        int have = game_.Player().ItemCount(mo.item);
        ss << L"  (拥有:" << have << L")";
        Text(hdc, bx + 48, y + 8, ss.str());
        if (!full)
            Button(hdc, kMerchantSellBase + static_cast<int>(i), RECT{bx + 370, y + 6, bx + 440, y + 36}, L"出售");
        y += 48;
    }
}

void FarmWindow::DrawDailyTasks(HDC hdc) {
    auto& dts = game_.DailyTasks();
    const auto& tasks = dts.TodayTasks();
    DrawPanelFrame(hdc, L"每日任务", kPanelX, kPanelY, kPanelW, kPanelH);
    int y = kPanelY + 58, bx = kPanelX + kB;
    if (tasks.empty()) { Text(hdc, bx, y, L"今天的任务还未刷新。"); return; }
    for (const DailyTask& t : tasks) {
        // Task row background
        RECT row{bx, y, bx + kPanelW - kB * 2, y + 50};
        DrawTextureOrFill(hdc, L"order_card", row, t.completed ? RGB(220, 255, 220) : RGB(250, 242, 214));

        // Task name
        COLORREF c = t.completed ? RGB(60, 170, 60) : RGB(58, 54, 39);
        SetTextColor(hdc, c);
        Text(hdc, bx + 12, y + 6, Utf8ToWide(t.name));

        // Progress bar
        RECT bar{bx + 12, y + 28, bx + 400, y + 38};
        Fill(hdc, bar, RGB(200, 200, 200));
        int pw = t.target > 0 ? (t.current * 384 / t.target) : 0;
        if (pw > 0) {
            RECT fill{bx + 14, y + 30, bx + 14 + pw, y + 36};
            Fill(hdc, fill, t.completed ? RGB(80, 200, 80) : RGB(100, 150, 200));
        }
        SetTextColor(hdc, RGB(58, 54, 39));

        // Progress text
        std::wstringstream ss;
        ss << t.current << L"/" << t.target << L"  " << t.reward_gold << L"金 +" << t.reward_exp << L"经验";
        if (!t.reward_text.empty()) ss << L" +" << Utf8ToWide(t.reward_text);
        if (t.completed) ss << L"  ✓已领取";
        Text(hdc, bx + 420, y + 16, ss.str());

        y += 58;
    }
}

void FarmWindow::SetMessage(ErrorCode code) {
    message_ = ErrorMessage(code);
}

void FarmWindow::SetMessage(const std::wstring& text) {
    message_ = text;
}

std::wstring FarmWindow::ItemLine(ItemId item, int quantity) const {
    std::wstringstream ss;
    ss << ItemName(item) << L" x" << quantity;
    if (game_.Player().IsItemLocked(item)) {
        ss << L"（已锁定）";
    }
    return ss.str();
}

std::wstring FarmWindow::SavePath() const {
    return L"saves/save01.farm";
}

bool FarmWindow::TryContinue() {
    if (!SaveManager::Exists(Narrow(SavePath()))) {
        game_ = Game::NewGame();
        SetMessage(L"没有找到存档，已开始新游戏。");
        return true;
    }
    auto loaded = game_.Load(Narrow(SavePath()));
    SetMessage(loaded.code);
    return loaded.ok();
}

int FarmWindow::FirstFacility(RanchFacilityKind kind) const {
    for (const RanchFacilityView& facility : game_.Ranch().FacilityViews()) {
        if (facility.kind == kind) {
            return facility.id;
        }
    }
    return -1;
}

void FarmWindow::RegisterItemRect(ItemId item, RECT rect) {
    hover_rects_.push_back({item, rect});
}

void FarmWindow::DrawItemTooltip(HDC hdc) {
    if (!hover_active_ || screen_ == Screen::Menu) return;
    const ItemInfo& info = GetItemInfo(hovered_item_);
    const int tw = 300, th = 130;
    int tx = hover_mx_ + 16, ty = hover_my_ + 16;
    if (tx + tw > kDesignW - 10) tx = hover_mx_ - tw - 10;
    if (ty + th > kDesignH - 10) ty = hover_my_ - th - 10;
    if (tx < 5) tx = 5; if (ty < 5) ty = 5;

    // Tooltip card
    RECT card{tx, ty, tx + tw, ty + th};
    DrawTextureOrFill(hdc, L"order_card", card, RGB(252, 248, 235));
    FrameRect(hdc, &card, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    SetTextColor(hdc, RGB(58, 54, 39));
    // Item icon + name
    DrawTextureOrFill(hdc, ItemTextureKey(hovered_item_), RECT{tx + 8, ty + 6, tx + 36, ty + 34}, RGB(226, 196, 126));
    Text(hdc, tx + 44, ty + 10, ItemName(hovered_item_));

    // Price
    std::wstringstream ss;
    ss << L"售价: " << info.sell_price << L" 金币  购买: " << info.buy_price << L" 金币";
    Text(hdc, tx + 12, ty + 40, ss.str());

    // Usage + source
    const wchar_t* usage = L"出售/交付订单";
    const wchar_t* source = L"种植获得";
    Screen target = BestScreenForItem(hovered_item_);
    switch (info.category) {
        case ItemCategory::Seed: usage = L"种植作物"; source = L"商店购买"; break;
        case ItemCategory::Crop: usage = L"出售/交付订单/加工"; source = L"种植获得"; break;
        case ItemCategory::AnimalProduct: usage = L"出售/交付订单"; source = L"牧场收获"; break;
        case ItemCategory::Feed: usage = L"喂养动物"; source = L"饲料坊加工"; break;
        case ItemCategory::ProcessedGood: usage = L"出售/交付订单"; source = L"饲料坊加工"; break;
        case ItemCategory::Consumable: usage = L"加速生长"; source = L"商店购买"; break;
    }
    Text(hdc, tx + 12, ty + 62, (std::wstring(L"用途: ") + usage).c_str());
    Text(hdc, tx + 12, ty + 82, (std::wstring(L"来源: ") + source).c_str());

    // Click to navigate hint
    SetTextColor(hdc, RGB(60, 100, 200));
    const wchar_t* screen_names[] = {L"", L"农田", L"牧场", L"饲料坊", L"订单", L"仓库", L"商店", L"解锁", L"成就", L"存档"};
    std::wstring nav = L"点击跳转 → " + std::wstring(screen_names[static_cast<int>(target)]);
    Text(hdc, tx + 12, ty + 104, nav.c_str());
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::ShowMaterialPopup(ItemId item) {
    popup_item_ = item;
    popup_visible_ = true;
}

Screen FarmWindow::BestScreenForItem(ItemId item) const {
    switch (GetItemInfo(item).category) {
        case ItemCategory::Seed:            return Screen::Shop;
        case ItemCategory::Crop:            return Screen::Farm;
        case ItemCategory::AnimalProduct:   return Screen::Ranch;
        case ItemCategory::Feed:            return Screen::Workshop;
        case ItemCategory::ProcessedGood:   return Screen::Workshop;
        case ItemCategory::Consumable:      return Screen::Shop;
        default:                            return Screen::Shop;
    }
}

void FarmWindow::DrawMaterialPopup(HDC hdc) {
    // Semi-transparent overlay at design resolution
    for (int i = 0; i < 5; ++i) {
        RECT stripe{0, i * 2, kDesignW, i * 2 + 1};
        Fill(hdc, stripe, RGB(0, 0, 0));
    }
    // Popup card centered
    int pw = 480, ph = 200, px = (kDesignW - pw) / 2, py = (kDesignH - ph) / 2;
    DrawPanelFrame(hdc, L"材料不足", px, py, pw, ph);
    SetTextColor(hdc, RGB(58, 54, 39));
    // Item icon
    DrawTextureOrFill(hdc, ItemTextureKey(popup_item_), RECT{px + 30, py + 60, px + 70, py + 100}, RGB(226, 196, 126));
    // Text
    std::wstring msg = L"缺少 " + std::wstring(ItemName(popup_item_)) + L"，是否前往获取？";
    Text(hdc, px + 90, py + 72, msg);
    Text(hdc, px + 30, py + 110, Utf8ToWide(std::string("获取途径：") + ToString(GetItemInfo(popup_item_).category)));
    // Buttons
    Button(hdc, kPopupCancel, RECT{px + pw - 240, py + ph - 48, px + pw - 140, py + ph - 12}, L"取消");
    Button(hdc, kPopupGo, RECT{px + pw - 130, py + ph - 48, px + pw - 30, py + ph - 12}, L"前往获取");
}

}  // namespace

int RunFarmApp() {
    FarmWindow window;
    return window.Run();
}

}  // namespace farm::ui

#endif
