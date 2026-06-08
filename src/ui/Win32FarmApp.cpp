#ifdef _WIN32

#include "farm/ui/Win32FarmApp.h"

#include "farm/core/Game.h"
#include "farm/core/UnlockGraph.h"
#include "farm/persistence/SaveManager.h"
#include "farm/ui/TextureManager.h"

#include <windows.h>

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
constexpr int kTabUnlock = 16;
constexpr int kTabSave = 17;
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
    int selected_order_ = 0;
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
        case WM_LBUTTONDOWN: {
            const int x = LOWORD(lparam);
            const int y = HIWORD(lparam);
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
    HDC memory = CreateCompatibleDC(target);
    HBITMAP bitmap = CreateCompatibleBitmap(target, width, height);
    HGDIOBJ old = SelectObject(memory, bitmap);
    Paint(memory);
    BitBlt(target, 0, 0, width, height, memory, 0, 0, SRCCOPY);
    SelectObject(memory, old);
    DeleteObject(bitmap);
    DeleteDC(memory);
}

void FarmWindow::Paint(HDC hdc) {
    buttons_.clear();
    if (font_ != nullptr) {
        SelectObject(hdc, font_);
    }
    RECT area;
    GetClientRect(hwnd_, &area);
    DrawTextureOrFill(hdc, L"grass", area, RGB(238, 226, 190));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(58, 54, 39));
    if (screen_ == Screen::Menu) {
        DrawMenu(hdc);
    } else {
        DrawGame(hdc);
    }
}

void FarmWindow::DrawMenu(HDC hdc) {
    Text(hdc, 70, 70, L"农场游戏");
    Text(hdc, 72, 110, L"1 秒现实时间 = 2 分钟游戏时间。通过 DAG 解锁种子、土地和动物。");
    Button(hdc, kBtnNew, RECT{70, 170, 260, 215}, L"新游戏");
    Button(hdc, kBtnContinue, RECT{70, 230, 260, 275}, L"继续游戏");
    Text(hdc, 70, 330, L"目标：种植、加工饲料、养殖、交付订单、升级并解锁更多内容。");
    Text(hdc, 70, 365, message_);
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
        case Screen::Save:
            DrawSave(hdc);
            break;
        case Screen::Menu:
            break;
    }
    Fill(hdc, RECT{20, 630, 1060, 675}, RGB(250, 242, 214));
    Text(hdc, 35, 644, message_);
}

void FarmWindow::DrawStatus(HDC hdc) {
    DrawTextureOrFill(hdc, L"top_bar", RECT{0, 0, 1120, 62}, RGB(106, 150, 87));
    SetTextColor(hdc, RGB(255, 250, 230));
    const TimeSnapshot time = game_.Time().Snapshot();
    const WeatherSnapshot weather = game_.Weather().Snapshot();
    DrawTextureOrFill(hdc, L"item_coin", RECT{22, 14, 46, 38}, RGB(237, 190, 67));
    DrawTextureOrFill(hdc, WeatherTextureKey(weather.weather), RECT{650, 12, 690, 52},
                      RGB(198, 218, 128));
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
    Button(hdc, kTabFarm, RECT{25, 75, 100, 112}, L"农田");
    Button(hdc, kTabRanch, RECT{106, 75, 181, 112}, L"牧场");
    Button(hdc, kTabWorkshop, RECT{187, 75, 282, 112}, L"饲料坊");
    Button(hdc, kTabOrders, RECT{288, 75, 363, 112}, L"订单");
    Button(hdc, kTabWarehouse, RECT{369, 75, 444, 112}, L"仓库");
    Button(hdc, kTabShop, RECT{450, 75, 525, 112}, L"商店");
    Button(hdc, kTabUnlock, RECT{531, 75, 606, 112}, L"解锁");
    Button(hdc, kTabSave, RECT{612, 75, 687, 112}, L"存档");
    Button(hdc, kTick, RECT{735, 75, 810, 112}, L"+2分");
    Button(hdc, kPause, RECT{816, 75, 891, 112}, L"暂停");
    Button(hdc, kSpeed1, RECT{897, 75, 957, 112}, L"1x");
    Button(hdc, kSpeed2, RECT{963, 75, 1023, 112}, L"2x");
    Button(hdc, kSpeed4, RECT{1029, 75, 1089, 112}, L"4x");
}

