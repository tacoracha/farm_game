#ifdef _WIN32

#include "farm/ui/Win32FarmApp.h"

#include "farm/core/Game.h"
#include "farm/core/UnlockGraph.h"
#include "farm/persistence/SaveManager.h"
#include "farm/ui/TextureManager.h"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
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
    Fishing,
    Unlock,
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
constexpr int kTabFishing = 16;
constexpr int kTabUnlock = 17;
constexpr int kTabSave = 18;
constexpr int kOpenUtilityMenu = 19;
constexpr int kTick = 20;
constexpr int kPause = 21;
constexpr int kSpeed1 = 22;
constexpr int kSpeed2 = 23;
constexpr int kSpeed4 = 24;
constexpr int kUtilityClose = 30;
constexpr int kUtilityOpenSave = 31;
constexpr int kUtilityOpenUnlock = 32;

constexpr UINT_PTR kGameTimerId = 1;
constexpr UINT_PTR kTransitionTimerId = 2;
constexpr UINT_PTR kMessageTimerId = 3;
constexpr ULONGLONG kTransitionDurationMs = 320;
constexpr int kGameContentTop = 120;
constexpr int kTransitionDriftPixels = 12;
constexpr ULONGLONG kMessageVisibleMs = 3600;
constexpr ULONGLONG kMessageErrorVisibleMs = 5200;
constexpr ULONGLONG kMessageFadeMs = 700;

constexpr int kPlantWheat = 100;
constexpr int kPlantCorn = 101;
constexpr int kPlantCarrot = 102;
constexpr int kPlantTomato = 103;
constexpr int kWater = 104;
constexpr int kFertilize = 105;
constexpr int kHarvest = 106;
constexpr int kExpand = 107;
constexpr int kFarmModeField = 108;
constexpr int kFarmModeGreenhouse = 109;
constexpr int kBuildGreenhouse = 110;

constexpr int kFarmPlotLeft = 40;
constexpr int kFarmPlotTop = 165;
constexpr int kFarmPlotSize = 65;
constexpr int kFarmPlotStep = 80;
constexpr int kFarmPlotColumns = 6;
constexpr int kFarmPlotRows = 3;

constexpr int kBuildCowBarn = 200;
constexpr int kBuildSheepPen = 201;
constexpr int kBuyChicken = 202;
constexpr int kBuyCow = 203;
constexpr int kBuySheep = 204;
constexpr int kFeedAll = 205;
constexpr int kHarvestAll = 206;
constexpr int kFeedSelected = 207;
constexpr int kHarvestSelected = 208;
constexpr int kRanchPagePrevious = 209;
constexpr int kRanchPageNext = 210;
constexpr int kSelectRanchFacilityBase = 220;
constexpr int kRanchFacilitiesPerPage = 3;

constexpr int kMakeChickenFeed = 300;
constexpr int kMakeChickenFeed3 = 301;
constexpr int kMakeCowFeed = 302;
constexpr int kClaimFeed = 303;

constexpr int kCompleteOrder = 400;
constexpr int kAbandonOrder = 401;
constexpr int kOrderBase = 420;

constexpr int kBuyWheatSeed = 500;
constexpr int kBuyCornSeed = 501;
constexpr int kBuyCarrotSeed = 502;
constexpr int kBuyTomatoSeed = 503;
constexpr int kBuyFertilizer = 504;

constexpr int kSellWheat = 600;
constexpr int kSellCorn = 601;
constexpr int kSellCarrot = 602;
constexpr int kSellTomato = 603;
constexpr int kSellEgg = 604;
constexpr int kSellMilk = 605;
constexpr int kSellWool = 606;
constexpr int kWarehouseSellSelected = 607;
constexpr int kWarehouseToggleLock = 608;
constexpr int kWarehouseUpgrade = 609;
constexpr int kWarehousePagePrevious = 610;
constexpr int kWarehousePageNext = 611;
constexpr int kWarehouseItemBase = 620;
constexpr int kWarehouseItemsPerPage = 8;

constexpr int kSave = 700;
constexpr int kLoad = 701;
constexpr int kNewGame = 702;

constexpr int kFishingCast = 800;
constexpr int kFishingReel = 801;
constexpr int kFishingBuyBait = 802;
constexpr int kFishingUpgradeRod = 803;
constexpr int kFishingSellAll = 804;
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

const wchar_t* ItemCategoryName(ItemCategory category) {
    switch (category) {
        case ItemCategory::Seed:
            return L"种子";
        case ItemCategory::Crop:
            return L"作物";
        case ItemCategory::Feed:
            return L"饲料";
        case ItemCategory::AnimalProduct:
            return L"畜产品";
        case ItemCategory::Consumable:
            return L"消耗品";
    }
    return L"物品";
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

AnimalKind FacilityAnimalKind(RanchFacilityKind kind) {
    switch (kind) {
        case RanchFacilityKind::ChickenCoop:
            return AnimalKind::Chicken;
        case RanchFacilityKind::CowBarn:
            return AnimalKind::Cow;
        case RanchFacilityKind::SheepPen:
            return AnimalKind::Sheep;
        case RanchFacilityKind::PigPen:
            return AnimalKind::Pig;
    }
    return AnimalKind::Chicken;
}

const wchar_t* AnimalName(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken:
            return L"鸡";
        case AnimalKind::Cow:
            return L"牛";
        case AnimalKind::Sheep:
            return L"羊";
        case AnimalKind::Pig:
            return L"猪";
    }
    return L"动物";
}

ItemId AnimalFeed(AnimalKind kind) {
    return kind == AnimalKind::Chicken ? ItemId::ChickenFeed : ItemId::CowFeed;
}

ItemId AnimalProduct(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken:
            return ItemId::Egg;
        case AnimalKind::Cow:
            return ItemId::Milk;
        case AnimalKind::Sheep:
        case AnimalKind::Pig:
            return ItemId::Wool;
    }
    return ItemId::Egg;
}

int AnimalPurchaseCost(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken:
            return kChickenCost;
        case AnimalKind::Cow:
            return kCowCost;
        case AnimalKind::Sheep:
        case AnimalKind::Pig:
            return kSheepCost;
    }
    return kChickenCost;
}

UnlockId AnimalUnlock(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken:
            return UnlockId::Chicken;
        case AnimalKind::Cow:
            return UnlockId::Cow;
        case AnimalKind::Sheep:
        case AnimalKind::Pig:
            return UnlockId::Sheep;
    }
    return UnlockId::Chicken;
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

std::wstring FishDisplayName(int fish_id) {
    switch (fish_id) {
        case 1:
            return L"\u6625\u9ca4";
        case 2:
            return L"\u6eaa\u9cdf";
        case 3:
            return L"\u8349\u9c7c";
        case 4:
            return L"\u590f\u9c88";
        case 5:
            return L"\u65e5\u9c88";
        case 6:
            return L"\u9752\u9ccd";
        case 7:
            return L"\u91d1\u9c88";
        case 8:
            return L"\u51b0\u9ca4";
        case 9:
            return L"\u94f6\u68ad";
        case 10:
            return L"\u6708\u9ca4";
        case 11:
            return L"\u5f69\u8679\u9cdf";
        case 12:
            return L"\u9ed1\u9c7c";
        case 13:
            return L"\u91d1\u9f99\u9c7c";
        case 14:
            return L"\u9526\u9ca4\u738b";
        case 15:
            return L"\u9c9f\u9c7c";
    }
    return L"\u672a\u77e5\u9c7c";
}

std::wstring FishRarityName(FishRarity rarity) {
    switch (rarity) {
        case FishRarity::Common:
            return L"\u5e38\u89c1";
        case FishRarity::Rare:
            return L"\u7a00\u6709";
        case FishRarity::Legendary:
            return L"\u4f20\u8bf4";
    }
    return L"\u5e38\u89c1";
}

std::wstring FishingStateText(FishingState state) {
    switch (state) {
        case FishingState::Idle:
            return L"\u6e56\u9762\u5e73\u9759\uff0c\u53ef\u4ee5\u629b\u7aff";
        case FishingState::Waiting:
            return L"\u9c7c\u6f02\u5728\u8f7b\u8f7b\u6447\u52a8...";
        case FishingState::Biting:
            return L"\u9c7c\u4e0a\u94a9\u4e86\uff0c\u5bf9\u51c6\u7eff\u8272\u533a\u57df\u6536\u7aff";
        case FishingState::ReeledIn:
            return L"\u5df2\u6536\u7aff";
    }
    return L"";
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
    void SwitchScreen(Screen next_screen);
    void FinishTransition();
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
    void DrawFishing(HDC hdc);
    void DrawUnlock(HDC hdc);
    void DrawSave(HDC hdc);
    void DrawUtilityMenu(HDC hdc);
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
    int SelectedFacility(RanchFacilityKind kind) const;

    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr;
    TextureManager textures_;
    Game game_ = Game::NewGame();
    Screen screen_ = Screen::Menu;
    Screen previous_screen_ = Screen::Menu;
    HBITMAP previous_frame_ = nullptr;
    HBITMAP transition_frame_ = nullptr;
    int previous_frame_width_ = 0;
    int previous_frame_height_ = 0;
    int transition_frame_width_ = 0;
    int transition_frame_height_ = 0;
    ULONGLONG transition_started_at_ = 0;
    int transition_direction_ = 1;
    bool transition_active_ = false;
    std::wstring message_ = L"准备就绪。";
    ULONGLONG message_changed_at_ = 0;
    bool message_is_error_ = false;
    bool utility_menu_open_ = false;
    std::vector<UiButton> buttons_;
    int selected_plot_ = 0;
    int selected_greenhouse_plot_ = 0;
    bool farm_show_greenhouse_ = false;
    int selected_order_ = 0;
    int selected_ranch_facility_id_ = 1;
    int ranch_facility_page_ = 0;
    ItemId selected_warehouse_item_ = ItemId::WheatSeed;
    int warehouse_page_ = 0;
};

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
    textures_.Load(L"menu_background", L"assets/textures/ui/menu_background.png", L"");
    textures_.Load(L"farm_background", L"assets/textures/terrain/farm_background.png", L"");
    textures_.Load(L"ranch_background", L"assets/textures/terrain/ranch_background.png", L"");
    textures_.Load(L"workshop_background", L"assets/textures/terrain/workshop_background.png", L"");
    textures_.Load(L"orders_background", L"assets/textures/terrain/orders_background.png", L"");
    textures_.Load(L"warehouse_background", L"assets/textures/terrain/warehouse_background.png",
                   L"");
    textures_.Load(L"shop_background", L"assets/textures/terrain/shop_background.png", L"");
    textures_.Load(L"fishing_background", L"assets/textures/terrain/fishing_background.png", L"");
    textures_.Load(L"unlock_background", L"assets/textures/terrain/unlock_background.png", L"");
    textures_.Load(L"save_background", L"assets/textures/terrain/save_background.png", L"");
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
    textures_.Load(L"animal_chicken_scene",
                   L"assets/textures/animals/chicken/chicken_scene.png", L"");
    textures_.Load(L"animal_cow_scene", L"assets/textures/animals/cow/cow_scene.png", L"");
    textures_.Load(L"animal_sheep_scene", L"assets/textures/animals/sheep/sheep_scene.png", L"");
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
    SetTimer(hwnd_, kGameTimerId, 1000, nullptr);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

