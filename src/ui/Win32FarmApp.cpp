#ifdef _WIN32

#include "farm/ui/Win32FarmApp.h"

#include "farm/core/Game.h"
#include "farm/persistence/SaveManager.h"

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace farm::ui {
namespace {

enum class Screen {
    Menu,
    Farm,
    Coop,
    Workshop,
    Orders,
    Warehouse,
    Shop,
    Save,
};

struct Button {
    RECT rect{};
    int id = 0;
    std::wstring text;
};

class FarmWindow {
public:
    int Run();
    ~FarmWindow();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    void Paint(HDC hdc);
    void DrawMenu(HDC hdc);
    void DrawGame(HDC hdc);
    void DrawTabs(HDC hdc);
    void DrawStatus(HDC hdc);
    void DrawFarm(HDC hdc);
    void DrawCoop(HDC hdc);
    void DrawWorkshop(HDC hdc);
    void DrawOrders(HDC hdc);
    void DrawWarehouse(HDC hdc);
    void DrawShop(HDC hdc);
    void DrawSave(HDC hdc);
    void AddButton(HDC hdc, int id, RECT rect, const std::wstring& text);
    void FillRectColor(HDC hdc, RECT rect, COLORREF color);
    void Text(HDC hdc, int x, int y, const std::wstring& text);
    void OnButton(int id);
    void SetMessage(ErrorCode code);
    void SetMessage(const std::wstring& text);
    std::wstring SavePath() const;
    std::wstring Widen(const char* text) const;
    std::wstring ItemLine(ItemId item, int quantity) const;
    bool TryContinue();

    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr;
    Game game_ = Game::NewGame();
    Screen screen_ = Screen::Menu;
    std::wstring message_ = L"准备就绪。";
    std::vector<Button> buttons_;
    int selected_plot_ = 0;
    int selected_order_ = 0;
};

constexpr int kBtnNew = 1;
constexpr int kBtnContinue = 2;
constexpr int kTabFarm = 10;
constexpr int kTabCoop = 11;
constexpr int kTabWorkshop = 12;
constexpr int kTabOrders = 13;
constexpr int kTabWarehouse = 14;
constexpr int kTabShop = 15;
constexpr int kTabSave = 16;
constexpr int kTick = 20;
constexpr int kPause = 21;
constexpr int kSpeed1 = 22;
constexpr int kSpeed2 = 23;
constexpr int kSpeed4 = 24;
constexpr int kPlantWheat = 100;
constexpr int kPlantCorn = 101;
constexpr int kPlantCarrot = 102;
constexpr int kWater = 103;
constexpr int kFertilize = 104;
constexpr int kHarvest = 105;
constexpr int kExpand = 106;
constexpr int kBuyChicken = 200;
constexpr int kFeedAll = 201;
constexpr int kHarvestAll = 202;
constexpr int kFeedOnceBase = 220;
constexpr int kHarvestOnceBase = 240;
constexpr int kMakeFeed = 300;
constexpr int kMakeFeed5 = 301;
constexpr int kClaimFeed = 302;
constexpr int kCompleteOrder = 400;
constexpr int kAbandonOrder = 401;
constexpr int kOrderBase = 420;
constexpr int kBuyWheatSeed = 500;
constexpr int kBuyCornSeed = 501;
constexpr int kBuyCarrotSeed = 502;
constexpr int kBuyFertilizer = 503;
constexpr int kSellWheat = 600;
constexpr int kSellCorn = 601;
constexpr int kSellCarrot = 602;
constexpr int kSellEgg = 603;
constexpr int kSave = 700;
constexpr int kLoad = 701;
constexpr int kNewGame = 702;

std::string Narrow(const std::wstring& text) {
    return std::string(text.begin(), text.end());
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
        case ItemId::Wheat:
            return L"小麦";
        case ItemId::Corn:
            return L"玉米";
        case ItemId::Carrot:
            return L"胡萝卜";
        case ItemId::ChickenFeed:
            return L"鸡饲料";
        case ItemId::Egg:
            return L"鸡蛋";
        case ItemId::Fertilizer:
            return L"肥料";
    }
    return L"未知物品";
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
            return L"该物品已锁定，不能消耗或出售。";
        case ErrorCode::CannotSell:
            return L"该物品不能出售。";
        case ErrorCode::NotASeed:
            return L"这不是种子。";
        case ErrorCode::NotFertilizer:
            return L"这不是肥料。";
        case ErrorCode::SeedNotUnlocked:
            return L"种子尚未解锁。";
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
            return L"设施不存在。";
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
    const ATOM registered = RegisterClassExW(&wc);
    if (registered == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        std::ofstream log("farm_game_startup.log", std::ios::trunc);
        log << "RegisterClassExW failed: " << GetLastError() << "\n";
        MessageBoxW(nullptr, L"农场游戏无法注册窗口类，请查看 farm_game_startup.log。",
                    L"农场游戏", MB_ICONERROR | MB_OK);
        return 1;
    }

    hwnd_ = CreateWindowExW(0, kClassName, L"农场游戏", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                            CW_USEDEFAULT, 1080, 720, nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr) {
        std::ofstream log("farm_game_startup.log", std::ios::trunc);
        log << "CreateWindowExW failed: " << GetLastError() << "\n";
        MessageBoxW(nullptr, L"农场游戏无法创建主窗口，请查看 farm_game_startup.log。",
                    L"农场游戏", MB_ICONERROR | MB_OK);
        return 1;
    }
    font_ = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
    ShowWindow(hwnd_, SW_SHOW);
    SetTimer(hwnd_, 1, 900, nullptr);

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
            Paint(hdc);
            EndPaint(hwnd_, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            const int x = LOWORD(lparam);
            const int y = HIWORD(lparam);
            for (const Button& button : buttons_) {
                if (x >= button.rect.left && x <= button.rect.right && y >= button.rect.top &&
                    y <= button.rect.bottom) {
                    OnButton(button.id);
                    InvalidateRect(hwnd_, nullptr, TRUE);
                    return 0;
                }
            }
            if (screen_ == Screen::Farm && y >= 165 && y < 405 && x >= 40 && x < 520) {
                const int col = (x - 40) / 80;
                const int row = (y - 165) / 80;
                selected_plot_ = row * 6 + col;
                InvalidateRect(hwnd_, nullptr, TRUE);
            }
            return 0;
        }
        case WM_TIMER:
            if (screen_ != Screen::Menu) {
                game_.AdvanceBySpeed();
                game_.AutoSaveIfNeeded(Narrow(SavePath()));
                InvalidateRect(hwnd_, nullptr, TRUE);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd_, msg, wparam, lparam);
}

void FarmWindow::Paint(HDC hdc) {
    buttons_.clear();
    if (font_ != nullptr) {
        SelectObject(hdc, font_);
    }
    RECT area;
    GetClientRect(hwnd_, &area);
    FillRectColor(hdc, area, RGB(238, 226, 190));
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
    Text(hdc, 72, 110, L"C++17 实现的农场经营小游戏。");
    AddButton(hdc, kBtnNew, RECT{70, 170, 260, 215}, L"新游戏");
    AddButton(hdc, kBtnContinue, RECT{70, 230, 260, 275}, L"继续游戏");
    Text(hdc, 70, 330, L"循环：买种子、种植、收割、制作饲料、喂鸡、出售或交付订单。");
    Text(hdc, 70, 365, message_);
}

void FarmWindow::DrawGame(HDC hdc) {
    DrawStatus(hdc);
    DrawTabs(hdc);
    switch (screen_) {
        case Screen::Farm:
            DrawFarm(hdc);
            break;
        case Screen::Coop:
            DrawCoop(hdc);
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
        case Screen::Save:
            DrawSave(hdc);
            break;
        case Screen::Menu:
            break;
    }
    FillRectColor(hdc, RECT{20, 630, 1030, 675}, RGB(250, 242, 214));
    Text(hdc, 35, 644, message_);
}

void FarmWindow::DrawStatus(HDC hdc) {
    FillRectColor(hdc, RECT{0, 0, 1080, 62}, RGB(106, 150, 87));
    SetTextColor(hdc, RGB(255, 250, 230));
    const TimeSnapshot time = game_.Time().Snapshot();
    const WeatherSnapshot weather = game_.Weather().Snapshot();
    std::wstringstream ss;
    ss << L"金币 " << game_.Player().Gold() << L"   等级 " << game_.Player().Level()
       << L"   经验 " << game_.Player().Experience() << L"/"
       << game_.Player().ExpToNextLevel() << L"   第 " << time.day << L" 天 "
       << time.hour << L":00   天气 " << WeatherName(weather.weather) << L"   速度 "
       << SpeedName(time.speed);
    Text(hdc, 24, 20, ss.str());
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::DrawTabs(HDC hdc) {
    AddButton(hdc, kTabFarm, RECT{25, 75, 115, 112}, L"农田");
    AddButton(hdc, kTabCoop, RECT{123, 75, 213, 112}, L"鸡圈");
    AddButton(hdc, kTabWorkshop, RECT{221, 75, 333, 112}, L"饲料坊");
    AddButton(hdc, kTabOrders, RECT{341, 75, 431, 112}, L"订单");
    AddButton(hdc, kTabWarehouse, RECT{439, 75, 549, 112}, L"仓库");
    AddButton(hdc, kTabShop, RECT{557, 75, 647, 112}, L"商店");
    AddButton(hdc, kTabSave, RECT{655, 75, 745, 112}, L"存档");
    AddButton(hdc, kTick, RECT{780, 75, 865, 112}, L"+1刻");
    AddButton(hdc, kPause, RECT{873, 75, 958, 112}, L"暂停");
}

void FarmWindow::DrawFarm(HDC hdc) {
    Text(hdc, 40, 130, L"农田地块");
    const auto plots = game_.Planting().View(game_.Time().CurrentTick());
    for (std::size_t i = 0; i < plots.size(); ++i) {
        const int row = static_cast<int>(i) / 6;
        const int col = static_cast<int>(i) % 6;
        RECT r{40 + col * 80, 165 + row * 80, 105 + col * 80, 230 + row * 80};
        COLORREF color = RGB(139, 94, 52);
        if (plots[i].state == PlotState::Growing) {
            color = plots[i].water == PlotWaterState::Watered ? RGB(84, 152, 80) : RGB(132, 169, 75);
        } else if (plots[i].state == PlotState::Mature) {
            color = RGB(237, 190, 67);
        }
        FillRectColor(hdc, r, color);
        if (static_cast<int>(i) == selected_plot_) {
            HBRUSH border = CreateSolidBrush(RGB(40, 40, 40));
            FrameRect(hdc, &r, border);
            DeleteObject(border);
        }
        std::wstringstream ss;
        ss << static_cast<int>(i) << L" " << ItemName(plots[i].crop);
        Text(hdc, r.left + 5, r.top + 8, ss.str());
        if (plots[i].remaining_ticks > 0) {
            std::wstringstream rt;
            rt << plots[i].remaining_ticks << L"t";
            Text(hdc, r.left + 18, r.top + 35, rt.str());
        }
    }
    Text(hdc, 570, 150, L"选中地块操作");
    AddButton(hdc, kPlantWheat, RECT{570, 190, 740, 228}, L"种小麦");
    AddButton(hdc, kPlantCorn, RECT{570, 236, 740, 274}, L"种玉米");
    AddButton(hdc, kPlantCarrot, RECT{570, 282, 740, 320}, L"种胡萝卜");
    AddButton(hdc, kWater, RECT{760, 190, 910, 228}, L"浇水");
    AddButton(hdc, kFertilize, RECT{760, 236, 910, 274}, L"施肥");
    AddButton(hdc, kHarvest, RECT{760, 282, 910, 320}, L"收割");
    AddButton(hdc, kExpand, RECT{570, 340, 740, 378}, L"扩建地块");
}

void FarmWindow::DrawCoop(HDC hdc) {
    const auto facilities = game_.Ranch().FacilityViews();
    Text(hdc, 40, 140, L"鸡圈");
    if (!facilities.empty()) {
        std::wstringstream ss;
        ss << L"容量 " << facilities[0].animal_count << L"/" << facilities[0].capacity
           << L"   可收鸡蛋 " << facilities[0].ready_count;
        Text(hdc, 40, 175, ss.str());
    }
    AddButton(hdc, kBuyChicken, RECT{40, 220, 190, 258}, L"购买鸡");
    AddButton(hdc, kFeedAll, RECT{205, 220, 355, 258}, L"全部喂食");
    AddButton(hdc, kHarvestAll, RECT{370, 220, 520, 258}, L"全部收获");
    const auto animals = game_.Ranch().AnimalViews(1, game_.Time().CurrentTick());
    for (std::size_t i = 0; i < animals.size(); ++i) {
        const int y = 295 + static_cast<int>(i) * 58;
        std::wstringstream ss;
        ss << L"鸡 #" << animals[i].id << L"  ";
        if (animals[i].state == AnimalState::Idle) {
            ss << L"空闲";
        } else if (animals[i].state == AnimalState::Producing) {
            ss << L"生产中，还剩 " << animals[i].remaining_ticks << L" 刻";
        } else {
            ss << L"鸡蛋可收";
        }
        Text(hdc, 50, y, ss.str());
        AddButton(hdc, kFeedOnceBase + static_cast<int>(i), RECT{520, y - 8, 610, y + 28}, L"喂食");
        AddButton(hdc, kHarvestOnceBase + static_cast<int>(i), RECT{625, y - 8, 735, y + 28},
                  L"收获");
    }
}

void FarmWindow::DrawWorkshop(HDC hdc) {
    const WorkshopView view = game_.Workshop().View();
    Text(hdc, 40, 140, L"饲料坊");
    std::wstringstream ss;
    ss << L"队列 " << view.queue_count << L"/" << view.queue_capacity << L"   货架 "
       << view.shelf_count << L"/" << view.shelf_capacity << L"   当前剩余 "
       << view.active_remaining_ticks << L" 刻";
    Text(hdc, 40, 180, ss.str());
    Text(hdc, 40, 225, L"配方：小麦 x2 -> 鸡饲料 x1");
    AddButton(hdc, kMakeFeed, RECT{40, 275, 195, 315}, L"生产 1 份");
    AddButton(hdc, kMakeFeed5, RECT{210, 275, 365, 315}, L"生产 3 份");
    AddButton(hdc, kClaimFeed, RECT{380, 275, 545, 315}, L"领取饲料");
}

void FarmWindow::DrawOrders(HDC hdc) {
    Text(hdc, 40, 135, L"订单");
    const auto& orders = game_.Orders().Orders();
    for (std::size_t i = 0; i < orders.size(); ++i) {
        const OrderData& order = orders[i];
        const int y = 175 + static_cast<int>(i) * 80;
        RECT card{40, y, 780, y + 60};
        FillRectColor(hdc, card, selected_order_ == static_cast<int>(i) ? RGB(255, 230, 160)
                                                                        : RGB(250, 242, 214));
        std::wstringstream ss;
        ss << L"槽位 " << i << L"  " << ItemName(order.item) << L" x" << order.quantity
           << L"  奖励 " << order.reward_gold << L" 金币 +" << order.reward_exp << L" 经验";
        if (order.state == OrderState::CoolingDown) {
            ss << L"  冷却中";
        }
        Text(hdc, 58, y + 18, ss.str());
        AddButton(hdc, kOrderBase + static_cast<int>(i), RECT{800, y + 10, 885, y + 48},
                  L"选择");
    }
    AddButton(hdc, kCompleteOrder, RECT{40, 520, 190, 560}, L"交付订单");
    AddButton(hdc, kAbandonOrder, RECT{205, 520, 355, 560}, L"放弃订单");
}

void FarmWindow::DrawWarehouse(HDC hdc) {
    std::wstringstream cap;
    cap << L"仓库 " << game_.Player().WarehouseUsed() << L"/"
        << game_.Player().WarehouseCapacity();
    Text(hdc, 40, 140, cap.str());
    int y = 180;
    for (const InventoryItemView& item : game_.Player().InventoryView()) {
        Text(hdc, 55, y, ItemLine(item.item, item.quantity));
        y += 30;
    }
    AddButton(hdc, kSellWheat, RECT{520, 180, 660, 218}, L"卖小麦");
    AddButton(hdc, kSellCorn, RECT{520, 228, 660, 266}, L"卖玉米");
    AddButton(hdc, kSellCarrot, RECT{520, 276, 660, 314}, L"卖胡萝卜");
    AddButton(hdc, kSellEgg, RECT{520, 324, 660, 362}, L"卖鸡蛋");
}

void FarmWindow::DrawShop(HDC hdc) {
    Text(hdc, 40, 140, L"商店");
    AddButton(hdc, kBuyWheatSeed, RECT{50, 190, 230, 230}, L"小麦种子 x1");
    AddButton(hdc, kBuyCornSeed, RECT{50, 240, 230, 280}, L"玉米种子 x1");
    AddButton(hdc, kBuyCarrotSeed, RECT{50, 290, 230, 330}, L"胡萝卜种子 x1");
    AddButton(hdc, kBuyFertilizer, RECT{50, 340, 230, 380}, L"肥料 x1");
}

void FarmWindow::DrawSave(HDC hdc) {
    Text(hdc, 40, 140, L"存档 / 读档");
    Text(hdc, 40, 180, L"存档文件：saves/save01.farm");
    AddButton(hdc, kSave, RECT{50, 230, 200, 270}, L"手动保存");
    AddButton(hdc, kLoad, RECT{220, 230, 370, 270}, L"读取存档");
    AddButton(hdc, kNewGame, RECT{390, 230, 540, 270}, L"新游戏");
}

void FarmWindow::AddButton(HDC hdc, int id, RECT rect, const std::wstring& text) {
    buttons_.push_back(Button{rect, id, text});
    FillRectColor(hdc, rect, RGB(188, 126, 62));
    FrameRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetTextColor(hdc, RGB(255, 250, 230));
    Text(hdc, rect.left + 12, rect.top + 11, text);
    SetTextColor(hdc, RGB(58, 54, 39));
}

void FarmWindow::FillRectColor(HDC hdc, RECT rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void FarmWindow::Text(HDC hdc, int x, int y, const std::wstring& text) {
    TextOutW(hdc, x, y, text.c_str(), static_cast<int>(text.size()));
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
        SetMessage(L"时间推进了一刻。");
        return;
    }
    if (id == kPause) {
        if (game_.Time().IsPaused()) {
            game_.Time().Resume();
        } else {
            game_.Time().Pause();
        }
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
    } else if (id == kBuyChicken) {
        auto bought = game_.Ranch().BuyAnimal(game_.Player(), 1, AnimalKind::Chicken);
        result = bought.ok() ? Result<void>::success() : Result<void>::failure(bought.code);
    } else if (id == kFeedAll) {
        auto fed = game_.Ranch().BatchFeed(game_.Player(), 1, game_.Time().CurrentTick());
        result = fed.ok() ? Result<void>::success() : Result<void>::failure(fed.code);
    } else if (id == kHarvestAll) {
        auto got = game_.Ranch().BatchHarvest(game_.Player(), 1);
        result = got.ok() ? Result<void>::success() : Result<void>::failure(got.code);
    } else if (id >= kFeedOnceBase && id < kFeedOnceBase + 20) {
        const auto animals = game_.Ranch().AnimalViews(1, game_.Time().CurrentTick());
        const int index = id - kFeedOnceBase;
        if (index < static_cast<int>(animals.size())) {
            result = game_.Ranch().FeedAnimal(game_.Player(), 1, animals[index].id,
                                              game_.Time().CurrentTick());
        }
    } else if (id >= kHarvestOnceBase && id < kHarvestOnceBase + 20) {
        const auto animals = game_.Ranch().AnimalViews(1, game_.Time().CurrentTick());
        const int index = id - kHarvestOnceBase;
        if (index < static_cast<int>(animals.size())) {
            result = game_.Ranch().HarvestAnimal(game_.Player(), 1, animals[index].id);
        }
    } else if (id == kMakeFeed) {
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::ChickenFeed, 1);
    } else if (id == kMakeFeed5) {
        result = game_.Workshop().StartProduction(game_.Player(), RecipeId::ChickenFeed, 3);
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
    } else if (id == kBuyFertilizer) {
        result = game_.Shop().BuyItem(game_.Player(), ItemId::Fertilizer, 1);
    } else if (id == kSellWheat) {
        result = game_.Player().TrySellItem(ItemId::Wheat, 1);
    } else if (id == kSellCorn) {
        result = game_.Player().TrySellItem(ItemId::Corn, 1);
    } else if (id == kSellCarrot) {
        result = game_.Player().TrySellItem(ItemId::Carrot, 1);
    } else if (id == kSellEgg) {
        result = game_.Player().TrySellItem(ItemId::Egg, 1);
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

void FarmWindow::SetMessage(ErrorCode code) { message_ = ErrorMessage(code); }

void FarmWindow::SetMessage(const std::wstring& text) { message_ = text; }

std::wstring FarmWindow::SavePath() const { return L"saves/save01.farm"; }

std::wstring FarmWindow::Widen(const char* text) const {
    std::string s(text);
    return std::wstring(s.begin(), s.end());
}

std::wstring FarmWindow::ItemLine(ItemId item, int quantity) const {
    std::wstringstream ss;
    ss << ItemName(item) << L" x" << quantity;
    if (game_.Player().IsItemLocked(item)) {
        ss << L"（已锁定）";
    }
    return ss.str();
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

}  // namespace

int RunFarmApp() {
    FarmWindow window;
    return window.Run();
}

}  // namespace farm::ui

#endif