void FarmWindow::DrawFarm(HDC hdc) {
    Text(hdc, 40, 130, L"农田地块");
    const auto plots = game_.Planting().View(game_.Time().CurrentTick());
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
    Text(hdc, 570, 145, L"选中地块操作");
    Button(hdc, kPlantWheat, RECT{570, 185, 720, 223}, L"种小麦");
    Button(hdc, kPlantCorn, RECT{570, 231, 720, 269}, L"种玉米");
    Button(hdc, kPlantCarrot, RECT{570, 277, 720, 315}, L"种胡萝卜");
    Button(hdc, kPlantTomato, RECT{570, 323, 720, 361}, L"种番茄");
    Button(hdc, kWater, RECT{750, 185, 900, 223}, L"浇水");
    Button(hdc, kFertilize, RECT{750, 231, 900, 269}, L"施肥");
    Button(hdc, kHarvest, RECT{750, 277, 900, 315}, L"收割");
    Button(hdc, kExpand, RECT{750, 323, 900, 361}, L"购买土地");
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
            if (animal.state == AnimalState::Ready) {
                Text(hdc, animal_x + 4, y + 74, L"可收");
            }
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
    const WorkshopView view = game_.Workshop().View();
    Text(hdc, 40, 140, L"饲料坊");
    DrawTextureOrFill(hdc, L"building_feed_mill", RECT{45, 175, 165, 295}, RGB(190, 134, 84));
    std::wstringstream ss;
    ss << L"队列 " << view.queue_count << L"/" << view.queue_capacity << L"   货架 "
       << view.shelf_count << L"/" << view.shelf_capacity << L"   鸡饲料 "
       << view.chicken_feed_shelf << L"   牛饲料 " << view.cow_feed_shelf << L"   当前剩余 "
       << view.active_remaining_ticks * kGameMinutesPerTick << L" 分钟";
    Text(hdc, 190, 180, ss.str());
    DrawTextureOrFill(hdc, L"item_feed", RECT{190, 220, 238, 268}, RGB(226, 196, 126));
    DrawTextureOrFill(hdc, L"item_wheat", RECT{265, 220, 313, 268}, RGB(226, 196, 126));
    DrawTextureOrFill(hdc, L"item_corn", RECT{340, 220, 388, 268}, RGB(226, 196, 126));
    DrawTextureOrFill(hdc, L"item_carrot", RECT{415, 220, 463, 268}, RGB(226, 196, 126));
    Text(hdc, 190, 285, L"配方：小麦 x2 -> 鸡饲料；玉米 x2 + 胡萝卜 x1 -> 牛饲料");
    Button(hdc, kMakeChickenFeed, RECT{190, 335, 345, 375}, L"鸡饲料 x1");
    Button(hdc, kMakeChickenFeed3, RECT{360, 335, 515, 375}, L"鸡饲料 x3");
    Button(hdc, kMakeCowFeed, RECT{530, 335, 685, 375}, L"牛饲料 x1");
    Button(hdc, kClaimFeed, RECT{700, 335, 855, 375}, L"领取饲料");
}

void FarmWindow::DrawOrders(HDC hdc) {
    Text(hdc, 40, 135, L"订单");
    const auto& orders = game_.Orders().Orders();
    for (std::size_t i = 0; i < orders.size(); ++i) {
        const OrderData& order = orders[i];
        const int y = 175 + static_cast<int>(i) * 80;
        RECT card{40, y, 780, y + 60};
        DrawTextureOrFill(hdc, L"order_card", card,
                          selected_order_ == static_cast<int>(i) ? RGB(255, 230, 160)
                                                                 : RGB(250, 242, 214));
        DrawTextureOrFill(hdc, ItemTextureKey(order.item), RECT{55, y + 10, 95, y + 50},
                          RGB(226, 196, 126));
        std::wstringstream ss;
        ss << L"槽位 " << i << L"  " << ItemName(order.item) << L" x" << order.quantity
           << L"  奖励 " << order.reward_gold << L" 金币 +" << order.reward_exp << L" 经验";
        if (order.state == OrderState::CoolingDown) {
            ss << L"  冷却中";
        }
        Text(hdc, 110, y + 18, ss.str());
        Button(hdc, kOrderBase + static_cast<int>(i), RECT{800, y + 10, 885, y + 48}, L"选择");
    }
    Button(hdc, kCompleteOrder, RECT{40, 520, 190, 560}, L"交付订单");
    Button(hdc, kAbandonOrder, RECT{205, 520, 355, 560}, L"放弃订单");
}

void FarmWindow::DrawWarehouse(HDC hdc) {
    std::wstringstream cap;
    cap << L"仓库 " << game_.Player().WarehouseUsed() << L"/"
        << game_.Player().WarehouseCapacity();
    Text(hdc, 40, 140, cap.str());
    DrawTextureOrFill(hdc, L"building_warehouse", RECT{45, 175, 155, 285}, RGB(190, 134, 84));
    int y = 180;
    for (const InventoryItemView& item : game_.Player().InventoryView()) {
        RECT slot{190, y - 7, 235, y + 38};
        DrawTextureOrFill(hdc, L"inventory_slot", slot, RGB(250, 242, 214));
        DrawTextureOrFill(hdc, ItemTextureKey(item.item), RECT{197, y, 228, y + 31},
                          RGB(226, 196, 126));
        Text(hdc, 250, y + 5, ItemLine(item.item, item.quantity));
        y += 30;
    }
    Button(hdc, kSellWheat, RECT{520, 180, 660, 218}, L"卖小麦");
    Button(hdc, kSellCorn, RECT{520, 228, 660, 266}, L"卖玉米");
    Button(hdc, kSellCarrot, RECT{520, 276, 660, 314}, L"卖胡萝卜");
    Button(hdc, kSellTomato, RECT{520, 324, 660, 362}, L"卖番茄");
    Button(hdc, kSellEgg, RECT{680, 180, 820, 218}, L"卖鸡蛋");
    Button(hdc, kSellMilk, RECT{680, 228, 820, 266}, L"卖牛奶");
    Button(hdc, kSellWool, RECT{680, 276, 820, 314}, L"卖羊毛");
}