FarmWindow::~FarmWindow() {
    if (previous_frame_ != nullptr) {
        DeleteObject(previous_frame_);
        previous_frame_ = nullptr;
    }
    if (transition_frame_ != nullptr) {
        DeleteObject(transition_frame_);
        transition_frame_ = nullptr;
    }
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
        case WM_LBUTTONDOWN: {
            if (transition_active_) {
                return 0;
            }
            const int x = LOWORD(lparam);
            const int y = HIWORD(lparam);
            for (auto it = buttons_.rbegin(); it != buttons_.rend(); ++it) {
                if (x >= it->rect.left && x <= it->rect.right && y >= it->rect.top &&
                    y <= it->rect.bottom) {
                    OnButton(it->id);
                    InvalidateRect(hwnd_, nullptr, FALSE);
                    return 0;
                }
            }
            if (utility_menu_open_) {
                if (x < 120 || x > 1000 || y < 128 || y > 632) {
                    utility_menu_open_ = false;
                }
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
            if (screen_ == Screen::Farm && y >= kFarmPlotTop &&
                y < kFarmPlotTop + kFarmPlotRows * kFarmPlotStep && x >= kFarmPlotLeft &&
                x < kFarmPlotLeft + kFarmPlotColumns * kFarmPlotStep) {
                const int row = (y - kFarmPlotTop) / kFarmPlotStep;
                const int col = (x - kFarmPlotLeft) / kFarmPlotStep;
                const int local_x = (x - kFarmPlotLeft) % kFarmPlotStep;
                const int local_y = (y - kFarmPlotTop) % kFarmPlotStep;
                const int plot_index = row * kFarmPlotColumns + col;
                if (local_x < kFarmPlotSize && local_y < kFarmPlotSize) {
                    const int unlocked_plots =
                        farm_show_greenhouse_
                            ? static_cast<int>(game_.Planting().GreenhousePlots().size())
                            : static_cast<int>(game_.Planting().Plots().size());
                    if (plot_index < unlocked_plots) {
                        if (farm_show_greenhouse_) {
                            selected_greenhouse_plot_ = plot_index;
                        } else {
                            selected_plot_ = plot_index;
                        }
                    } else {
                        SetMessage(farm_show_greenhouse_
                                       ? L"\u8fd9\u683c\u6e29\u5ba4\u5730\u5757\u5c1a\u672a\u5efa\u9020\u3002"
                                       : L"\u8fd9\u5757\u571f\u5730\u5c1a\u672a\u6269\u5efa\u3002");
                    }
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
            }
            return 0;
        }
        case WM_TIMER: {
            const UINT_PTR timer_id = static_cast<UINT_PTR>(wparam);
            if (timer_id == kTransitionTimerId) {
                if (GetTickCount64() - transition_started_at_ >= kTransitionDurationMs) {
                    FinishTransition();
                }
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
            if (timer_id == kMessageTimerId) {
                const ULONGLONG visible_ms =
                    message_is_error_ ? kMessageErrorVisibleMs : kMessageVisibleMs;
                if (GetTickCount64() - message_changed_at_ >= visible_ms) {
                    KillTimer(hwnd_, kMessageTimerId);
                }
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
            if (timer_id == kGameTimerId && screen_ != Screen::Menu) {
                game_.AdvanceBySpeed();
                if (!game_.LastEventMessage().empty()) {
                    SetMessage(Utf8ToWide(game_.LastEventMessage()));
                    game_.ClearLastEventMessage();
                }
                game_.AutoSaveIfNeeded(Narrow(SavePath()));
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            return 0;
        }
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

    if (transition_active_ && previous_frame_ != nullptr &&
        previous_frame_width_ == width && previous_frame_height_ == height) {
        if (transition_frame_ == nullptr || transition_frame_width_ != width ||
            transition_frame_height_ != height) {
            if (transition_frame_ != nullptr) {
                DeleteObject(transition_frame_);
            }
            HDC render_dc = CreateCompatibleDC(target);
            transition_frame_ = CreateCompatibleBitmap(target, width, height);
            HGDIOBJ render_old = SelectObject(render_dc, transition_frame_);
            Paint(render_dc);
            SelectObject(render_dc, render_old);
            DeleteDC(render_dc);
            transition_frame_width_ = width;
            transition_frame_height_ = height;
        }

        const ULONGLONG elapsed = GetTickCount64() - transition_started_at_;
        const double linear_progress =
            std::min(1.0, static_cast<double>(elapsed) /
                              static_cast<double>(kTransitionDurationMs));
        const double eased_progress =
            linear_progress * linear_progress * linear_progress *
            (linear_progress * (linear_progress * 6.0 - 15.0) + 10.0);
        const int incoming_offset = static_cast<int>(
            (1.0 - eased_progress) * static_cast<double>(kTransitionDriftPixels));
        const int content_top =
            previous_screen_ == Screen::Menu || screen_ == Screen::Menu ? 0 : kGameContentTop;

        HDC previous_dc = CreateCompatibleDC(target);
        HGDIOBJ previous_old = SelectObject(previous_dc, previous_frame_);
        HDC incoming_dc = CreateCompatibleDC(target);
        HGDIOBJ incoming_old = SelectObject(incoming_dc, transition_frame_);
        BitBlt(target, 0, 0, width, height, incoming_dc, 0, 0, SRCCOPY);
        BitBlt(target, 0, content_top, width, height - content_top, previous_dc, 0, content_top,
               SRCCOPY);

        using AlphaBlendFunction = BOOL(WINAPI*)(HDC, int, int, int, int, HDC, int, int, int,
                                                 int, BLENDFUNCTION);
        static const AlphaBlendFunction alpha_blend = [] {
            HMODULE module = LoadLibraryW(L"msimg32.dll");
            return module == nullptr
                       ? nullptr
                       : reinterpret_cast<AlphaBlendFunction>(
                             GetProcAddress(module, "AlphaBlend"));
        }();
        const BYTE opacity =
            static_cast<BYTE>(std::clamp(eased_progress * 255.0, 0.0, 255.0));
        const int incoming_x =
            transition_direction_ > 0 ? incoming_offset : -incoming_offset;
        if (alpha_blend != nullptr && opacity > 0) {
            BLENDFUNCTION blend{};
            blend.BlendOp = AC_SRC_OVER;
            blend.SourceConstantAlpha = opacity;
            alpha_blend(target, incoming_x, content_top, width, height - content_top, incoming_dc,
                        0, content_top, width, height - content_top, blend);
        } else if (eased_progress >= 0.5) {
            BitBlt(target, 0, content_top, width, height - content_top, incoming_dc, 0,
                   content_top, SRCCOPY);
        }
        SelectObject(incoming_dc, incoming_old);
        DeleteDC(incoming_dc);
        SelectObject(previous_dc, previous_old);
        DeleteDC(previous_dc);
        return;
    }

    if (transition_active_) {
        FinishTransition();
    }

    HDC memory = CreateCompatibleDC(target);
    HBITMAP bitmap = CreateCompatibleBitmap(target, width, height);
    HGDIOBJ old = SelectObject(memory, bitmap);
    Paint(memory);
    BitBlt(target, 0, 0, width, height, memory, 0, 0, SRCCOPY);
    SelectObject(memory, old);
    DeleteDC(memory);

    if (previous_frame_ != nullptr) {
        DeleteObject(previous_frame_);
    }
    previous_frame_ = bitmap;
    previous_frame_width_ = width;
    previous_frame_height_ = height;
}

void FarmWindow::Paint(HDC hdc) {
    buttons_.clear();
    if (font_ != nullptr) {
        SelectObject(hdc, font_);
    }
    RECT area;
    GetClientRect(hwnd_, &area);
    if (screen_ == Screen::Menu) {
        DrawTextureOrFill(hdc, L"menu_background", area, RGB(128, 180, 104));
    } else if (screen_ == Screen::Farm) {
        DrawTextureOrFill(hdc, L"farm_background", area, RGB(128, 180, 104));
    } else if (screen_ == Screen::Ranch) {
        DrawTextureOrFill(hdc, L"ranch_background", area, RGB(128, 180, 104));
    } else if (screen_ == Screen::Workshop) {
        DrawTextureOrFill(hdc, L"workshop_background", area, RGB(128, 180, 104));
    } else if (screen_ == Screen::Orders) {
        DrawTextureOrFill(hdc, L"orders_background", area, RGB(128, 180, 104));
    } else if (screen_ == Screen::Warehouse) {
        DrawTextureOrFill(hdc, L"warehouse_background", area, RGB(128, 180, 104));
    } else if (screen_ == Screen::Shop) {
        DrawTextureOrFill(hdc, L"shop_background", area, RGB(128, 180, 104));
    } else if (screen_ == Screen::Fishing) {
        DrawTextureOrFill(hdc, L"fishing_background", area, RGB(78, 154, 177));
    } else if (screen_ == Screen::Unlock) {
        DrawTextureOrFill(hdc, L"unlock_background", area, RGB(128, 180, 104));
    } else if (screen_ == Screen::Save) {
        DrawTextureOrFill(hdc, L"save_background", area, RGB(128, 180, 104));
    } else {
        DrawTextureOrFill(hdc, L"grass", area, RGB(238, 226, 190));
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(58, 54, 39));
    if (screen_ == Screen::Menu) {
        DrawMenu(hdc);
    } else {
        DrawGame(hdc);
    }
}

void FarmWindow::SwitchScreen(Screen next_screen) {
    if (screen_ == next_screen) {
        utility_menu_open_ = false;
        return;
    }
    utility_menu_open_ = false;
    previous_screen_ = screen_;
    transition_direction_ =
        static_cast<int>(next_screen) >= static_cast<int>(screen_) ? 1 : -1;
    if (transition_frame_ != nullptr) {
        DeleteObject(transition_frame_);
        transition_frame_ = nullptr;
    }
    transition_frame_width_ = 0;
    transition_frame_height_ = 0;
    screen_ = next_screen;
    transition_started_at_ = GetTickCount64();
    transition_active_ = previous_frame_ != nullptr;
    if (transition_active_) {
        SetTimer(hwnd_, kTransitionTimerId, 16, nullptr);
    }
}

void FarmWindow::FinishTransition() {
    transition_active_ = false;
    KillTimer(hwnd_, kTransitionTimerId);
    if (transition_frame_ != nullptr) {
        if (previous_frame_ != nullptr) {
            DeleteObject(previous_frame_);
        }
        previous_frame_ = transition_frame_;
        previous_frame_width_ = transition_frame_width_;
        previous_frame_height_ = transition_frame_height_;
        transition_frame_ = nullptr;
        transition_frame_width_ = 0;
        transition_frame_height_ = 0;
    }
}

void FarmWindow::DrawMenu(HDC hdc) {
    HFONT title_font = CreateFontW(54, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                   DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    HGDIOBJ old_font = SelectObject(hdc, title_font);
    SetTextColor(hdc, RGB(42, 63, 35));
    Text(hdc, 67, 65, L"田园时光");
    SetTextColor(hdc, RGB(255, 248, 218));
    Text(hdc, 63, 61, L"田园时光");
    SelectObject(hdc, old_font);
    DeleteObject(title_font);

    HFONT subtitle_font =
        CreateFontW(22, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    old_font = SelectObject(hdc, subtitle_font);
    SetTextColor(hdc, RGB(45, 67, 38));
    Text(hdc, 67, 128, L"播种、经营、养殖，打造属于你的农场");
    SetTextColor(hdc, RGB(255, 248, 218));
    Text(hdc, 65, 126, L"播种、经营、养殖，打造属于你的农场");
    SelectObject(hdc, old_font);
    DeleteObject(subtitle_font);

    Button(hdc, kBtnNew, RECT{65, 190, 275, 238}, L"新游戏");
    Button(hdc, kBtnContinue, RECT{65, 252, 275, 300}, L"继续游戏");

    SetTextColor(hdc, RGB(255, 248, 218));
    Text(hdc, 65, 344, L"种植作物 · 加工饲料 · 经营牧场 · 完成订单");
    Text(hdc, 65, 382, message_);
    SetTextColor(hdc, RGB(58, 54, 39));
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
        case Screen::Fishing:
            DrawFishing(hdc);
            break;
        case Screen::Unlock:
            DrawUnlock(hdc);
            break;
        case Screen::Save:
            DrawSave(hdc);
            break;
        case Screen::Menu:
            break;
    }

    if (utility_menu_open_) {
        DrawUtilityMenu(hdc);
    }

    if (message_changed_at_ == 0) {
        return;
    }
    const ULONGLONG elapsed = GetTickCount64() - message_changed_at_;
    const ULONGLONG visible_ms =
        message_is_error_ ? kMessageErrorVisibleMs : kMessageVisibleMs;
    if (elapsed >= visible_ms) {
        return;
    }

    double opacity = 1.0;
    if (elapsed < 180) {
        const double progress = static_cast<double>(elapsed) / 180.0;
        opacity = progress * progress * (3.0 - 2.0 * progress);
    } else if (elapsed > visible_ms - kMessageFadeMs) {
        const double progress =
            static_cast<double>(visible_ms - elapsed) / static_cast<double>(kMessageFadeMs);
        opacity = progress * progress * (3.0 - 2.0 * progress);
    }

    SIZE text_size{};
    GetTextExtentPoint32W(hdc, message_.c_str(), static_cast<int>(message_.size()), &text_size);
    const int toast_width = std::clamp(static_cast<int>(text_size.cx) + 64, 230, 680);
    const int toast_height = 42;
    const int toast_x = 28;
    const int toast_y = 622 + static_cast<int>((1.0 - opacity) * 8.0);

    HDC toast_dc = CreateCompatibleDC(hdc);
    HBITMAP toast_bitmap = CreateCompatibleBitmap(hdc, toast_width, toast_height);
    HGDIOBJ old_bitmap = SelectObject(toast_dc, toast_bitmap);
    if (font_ != nullptr) {
        SelectObject(toast_dc, font_);
    }
    SetBkMode(toast_dc, TRANSPARENT);
    Fill(toast_dc, RECT{0, 0, toast_width, toast_height}, RGB(255, 246, 216));
    Fill(toast_dc, RECT{0, 0, 6, toast_height},
         message_is_error_ ? RGB(180, 72, 52) : RGB(83, 145, 49));
    HBRUSH toast_border = CreateSolidBrush(RGB(143, 96, 48));
    RECT toast_rect{0, 0, toast_width, toast_height};
    FrameRect(toast_dc, &toast_rect, toast_border);
    DeleteObject(toast_border);
    SetTextColor(toast_dc, RGB(66, 46, 27));
    TextOutW(toast_dc, 22, 10, message_.c_str(), static_cast<int>(message_.size()));

    using AlphaBlendFunction = BOOL(WINAPI*)(HDC, int, int, int, int, HDC, int, int, int, int,
                                             BLENDFUNCTION);
    static const AlphaBlendFunction alpha_blend = [] {
        HMODULE module = LoadLibraryW(L"msimg32.dll");
        return module == nullptr
                   ? nullptr
                   : reinterpret_cast<AlphaBlendFunction>(GetProcAddress(module, "AlphaBlend"));
    }();
    if (alpha_blend != nullptr) {
        BLENDFUNCTION blend{};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha =
            static_cast<BYTE>(std::clamp(opacity * 255.0, 0.0, 255.0));
        alpha_blend(hdc, toast_x, toast_y, toast_width, toast_height, toast_dc, 0, 0, toast_width,
                    toast_height, blend);
    } else if (opacity >= 0.5) {
        BitBlt(hdc, toast_x, toast_y, toast_width, toast_height, toast_dc, 0, 0, SRCCOPY);
    }
    SelectObject(toast_dc, old_bitmap);
    DeleteObject(toast_bitmap);
    DeleteDC(toast_dc);
}

void FarmWindow::DrawStatus(HDC hdc) {
    const TimeSnapshot time = game_.Time().Snapshot();
    const WeatherSnapshot weather = game_.Weather().Snapshot();
    const COLORREF parchment = RGB(255, 241, 205);
    const COLORREF parchment_light = RGB(255, 248, 224);
    const COLORREF wood = RGB(121, 74, 35);
    const COLORREF wood_light = RGB(185, 124, 61);
    const COLORREF text_dark = RGB(64, 43, 24);
    const COLORREF green = RGB(92, 148, 45);
    const COLORREF gold = RGB(226, 166, 44);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto StatusCard = [&](RECT rect) {
        Fill(hdc, rect, parchment);
        Frame(rect, wood);
        RECT inner = rect;
        InflateRect(&inner, -3, -3);
        Frame(inner, wood_light);
    };

    HFONT status_font =
        CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    HGDIOBJ old_font = SelectObject(hdc, status_font);
    SetTextColor(hdc, text_dark);

    RECT resource_card{18, 8, 190, 59};
    StatusCard(resource_card);
    DrawTextureOrFill(hdc, L"item_coin", RECT{31, 20, 58, 47}, gold);
    Text(hdc, 68, 23, std::to_wstring(game_.Player().Gold()) + L" 金币");

    RECT progress_card{200, 8, 470, 59};
    StatusCard(progress_card);
    Text(hdc, 215, 14, L"等级 " + std::to_wstring(game_.Player().Level()));
    const int experience = game_.Player().Experience();
    const int experience_target = std::max(1, game_.Player().ExpToNextLevel());
    Text(hdc, 370, 14,
         std::to_wstring(experience) + L"/" + std::to_wstring(experience_target));
    RECT exp_back{215, 38, 450, 49};
    Fill(hdc, exp_back, RGB(186, 157, 104));
    RECT exp_fill = exp_back;
    exp_fill.right =
        exp_back.left + (exp_back.right - exp_back.left) *
                            std::min(experience, experience_target) / experience_target;
    Fill(hdc, exp_fill, green);
    Frame(exp_back, wood);

    RECT time_card{480, 8, 705, 59};
    StatusCard(time_card);
    std::wstringstream time_text;
    time_text << L"第 " << time.day << L" 天   " << std::setw(2) << std::setfill(L'0')
              << time.hour << L":" << std::setw(2) << time.minute;
    Text(hdc, 500, 23, time_text.str());

    RECT weather_card{715, 8, 900, 59};
    StatusCard(weather_card);
    DrawTextureOrFill(hdc, WeatherTextureKey(weather.weather), RECT{728, 14, 770, 54},
                      RGB(198, 218, 128));
    Text(hdc, 782, 23, WeatherName(weather.weather));

    RECT speed_card{910, 8, 1085, 59};
    StatusCard(speed_card);
    Fill(hdc, RECT{925, 20, 951, 47}, time.paused ? RGB(177, 91, 61) : green);
    Frame(RECT{925, 20, 951, 47}, wood);
    Text(hdc, 962, 23,
         time.paused ? L"已暂停" : L"速度 " + std::wstring(SpeedName(time.speed)));

    SelectObject(hdc, old_font);
    DeleteObject(status_font);
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::DrawTabs(HDC hdc) {
    const COLORREF parchment = RGB(255, 241, 205);
    const COLORREF parchment_active = RGB(255, 249, 226);
    const COLORREF wood = RGB(121, 74, 35);
    const COLORREF wood_mid = RGB(173, 108, 50);
    const COLORREF text_dark = RGB(64, 43, 24);
    const COLORREF text_light = RGB(255, 247, 218);
    const COLORREF green = RGB(92, 148, 45);
    const COLORREF blue = RGB(70, 126, 162);
    const TimeSnapshot time = game_.Time().Snapshot();

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto DrawTab = [&](int id, Screen tab_screen, RECT rect, const std::wstring& label,
                       const std::wstring& icon_key) {
        const bool selected = screen_ == tab_screen;
        buttons_.push_back(UiButton{rect, id});
        Fill(hdc, rect, selected ? parchment_active : wood_mid);
        Frame(rect, selected ? RGB(214, 157, 43) : wood);
        if (selected) {
            Fill(hdc, RECT{rect.left + 2, rect.bottom - 5, rect.right - 2, rect.bottom - 2},
                 green);
        }
        DrawTextureOrFill(hdc, icon_key,
                          RECT{rect.left + 7, rect.top + 8, rect.left + 27, rect.top + 28},
                          selected ? RGB(224, 190, 107) : RGB(204, 154, 84));
        SetTextColor(hdc, selected ? text_dark : text_light);
        Text(hdc, rect.left + 31, rect.top + 9, label);
    };
    auto ControlButton = [&](int id, RECT rect, const std::wstring& label, bool selected,
                             COLORREF selected_color) {
        buttons_.push_back(UiButton{rect, id});
        Fill(hdc, rect, selected ? selected_color : parchment);
        Frame(rect, selected ? wood : RGB(157, 111, 61));
        SetTextColor(hdc, selected ? text_light : text_dark);
        Text(hdc, rect.left + 10, rect.top + 9, label);
    };

    HFONT navigation_font =
        CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    HGDIOBJ old_font = SelectObject(hdc, navigation_font);

    DrawTab(kTabFarm, Screen::Farm, RECT{18, 73, 87, 114}, L"\u519c\u7530", L"soil");
    DrawTab(kTabRanch, Screen::Ranch, RECT{92, 73, 161, 114}, L"\u7267\u573a",
            L"animal_chicken_idle");
    DrawTab(kTabWorkshop, Screen::Workshop, RECT{166, 73, 255, 114}, L"\u9972\u6599\u574a",
            L"building_feed_mill");
    DrawTab(kTabOrders, Screen::Orders, RECT{260, 73, 329, 114}, L"\u8ba2\u5355", L"order_card");
    DrawTab(kTabWarehouse, Screen::Warehouse, RECT{334, 73, 403, 114}, L"\u4ed3\u5e93",
            L"building_warehouse");
    DrawTab(kTabShop, Screen::Shop, RECT{408, 73, 477, 114}, L"\u5546\u5e97", L"building_shop");
    DrawTab(kTabFishing, Screen::Fishing, RECT{482, 73, 551, 114}, L"\u9493\u9c7c",
            L"weather_rainy");
    DrawTab(kTabUnlock, Screen::Unlock, RECT{556, 73, 625, 114}, L"\u89e3\u9501", L"seed_corn");
    DrawTab(kOpenUtilityMenu, Screen::Menu, RECT{630, 73, 699, 114}, L"\u83dc\u5355",
            L"inventory_slot");

    RECT control_panel{718, 69, 1098, 118};
    Fill(hdc, control_panel, RGB(241, 219, 174));
    Frame(control_panel, wood);
    ControlButton(kTick, RECT{728, 75, 798, 112}, L"+2分", false, green);
    ControlButton(kPause, RECT{804, 75, 874, 112}, time.paused ? L"继续" : L"暂停",
                  time.paused, RGB(177, 91, 61));
    ControlButton(kSpeed1, RECT{884, 75, 946, 112}, L"1x",
                  !time.paused && time.speed == GameSpeed::Normal, blue);
    ControlButton(kSpeed2, RECT{950, 75, 1012, 112}, L"2x",
                  !time.paused && time.speed == GameSpeed::Fast, blue);
    ControlButton(kSpeed4, RECT{1016, 75, 1088, 112}, L"4x",
                  !time.paused && time.speed == GameSpeed::VeryFast, blue);

    SelectObject(hdc, old_font);
    DeleteObject(navigation_font);
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::DrawFarm(HDC hdc) {
    const bool greenhouse_mode = farm_show_greenhouse_;
    const auto field_plots = game_.Planting().View(game_.Time().CurrentTick());
    const auto greenhouse_plots = game_.Planting().GreenhouseView(game_.Time().CurrentTick());
    const std::vector<PlotView>& plots = greenhouse_mode ? greenhouse_plots : field_plots;
    const int max_plots = greenhouse_mode ? kGreenhouseMaxPlots : kMaxPlotCount;
    const int selected_index = greenhouse_mode ? selected_greenhouse_plot_ : selected_plot_;

    const COLORREF text_dark = RGB(65, 44, 25);
    const COLORREF text_muted = RGB(116, 89, 58);
    const COLORREF wood_dark = RGB(121, 75, 36);
    const COLORREF panel_fill = RGB(255, 241, 205);
    const COLORREF disabled_fill = RGB(184, 171, 142);
    const COLORREF mode_green = RGB(92, 148, 45);
    const COLORREF mode_blue = RGB(70, 126, 162);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto ModeButton = [&](int id, RECT rect, const std::wstring& label, bool selected,
                          COLORREF color) {
        buttons_.push_back(UiButton{rect, id});
        Fill(hdc, rect, selected ? color : RGB(241, 219, 174));
        Frame(rect, selected ? wood_dark : RGB(157, 111, 61));
        SetTextColor(hdc, selected ? RGB(255, 247, 218) : text_dark);
        Text(hdc, rect.left + 12, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };

    DrawTextureOrFill(hdc, L"wood_card", RECT{25, 122, 525, 154}, RGB(250, 242, 214));
    std::wstringstream title;
    title << (greenhouse_mode ? L"\u6e29\u5ba4" : L"\u519c\u7530") << L"  "
          << plots.size() << L"/" << max_plots << L"    "
          << (greenhouse_mode ? L"\u65e0\u5b63\u8282\u9650\u5236" : L"\u70b9\u51fb\u5730\u5757\u540e\u5728\u53f3\u4fa7\u64cd\u4f5c");
    Text(hdc, 40, 130, title.str());
    ModeButton(kFarmModeField, RECT{350, 127, 432, 151}, L"\u9732\u5929", !greenhouse_mode,
               mode_green);
    ModeButton(kFarmModeGreenhouse, RECT{438, 127, 516, 151}, L"\u6e29\u5ba4", greenhouse_mode,
               mode_blue);

    for (int i = 0; i < max_plots; ++i) {
        const int row = i / kFarmPlotColumns;
        const int col = i % kFarmPlotColumns;
        RECT r{kFarmPlotLeft + col * kFarmPlotStep, kFarmPlotTop + row * kFarmPlotStep,
               kFarmPlotLeft + col * kFarmPlotStep + kFarmPlotSize,
               kFarmPlotTop + row * kFarmPlotStep + kFarmPlotSize};

        if (i >= static_cast<int>(plots.size())) {
            Fill(hdc, r, greenhouse_mode ? RGB(86, 132, 118) : RGB(104, 148, 78));
            SetTextColor(hdc, RGB(238, 232, 196));
            Text(hdc, r.left + 5, r.top + 4, std::to_wstring(i + 1));
            Text(hdc, r.left + 12, r.top + 28,
                 greenhouse_mode ? L"\u672a\u5efa" : L"\u9501\u5b9a");
            SetTextColor(hdc, text_dark);
            continue;
        }

        const PlotView& plot = plots[static_cast<std::size_t>(i)];
        const std::wstring ground =
            plot.water == PlotWaterState::Watered ? L"soil_wet" : L"soil";
        DrawTextureOrFill(hdc, ground, r,
                          greenhouse_mode ? RGB(118, 104, 68) : RGB(139, 94, 52));
        if (greenhouse_mode) {
            Frame(RECT{r.left + 2, r.top + 2, r.right - 2, r.bottom - 2}, RGB(95, 157, 132));
        }
        if (plot.state != PlotState::Idle) {
            RECT crop_rect{r.left + 9, r.top + 7, r.right - 9, r.bottom - 14};
            DrawTextureOrFill(hdc, CropTextureKey(plot.crop, CropStage(plot)), crop_rect,
                              plot.state == PlotState::Mature ? RGB(237, 190, 67)
                                                              : RGB(136, 170, 69));
        }

        const bool selected = i == selected_index;
        const bool mature = plot.state == PlotState::Mature;
        if (selected || mature) {
            HBRUSH border =
                CreateSolidBrush(selected ? RGB(255, 222, 72) : RGB(242, 176, 48));
            RECT border_rect = r;
            FrameRect(hdc, &border_rect, border);
            if (selected) {
                InflateRect(&border_rect, -2, -2);
                FrameRect(hdc, &border_rect, border);
            }
            DeleteObject(border);
        }

        RECT number_badge{r.left + 3, r.top + 3, r.left + 23, r.top + 21};
        Fill(hdc, number_badge, selected ? RGB(255, 224, 94) : RGB(250, 242, 214));
        Text(hdc, number_badge.left + 5, number_badge.top + 1, std::to_wstring(i + 1));

        if (plot.state == PlotState::Idle) {
            SetTextColor(hdc, RGB(255, 244, 207));
            Text(hdc, r.left + 17, r.top + 40, L"\u7a7a\u95f2");
            SetTextColor(hdc, text_dark);
        } else if (plot.state == PlotState::Mature) {
            RECT ready_badge{r.left + 5, r.bottom - 21, r.right - 5, r.bottom - 4};
            Fill(hdc, ready_badge, RGB(255, 224, 94));
            Text(hdc, ready_badge.left + 10, ready_badge.top, L"\u6210\u719f");
        } else if (plot.remaining_ticks > 0) {
            RECT time_badge{r.left + 5, r.bottom - 21, r.right - 5, r.bottom - 4};
            Fill(hdc, time_badge, RGB(250, 242, 214));
            Text(hdc, time_badge.left + 8, time_badge.top,
                 std::to_wstring(plot.remaining_ticks * kGameMinutesPerTick) + L"\u5206");
        }

        if (plot.water == PlotWaterState::Watered) {
            SetTextColor(hdc, RGB(215, 240, 255));
            Text(hdc, r.right - 20, r.top + 3, L"\u6c34");
        }
        if (plot.fertilized) {
            SetTextColor(hdc, RGB(255, 235, 153));
            Text(hdc, r.right - 20, r.top + 22, L"\u80a5");
        }
        SetTextColor(hdc, text_dark);
    }

    DrawTextureOrFill(hdc, L"wood_card", RECT{545, 122, 950, 540}, panel_fill);

    HFONT action_font =
        CreateFontW(17, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    auto FarmAction = [&](int id, RECT rect, const std::wstring& label,
                          const std::wstring& icon, COLORREF color, bool enabled) {
        if (enabled) {
            buttons_.push_back(UiButton{rect, id});
        }
        RECT shadow{rect.left + 2, rect.top + 3, rect.right + 2, rect.bottom + 3};
        Fill(hdc, shadow, RGB(117, 75, 39));
        Fill(hdc, rect, enabled ? color : disabled_fill);
        Frame(rect, enabled ? wood_dark : RGB(132, 123, 102));
        RECT icon_rect{rect.left + 7, rect.top + 7, rect.left + 33, rect.bottom - 7};
        DrawTextureOrFill(hdc, icon, icon_rect,
                          enabled ? RGB(255, 236, 184) : RGB(203, 194, 171));
        HGDIOBJ old_action_font = SelectObject(hdc, action_font);
        SetTextColor(hdc, enabled ? RGB(255, 250, 230) : RGB(111, 103, 86));
        Text(hdc, rect.left + 39, rect.top + 9, label);
        SetTextColor(hdc, text_dark);
        SelectObject(hdc, old_action_font);
    };

    HFONT heading_font =
        CreateFontW(21, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    HGDIOBJ old_font = SelectObject(hdc, heading_font);
    SetTextColor(hdc, text_dark);
    Text(hdc, 570, 150, greenhouse_mode ? L"\u6e29\u5ba4\u7ba1\u7406" : L"\u571f\u5730\u7ba1\u7406");
    SelectObject(hdc, old_font);
    DeleteObject(heading_font);

    const PlotView* selected = nullptr;
    if (selected_index >= 0 && selected_index < static_cast<int>(plots.size())) {
        selected = &plots[static_cast<std::size_t>(selected_index)];
    }
    RECT selected_badge{565, 180, 930, 213};
    Fill(hdc, selected_badge, RGB(255, 248, 224));
    Frame(selected_badge, RGB(197, 151, 84));
    std::wstringstream selected_info;
    if (selected == nullptr) {
        selected_info << (greenhouse_mode ? L"\u8bf7\u5148\u9009\u62e9\u5df2\u5efa\u9020\u6e29\u5ba4\u5730\u5757"
                                          : L"\u8bf7\u5148\u9009\u62e9\u5df2\u89e3\u9501\u571f\u5730");
    } else {
        selected_info << L"\u7b2c " << selected_index + 1 << L" \u5757   ";
        if (selected->state == PlotState::Idle) {
            selected_info << L"\u7a7a\u95f2\uff0c\u53ef\u64ad\u79cd";
        } else if (selected->state == PlotState::Mature) {
            selected_info << ItemName(selected->crop) << L"\u6210\u719f\uff0c\u53ef\u6536\u83b7";
        } else {
            selected_info << ItemName(selected->crop) << L"\u751f\u957f\u4e2d";
        }
    }
    Text(hdc, 579, 187, selected_info.str());

    const bool can_plant = selected != nullptr && selected->state == PlotState::Idle;
    const bool can_tend = selected != nullptr && selected->state == PlotState::Growing;
    const bool can_harvest = selected != nullptr && selected->state == PlotState::Mature;

    SetTextColor(hdc, text_muted);
    Text(hdc, 566, 222, greenhouse_mode ? L"\u6e29\u5ba4\u64ad\u79cd" : L"\u64ad\u79cd");
    FarmAction(kPlantWheat, RECT{565, 245, 650, 285}, L"\u5c0f\u9ea6", L"seed_wheat",
               RGB(207, 139, 54), can_plant);
    FarmAction(kPlantCorn, RECT{658, 245, 743, 285}, L"\u7389\u7c73", L"seed_corn",
               RGB(207, 139, 54), can_plant);
    FarmAction(kPlantCarrot, RECT{751, 245, 836, 285}, L"\u80e1\u841d\u535c", L"seed_carrot",
               RGB(207, 139, 54), can_plant);
    FarmAction(kPlantTomato, RECT{844, 245, 929, 285}, L"\u756a\u8304", L"seed_tomato",
               RGB(207, 139, 54), can_plant);

    SetTextColor(hdc, text_muted);
    Text(hdc, 566, 297, greenhouse_mode ? L"\u6e29\u5ba4\u517b\u62a4" : L"\u571f\u5730\u517b\u62a4");
    FarmAction(kWater, RECT{565, 320, 743, 362}, L"\u6d47\u6c34", L"weather_rainy",
               RGB(75, 145, 177), can_tend && selected->water == PlotWaterState::Dry);
    FarmAction(kFertilize, RECT{751, 320, 929, 362},
               greenhouse_mode ? L"\u6e29\u5ba4\u4e0d\u65bd\u80a5" : L"\u65bd\u80a5",
               L"item_fertilizer", RGB(195, 149, 48),
               !greenhouse_mode && can_tend && !selected->fertilized);
    FarmAction(kHarvest, RECT{565, 377, 929, 421}, L"\u6536\u5272\u5f53\u524d\u4f5c\u7269", L"item_wheat",
               RGB(78, 143, 45), can_harvest);

    HBRUSH divider = CreateSolidBrush(RGB(205, 163, 96));
    RECT divider_rect{565, 439, 929, 440};
    FillRect(hdc, &divider_rect, divider);
    DeleteObject(divider);

    SetTextColor(hdc, text_muted);
    if (greenhouse_mode) {
        const bool can_build = static_cast<int>(greenhouse_plots.size()) < kGreenhouseMaxPlots;
        Text(hdc, 566, 453,
             L"\u6e29\u5ba4\u5730\u5757    " + std::to_wstring(kGreenhouseBuildCost) +
                 L" \u91d1\u5e01 / \u5757");
        FarmAction(kBuildGreenhouse, RECT{700, 478, 929, 518},
                   can_build ? L"\u5efa\u9020\u4e00\u5757\u6e29\u5ba4" : L"\u6e29\u5ba4\u5df2\u6ee1",
                   L"soil_wet", RGB(64, 130, 119), can_build);
    } else {
        const bool can_expand = static_cast<int>(field_plots.size()) < kMaxPlotCount;
        const int extra = static_cast<int>(field_plots.size()) - kInitialPlotCount;
        Text(hdc, 566, 453,
             can_expand ? L"\u6269\u5efa\u571f\u5730    " +
                              std::to_wstring(kPlotExpansionBaseCost + extra * 20) +
                              L" \u91d1\u5e01"
                        : L"\u571f\u5730\u5df2\u6269\u5efa\u81f3\u4e0a\u9650");
        FarmAction(kExpand, RECT{740, 478, 929, 518},
                   can_expand ? L"\u8d2d\u4e70\u4e0b\u4e00\u5757" : L"\u5df2\u8fbe\u4e0a\u9650",
                   L"soil", RGB(164, 103, 48), can_expand);
    }
    DeleteObject(action_font);
    SetTextColor(hdc, text_dark);
}
void FarmWindow::DrawRanch(HDC hdc) {
    const COLORREF parchment = RGB(255, 240, 199);
    const COLORREF parchment_light = RGB(255, 248, 222);
    const COLORREF text_dark = RGB(66, 44, 25);
    const COLORREF text_muted = RGB(116, 86, 54);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF wood_mid = RGB(157, 98, 43);
    const COLORREF grass = RGB(132, 176, 61);
    const COLORREF grass_light = RGB(164, 197, 78);
    const COLORREF fence = RGB(156, 100, 49);
    const COLORREF action_green = RGB(77, 147, 39);
    const COLORREF disabled_fill = RGB(177, 163, 132);
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto RanchButton = [&](int id, RECT rect, const std::wstring& label, bool enabled = true,
                           COLORREF color = RGB(77, 147, 39)) {
        if (enabled) {
            buttons_.push_back(UiButton{rect, id});
        }
        RECT shadow{rect.left + 2, rect.top + 3, rect.right + 2, rect.bottom + 3};
        Fill(hdc, shadow, RGB(105, 68, 35));
        Fill(hdc, rect, enabled ? color : disabled_fill);
        Frame(rect, enabled ? wood_dark : RGB(117, 108, 88));
        SetTextColor(hdc, enabled ? RGB(255, 250, 226) : RGB(226, 217, 194));
        Text(hdc, rect.left + 12, rect.top + 8, label);
        SetTextColor(hdc, text_dark);
    };
    auto SceneAnimalKey = [&](AnimalKind kind) -> std::wstring {
        switch (kind) {
            case AnimalKind::Chicken:
                return L"animal_chicken_scene";
            case AnimalKind::Cow:
                return L"animal_cow_scene";
            case AnimalKind::Sheep:
                return L"animal_sheep_scene";
            case AnimalKind::Pig:
                return L"animal_pig_idle";
        }
        return L"animal_unknown";
    };
    auto DrawFence = [&](RECT rect) {
        Fill(hdc, RECT{rect.left, rect.top, rect.right, rect.top + 5}, fence);
        Fill(hdc, RECT{rect.left, rect.bottom - 5, rect.right, rect.bottom}, fence);
        for (int x = rect.left + 8; x < rect.right; x += 34) {
            Fill(hdc, RECT{x, rect.top - 4, x + 7, rect.top + 13}, RGB(128, 78, 38));
            Fill(hdc, RECT{x, rect.bottom - 13, x + 7, rect.bottom + 4}, RGB(128, 78, 38));
        }
    };
    auto DrawCounter = [&](int x, const std::wstring& icon, const std::wstring& label,
                           int count) {
        DrawTextureOrFill(hdc, icon, RECT{x, 136, x + 28, 164}, RGB(236, 192, 91));
        Text(hdc, x + 33, 139, label + L" " + std::to_wstring(count));
    };

    std::vector<RanchFacilityView> facilities = game_.Ranch().FacilityViews();
    if (facilities.empty()) {
        selected_ranch_facility_id_ = -1;
        ranch_facility_page_ = 0;
    } else {
        bool selected_exists = false;
        for (const RanchFacilityView& facility : facilities) {
            selected_exists = selected_exists || facility.id == selected_ranch_facility_id_;
        }
        if (!selected_exists) {
            selected_ranch_facility_id_ = facilities.front().id;
        }
    }

    const int page_count =
        std::max(1, (static_cast<int>(facilities.size()) + kRanchFacilitiesPerPage - 1) /
                        kRanchFacilitiesPerPage);
    ranch_facility_page_ = std::max(0, std::min(ranch_facility_page_, page_count - 1));

    Fill(hdc, RECT{35, 128, 200, 168}, parchment);
    Frame(RECT{35, 128, 200, 168}, wood_dark);
    HFONT title_font =
        CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    HGDIOBJ old_font = SelectObject(hdc, title_font);
    Text(hdc, 55, 137, L"牧场生活");
    SelectObject(hdc, old_font);
    DeleteObject(title_font);

    Fill(hdc, RECT{215, 128, 872, 168}, parchment);
    Frame(RECT{215, 128, 872, 168}, wood_dark);
    DrawCounter(230, L"item_feed", L"鸡料", game_.Player().ItemCount(ItemId::ChickenFeed));
    DrawCounter(350, L"item_feed", L"牛料", game_.Player().ItemCount(ItemId::CowFeed));
    DrawCounter(470, L"item_egg", L"鸡蛋", game_.Player().ItemCount(ItemId::Egg));
    DrawCounter(590, L"item_milk", L"牛奶", game_.Player().ItemCount(ItemId::Milk));
    DrawCounter(710, L"item_wool", L"羊毛", game_.Player().ItemCount(ItemId::Wool));

    RanchButton(kRanchPagePrevious, RECT{885, 128, 935, 168}, L"<",
                ranch_facility_page_ > 0, RGB(170, 112, 53));
    Fill(hdc, RECT{942, 128, 997, 168}, parchment_light);
    Frame(RECT{942, 128, 997, 168}, wood_dark);
    Text(hdc, 951, 138,
         std::to_wstring(ranch_facility_page_ + 1) + L"/" + std::to_wstring(page_count));
    RanchButton(kRanchPageNext, RECT{1004, 128, 1055, 168}, L">",
                ranch_facility_page_ + 1 < page_count, RGB(170, 112, 53));

    const int first_facility = ranch_facility_page_ * kRanchFacilitiesPerPage;
    for (int slot = 0; slot < kRanchFacilitiesPerPage; ++slot) {
        const int facility_index = first_facility + slot;
        const int x = 35 + slot * 350;
        RECT card{x, 180, x + 330, 326};
        Fill(hdc, card, grass);
        Fill(hdc, RECT{x + 4, 184, x + 326, 215}, wood_mid);
        Fill(hdc, RECT{x + 4, 278, x + 326, 322}, RGB(191, 151, 82));
        DrawFence(RECT{x + 8, 218, x + 322, 313});

        if (facility_index >= static_cast<int>(facilities.size())) {
            Frame(card, RGB(143, 98, 53));
            SetTextColor(hdc, RGB(255, 246, 214));
            Text(hdc, x + 18, 188, L"待建设的围栏");
            SetTextColor(hdc, text_muted);
            Text(hdc, x + 103, 246, L"空设施位");
            Text(hdc, x + 75, 286, L"建造牛棚或羊圈后开放");
            SetTextColor(hdc, text_dark);
            continue;
        }

        const RanchFacilityView& facility =
            facilities[static_cast<std::size_t>(facility_index)];
        const bool is_selected = facility.id == selected_ranch_facility_id_;
        buttons_.push_back(UiButton{card, kSelectRanchFacilityBase + slot});
        Frame(card, is_selected ? RGB(255, 209, 69) : wood_dark);
        if (is_selected) {
            RECT inner = card;
            InflateRect(&inner, -3, -3);
            Frame(inner, RGB(255, 234, 121));
        }

        SetTextColor(hdc, RGB(255, 247, 218));
        std::wstringstream facility_title;
        facility_title << FacilityName(facility.kind) << L" #" << facility.id << L"   "
                       << facility.animal_count << L"/" << facility.capacity;
        Text(hdc, x + 14, 188, facility_title.str());
        DrawTextureOrFill(hdc, FacilityTextureKey(facility.kind),
                          RECT{x + 14, 224, x + 82, 286}, RGB(175, 105, 52));

        const AnimalKind kind = FacilityAnimalKind(facility.kind);
        const auto card_animals =
            game_.Ranch().AnimalViews(facility.id, game_.Time().CurrentTick());
        const int visible_count = std::min(3, static_cast<int>(card_animals.size()));
        for (int i = 0; i < visible_count; ++i) {
            const int animal_x = x + 95 + i * 68;
            const int animal_width = kind == AnimalKind::Chicken ? 48 : 62;
            DrawTextureOrFill(hdc, SceneAnimalKey(kind),
                              RECT{animal_x, 224, animal_x + animal_width, 283},
                              RGB(240, 222, 174));
        }
        SetTextColor(hdc, text_dark);
        std::wstringstream state;
        state << L"待喂 " << facility.idle_count << L"   生产 " << facility.producing_count
              << L"   可收 " << facility.ready_count;
        Text(hdc, x + 18, 293, state.str());
    }

    const RanchFacilityView* selected = nullptr;
    for (const RanchFacilityView& facility : facilities) {
        if (facility.id == selected_ranch_facility_id_) {
            selected = &facility;
            break;
        }
    }

    RECT scene{35, 342, 700, 610};
    Fill(hdc, scene, grass_light);
    Fill(hdc, RECT{39, 346, 696, 383}, wood_mid);
    Fill(hdc, RECT{39, 540, 696, 606}, RGB(191, 151, 82));
    Frame(scene, wood_dark);
    DrawFence(RECT{48, 395, 686, 585});

    if (selected == nullptr) {
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, 55, 352, L"当前设施");
        SetTextColor(hdc, text_muted);
        Text(hdc, 275, 470, L"暂无牧场设施");
    } else {
        const AnimalKind animal_kind = FacilityAnimalKind(selected->kind);
        const auto animals =
            game_.Ranch().AnimalViews(selected->id, game_.Time().CurrentTick());
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, 55, 352,
             std::wstring(FacilityName(selected->kind)) + L" #" +
                 std::to_wstring(selected->id) + L"    " +
                 std::to_wstring(selected->animal_count) + L"/" +
                 std::to_wstring(selected->capacity));
        DrawTextureOrFill(hdc, FacilityTextureKey(selected->kind), RECT{55, 402, 170, 510},
                          RGB(179, 111, 57));

        const int slot_width = 150;
        for (int slot = 0; slot < selected->capacity; ++slot) {
            const int x = 190 + slot * slot_width;
            if (slot >= static_cast<int>(animals.size())) {
                Frame(RECT{x + 8, 420, x + 130, 548}, RGB(174, 125, 69));
                SetTextColor(hdc, RGB(116, 86, 54));
                Text(hdc, x + 44, 472, L"空位");
                continue;
            }

            const AnimalView& animal = animals[static_cast<std::size_t>(slot)];
            const int sprite_width = animal.kind == AnimalKind::Chicken ? 86 : 122;
            DrawTextureOrFill(hdc, SceneAnimalKey(animal.kind),
                              RECT{x + 18, 405, x + 18 + sprite_width, 510},
                              RGB(244, 224, 171));

            RECT badge{x + 13, 515, x + 132, 544};
            COLORREF badge_color = RGB(183, 125, 51);
            std::wstring badge_text = L"待喂";
            if (animal.state == AnimalState::Ready) {
                badge_color = RGB(80, 150, 43);
                badge_text = L"可收获";
            } else if (animal.state == AnimalState::Producing) {
                badge_color = RGB(70, 139, 178);
                badge_text = L"生产中 " +
                             std::to_wstring(animal.remaining_ticks * kGameMinutesPerTick) +
                             L"分";
            }
            Fill(hdc, badge, badge_color);
            Frame(badge, wood_dark);
            SetTextColor(hdc, RGB(255, 249, 224));
            Text(hdc, badge.left + 11, badge.top + 4, badge_text);
            if (animal.state == AnimalState::Ready) {
                DrawTextureOrFill(hdc, ItemTextureKey(AnimalProduct(animal.kind)),
                                  RECT{x + 101, 480, x + 129, 508}, RGB(240, 195, 79));
            }
        }

        SetTextColor(hdc, text_dark);
        Text(hdc, 55, 566,
             std::wstring(AnimalName(animal_kind)) + L"使用" +
                 ItemName(AnimalFeed(animal_kind)) + L"，产出" +
                 ItemName(AnimalProduct(animal_kind)));
    }

    RECT controls{715, 342, 1075, 610};
    Fill(hdc, controls, parchment);
    Frame(controls, wood_dark);
    Fill(hdc, RECT{719, 346, 1071, 383}, wood_mid);
    SetTextColor(hdc, RGB(255, 246, 214));
    Text(hdc, 735, 352, L"当前设施经营");
    SetTextColor(hdc, text_dark);

    bool any_idle = false;
    bool any_ready = false;
    for (const RanchFacilityView& facility : facilities) {
        any_idle = any_idle || facility.idle_count > 0;
        any_ready = any_ready || facility.ready_count > 0;
    }

    if (selected != nullptr) {
        const AnimalKind animal_kind = FacilityAnimalKind(selected->kind);
        const int animal_cost = AnimalPurchaseCost(animal_kind);
        const bool animal_unlocked = game_.Player().IsUnlocked(AnimalUnlock(animal_kind));
        const bool has_capacity = selected->animal_count < selected->capacity;
        const bool can_buy =
            animal_unlocked && has_capacity && game_.Player().Gold() >= animal_cost;
        std::wstring buy_label = L"购买" + std::wstring(AnimalName(animal_kind)) + L"  " +
                                 std::to_wstring(animal_cost) + L"金币";
        if (!animal_unlocked) {
            buy_label = std::wstring(AnimalName(animal_kind)) + L"未解锁";
        } else if (!has_capacity) {
            buy_label = std::wstring(FacilityName(selected->kind)) + L"已满";
        }
        const int buy_button = animal_kind == AnimalKind::Chicken
                                   ? kBuyChicken
                                   : animal_kind == AnimalKind::Cow ? kBuyCow : kBuySheep;
        RanchButton(buy_button, RECT{735, 400, 1055, 440}, buy_label, can_buy,
                    RGB(174, 108, 45));
        const int feed_count = game_.Player().ItemCount(AnimalFeed(animal_kind));
        RanchButton(kFeedSelected, RECT{735, 450, 890, 490}, L"喂食当前设施",
                    selected->idle_count > 0 && feed_count > 0, RGB(78, 145, 42));
        RanchButton(kHarvestSelected, RECT{900, 450, 1055, 490}, L"收获当前设施",
                    selected->ready_count > 0, RGB(78, 145, 42));
    }

    RanchButton(kFeedAll, RECT{735, 510, 890, 548}, L"全牧场喂食", any_idle,
                RGB(151, 105, 54));
    RanchButton(kHarvestAll, RECT{900, 510, 1055, 548}, L"全牧场收获", any_ready,
                RGB(151, 105, 54));

    const bool cow_unlocked =
        game_.Player().IsUnlocked(UnlockId::CowBarn) && game_.Player().Gold() >= 120;
    const bool sheep_unlocked =
        game_.Player().IsUnlocked(UnlockId::SheepPen) && game_.Player().Gold() >= 160;
    RanchButton(kBuildCowBarn, RECT{735, 560, 890, 596},
                game_.Player().IsUnlocked(UnlockId::CowBarn) ? L"建牛棚 120"
                                                             : L"牛棚未解锁",
                cow_unlocked, RGB(170, 112, 53));
    RanchButton(kBuildSheepPen, RECT{900, 560, 1055, 596},
                game_.Player().IsUnlocked(UnlockId::SheepPen) ? L"建羊圈 160"
                                                              : L"羊圈未解锁",
                sheep_unlocked, RGB(170, 112, 53));
    SetTextColor(hdc, text_dark);
}

void FarmWindow::DrawWorkshop(HDC hdc) {
    const WorkshopView view = game_.Workshop().View();
    const auto& queue = game_.Workshop().Queue();
    const COLORREF panel_fill = RGB(255, 239, 198);
    const COLORREF slot_fill = RGB(250, 225, 174);
    const COLORREF text_dark = RGB(63, 43, 24);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF wood_mid = RGB(151, 94, 43);
    const COLORREF action_green = RGB(86, 151, 34);
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto WorkshopButton = [&](int id, RECT rect, const std::wstring& label) {
        buttons_.push_back(UiButton{rect, id});
        Fill(hdc, rect, action_green);
        Frame(rect, RGB(45, 91, 24));
        SetTextColor(hdc, RGB(255, 250, 224));
        Text(hdc, rect.left + 12, rect.top + 8, label);
        SetTextColor(hdc, text_dark);
    };
    auto SectionHeader = [&](RECT rect, const std::wstring& label) {
        Fill(hdc, rect, wood_mid);
        Frame(rect, wood_dark);
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, rect.left + 18, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };

    RECT title_bar{35, 130, 285, 168};
    Fill(hdc, title_bar, RGB(255, 239, 198));
    Frame(title_bar, wood_dark);
    Text(hdc, 55, 140, L"饲料坊");

    std::wstringstream materials;
    materials << L"小麦 " << game_.Player().ItemCount(ItemId::Wheat) << L"    玉米 "
              << game_.Player().ItemCount(ItemId::Corn) << L"    胡萝卜 "
              << game_.Player().ItemCount(ItemId::Carrot);
    RECT material_bar{300, 130, 720, 168};
    Fill(hdc, material_bar, RGB(255, 239, 198));
    Frame(material_bar, wood_dark);
    Text(hdc, 320, 140, materials.str());

    RECT workbench{45, 315, 1075, 610};
    Fill(hdc, workbench, panel_fill);
    Frame(workbench, wood_dark);
    RECT inner_frame{49, 319, 1071, 606};
    Frame(inner_frame, RGB(194, 132, 67));

    Frame(RECT{382, 328, 383, 593}, RGB(194, 132, 67));
    Frame(RECT{728, 328, 729, 593}, RGB(194, 132, 67));

    SectionHeader(RECT{65, 330, 360, 365}, L"配方");
    SectionHeader(RECT{402, 330, 708, 365},
                  L"加工队列  " + std::to_wstring(view.queue_count) + L"/" +
                      std::to_wstring(view.queue_capacity));
    SectionHeader(RECT{748, 330, 1055, 365},
                  L"成品货架  " + std::to_wstring(view.shelf_count) + L"/" +
                      std::to_wstring(view.shelf_capacity));

    RECT chicken_recipe{65, 380, 360, 462};
    Fill(hdc, chicken_recipe, slot_fill);
    Frame(chicken_recipe, RGB(180, 126, 69));
    DrawTextureOrFill(hdc, L"item_wheat", RECT{78, 392, 118, 432}, RGB(226, 196, 126));
    Text(hdc, 83, 435, L"x2");
    Text(hdc, 127, 405, L"→");
    DrawTextureOrFill(hdc, L"item_feed", RECT{150, 392, 190, 432}, RGB(226, 196, 126));
    Text(hdc, 195, 394, L"鸡饲料");
    Text(hdc, 195, 418, L"20 分钟");
    WorkshopButton(kMakeChickenFeed, RECT{246, 385, 296, 423}, L"x1");
    WorkshopButton(kMakeChickenFeed3, RECT{302, 385, 352, 423}, L"x3");

    RECT cow_recipe{65, 478, 360, 575};
    Fill(hdc, cow_recipe, slot_fill);
    Frame(cow_recipe, RGB(180, 126, 69));
    DrawTextureOrFill(hdc, L"item_corn", RECT{76, 490, 112, 526}, RGB(226, 196, 126));
    Text(hdc, 80, 530, L"x2");
    Text(hdc, 118, 501, L"+");
    DrawTextureOrFill(hdc, L"item_carrot", RECT{137, 490, 173, 526}, RGB(226, 196, 126));
    Text(hdc, 141, 530, L"x1");
    Text(hdc, 179, 501, L"→");
    DrawTextureOrFill(hdc, L"item_feed", RECT{198, 490, 234, 526}, RGB(226, 196, 126));
    Text(hdc, 240, 490, L"牛饲料");
    Text(hdc, 240, 514, L"32 分钟");
    WorkshopButton(kMakeCowFeed, RECT{277, 532, 347, 568}, L"制作");

    for (int i = 0; i < kFeedMillQueueCapacity; ++i) {
        const int x = 407 + i * 100;
        RECT slot{x, 392, x + 86, 500};
        Fill(hdc, slot, i < static_cast<int>(queue.size()) ? slot_fill : RGB(239, 217, 177));
        Frame(slot, RGB(167, 116, 66));
        std::wstring slot_number = std::to_wstring(i + 1);
        Text(hdc, x + 38, 374, slot_number);
        if (i >= static_cast<int>(queue.size())) {
            SetTextColor(hdc, RGB(117, 91, 62));
            Text(hdc, x + 27, 435, L"空闲");
            SetTextColor(hdc, text_dark);
            continue;
        }

        const ProductionJob& job = queue[static_cast<std::size_t>(i)];
        DrawTextureOrFill(hdc, L"item_feed", RECT{x + 18, 405, x + 68, 455},
                          RGB(226, 196, 126));
        Text(hdc, x + 16, 462, job.recipe == RecipeId::ChickenFeed ? L"鸡饲料" : L"牛饲料");
    }

    if (!queue.empty()) {
        const ProductionJob& active = queue.front();
        const int total_ticks =
            active.recipe == RecipeId::ChickenFeed ? kChickenFeedTicks : kCowFeedTicks;
        const int completed = std::max(0, total_ticks - active.remaining_ticks);
        RECT progress_back{407, 520, 693, 536};
        Fill(hdc, progress_back, RGB(117, 80, 45));
        RECT progress_fill = progress_back;
        progress_fill.right =
            progress_back.left + (progress_back.right - progress_back.left) * completed /
                                     std::max(1, total_ticks);
        Fill(hdc, progress_fill, RGB(62, 166, 218));
        Frame(progress_back, wood_dark);
        std::wstringstream remaining;
        remaining << L"当前任务剩余 " << active.remaining_ticks * kGameMinutesPerTick << L" 分钟";
        Text(hdc, 407, 548, remaining.str());
    } else {
        SetTextColor(hdc, RGB(117, 91, 62));
        Text(hdc, 407, 535, L"队列空闲，可选择左侧配方开始加工");
        SetTextColor(hdc, text_dark);
    }

    std::vector<ItemId> shelf_items;
    shelf_items.insert(shelf_items.end(), static_cast<std::size_t>(view.chicken_feed_shelf),
                       ItemId::ChickenFeed);
    shelf_items.insert(shelf_items.end(), static_cast<std::size_t>(view.cow_feed_shelf),
                       ItemId::CowFeed);
    for (int i = 0; i < kFeedMillShelfCapacity; ++i) {
        const int x = 755 + i * 72;
        RECT slot{x, 392, x + 62, 480};
        Fill(hdc, slot, RGB(151, 99, 48));
        Frame(slot, wood_dark);
        if (i < static_cast<int>(shelf_items.size())) {
            DrawTextureOrFill(hdc, L"item_feed", RECT{x + 9, 402, x + 53, 446},
                              RGB(226, 196, 126));
            Text(hdc, x + 10, 451,
                 shelf_items[static_cast<std::size_t>(i)] == ItemId::ChickenFeed ? L"鸡料"
                                                                                 : L"牛料");
        } else {
            SetTextColor(hdc, RGB(224, 188, 138));
            Text(hdc, x + 23, 425, L"空");
            SetTextColor(hdc, text_dark);
        }
    }

    WorkshopButton(kClaimFeed, RECT{815, 505, 995, 548}, L"领取 1 个");
    if (view.shelf_count >= view.shelf_capacity) {
        SetTextColor(hdc, RGB(157, 63, 38));
        Text(hdc, 790, 565, L"货架已满，生产暂停");
    } else if (view.shelf_count == 0) {
        SetTextColor(hdc, RGB(117, 91, 62));
        Text(hdc, 815, 565, L"暂无可领取成品");
    } else {
        Text(hdc, 815, 565, L"领取后存入仓库");
    }
    SetTextColor(hdc, text_dark);
}

void FarmWindow::DrawOrders(HDC hdc) {
    const auto& orders = game_.Orders().Orders();
    const COLORREF panel_fill = RGB(255, 239, 198);
    const COLORREF card_fill = RGB(250, 225, 174);
    const COLORREF muted_fill = RGB(218, 207, 177);
    const COLORREF text_dark = RGB(63, 43, 24);
    const COLORREF text_muted = RGB(117, 91, 62);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF wood_mid = RGB(151, 94, 43);
    const COLORREF action_green = RGB(86, 151, 34);
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto SectionHeader = [&](RECT rect, const std::wstring& label) {
        Fill(hdc, rect, wood_mid);
        Frame(rect, wood_dark);
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, rect.left + 16, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };
    auto OrderButton = [&](int id, RECT rect, const std::wstring& label, bool enabled,
                           bool primary) {
        if (enabled) {
            buttons_.push_back(UiButton{rect, id});
        }
        Fill(hdc, rect,
             enabled ? (primary ? action_green : RGB(176, 112, 50)) : RGB(174, 158, 124));
        Frame(rect, enabled ? (primary ? RGB(45, 91, 24) : RGB(123, 72, 33))
                            : RGB(132, 119, 91));
        SetTextColor(hdc, enabled ? RGB(255, 250, 224) : RGB(235, 228, 207));
        Text(hdc, rect.left + 16, rect.top + 8, label);
        SetTextColor(hdc, text_dark);
    };

    if (orders.empty()) {
        selected_order_ = 0;
    } else {
        selected_order_ =
            std::max(0, std::min(selected_order_, static_cast<int>(orders.size()) - 1));
    }

    int available_count = 0;
    int deliverable_count = 0;
    for (int i = 0; i < static_cast<int>(orders.size()); ++i) {
        const OrderData& order = orders[static_cast<std::size_t>(i)];
        if (order.state == OrderState::Available && !order.locked) {
            ++available_count;
        }
        if (game_.Orders().CanDeliver(game_.Player(), i) &&
            !game_.Player().IsItemLocked(order.requirements[0].item)) {
            ++deliverable_count;
        }
    }

    RECT title_bar{35, 130, 235, 168};
    Fill(hdc, title_bar, panel_fill);
    Frame(title_bar, wood_dark);
    Text(hdc, 55, 140, L"乡村订单站");

    std::wstringstream overview;
    overview << L"可接订单 " << available_count << L"    可立即交付 " << deliverable_count
             << L"    完成交付后 8 分钟刷新，放弃后 16 分钟刷新";
    RECT overview_bar{250, 130, 1060, 168};
    Fill(hdc, overview_bar, panel_fill);
    Frame(overview_bar, wood_dark);
    Text(hdc, 270, 140, overview.str());

    RECT order_desk{45, 315, 1075, 610};
    Fill(hdc, order_desk, panel_fill);
    Frame(order_desk, wood_dark);
    RECT inner_frame{49, 319, 1071, 606};
    Frame(inner_frame, RGB(194, 132, 67));
    Frame(RECT{700, 328, 701, 593}, RGB(194, 132, 67));

    SectionHeader(RECT{65, 330, 680, 365}, L"委托板  4 个订单槽位");
    SectionHeader(RECT{720, 330, 1055, 365}, L"订单详情");

    for (int i = 0; i < kOrderSlotCount; ++i) {
        const int row = i / 2;
        const int col = i % 2;
        const int x = 65 + col * 310;
        const int y = 378 + row * 105;
        RECT card{x, y, x + 290, y + 92};

        if (i >= static_cast<int>(orders.size())) {
            Fill(hdc, card, muted_fill);
            SetTextColor(hdc, text_muted);
            Text(hdc, x + 105, y + 34, L"空订单位");
            SetTextColor(hdc, text_dark);
            continue;
        }

        const OrderData& order = orders[static_cast<std::size_t>(i)];
        const bool selected = selected_order_ == i;
        const bool locked = order.locked || order.state == OrderState::Locked;
        const bool cooling = order.state == OrderState::CoolingDown;
        const int owned = game_.Player().ItemCount(order.requirements[0].item);
        const bool deliverable = game_.Orders().CanDeliver(game_.Player(), i) &&
                                 !game_.Player().IsItemLocked(order.requirements[0].item);

        buttons_.push_back(UiButton{card, kOrderBase + i});
        Fill(hdc, card, cooling || locked ? muted_fill : card_fill);
        Frame(card, selected ? RGB(238, 169, 45) : RGB(180, 126, 69));
        if (selected) {
            RECT selected_frame = card;
            InflateRect(&selected_frame, -3, -3);
            Frame(selected_frame, RGB(238, 169, 45));
        }
        if (deliverable) {
            Fill(hdc, RECT{x + 1, y + 1, x + 6, y + 91}, action_green);
        }

        DrawTextureOrFill(hdc, ItemTextureKey(order.requirements[0].item), RECT{x + 12, y + 16, x + 68, y + 72},
                          RGB(226, 196, 126));

        std::wstringstream requirement;
        requirement << ItemName(order.requirements[0].item) << L"  x" << order.requirements[0].quantity;
        Text(hdc, x + 80, y + 10, requirement.str());

        if (cooling) {
            const int ticks_left =
                std::max(0, order.cooldown_until_tick - game_.Time().CurrentTick());
            SetTextColor(hdc, text_muted);
            Text(hdc, x + 80, y + 39,
                 L"刷新倒计时  " +
                     std::to_wstring(ticks_left * kGameMinutesPerTick) + L" 分钟");
            SetTextColor(hdc, text_dark);
        } else if (locked) {
            SetTextColor(hdc, text_muted);
            Text(hdc, x + 80, y + 39, L"订单已锁定");
            SetTextColor(hdc, text_dark);
        } else {
            std::wstringstream stock;
            stock << L"库存 " << owned << L"/" << order.requirements[0].quantity;
            Text(hdc, x + 80, y + 37, stock.str());

            RECT progress_back{x + 158, y + 41, x + 272, y + 54};
            Fill(hdc, progress_back, RGB(174, 143, 94));
            RECT progress_fill = progress_back;
            progress_fill.right =
                progress_back.left +
                (progress_back.right - progress_back.left) *
                    std::min(owned, order.requirements[0].quantity) / std::max(1, order.requirements[0].quantity);
            Fill(hdc, progress_fill,
                 owned >= order.requirements[0].quantity ? action_green : RGB(219, 157, 54));
        }

        std::wstringstream reward;
        reward << L"金币 " << order.reward_gold << L"    经验 +" << order.reward_exp;
        SetTextColor(hdc, cooling || locked ? text_muted : RGB(128, 75, 25));
        Text(hdc, x + 80, y + 65, reward.str());
        SetTextColor(hdc, text_dark);
    }

    if (!orders.empty()) {
        const OrderData& selected =
            orders[static_cast<std::size_t>(selected_order_)];
        const bool locked =
            selected.locked || selected.state == OrderState::Locked;
        const bool cooling = selected.state == OrderState::CoolingDown;
        const int owned = game_.Player().ItemCount(selected.requirements[0].item);
        const bool protected_item = game_.Player().IsItemLocked(selected.requirements[0].item);
        const bool can_deliver =
            game_.Orders().CanDeliver(game_.Player(), selected_order_) && !protected_item;

        DrawTextureOrFill(hdc, ItemTextureKey(selected.requirements[0].item), RECT{735, 385, 805, 455},
                          RGB(226, 196, 126));
        std::wstringstream selected_title;
        selected_title << L"订单 #" << selected.id << L"  " << ItemName(selected.requirements[0].item);
        Text(hdc, 825, 385, selected_title.str());

        std::wstringstream selected_stock;
        selected_stock << L"需要 " << selected.requirements[0].quantity << L"    当前库存 " << owned;
        Text(hdc, 825, 418, selected_stock.str());

        std::wstringstream selected_reward;
        selected_reward << L"奖励  " << selected.reward_gold << L" 金币    +"
                        << selected.reward_exp << L" 经验";
        SetTextColor(hdc, RGB(128, 75, 25));
        Text(hdc, 735, 470, selected_reward.str());
        SetTextColor(hdc, text_dark);

        if (cooling) {
            const int ticks_left =
                std::max(0, selected.cooldown_until_tick - game_.Time().CurrentTick());
            SetTextColor(hdc, text_muted);
            Text(hdc, 735, 505,
                 L"订单刷新中，剩余 " +
                     std::to_wstring(ticks_left * kGameMinutesPerTick) + L" 分钟");
            SetTextColor(hdc, text_dark);
        } else if (locked) {
            SetTextColor(hdc, text_muted);
            Text(hdc, 735, 505, L"该订单已锁定，暂时不能操作");
            SetTextColor(hdc, text_dark);
        } else if (protected_item) {
            SetTextColor(hdc, RGB(157, 63, 38));
            Text(hdc, 735, 505, L"该物品已受保护，请先在仓库解除锁定");
            SetTextColor(hdc, text_dark);
        } else if (owned < selected.requirements[0].quantity) {
            SetTextColor(hdc, text_muted);
            Text(hdc, 735, 505,
                 L"还需要 " + std::to_wstring(selected.requirements[0].quantity - owned) + L" 个" +
                     ItemName(selected.requirements[0].item));
            SetTextColor(hdc, text_dark);
        } else {
            SetTextColor(hdc, action_green);
            Text(hdc, 735, 505, L"物资齐全，可以立即交付");
            SetTextColor(hdc, text_dark);
        }

        OrderButton(kCompleteOrder, RECT{735, 545, 885, 585}, L"交付订单",
                    can_deliver, true);
        OrderButton(kAbandonOrder, RECT{900, 545, 1045, 585}, L"放弃订单",
                    !cooling && !locked, false);
    }
}

void FarmWindow::DrawWarehouse(HDC hdc) {
    const COLORREF panel_fill = RGB(255, 239, 198);
    const COLORREF slot_fill = RGB(250, 225, 174);
    const COLORREF text_dark = RGB(63, 43, 24);
    const COLORREF text_muted = RGB(117, 91, 62);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF wood_mid = RGB(151, 94, 43);
    const COLORREF action_green = RGB(86, 151, 34);
    const COLORREF disabled_fill = RGB(174, 158, 124);
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto SectionHeader = [&](RECT rect, const std::wstring& label) {
        Fill(hdc, rect, wood_mid);
        Frame(rect, wood_dark);
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, rect.left + 16, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };
    auto WarehouseButton = [&](int id, RECT rect, const std::wstring& label, bool enabled,
                               COLORREF color) {
        if (enabled) {
            buttons_.push_back(UiButton{rect, id});
        }
        Fill(hdc, rect, enabled ? color : disabled_fill);
        Frame(rect, enabled ? RGB(75, 83, 38) : RGB(132, 119, 91));
        SetTextColor(hdc, enabled ? RGB(255, 250, 224) : RGB(235, 228, 207));
        Text(hdc, rect.left + 14, rect.top + 8, label);
        SetTextColor(hdc, text_dark);
    };

    std::vector<InventoryItemView> inventory = game_.Player().InventoryView();
    if (!inventory.empty()) {
        bool selected_exists = false;
        for (const InventoryItemView& item : inventory) {
            selected_exists = selected_exists || item.item == selected_warehouse_item_;
        }
        if (!selected_exists) {
            selected_warehouse_item_ = inventory.front().item;
        }
    }

    const int page_count =
        std::max(1, (static_cast<int>(inventory.size()) + kWarehouseItemsPerPage - 1) /
                        kWarehouseItemsPerPage);
    warehouse_page_ = std::max(0, std::min(warehouse_page_, page_count - 1));

    const int used = game_.Player().WarehouseUsed();
    const int capacity = game_.Player().WarehouseCapacity();
    RECT title_bar{35, 130, 225, 168};
    Fill(hdc, title_bar, panel_fill);
    Frame(title_bar, wood_dark);
    Text(hdc, 55, 140, L"农场仓库");

    RECT capacity_bar{240, 130, 710, 168};
    Fill(hdc, capacity_bar, panel_fill);
    Frame(capacity_bar, wood_dark);
    std::wstringstream capacity_text;
    capacity_text << L"容量 " << used << L"/" << capacity << L"    剩余 "
                  << game_.Player().WarehouseRemaining() << L" 格";
    Text(hdc, 260, 140, capacity_text.str());
    RECT progress_back{485, 142, 690, 156};
    Fill(hdc, progress_back, RGB(174, 143, 94));
    RECT progress_fill = progress_back;
    progress_fill.right =
        progress_back.left + (progress_back.right - progress_back.left) * used /
                                 std::max(1, capacity);
    const bool nearly_full = used * 100 >= capacity * 80;
    Fill(hdc, progress_fill, nearly_full ? RGB(219, 132, 44) : action_green);
    Frame(progress_back, wood_dark);

    const bool can_upgrade = game_.Player().WarehouseLevel() < kWarehouseMaxLevel &&
                             game_.Player().Gold() >= kWarehouseUpgradeCosts[game_.Player().WarehouseLevel()];
    WarehouseButton(kWarehouseUpgrade, RECT{720, 130, 875, 168}, L"扩容 +20",
                    can_upgrade, RGB(65, 132, 177));
    WarehouseButton(kWarehousePagePrevious, RECT{885, 130, 930, 168}, L"<",
                    warehouse_page_ > 0, RGB(176, 112, 50));
    RECT page_box{935, 130, 995, 168};
    Fill(hdc, page_box, panel_fill);
    Frame(page_box, wood_dark);
    Text(hdc, 947, 140,
         std::to_wstring(warehouse_page_ + 1) + L"/" + std::to_wstring(page_count));
    WarehouseButton(kWarehousePageNext, RECT{1000, 130, 1045, 168}, L">",
                    warehouse_page_ + 1 < page_count, RGB(176, 112, 50));

    RECT warehouse_panel{45, 315, 1075, 610};
    Fill(hdc, warehouse_panel, panel_fill);
    Frame(warehouse_panel, wood_dark);
    RECT inner_frame{49, 319, 1071, 606};
    Frame(inner_frame, RGB(194, 132, 67));
    Frame(RECT{700, 328, 701, 593}, RGB(194, 132, 67));

    SectionHeader(RECT{65, 330, 680, 365},
                  L"库存物品  " + std::to_wstring(inventory.size()) + L" 种");
    SectionHeader(RECT{720, 330, 1055, 365}, L"物品详情");

    const int first_item = warehouse_page_ * kWarehouseItemsPerPage;
    for (int slot = 0; slot < kWarehouseItemsPerPage; ++slot) {
        const int index = first_item + slot;
        const int row = slot / 4;
        const int col = slot % 4;
        const int x = 65 + col * 150;
        const int y = 380 + row * 100;
        RECT item_slot{x, y, x + 138, y + 88};

        if (index >= static_cast<int>(inventory.size())) {
            Fill(hdc, item_slot, RGB(239, 217, 177));
            SetTextColor(hdc, RGB(165, 139, 99));
            Text(hdc, x + 45, y + 34, L"空格");
            SetTextColor(hdc, text_dark);
            continue;
        }

        const InventoryItemView& item = inventory[static_cast<std::size_t>(index)];
        const bool selected = item.item == selected_warehouse_item_;
        buttons_.push_back(UiButton{item_slot, kWarehouseItemBase + slot});
        Fill(hdc, item_slot, slot_fill);
        Frame(item_slot, selected ? RGB(238, 169, 45) : RGB(180, 126, 69));
        if (selected) {
            RECT selected_frame = item_slot;
            InflateRect(&selected_frame, -3, -3);
            Frame(selected_frame, RGB(238, 169, 45));
        }

        DrawTextureOrFill(hdc, ItemTextureKey(item.item), RECT{x + 41, y + 8, x + 97, y + 64},
                          RGB(226, 196, 126));
        Text(hdc, x + 8, y + 65, ItemName(item.item));
        Text(hdc, x + 101, y + 7, L"x" + std::to_wstring(item.quantity));
        if (item.locked) {
            SetTextColor(hdc, RGB(157, 90, 20));
            Text(hdc, x + 8, y + 7, L"保护");
            SetTextColor(hdc, text_dark);
        }
    }

    if (inventory.empty()) {
        SetTextColor(hdc, text_muted);
        Text(hdc, 270, 445, L"仓库暂时没有物品");
        SetTextColor(hdc, text_dark);
        return;
    }

    const InventoryItemView* selected = nullptr;
    for (const InventoryItemView& item : inventory) {
        if (item.item == selected_warehouse_item_) {
            selected = &item;
            break;
        }
    }
    if (selected == nullptr) {
        return;
    }

    const ItemInfo& info = GetItemInfo(selected->item);
    DrawTextureOrFill(hdc, ItemTextureKey(selected->item), RECT{735, 385, 815, 465},
                      RGB(226, 196, 126));
    Text(hdc, 835, 385, ItemName(selected->item));
    Text(hdc, 835, 418, ItemCategoryName(info.category));
    Text(hdc, 835, 450, L"数量  " + std::to_wstring(selected->quantity));

    std::wstringstream price;
    if (info.sell_price > 0) {
        price << L"单价  " << info.sell_price << L" 金币";
    } else {
        price << L"该物品不可出售";
    }
    Text(hdc, 735, 485, price.str());

    SetTextColor(hdc, selected->locked ? RGB(157, 90, 20) : action_green);
    Text(hdc, 735, 515, selected->locked ? L"已保护：不可出售或消耗"
                                         : L"未保护：可出售或用于生产");
    SetTextColor(hdc, text_dark);

    WarehouseButton(kWarehouseSellSelected, RECT{735, 550, 855, 590}, L"出售 1 个",
                    info.sell_price > 0 && !selected->locked && selected->quantity > 0,
                    action_green);
    WarehouseButton(kWarehouseToggleLock, RECT{870, 550, 990, 590},
                    selected->locked ? L"解除保护" : L"保护物品", true,
                    RGB(176, 112, 50));
}

void FarmWindow::DrawShop(HDC hdc) {
    const COLORREF panel_fill = RGB(255, 239, 198);
    const COLORREF card_fill = RGB(250, 225, 174);
    const COLORREF locked_fill = RGB(197, 184, 154);
    const COLORREF text_dark = RGB(63, 43, 24);
    const COLORREF text_muted = RGB(120, 93, 61);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF wood_mid = RGB(151, 94, 43);
    const COLORREF action_green = RGB(86, 151, 34);
    const COLORREF warning_orange = RGB(199, 116, 35);
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto SectionHeader = [&](RECT rect, const std::wstring& label) {
        Fill(hdc, rect, wood_mid);
        Frame(rect, wood_dark);
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, rect.left + 16, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };
    auto ShopButton = [&](int id, RECT rect, const std::wstring& label, bool enabled) {
        if (enabled) {
            buttons_.push_back(UiButton{rect, id});
        }
        Fill(hdc, rect, enabled ? action_green : RGB(164, 150, 119));
        Frame(rect, enabled ? RGB(75, 108, 32) : RGB(129, 116, 89));
        SetTextColor(hdc, enabled ? RGB(255, 250, 224) : RGB(233, 225, 204));
        Text(hdc, rect.left + 10, rect.top + 6, label);
        SetTextColor(hdc, text_dark);
    };
    auto ButtonIdForItem = [](ItemId item) {
        switch (item) {
            case ItemId::WheatSeed:
                return kBuyWheatSeed;
            case ItemId::CornSeed:
                return kBuyCornSeed;
            case ItemId::CarrotSeed:
                return kBuyCarrotSeed;
            case ItemId::TomatoSeed:
                return kBuyTomatoSeed;
            case ItemId::Fertilizer:
                return kBuyFertilizer;
            default:
                return 0;
        }
    };

    const int gold = game_.Player().Gold();
    const int warehouse_used = game_.Player().WarehouseUsed();
    const int warehouse_capacity = game_.Player().WarehouseCapacity();
    const int warehouse_remaining = game_.Player().WarehouseRemaining();

    RECT title_bar{35, 130, 225, 168};
    Fill(hdc, title_bar, panel_fill);
    Frame(title_bar, wood_dark);
    Text(hdc, 55, 140, L"农场商店");

    RECT resource_bar{240, 130, 1045, 168};
    Fill(hdc, resource_bar, panel_fill);
    Frame(resource_bar, wood_dark);
    DrawTextureOrFill(hdc, L"item_coin", RECT{258, 137, 282, 161}, RGB(237, 190, 67));
    Text(hdc, 292, 140, L"金币 " + std::to_wstring(gold));
    Text(hdc, 465, 140,
         L"仓库 " + std::to_wstring(warehouse_used) + L"/" +
             std::to_wstring(warehouse_capacity));
    SetTextColor(hdc, warehouse_remaining > 0 ? action_green : warning_orange);
    Text(hdc, 680, 140, L"剩余空间 " + std::to_wstring(warehouse_remaining));
    SetTextColor(hdc, text_dark);

    RECT hint_bar{35, 180, 470, 220};
    Fill(hdc, hint_bar, RGB(255, 246, 216));
    Frame(hint_bar, RGB(184, 124, 62));
    Text(hdc, 52, 191, L"种子与肥料商店 · 每次购买 1 个");

    RECT shop_panel{45, 315, 1075, 610};
    Fill(hdc, shop_panel, panel_fill);
    Frame(shop_panel, wood_dark);
    RECT inner_frame{49, 319, 1071, 606};
    Frame(inner_frame, RGB(194, 132, 67));
    Frame(RECT{690, 328, 691, 593}, RGB(194, 132, 67));

    SectionHeader(RECT{65, 330, 670, 365}, L"商品货架");
    SectionHeader(RECT{710, 330, 1055, 365}, L"购买说明");

    const std::vector<ShopItemView> goods = game_.Shop().Items(game_.Player());
    for (std::size_t index = 0; index < goods.size(); ++index) {
        const ShopItemView& good = goods[index];
        const int row = static_cast<int>(index) / 3;
        const int col = static_cast<int>(index) % 3;
        const int x = 65 + col * 202;
        const int y = 375 + row * 105;
        RECT card{x, y, x + 188, y + 94};
        const bool has_gold = gold >= good.unit_price;
        const bool has_space = warehouse_remaining > 0;
        const bool can_buy = good.unlocked && has_gold && has_space;

        Fill(hdc, card, good.unlocked ? card_fill : locked_fill);
        Frame(card, good.unlocked ? RGB(180, 126, 69) : RGB(133, 119, 91));
        DrawTextureOrFill(hdc, ItemTextureKey(good.item),
                          RECT{x + 10, y + 10, x + 58, y + 58}, RGB(226, 196, 126));

        SetTextColor(hdc, good.unlocked ? text_dark : text_muted);
        Text(hdc, x + 68, y + 9, ItemName(good.item));
        Text(hdc, x + 68, y + 34,
             L"持有 " + std::to_wstring(game_.Player().ItemCount(good.item)));
        DrawTextureOrFill(hdc, L"item_coin", RECT{x + 12, y + 65, x + 34, y + 87},
                          RGB(237, 190, 67));
        Text(hdc, x + 40, y + 66, std::to_wstring(good.unit_price));

        std::wstring action = L"购买 1 个";
        if (!good.unlocked) {
            action = L"未解锁";
        } else if (!has_space) {
            action = L"仓库已满";
        } else if (!has_gold) {
            action = L"金币不足";
        }
        ShopButton(ButtonIdForItem(good.item), RECT{x + 91, y + 60, x + 178, y + 89},
                   action, can_buy);
    }

    SetTextColor(hdc, text_dark);
    Text(hdc, 730, 385, L"购买条件");
    Text(hdc, 745, 425, L"1. 种子已在解锁页开放");
    Text(hdc, 745, 458, L"2. 金币不少于商品单价");
    Text(hdc, 745, 491, L"3. 仓库至少保留 1 格空间");

    SetTextColor(hdc, warehouse_remaining > 0 ? action_green : warning_orange);
    Text(hdc, 730, 535,
         warehouse_remaining > 0 ? L"当前仓库可以接收新商品"
                                 : L"当前仓库已满，请先整理仓库");
    SetTextColor(hdc, text_dark);
    Text(hdc, 730, 568, L"未解锁种子请前往顶部“解锁”页面。");
}

void FarmWindow::DrawFishing(HDC hdc) {
    const COLORREF panel_fill = RGB(255, 239, 198);
    const COLORREF card_fill = RGB(247, 224, 179);
    const COLORREF text_dark = RGB(63, 43, 24);
    const COLORREF text_muted = RGB(116, 92, 61);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF wood_mid = RGB(159, 96, 42);
    const COLORREF green = RGB(82, 150, 34);
    const COLORREF blue = RGB(61, 132, 170);
    const COLORREF disabled = RGB(166, 153, 124);
    const auto& fishing = game_.Fishing();
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto SmallButton = [&](int id, RECT rect, const std::wstring& label, bool enabled,
                           COLORREF color) {
        if (enabled) {
            buttons_.push_back(UiButton{rect, id});
        }
        Fill(hdc, rect, enabled ? color : disabled);
        Frame(rect, enabled ? wood_dark : RGB(126, 113, 86));
        SetTextColor(hdc, enabled ? RGB(255, 249, 223) : RGB(232, 224, 201));
        Text(hdc, rect.left + 12, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };
    auto SectionHeader = [&](RECT rect, const std::wstring& label) {
        Fill(hdc, rect, wood_mid);
        Frame(rect, wood_dark);
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, rect.left + 16, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };

    RECT title_bar{35, 130, 300, 170};
    Fill(hdc, title_bar, panel_fill);
    Frame(title_bar, wood_dark);
    DrawTextureOrFill(hdc, L"weather_rainy", RECT{52, 136, 88, 166}, RGB(138, 188, 208));
    Text(hdc, 100, 140, L"\u6e56\u8fb9\u9493\u9c7c");

    RECT stats_bar{320, 130, 1090, 170};
    Fill(hdc, stats_bar, panel_fill);
    Frame(stats_bar, wood_dark);
    Text(hdc, 342, 140, L"\u9c7c\u9975 " + std::to_wstring(fishing.BaitCount()));
    Text(hdc, 470, 140, L"\u9c7c\u7aff Lv." + std::to_wstring(fishing.RodLevel()));
    Text(hdc, 625, 140, L"\u7d2f\u8ba1\u9c7c\u83b7 " + std::to_wstring(fishing.TotalCatches()));
    Text(hdc, 825, 140, L"\u9c7c\u7bd3\u4ef7\u503c " +
                         std::to_wstring(fishing.CollectionValue()));

    HPEN rod_pen = CreatePen(PS_SOLID, 4, RGB(93, 55, 24));
    HGDIOBJ old_pen = SelectObject(hdc, rod_pen);
    MoveToEx(hdc, 215, 292, nullptr);
    LineTo(hdc, 492, 386);
    SelectObject(hdc, old_pen);
    DeleteObject(rod_pen);
    Fill(hdc, RECT{178, 276, 236, 289}, RGB(111, 64, 29));
    Fill(hdc, RECT{489, 374, 507, 405}, RGB(184, 59, 41));
    Fill(hdc, RECT{489, 405, 507, 426}, RGB(255, 248, 226));
    HPEN ripple_pen = CreatePen(PS_SOLID, 2, RGB(213, 245, 249));
    old_pen = SelectObject(hdc, ripple_pen);
    Arc(hdc, 445, 385, 550, 452, 445, 418, 550, 418);
    Arc(hdc, 325, 335, 420, 390, 325, 362, 420, 362);
    SelectObject(hdc, old_pen);
    DeleteObject(ripple_pen);

    RECT guide_panel{760, 190, 1090, 535};
    Fill(hdc, guide_panel, RGB(255, 242, 211));
    Frame(guide_panel, wood_dark);
    SectionHeader(RECT{780, 210, 1070, 244}, L"\u9c7c\u7c7b\u56fe\u9274");
    Text(hdc, 790, 258, L"\u5df2\u53d1\u73b0 / \u672a\u53d1\u73b0");

    const auto& fish_list = FishingSystem::FishList();
    for (std::size_t i = 0; i < fish_list.size() && i < 10; ++i) {
        const FishDef& fish = fish_list[i];
        const int row = static_cast<int>(i) / 2;
        const int col = static_cast<int>(i) % 2;
        const int x = 790 + col * 140;
        const int y = 288 + row * 42;
        const int count = fishing.CollectionCount(fish.id);
        RECT slot{x, y, x + 120, y + 32};
        Fill(hdc, slot, count > 0 ? RGB(255, 247, 224) : RGB(214, 199, 162));
        Frame(slot, count > 0 ? RGB(185, 128, 64) : RGB(143, 126, 94));
        SetTextColor(hdc, count > 0 ? text_dark : text_muted);
        Text(hdc, x + 8, y + 6,
             (count > 0 ? FishDisplayName(fish.id) : L"???") + L" x" +
                 std::to_wstring(count));
    }

    RECT bottom{35, 540, 1090, 642};
    Fill(hdc, bottom, RGB(255, 242, 211));
    Frame(bottom, wood_dark);
    SectionHeader(RECT{55, 554, 245, 586}, L"\u6536\u7aff\u5224\u5b9a");
    SetTextColor(hdc, text_muted);
    Text(hdc, 265, 560, FishingStateText(fishing.State()));
    SetTextColor(hdc, text_dark);

    RECT bar{265, 592, 720, 612};
    Fill(hdc, bar, RGB(219, 196, 154));
    Frame(bar, RGB(130, 93, 55));
    if (fishing.State() == FishingState::Biting) {
        const int gs = std::clamp(fishing.GetReelGreenStart(), 0, 100);
        const int ge = std::clamp(fishing.GetReelGreenEnd(), 0, 100);
        const int px = bar.left + (bar.right - bar.left) * fishing.GetReelPos() / 100;
        RECT zone{bar.left + (bar.right - bar.left) * gs / 100, bar.top + 1,
                  bar.left + (bar.right - bar.left) * ge / 100, bar.bottom - 1};
        Fill(hdc, zone, green);
        Fill(hdc, RECT{px - 3, bar.top - 9, px + 3, bar.bottom + 9}, RGB(185, 67, 45));
    } else if (fishing.State() == FishingState::Waiting) {
        const int pulse = static_cast<int>((GetTickCount64() / 100) % 100);
        RECT wait_fill{bar.left + 1, bar.top + 1,
                       bar.left + (bar.right - bar.left) * pulse / 100, bar.bottom - 1};
        Fill(hdc, wait_fill, blue);
    } else {
        Fill(hdc, RECT{bar.left + 1, bar.top + 1, bar.left + 120, bar.bottom - 1}, blue);
    }

    const bool can_cast = fishing.CanFish();
    const bool can_reel = fishing.State() == FishingState::Biting;
    SmallButton(kFishingCast, RECT{750, 556, 850, 592}, L"\u629b\u7aff", can_cast, green);
    SmallButton(kFishingReel, RECT{862, 556, 962, 592}, L"\u6536\u7aff", can_reel, blue);
    SmallButton(kFishingBuyBait, RECT{974, 556, 1070, 592}, L"\u9c7c\u9975 x5", true,
                green);
    SmallButton(kFishingUpgradeRod, RECT{750, 602, 900, 632},
                L"\u5347\u7ea7\u9c7c\u7aff " + std::to_wstring(fishing.RodUpgradeCost()),
                fishing.RodUpgradeCost() > 0, blue);
    SmallButton(kFishingSellAll, RECT{914, 602, 1070, 632}, L"\u51fa\u552e\u9c7c\u7bd3",
                fishing.CollectionValue() > 0, green);
}

void FarmWindow::DrawUnlock(HDC hdc) {
    const COLORREF panel_fill = RGB(255, 239, 198);
    const COLORREF node_fill = RGB(250, 225, 174);
    const COLORREF text_dark = RGB(63, 43, 24);
    const COLORREF text_muted = RGB(119, 98, 73);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF green = RGB(91, 155, 43);
    const COLORREF amber = RGB(224, 157, 37);
    const COLORREF gray = RGB(154, 143, 119);
    const COLORREF purple = RGB(143, 116, 151);
    const COLORREF red = RGB(190, 91, 61);
    const COLORREF seed_line = RGB(78, 145, 48);
    const COLORREF ranch_line = RGB(46, 124, 177);
    const COLORREF dependency_line = RGB(205, 137, 28);
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto NodeRect = [](UnlockId id) {
        switch (id) {
            case UnlockId::WheatSeed:
                return RECT{65, 295, 210, 371};
            case UnlockId::CornSeed:
                return RECT{230, 295, 375, 371};
            case UnlockId::CarrotSeed:
                return RECT{395, 295, 540, 371};
            case UnlockId::TomatoSeed:
                return RECT{560, 295, 705, 371};
            case UnlockId::ExtraLand:
                return RECT{230, 505, 375, 581};
            case UnlockId::ChickenCoop:
                return RECT{65, 400, 210, 476};
            case UnlockId::CowBarn:
                return RECT{395, 400, 540, 476};
            case UnlockId::SheepPen:
                return RECT{560, 400, 705, 476};
            case UnlockId::Chicken:
                return RECT{65, 505, 210, 581};
            case UnlockId::Cow:
                return RECT{395, 505, 540, 581};
            case UnlockId::Sheep:
                return RECT{560, 505, 705, 581};
        }
        return RECT{65, 295, 210, 371};
    };
    auto NodeTexture = [](UnlockId id) -> std::wstring {
        switch (id) {
            case UnlockId::WheatSeed:
                return L"seed_wheat";
            case UnlockId::CornSeed:
                return L"seed_corn";
            case UnlockId::CarrotSeed:
                return L"seed_carrot";
            case UnlockId::TomatoSeed:
                return L"seed_tomato";
            case UnlockId::ExtraLand:
                return L"soil";
            case UnlockId::ChickenCoop:
                return L"building_chicken_coop";
            case UnlockId::CowBarn:
                return L"building_cow_barn";
            case UnlockId::SheepPen:
                return L"building_sheep_pen";
            case UnlockId::Chicken:
                return L"animal_chicken_idle";
            case UnlockId::Cow:
                return L"animal_cow_idle";
            case UnlockId::Sheep:
                return L"animal_sheep_idle";
        }
        return L"item_unknown";
    };
    auto DrawPath = [&](const std::vector<POINT>& points, COLORREF color) {
        if (points.size() < 2) {
            return;
        }
        HPEN pen = CreatePen(PS_SOLID, 3, color);
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, points.front().x, points.front().y, nullptr);
        for (std::size_t i = 1; i < points.size(); ++i) {
            LineTo(hdc, points[i].x, points[i].y);
        }
        const POINT& end = points.back();
        const POINT& previous = points[points.size() - 2];
        if (end.x != previous.x) {
            const int direction = end.x > previous.x ? 1 : -1;
            MoveToEx(hdc, end.x, end.y, nullptr);
            LineTo(hdc, end.x - direction * 7, end.y - 5);
            MoveToEx(hdc, end.x, end.y, nullptr);
            LineTo(hdc, end.x - direction * 7, end.y + 5);
        } else {
            const int direction = end.y > previous.y ? 1 : -1;
            MoveToEx(hdc, end.x, end.y, nullptr);
            LineTo(hdc, end.x - 5, end.y - direction * 7);
            MoveToEx(hdc, end.x, end.y, nullptr);
            LineTo(hdc, end.x + 5, end.y - direction * 7);
        }
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
    };

    const auto& nodes = UnlockGraph::Nodes();
    int unlocked_count = 0;
    for (const UnlockNode& node : nodes) {
        if (game_.Player().IsUnlocked(node.id)) {
            ++unlocked_count;
        }
    }

    RECT title_bar{35, 130, 310, 168};
    Fill(hdc, title_bar, panel_fill);
    Frame(title_bar, wood_dark);
    Text(hdc, 55, 140, L"农场发展路线");

    RECT resource_bar{325, 130, 1045, 168};
    Fill(hdc, resource_bar, panel_fill);
    Frame(resource_bar, wood_dark);
    Text(hdc, 345, 140, L"等级 " + std::to_wstring(game_.Player().Level()));
    DrawTextureOrFill(hdc, L"item_coin", RECT{470, 137, 494, 161}, RGB(237, 190, 67));
    Text(hdc, 504, 140, L"金币 " + std::to_wstring(game_.Player().Gold()));
    Text(hdc, 700, 140,
         L"进度 " + std::to_wstring(unlocked_count) + L"/" +
             std::to_wstring(nodes.size()));

    RECT hint_bar{35, 180, 655, 220};
    Fill(hdc, hint_bar, RGB(255, 246, 216));
    Frame(hint_bar, RGB(184, 124, 62));
    Text(hdc, 52, 191, L"沿连线逐步开放种子、土地、牧场设施和动物");

    RECT tree_panel{45, 250, 1075, 610};
    Fill(hdc, tree_panel, panel_fill);
    Frame(tree_panel, wood_dark);
    Frame(RECT{49, 254, 1071, 606}, RGB(194, 132, 67));
    Frame(RECT{725, 265, 726, 593}, RGB(194, 132, 67));

    SetTextColor(hdc, seed_line);
    Text(hdc, 65, 264, L"种植路线");
    SetTextColor(hdc, ranch_line);
    Text(hdc, 65, 378, L"养殖路线");
    SetTextColor(hdc, text_dark);

    DrawPath({POINT{210, 333}, POINT{230, 333}}, seed_line);
    DrawPath({POINT{375, 333}, POINT{395, 333}}, seed_line);
    DrawPath({POINT{540, 333}, POINT{560, 333}}, seed_line);
    DrawPath({POINT{302, 371}, POINT{302, 505}}, seed_line);
    DrawPath({POINT{467, 371}, POINT{467, 400}}, dependency_line);
    DrawPath({POINT{210, 438}, POINT{395, 438}}, ranch_line);
    DrawPath({POINT{540, 438}, POINT{560, 438}}, ranch_line);
    DrawPath({POINT{137, 476}, POINT{137, 505}}, ranch_line);
    DrawPath({POINT{467, 476}, POINT{467, 505}}, ranch_line);
    DrawPath({POINT{632, 476}, POINT{632, 505}}, ranch_line);

    HFONT status_font =
        CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    int index = 0;
    for (const UnlockNode& node : nodes) {
        const bool unlocked = game_.Player().IsUnlocked(node.id);
        const bool requirements_met = game_.Player().CanUnlock(node.id);
        const bool enough_gold = game_.Player().Gold() >= node.gold_cost;
        const bool can_unlock = requirements_met && enough_gold;
        const bool level_met = game_.Player().Level() >= node.required_level;

        COLORREF status_color = purple;
        std::wstring status = L"前置未满足";
        if (unlocked) {
            status_color = green;
            status = L"已解锁";
        } else if (!level_met) {
            status_color = gray;
            status = L"等级不足";
        } else if (requirements_met && !enough_gold) {
            status_color = red;
            status = L"金币不足";
        } else if (can_unlock) {
            status_color = amber;
            status = L"可解锁";
        }

        const RECT rect = NodeRect(node.id);
        Fill(hdc, rect, unlocked ? RGB(226, 239, 174) : node_fill);
        Frame(rect, status_color);
        DrawTextureOrFill(hdc, NodeTexture(node.id),
                          RECT{rect.left + 7, rect.top + 8, rect.left + 43, rect.top + 44},
                          RGB(226, 196, 126));
        SetTextColor(hdc, text_dark);
        Text(hdc, rect.left + 49, rect.top + 6, Utf8ToWide(node.name));
        Text(hdc, rect.left + 49, rect.top + 28,
             L"等级 " + std::to_wstring(node.required_level));
        DrawTextureOrFill(hdc, L"item_coin",
                          RECT{rect.left + 8, rect.top + 50, rect.left + 27, rect.top + 69},
                          RGB(237, 190, 67));
        Text(hdc, rect.left + 31, rect.top + 50, std::to_wstring(node.gold_cost));

        RECT status_rect{rect.left + 61, rect.top + 48, rect.right - 6, rect.bottom - 5};
        Fill(hdc, status_rect, status_color);
        Frame(status_rect, wood_dark);
        SetTextColor(hdc, RGB(255, 247, 219));
        HGDIOBJ old_font = SelectObject(hdc, status_font);
        Text(hdc, status_rect.left + 4, status_rect.top + 5, status);
        SelectObject(hdc, old_font);
        if (can_unlock) {
            buttons_.push_back(UiButton{status_rect, kUnlockBase + index});
        }
        ++index;
    }
    DeleteObject(status_font);

    SetTextColor(hdc, text_dark);
    Text(hdc, 755, 275, L"状态说明");
    const std::vector<std::pair<COLORREF, std::wstring>> legend = {
        {green, L"已解锁"}, {amber, L"可解锁"}, {gray, L"等级不足"},
        {purple, L"前置未满足"}, {red, L"金币不足"},
    };
    int legend_y = 310;
    for (const auto& [color, label] : legend) {
        RECT swatch{755, legend_y, 780, legend_y + 20};
        Fill(hdc, swatch, color);
        Frame(swatch, wood_dark);
        Text(hdc, 792, legend_y, label);
        legend_y += 35;
    }

    SetTextColor(hdc, seed_line);
    Text(hdc, 755, 500, L"绿色连线：种植发展");
    SetTextColor(hdc, ranch_line);
    Text(hdc, 755, 530, L"蓝色连线：牧场发展");
    SetTextColor(hdc, dependency_line);
    Text(hdc, 755, 560, L"金色连线：跨路线前置");
    SetTextColor(hdc, text_muted);
    Text(hdc, 755, 585, L"点击“可解锁”完成开放");
    SetTextColor(hdc, text_dark);
}

void FarmWindow::DrawSave(HDC hdc) {
    const COLORREF panel_fill = RGB(255, 239, 198);
    const COLORREF text_dark = RGB(63, 43, 24);
    const COLORREF text_muted = RGB(117, 91, 62);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF wood_mid = RGB(151, 94, 43);
    const COLORREF green = RGB(86, 151, 34);
    const COLORREF blue = RGB(55, 126, 178);
    const COLORREF red = RGB(181, 76, 48);
    const COLORREF disabled = RGB(164, 150, 119);
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto SectionHeader = [&](RECT rect, const std::wstring& label) {
        Fill(hdc, rect, wood_mid);
        Frame(rect, wood_dark);
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, rect.left + 16, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };
    auto SaveButton = [&](int id, RECT rect, const std::wstring& label, COLORREF color,
                          bool enabled) {
        if (enabled) {
            buttons_.push_back(UiButton{rect, id});
        }
        Fill(hdc, rect, enabled ? color : disabled);
        Frame(rect, enabled ? wood_dark : RGB(128, 115, 88));
        SetTextColor(hdc, enabled ? RGB(255, 250, 224) : RGB(232, 225, 205));
        Text(hdc, rect.left + 18, rect.top + 10, label);
        SetTextColor(hdc, text_dark);
    };

    const bool save_exists = SaveManager::Exists(Narrow(SavePath()));
    std::error_code file_error;
    const std::uintmax_t save_size =
        save_exists ? std::filesystem::file_size(std::filesystem::path(SavePath()), file_error)
                    : 0;
    const TimeSnapshot time = game_.Time().Snapshot();
    const int auto_save_elapsed = time.tick - game_.LastAutoSaveTick();
    const int ticks_until_auto_save =
        std::max(0, kAutoSaveEveryTicks - auto_save_elapsed);

    RECT title_bar{35, 130, 285, 168};
    Fill(hdc, title_bar, panel_fill);
    Frame(title_bar, wood_dark);
    Text(hdc, 55, 140, L"农场记录");

    RECT auto_save_bar{300, 130, 1045, 168};
    Fill(hdc, auto_save_bar, panel_fill);
    Frame(auto_save_bar, wood_dark);
    SetTextColor(hdc, green);
    Text(hdc, 325, 140, L"每 12 刻自动保存");
    SetTextColor(hdc, text_dark);
    Text(hdc, 565, 140,
         L"下次约 " + std::to_wstring(ticks_until_auto_save) + L" 刻");
    Text(hdc, 790, 140, L"存档版本 2");

    RECT hint_bar{35, 180, 640, 220};
    Fill(hdc, hint_bar, RGB(255, 246, 216));
    Frame(hint_bar, RGB(184, 124, 62));
    Text(hdc, 52, 191, L"单一存档槽 · 保存会覆盖现有记录 · 读取会结算离线进度");

    RECT save_panel{45, 315, 1075, 610};
    Fill(hdc, save_panel, panel_fill);
    Frame(save_panel, wood_dark);
    Frame(RECT{49, 319, 1071, 606}, RGB(194, 132, 67));
    Frame(RECT{365, 328, 366, 593}, RGB(194, 132, 67));
    Frame(RECT{700, 328, 701, 593}, RGB(194, 132, 67));

    SectionHeader(RECT{65, 330, 345, 365}, L"存档槽 1");
    SectionHeader(RECT{385, 330, 680, 365}, L"存档操作");
    SectionHeader(RECT{720, 330, 1055, 365}, L"保存内容");

    DrawTextureOrFill(hdc, L"building_warehouse", RECT{70, 380, 140, 450},
                      RGB(190, 134, 84));
    SetTextColor(hdc, save_exists ? green : text_muted);
    Text(hdc, 155, 382, save_exists ? L"已有存档" : L"空存档槽");
    SetTextColor(hdc, text_dark);
    Text(hdc, 155, 412, L"saves/save01.farm");
    SetTextColor(hdc, text_muted);
    Text(hdc, 155, 440,
         save_exists && !file_error
             ? L"文件大小 " + std::to_wstring(save_size) + L" 字节"
             : L"尚未生成存档文件");

    SetTextColor(hdc, text_dark);
    Text(hdc, 75, 480,
         L"第 " + std::to_wstring(time.day) + L" 天   等级 " +
             std::to_wstring(game_.Player().Level()));
    Text(hdc, 75, 510,
         L"金币 " + std::to_wstring(game_.Player().Gold()) + L"   仓库 " +
             std::to_wstring(game_.Player().WarehouseUsed()) + L"/" +
             std::to_wstring(game_.Player().WarehouseCapacity()));
    Text(hdc, 75, 540,
         L"农田 " + std::to_wstring(game_.Planting().Plots().size()) +
             L" 块   牧场设施 " +
             std::to_wstring(game_.Ranch().Facilities().size()));
    Text(hdc, 75, 570, L"天气 " + std::wstring(WeatherName(game_.Weather().Snapshot().weather)));

    SaveButton(kSave, RECT{405, 385, 660, 430}, L"保存当前进度", green, true);
    SaveButton(kLoad, RECT{405, 455, 660, 500}, L"读取存档", blue, save_exists);
    SaveButton(kNewGame, RECT{405, 525, 660, 570}, L"开始新游戏", red, true);
    SetTextColor(hdc, red);
    Text(hdc, 405, 582, L"未保存的当前进度将丢失");

    SetTextColor(hdc, text_dark);
    Text(hdc, 740, 382, L"农田：作物状态与土地扩建");
    Text(hdc, 740, 420, L"牧场：设施、动物与生产状态");
    Text(hdc, 740, 458, L"仓库：物品数量与保护状态");
    Text(hdc, 740, 496, L"订单：当前订单与冷却进度");
    Text(hdc, 740, 534, L"时间：天气、速度与游戏日期");
    SetTextColor(hdc, green);
    Text(hdc, 740, 575, L"读取时自动结算离线进度");
    SetTextColor(hdc, text_dark);
}

void FarmWindow::DrawUtilityMenu(HDC hdc) {
    const COLORREF panel_fill = RGB(255, 239, 198);
    const COLORREF card_fill = RGB(255, 247, 224);
    const COLORREF text_dark = RGB(63, 43, 24);
    const COLORREF text_muted = RGB(116, 92, 61);
    const COLORREF wood_dark = RGB(105, 65, 30);
    const COLORREF wood_mid = RGB(159, 96, 42);
    const COLORREF green = RGB(82, 150, 34);
    const COLORREF blue = RGB(61, 132, 170);
    const COLORREF amber = RGB(214, 145, 42);
    SetTextColor(hdc, text_dark);

    auto Frame = [&](RECT rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FrameRect(hdc, &rect, brush);
        DeleteObject(brush);
    };
    auto MenuButton = [&](int id, RECT rect, const std::wstring& label, COLORREF color) {
        buttons_.push_back(UiButton{rect, id});
        Fill(hdc, rect, color);
        Frame(rect, wood_dark);
        SetTextColor(hdc, RGB(255, 249, 223));
        Text(hdc, rect.left + 14, rect.top + 8, label);
        SetTextColor(hdc, text_dark);
    };
    auto SectionHeader = [&](RECT rect, const std::wstring& label) {
        Fill(hdc, rect, wood_mid);
        Frame(rect, wood_dark);
        SetTextColor(hdc, RGB(255, 246, 214));
        Text(hdc, rect.left + 16, rect.top + 7, label);
        SetTextColor(hdc, text_dark);
    };
    auto Progress = [&](RECT rect, int current, int target, COLORREF color) {
        Fill(hdc, rect, RGB(219, 196, 154));
        const int safe_target = std::max(1, target);
        RECT fill = rect;
        fill.right = rect.left + (rect.right - rect.left) *
                                     std::min(current, safe_target) / safe_target;
        Fill(hdc, fill, color);
        Frame(rect, RGB(130, 93, 55));
    };

    RECT panel{120, 128, 1000, 632};
    Fill(hdc, panel, panel_fill);
    Frame(panel, wood_dark);
    Frame(RECT{124, 132, 996, 628}, RGB(194, 132, 67));

    SectionHeader(RECT{145, 150, 975, 188}, L"\u83dc\u5355\u4e0e\u65e5\u5fd7");
    SetTextColor(hdc, text_muted);
    Text(hdc, 162, 202, L"\u5c06\u8f85\u52a9\u529f\u80fd\u6536\u7eb3\u5230\u8fd9\u91cc\uff0c\u4e0d\u518d\u6324\u5360\u9876\u90e8\u5bfc\u822a\u3002");
    SetTextColor(hdc, text_dark);

    RECT task_panel{145, 235, 525, 535};
    RECT achievement_panel{550, 235, 975, 535};
    Fill(hdc, task_panel, card_fill);
    Fill(hdc, achievement_panel, card_fill);
    Frame(task_panel, wood_dark);
    Frame(achievement_panel, wood_dark);
    SectionHeader(RECT{160, 250, 510, 284}, L"\u4eca\u65e5\u4efb\u52a1");
    SectionHeader(RECT{565, 250, 960, 284}, L"\u6210\u5c31\u8fdb\u5ea6");

    const auto& tasks = game_.DailyTasks().TodayTasks();
    if (tasks.empty()) {
        SetTextColor(hdc, text_muted);
        Text(hdc, 172, 314, L"\u65f6\u95f4\u63a8\u8fdb\u540e\u4f1a\u5237\u65b0\u4eca\u65e5\u4efb\u52a1\u3002");
    } else {
        const int count = std::min(3, static_cast<int>(tasks.size()));
        for (int i = 0; i < count; ++i) {
            const DailyTask& task = tasks[static_cast<std::size_t>(i)];
            const int y = 304 + i * 70;
            SetTextColor(hdc, task.completed ? green : text_dark);
            Text(hdc, 172, y, Utf8ToWide(task.name));
            SetTextColor(hdc, text_muted);
            Text(hdc, 172, y + 24,
                 std::to_wstring(task.current) + L"/" + std::to_wstring(task.target) +
                     L"  +" + std::to_wstring(task.reward_gold) + L"g +" +
                     std::to_wstring(task.reward_exp) + L"xp");
            Progress(RECT{335, y + 27, 492, y + 38}, task.current, task.target,
                     task.completed ? green : amber);
        }
    }

    const auto& achievements = game_.Achievements().All();
    int completed = 0;
    for (const Achievement& achievement : achievements) {
        if (achievement.completed) {
            ++completed;
        }
    }
    Text(hdc, 580, 304,
         L"\u5df2\u5b8c\u6210 " + std::to_wstring(completed) + L"/" +
             std::to_wstring(static_cast<int>(achievements.size())));
    Progress(RECT{765, 309, 940, 320}, completed,
             static_cast<int>(achievements.size()), green);

    int shown = 0;
    for (const Achievement& achievement : achievements) {
        if (shown >= 5) {
            break;
        }
        if (achievement.completed && shown < 3) {
            continue;
        }
        const int y = 342 + shown * 34;
        SetTextColor(hdc, achievement.completed ? green : text_dark);
        Text(hdc, 580, y, Utf8ToWide(achievement.name));
        SetTextColor(hdc, text_muted);
        Text(hdc, 780, y,
             std::to_wstring(achievement.current) + L"/" +
                 std::to_wstring(achievement.target));
        Progress(RECT{855, y + 5, 940, y + 15}, achievement.current, achievement.target,
                 achievement.completed ? green : blue);
        ++shown;
    }
    if (shown == 0) {
        SetTextColor(hdc, text_muted);
        Text(hdc, 580, 350, L"\u6210\u5c31\u5168\u90e8\u5b8c\u6210\uff0c\u5f88\u7a33\u3002");
    }

    MenuButton(kUtilityOpenUnlock, RECT{145, 560, 300, 602}, L"\u6253\u5f00\u89e3\u9501\u9875", amber);
    MenuButton(kUtilityOpenSave, RECT{320, 560, 475, 602}, L"\u6253\u5f00\u5b58\u6863\u9875", blue);
    MenuButton(kUtilityClose, RECT{820, 560, 975, 602}, L"\u5173\u95ed", green);
    SetTextColor(hdc, text_dark);
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
    DrawTextureOrFill(hdc, L"wood", rect, RGB(188, 126, 62));
    SetTextColor(hdc, RGB(255, 250, 230));
    Text(hdc, rect.left + 10, rect.top + 9, text);
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::OnButton(int id) {
    if (id == kBtnNew) {
        game_ = Game::NewGame();
        SwitchScreen(Screen::Farm);
        selected_ranch_facility_id_ = 1;
        ranch_facility_page_ = 0;
        selected_plot_ = 0;
        selected_greenhouse_plot_ = 0;
        farm_show_greenhouse_ = false;
        SetMessage(L"新游戏已开始。");
        return;
    }
    if (id == kBtnContinue) {
        if (TryContinue()) {
            SwitchScreen(Screen::Farm);
        }
        return;
    }
    if (id == kOpenUtilityMenu) {
        utility_menu_open_ = !utility_menu_open_;
        return;
    }
    if (id == kUtilityClose) {
        utility_menu_open_ = false;
        return;
    }
    if (id == kUtilityOpenSave) {
        utility_menu_open_ = false;
        SwitchScreen(Screen::Save);
        return;
    }
    if (id == kUtilityOpenUnlock) {
        utility_menu_open_ = false;
        SwitchScreen(Screen::Unlock);
        return;
    }
    if (id == kTabFarm) {
        SwitchScreen(Screen::Farm);
        return;
    }
    if (id == kTabRanch) {
        SwitchScreen(Screen::Ranch);
        return;
    }
    if (id == kTabWorkshop) {
        SwitchScreen(Screen::Workshop);
        return;
    }
    if (id == kTabOrders) {
        SwitchScreen(Screen::Orders);
        return;
    }
    if (id == kTabWarehouse) {
        SwitchScreen(Screen::Warehouse);
        return;
    }
    if (id == kTabShop) {
        SwitchScreen(Screen::Shop);
        return;
    }
    if (id == kTabFishing) {
        SwitchScreen(Screen::Fishing);
        return;
    }
    if (id == kTabUnlock) {
        SwitchScreen(Screen::Unlock);
        return;
    }
    if (id == kTabSave) {
        SwitchScreen(Screen::Save);
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
    if (id == kFarmModeField) {
        farm_show_greenhouse_ = false;
        return;
    }
    if (id == kFarmModeGreenhouse) {
        farm_show_greenhouse_ = true;
        return;
    }
    if (id == kFishingCast) {
        if (game_.Fishing().CanFish()) {
            game_.Fishing().CastLine(game_.Time().CurrentTick());
            SetMessage(L"\u5df2\u629b\u7aff\uff0c\u7b49\u5f85\u9c7c\u54ac\u94a9\u3002");
        } else if (game_.Fishing().BaitCount() <= 0) {
            SetMessage(L"\u9c7c\u9975\u4e0d\u8db3\uff0c\u8bf7\u5148\u8865\u5145\u9c7c\u9975\u3002");
        } else {
            SetMessage(L"\u6b63\u5728\u9493\u9c7c\uff0c\u8bf7\u7b49\u5f85\u9c7c\u54ac\u94a9\u3002");
        }
        return;
    }
    if (id == kFishingReel) {
        auto reeled = game_.Fishing().TryReelIn(game_.Time().CurrentTick());
        if (reeled.ok()) {
            const int fish_id = game_.Fishing().LastCaughtFish();
            SetMessage(L"\u6536\u83b7 " + FishDisplayName(fish_id) + L"\uff01");
        } else {
            SetMessage(L"\u5dee\u4e00\u70b9\uff0c\u9c7c\u6e9c\u8d70\u4e86\u3002");
        }
        return;
    }
    if (id == kFishingBuyBait) {
        auto bought = game_.Fishing().BuyBait(game_.Player(), 5);
        if (bought.ok()) {
            SetMessage(L"\u5df2\u8865\u5145 5 \u4efd\u9c7c\u9975\u3002");
        } else {
            SetMessage(bought.code);
        }
        return;
    }
    if (id == kFishingUpgradeRod) {
        auto upgraded = game_.Fishing().UpgradeRod(game_.Player());
        if (upgraded.ok()) {
            SetMessage(L"\u9c7c\u7aff\u5347\u7ea7\u6210\u529f\u3002");
        } else {
            SetMessage(upgraded.code);
        }
        return;
    }
    if (id == kFishingSellAll) {
        auto sold = game_.Fishing().SellAllFish(game_.Player());
        if (sold.ok()) {
            SetMessage(L"\u5df2\u51fa\u552e\u9c7c\u7bd3\uff0c\u83b7\u5f97 " +
                       std::to_wstring(sold.value) + L" \u91d1\u5e01\u3002");
        } else {
            SetMessage(sold.code);
        }
        return;
    }

    Result<void> result = Result<void>::success();
    if (id == kPlantWheat) {
        result = farm_show_greenhouse_
                     ? game_.Planting().TryPlantGreenhouseAt(
                           game_.Player(), selected_greenhouse_plot_, ItemId::WheatSeed,
                           game_.Time().CurrentTick())
                     : game_.Planting().TryPlantAt(game_.Player(), selected_plot_,
                                                   ItemId::WheatSeed,
                                                   game_.Time().CurrentTick(),
                                                   game_.Season().Current());
    } else if (id == kPlantCorn) {
        result = farm_show_greenhouse_
                     ? game_.Planting().TryPlantGreenhouseAt(
                           game_.Player(), selected_greenhouse_plot_, ItemId::CornSeed,
                           game_.Time().CurrentTick())
                     : game_.Planting().TryPlantAt(game_.Player(), selected_plot_,
                                                   ItemId::CornSeed,
                                                   game_.Time().CurrentTick(),
                                                   game_.Season().Current());
    } else if (id == kPlantCarrot) {
        result = farm_show_greenhouse_
                     ? game_.Planting().TryPlantGreenhouseAt(
                           game_.Player(), selected_greenhouse_plot_, ItemId::CarrotSeed,
                           game_.Time().CurrentTick())
                     : game_.Planting().TryPlantAt(game_.Player(), selected_plot_,
                                                   ItemId::CarrotSeed,
                                                   game_.Time().CurrentTick(),
                                                   game_.Season().Current());
    } else if (id == kPlantTomato) {
        result = farm_show_greenhouse_
                     ? game_.Planting().TryPlantGreenhouseAt(
                           game_.Player(), selected_greenhouse_plot_, ItemId::TomatoSeed,
                           game_.Time().CurrentTick())
                     : game_.Planting().TryPlantAt(game_.Player(), selected_plot_,
                                                   ItemId::TomatoSeed,
                                                   game_.Time().CurrentTick(),
                                                   game_.Season().Current());
    } else if (id == kWater) {
        result = farm_show_greenhouse_
                     ? game_.Planting().WaterGreenhousePlot(selected_greenhouse_plot_,
                                                            game_.Time().CurrentTick())
                     : game_.Planting().WaterPlot(selected_plot_, game_.Time().CurrentTick());
    } else if (id == kFertilize) {
        result = game_.Planting().ApplyFertilizer(game_.Player(), selected_plot_,
                                                  game_.Time().CurrentTick());
    } else if (id == kHarvest) {
        result = farm_show_greenhouse_
                     ? game_.Planting().HarvestGreenhouse(game_.Player(),
                                                          selected_greenhouse_plot_)
                     : game_.Planting().Harvest(game_.Player(), selected_plot_);
    } else if (id == kExpand) {
        auto expanded = game_.Planting().Expand(game_.Player());
        result = expanded.ok() ? Result<void>::success() : Result<void>::failure(expanded.code);
    } else if (id == kBuildGreenhouse) {
        auto built = game_.Planting().BuildGreenhouse(game_.Player());
        if (built.ok()) {
            selected_greenhouse_plot_ = built.value;
        }
        result = built.ok() ? Result<void>::success() : Result<void>::failure(built.code);
    } else if (id == kBuildCowBarn) {
        auto built = game_.Ranch().BuildFacility(game_.Player(), RanchFacilityKind::CowBarn);
        if (built.ok()) {
            selected_ranch_facility_id_ = built.value;
            const int facility_count = static_cast<int>(game_.Ranch().FacilityViews().size());
            ranch_facility_page_ = (facility_count - 1) / kRanchFacilitiesPerPage;
        }
        result = built.ok() ? Result<void>::success() : Result<void>::failure(built.code);
    } else if (id == kBuildSheepPen) {
        auto built = game_.Ranch().BuildFacility(game_.Player(), RanchFacilityKind::SheepPen);
        if (built.ok()) {
            selected_ranch_facility_id_ = built.value;
            const int facility_count = static_cast<int>(game_.Ranch().FacilityViews().size());
            ranch_facility_page_ = (facility_count - 1) / kRanchFacilitiesPerPage;
        }
        result = built.ok() ? Result<void>::success() : Result<void>::failure(built.code);
    } else if (id == kBuyChicken) {
        auto bought = game_.Ranch().BuyAnimal(
            game_.Player(), SelectedFacility(RanchFacilityKind::ChickenCoop),
            AnimalKind::Chicken);
        result = bought.ok() ? Result<void>::success() : Result<void>::failure(bought.code);
    } else if (id == kBuyCow) {
        auto bought = game_.Ranch().BuyAnimal(game_.Player(),
                                              SelectedFacility(RanchFacilityKind::CowBarn),
                                              AnimalKind::Cow);
        result = bought.ok() ? Result<void>::success() : Result<void>::failure(bought.code);
    } else if (id == kBuySheep) {
        auto bought = game_.Ranch().BuyAnimal(
            game_.Player(), SelectedFacility(RanchFacilityKind::SheepPen), AnimalKind::Sheep);
        result = bought.ok() ? Result<void>::success() : Result<void>::failure(bought.code);
    } else if (id == kFeedSelected) {
        auto fed = game_.Ranch().BatchFeed(game_.Player(), selected_ranch_facility_id_,
                                           game_.Time().CurrentTick());
        if (fed.ok()) {
            SetMessage(L"当前设施已喂食 " + std::to_wstring(fed.value) + L" 只动物。");
        } else {
            SetMessage(fed.code);
        }
        return;
    } else if (id == kHarvestSelected) {
        auto harvested =
            game_.Ranch().BatchHarvest(game_.Player(), selected_ranch_facility_id_);
        if (harvested.ok()) {
            SetMessage(L"当前设施已收获 " + std::to_wstring(harvested.value) + L" 个产品。");
        } else {
            SetMessage(harvested.code);
        }
        return;
    } else if (id == kRanchPagePrevious) {
        ranch_facility_page_ = std::max(0, ranch_facility_page_ - 1);
        return;
    } else if (id == kRanchPageNext) {
        const int facility_count = static_cast<int>(game_.Ranch().FacilityViews().size());
        const int page_count =
            std::max(1, (facility_count + kRanchFacilitiesPerPage - 1) /
                            kRanchFacilitiesPerPage);
        ranch_facility_page_ = std::min(page_count - 1, ranch_facility_page_ + 1);
        return;
    } else if (id >= kSelectRanchFacilityBase &&
               id < kSelectRanchFacilityBase + kRanchFacilitiesPerPage) {
        const int slot = id - kSelectRanchFacilityBase;
        const int index = ranch_facility_page_ * kRanchFacilitiesPerPage + slot;
        const auto facilities = game_.Ranch().FacilityViews();
        if (index >= 0 && index < static_cast<int>(facilities.size())) {
            selected_ranch_facility_id_ =
                facilities[static_cast<std::size_t>(index)].id;
        }
        return;
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
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::ChickenFeed, 1,
                                                   game_.Player().Level());
    } else if (id == kMakeChickenFeed3) {
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::ChickenFeed, 3,
                                                   game_.Player().Level());
    } else if (id == kMakeCowFeed) {
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::CowFeed, 1,
                                                   game_.Player().Level());
    } else if (id == kClaimFeed) {
        result = game_.Workshop().ClaimProduct(game_.Player());
    } else if (id >= kOrderBase && id < kOrderBase + 10) {
        selected_order_ = id - kOrderBase;
    } else if (id == kCompleteOrder) {
        result = game_.Orders().CompleteOrder(game_.Player(), selected_order_,
                                              game_.Time().CurrentTick());
    } else if (id == kAbandonOrder) {
        result = game_.Orders().AbandonOrder(selected_order_, game_.Time().CurrentTick());
    } else if (id == kBuyWheatSeed) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::WheatSeed, 1);
    } else if (id == kBuyCornSeed) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::CornSeed, 1);
    } else if (id == kBuyCarrotSeed) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::CarrotSeed, 1);
    } else if (id == kBuyTomatoSeed) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::TomatoSeed, 1);
    } else if (id == kBuyFertilizer) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::Fertilizer, 1);
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
    } else if (id == kWarehouseSellSelected) {
        result = game_.Player().TrySellItem(selected_warehouse_item_, 1);
    } else if (id == kWarehouseToggleLock) {
        const bool locked = game_.Player().IsItemLocked(selected_warehouse_item_);
        game_.Player().SetItemLocked(selected_warehouse_item_, !locked);
        SetMessage(locked ? L"已解除物品保护。" : L"已保护物品。");
        return;
    } else if (id == kWarehouseUpgrade) {
        result = game_.Player().UpgradeWarehouse();
    } else if (id == kWarehousePagePrevious) {
        warehouse_page_ = std::max(0, warehouse_page_ - 1);
        return;
    } else if (id == kWarehousePageNext) {
        const int item_count = static_cast<int>(game_.Player().InventoryView().size());
        const int page_count =
            std::max(1, (item_count + kWarehouseItemsPerPage - 1) /
                            kWarehouseItemsPerPage);
        warehouse_page_ = std::min(page_count - 1, warehouse_page_ + 1);
        return;
    } else if (id >= kWarehouseItemBase &&
               id < kWarehouseItemBase + kWarehouseItemsPerPage) {
        const int slot = id - kWarehouseItemBase;
        const int index = warehouse_page_ * kWarehouseItemsPerPage + slot;
        const auto inventory = game_.Player().InventoryView();
        if (index >= 0 && index < static_cast<int>(inventory.size())) {
            selected_warehouse_item_ =
                inventory[static_cast<std::size_t>(index)].item;
        }
        return;
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
    } else if (id == kNewGame) {
        game_ = Game::NewGame();
        selected_ranch_facility_id_ = 1;
        ranch_facility_page_ = 0;
        selected_plot_ = 0;
        selected_greenhouse_plot_ = 0;
        farm_show_greenhouse_ = false;
        SetMessage(L"新游戏已开始。");
        return;
    }
    SetMessage(result.code);
}

void FarmWindow::SetMessage(ErrorCode code) {
    message_ = ErrorMessage(code);
    message_is_error_ = code != ErrorCode::Ok;
    message_changed_at_ = GetTickCount64();
    SetTimer(hwnd_, kMessageTimerId, 33, nullptr);
}

void FarmWindow::SetMessage(const std::wstring& text) {
    message_ = text;
    message_is_error_ = false;
    message_changed_at_ = GetTickCount64();
    SetTimer(hwnd_, kMessageTimerId, 33, nullptr);
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

int FarmWindow::SelectedFacility(RanchFacilityKind kind) const {
    for (const RanchFacilityView& facility : game_.Ranch().FacilityViews()) {
        if (facility.id == selected_ranch_facility_id_ && facility.kind == kind) {
            return facility.id;
        }
    }
    return FirstFacility(kind);
}

}  // namespace

int RunFarmApp() {
    FarmWindow window;
    return window.Run();
}

}  // namespace farm::ui

#endif