void FarmWindow::DrawShop(HDC hdc) {
    Text(hdc, 40, 140, L"商店");
    DrawTextureOrFill(hdc, L"building_shop", RECT{45, 175, 165, 295}, RGB(190, 134, 84));
    const std::vector<std::pair<int, ItemId>> goods = {
        {kBuyWheatSeed, ItemId::WheatSeed},   {kBuyCornSeed, ItemId::CornSeed},
        {kBuyCarrotSeed, ItemId::CarrotSeed}, {kBuyTomatoSeed, ItemId::TomatoSeed},
        {kBuyFertilizer, ItemId::Fertilizer},
    };
    int y = 185;
    for (const auto& [button_id, item] : goods) {
        RECT card{210, y, 520, y + 58};
        DrawTextureOrFill(hdc, L"shop_card", card, RGB(250, 242, 214));
        DrawTextureOrFill(hdc, ItemTextureKey(item), RECT{222, y + 8, 264, y + 50},
                          RGB(226, 196, 126));
        std::wstringstream label;
        label << ItemName(item) << L" x1";
        Button(hdc, button_id, RECT{285, y + 10, 470, y + 48}, label.str());
        y += 68;
    }
    Text(hdc, 570, 200, L"锁定的种子需要先在“解锁”页购买节点。");
}

void FarmWindow::DrawUnlock(HDC hdc) {
    Text(hdc, 40, 135, L"DAG 解锁树");
    int y = 175;
    int index = 0;
    for (const UnlockNode& node : UnlockGraph::Nodes()) {
        std::wstringstream ss;
        ss << Utf8ToWide(node.name) << L"  等级 " << node.required_level << L"  费用 "
           << node.gold_cost << L"  状态 ";
        if (game_.Player().IsUnlocked(node.id)) {
            ss << L"已解锁";
        } else if (game_.Player().CanUnlock(node.id)) {
            ss << L"可解锁";
        } else {
            ss << L"前置未满足";
        }
        Text(hdc, 50, y, ss.str());
        Button(hdc, kUnlockBase + index, RECT{650, y - 8, 790, y + 28}, L"解锁");
        y += 38;
        ++index;
    }
}

void FarmWindow::DrawSave(HDC hdc) {
    Text(hdc, 40, 140, L"存档 / 读档");
    Text(hdc, 40, 180, L"存档文件：saves/save01.farm");
    Button(hdc, kSave, RECT{50, 230, 200, 270}, L"手动保存");
    Button(hdc, kLoad, RECT{220, 230, 370, 270}, L"读取存档");
    Button(hdc, kNewGame, RECT{390, 230, 540, 270}, L"新游戏");
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
    FrameRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetTextColor(hdc, RGB(255, 250, 230));
    Text(hdc, rect.left + 10, rect.top + 9, text);
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::OnButton(int id) {
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
        result = game_.Planting().TryPlantAt(game_.Player(), selected_plot_, ItemId::WheatSeed,
                                             game_.Time().CurrentTick());
    } else if (id == kPlantCorn) {
        result = game_.Planting().TryPlantAt(game_.Player(), selected_plot_, ItemId::CornSeed,
                                             game_.Time().CurrentTick());
    } else if (id == kPlantCarrot) {
        result = game_.Planting().TryPlantAt(game_.Player(), selected_plot_, ItemId::CarrotSeed,
                                             game_.Time().CurrentTick());
    } else if (id == kPlantTomato) {
        result = game_.Planting().TryPlantAt(game_.Player(), selected_plot_, ItemId::TomatoSeed,
                                             game_.Time().CurrentTick());
    } else if (id == kWater) {
        result = game_.Planting().WaterPlot(selected_plot_, game_.Time().CurrentTick());
    } else if (id == kFertilize) {
        result = game_.Planting().ApplyFertilizer(game_.Player(), selected_plot_,
                                                  game_.Time().CurrentTick());
    } else if (id == kHarvest) {
        result = game_.Planting().Harvest(game_.Player(), selected_plot_);
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
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::ChickenFeed, 1);
    } else if (id == kMakeChickenFeed3) {
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::ChickenFeed, 3);
    } else if (id == kMakeCowFeed) {
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::CowFeed, 1);
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
        SetMessage(L"新游戏已开始。");
        return;
    }
    SetMessage(result.code);
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

}  // namespace

int RunFarmApp() {
    FarmWindow window;
    return window.Run();
}

}  // namespace farm::ui

#endif
