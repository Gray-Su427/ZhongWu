#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "SimpleIni.h"
#include <iostream>
#include "unit.h"
#include "unit_types.h"
#include "DisplayConfig.h"
#include "GameData.h"
#include "board.h"
#include "game_phase.h"
#include "game_renderer.h"
#include <filesystem>
#include <memory>

enum class AppState {
    Start,
    Setting,
    Menu,
    Playing,
    Paused,
    End
};

// ==================== 全局游戏对象 ====================
static GamePhaseNS::GameManager g_gameManager;     // 游戏管理器（阶段控制、战斗逻辑）
static RendererNS::GameRenderer g_renderer;        // 渲染器（棋盘绘制、坐标转换）
static bool g_gameInitialized = false;             // 游戏是否已初始化（防止重复初始化）

// ==================== 拖拽状态 ====================
// 管理玩家拖拽单位的完整生命周期：按下→移动→释放
struct DragState {
    bool active = false;                           // 是否正在拖拽
    BoardNS::Placement source;                     // 拖拽来源位置（棋盘格子或Bench槽位）
    std::unique_ptr<Unit> unit;                    // 被拖拽的单位（临时从棋盘/Bench移出）
    sf::Vector2f mousePos{0.f, 0.f};              // 当前鼠标位置

    // 重置拖拽状态，释放单位所有权
    void reset() {
        active = false;
        source = {};
        unit.reset();
    }
};
static DragState g_drag;

// ==================== 初始化游戏 ====================
// 首次进入游戏时调用，添加测试单位到Bench并生成初始敌方
static void initGame() {
    if (!g_gameInitialized) {
        g_gameManager.addTestUnits();
        g_gameInitialized = true;
    }
}

// ==================== 渲染与处理启动界面 ====================
AppState updateAndRenderStartScreen(sf::RenderWindow& window, const std::vector<sf::Event>& /*events*/) {
    AppState nextState = AppState::Start;

    ImGui::SetNextWindowPos(ImVec2(
        window.getSize().x * 0.5f,
        window.getSize().y * 0.4f
    ), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("欢迎来到游戏", nullptr, 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove);

    if (ImGui::Button("开始游戏", ImVec2(200, 40))) {
        nextState = AppState::Menu;
    }
    if (ImGui::Button("设置", ImVec2(200, 40))) {
        nextState = AppState::Setting;
    }
    if (ImGui::Button("退出游戏", ImVec2(200, 40))) {
        window.close();
    }
    ImGui::End();

    return nextState;
}

// ==================== 渲染与处理设置界面 ====================
AppState updateAndRenderSettingScreen(sf::RenderWindow& window, const std::vector<sf::Event>& /*events*/){
    AppState nextstate = AppState::Setting;

    ImGui::SetNextWindowPos(ImVec2(
        window.getSize().x * 0.5f,
        window.getSize().y * 0.4f
    ), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("设置", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize);
    if (ImGui::Button("保存并退出", ImVec2(200, 40))){
        nextstate = AppState::Start;
    }
    ImGui::End();

    return nextstate;
}

// ==================== 渲染与处理菜单界面 ====================
AppState updateAndRenderMenuScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
    AppState nextState = AppState::Menu;

    for (const auto& event : events) {
        if (event.is<sf::Event::KeyPressed>() 
            && event.getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape) {
            nextState = AppState::Start;
        }
    }

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::Begin("#返回键", nullptr, 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground);

    if (ImGui::Button("返回")) {
        nextState = AppState::Start;
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(
        window.getSize().x * 0.5f,
        window.getSize().y * 0.5f
    ), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("菜单", nullptr, 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground);

    if (ImGui::Button("故事模式", ImVec2(200, 40))) {
        // 还没有完成
    }
    if (ImGui::Button("无尽模式", ImVec2(200, 40))) {
        initGame();
        nextState = AppState::Playing; 
    }
    ImGui::End();

    return nextState;
}

// ==================== 处理拖拽逻辑 ====================
// 开始拖拽：检测鼠标位置是否命中玩家单位，若是则从棋盘/Bench移出并进入拖拽状态
static void handleDragStart(const sf::Vector2f& mousePos) {
    if (g_gameManager.getCurrentPhase() != GamePhaseNS::Phase::Prep) return;
    if (g_drag.active) return;

    auto& board = g_gameManager.getBoard();
    BoardNS::Placement hit = g_renderer.hitTest(mousePos, board);
    if (!hit.isValid()) return;

    // 只能拖拽玩家单位
    Unit* unit = nullptr;
    if (hit.type == BoardNS::Placement::Board) {
        unit = board.getUnitOnBoard(hit.gridPos);
    } else if (hit.type == BoardNS::Placement::Bench) {
        unit = board.getUnitOnBench(hit.benchSlot.index);
    }

    if (!unit || unit->owner() != Unit::Owner::PlayerCtrl) return;

    g_drag.source = hit;
    g_drag.mousePos = mousePos;
    g_drag.active = true;

    // 从棋盘/Bench移出
    if (hit.type == BoardNS::Placement::Board) {
        g_drag.unit = board.removeFromBoard(hit.gridPos);
    } else {
        g_drag.unit = board.removeFromBench(hit.benchSlot.index);
    }
}

// 拖拽移动：更新鼠标位置（用于渲染拖拽中的单位）
static void handleDragMove(const sf::Vector2f& mousePos) {
    if (!g_drag.active) return;
    g_drag.mousePos = mousePos;
}

// 结束拖拽：检测放置目标（棋盘格子或Bench槽位），尝试放置或交换，失败则放回原位
static void handleDragEnd(const sf::Vector2f& mousePos) {
    if (!g_drag.active || !g_drag.unit) {
        g_drag.reset();
        return;
    }

    auto& board = g_gameManager.getBoard();

    // 检测放置目标
    BoardNS::GridPos targetCell = g_renderer.screenToBoardCell(mousePos);
    int targetSlot = g_renderer.screenToBenchSlot(mousePos);

    bool placed = false;

    if (targetCell.isValid()) {
        // 禁止在敌方半场（col >= 4）放置玩家单位
        if (targetCell.col >= board.getCols() / 2) {
            // 目标在敌方半场，不允许放置
        } else if (board.isBoardCellEmpty(targetCell)) {
            placed = board.placeOnBoard(targetCell, std::move(g_drag.unit));
        } else {
            // 目标有单位，尝试交换
            auto existing = board.removeFromBoard(targetCell);
            if (existing && existing->owner() == Unit::Owner::PlayerCtrl) {
                // 把现有单位放到来源位置
                if (g_drag.source.type == BoardNS::Placement::Board) {
                    board.placeOnBoard(g_drag.source.gridPos, std::move(existing));
                } else if (g_drag.source.type == BoardNS::Placement::Bench) {
                    board.placeOnBench(g_drag.source.benchSlot.index, std::move(existing));
                }
                placed = board.placeOnBoard(targetCell, std::move(g_drag.unit));
            } else {
                // 目标是敌方单位或空，还原
                if (existing) board.placeOnBoard(targetCell, std::move(existing));
            }
        }
    } else if (targetSlot >= 0) {
        // 放到Bench上
        if (board.isBenchSlotEmpty(targetSlot)) {
            placed = board.placeOnBench(targetSlot, std::move(g_drag.unit));
        }
    }

    // 如果放置失败，放回原位
    if (!placed && g_drag.unit) {
        if (g_drag.source.type == BoardNS::Placement::Board) {
            board.placeOnBoard(g_drag.source.gridPos, std::move(g_drag.unit));
        } else if (g_drag.source.type == BoardNS::Placement::Bench) {
            board.placeOnBench(g_drag.source.benchSlot.index, std::move(g_drag.unit));
        }
    }

    g_drag.reset();
}

// ==================== 渲染与处理游戏界面 ====================
// 游戏主界面：处理鼠标拖拽、更新游戏逻辑、渲染棋盘/Bench/单位、显示ImGui面板和悬浮提示
AppState updateAndRenderPlayingScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events, float dt) {
    AppState nextState = AppState::Playing;
    
    for (const auto& event : events) {
        if (event.is<sf::Event::KeyPressed>()
            && event.getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape) {
            nextState = AppState::Paused;
        }

        // 鼠标拖拽处理
        if (auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mb->button == sf::Mouse::Button::Left) {
                sf::Vector2f mousePos(mb->position.x, mb->position.y);
                handleDragStart(mousePos);
            }
        }
        if (auto* mb = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (mb->button == sf::Mouse::Button::Left) {
                sf::Vector2f mousePos(mb->position.x, mb->position.y);
                handleDragEnd(mousePos);
            }
        }
        if (event.is<sf::Event::MouseMoved>()) {
            auto* mm = event.getIf<sf::Event::MouseMoved>();
            sf::Vector2f mousePos(mm->position.x, mm->position.y);
            handleDragMove(mousePos);
        }
    }

    // 更新游戏逻辑
    g_gameManager.update(dt);

    // 更新渲染布局（每帧都更新，以响应窗口大小变化）
    g_renderer.updateLayout(window);

    // 渲染棋盘、Bench和单位
    auto& board = g_gameManager.getBoard();
    g_renderer.renderAll(window, board);

    // 渲染拖拽中的单位
    if (g_drag.active && g_drag.unit) {
        g_renderer.renderDraggedUnit(window, g_drag.unit.get(), g_drag.mousePos);
    }

    // 渲染鼠标悬停的单位信息（ImGui tooltip）
    if (!g_drag.active) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos(mousePixelPos.x, mousePixelPos.y);
        BoardNS::Placement hover = g_renderer.hitTest(mousePos, board);
        if (hover.isValid()) {
            Unit* hoverUnit = nullptr;
            if (hover.type == BoardNS::Placement::Board) {
                hoverUnit = board.getUnitOnBoard(hover.gridPos);
            } else {
                hoverUnit = board.getUnitOnBench(hover.benchSlot.index);
            }
            if (hoverUnit) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", hoverUnit->getUnitName().c_str());
                ImGui::Separator();
                ImGui::Text("HP: %d / %d", hoverUnit->getHP(), hoverUnit->getMaxHP());
                ImGui::Text("ATK: %d", hoverUnit->getATK());
                ImGui::Text("Range: %.0f", hoverUnit->getRange());
                if (hoverUnit->getMaxMana() > 0) {
                    ImGui::Text("Mana: %d / %d", hoverUnit->getMana(), hoverUnit->getMaxMana());
                }
                ImGui::Text("Owner: %s",
                    hoverUnit->owner() == Unit::Owner::PlayerCtrl ? "Player" :
                    hoverUnit->owner() == Unit::Owner::EnemyCtrl ? "Enemy" : "Neutral");
                if (!hoverUnit->getTraits().empty()) {
                    ImGui::Text("Traits:");
                    for (const auto& t : hoverUnit->getTraits()) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", t.c_str());
                    }
                }
                ImGui::EndTooltip();
            }
        }
    }

    // --- 右侧UI面板（ImGui） ---
    float panelX = g_renderer.getLayout().availWidth + 10.f;
    ImGui::SetNextWindowPos(ImVec2(panelX, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(240, static_cast<float>(window.getSize().y) - 20), ImGuiCond_Always);
    ImGui::Begin("游戏面板", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize);

    // 阶段信息
    const char* phaseStr = "未知";
    switch (g_gameManager.getCurrentPhase()) {
        case GamePhaseNS::Phase::Prep:   phaseStr = "准备阶段"; break;
        case GamePhaseNS::Phase::Combat: phaseStr = "战斗阶段"; break;
        case GamePhaseNS::Phase::Resolve:phaseStr = "结算阶段"; break;
    }
    ImGui::Text("第 %d 轮 - %s", g_gameManager.getCurrentRound(), phaseStr);
    ImGui::Text("玩家HP: %d", g_gameManager.getPlayerHP());
    ImGui::Separator();

    // 准备阶段操作
    if (g_gameManager.getCurrentPhase() == GamePhaseNS::Phase::Prep) {
        if (ImGui::Button("开始战斗", ImVec2(200, 35))) {
            g_gameManager.nextPhase();
        }
        ImGui::Text("场上: %d 个单位", board.getPlayerUnitCountOnBoard());
        ImGui::Text("Bench: %d 个单位", board.getBenchUnitCount());
    }

    // 战斗阶段信息
    if (g_gameManager.getCurrentPhase() == GamePhaseNS::Phase::Combat) {
        ImGui::Text("战斗进行中...");
        auto enemies = board.getEnemyUnitsOnBoard();
        auto players = board.getPlayerUnitsOnBoard();
        int aliveEnemies = 0, alivePlayers = 0;
        for (auto* u : enemies) if (u->isAlive()) aliveEnemies++;
        for (auto* u : players) if (u->isAlive()) alivePlayers++;
        ImGui::Text("存活玩家单位: %d", alivePlayers);
        ImGui::Text("存活敌方单位: %d", aliveEnemies);
    }

    // 结算阶段操作
    if (g_gameManager.getCurrentPhase() == GamePhaseNS::Phase::Resolve) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "回合结束！");
        if (ImGui::Button("下一轮", ImVec2(200, 35))) {
            g_gameManager.nextPhase();
        }
    }

    ImGui::Separator();

    // 调试工具
    if (ImGui::CollapsingHeader("调试工具")) {
        if (ImGui::Button("添加测试单位到Bench")) {
            auto w = std::make_unique<WarriorUnit>();
            w->setOwner(Unit::Owner::PlayerCtrl);
            int slot = board.findEmptyBenchSlot();
            if (slot >= 0) {
                board.placeOnBench(slot, std::move(w));
            }
        }
        if (ImGui::Button("重置游戏")) {
            board.clearAll();
            g_gameManager = GamePhaseNS::GameManager();
            g_gameManager.addTestUnits();
            g_gameInitialized = true;
        }
        ImGui::Text("窗口: %u x %u", window.getSize().x, window.getSize().y);
        ImGui::Text("缩放: %.2f", g_renderer.getLayout().uniformScale);
        ImGui::Text("格子: %.1f px", g_renderer.getLayout().cellSize);
    }

    ImGui::End();

    // 检查游戏结束
    if (g_gameManager.getPlayerHP() <= 0) {
        nextState = AppState::End;
    }

    return nextState;
}

// ==================== 渲染与处理暂停界面 ====================
AppState updateAndRenderPausedScreen(sf::RenderWindow& window, const std::vector<sf::Event>& /*events*/) {
    AppState nextState = AppState::Paused;

    // 暂停时仍然渲染游戏画面
    g_renderer.updateLayout(window);
    g_renderer.renderAll(window, g_gameManager.getBoard());

    const ImVec2 winSize(240.0f, 160.0f);
    ImGui::SetNextWindowPos(ImVec2(
        (window.getSize().x - winSize.x) * 0.5f,
        (window.getSize().y - winSize.y) * 0.5f
    ), ImGuiCond_Always);
    ImGui::Begin("暂停", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove);

    if (ImGui::Button("继续游戏", ImVec2(200, 40))) {
        nextState = AppState::Playing;
    }

    if (ImGui::Button("重新开始", ImVec2(200, 40))) {
        g_gameManager = GamePhaseNS::GameManager();
        g_gameManager.addTestUnits();
        g_drag.reset();
        nextState = AppState::Playing;
    }

    if (ImGui::Button("退出", ImVec2(200, 40))) {
        nextState = AppState::Start;
    }

    ImGui::End();
    
    return nextState;
}

// ==================== 渲染与处理结束界面 ====================
AppState updateAndRenderEndScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
    AppState nextState = AppState::End;

    for (const auto& e : events) {
        if (e.is<sf::Event::KeyPressed>() || e.is<sf::Event::MouseButtonPressed>()) {
            nextState = AppState::Start;
            return nextState;
        }
    }

    const ImVec2 winSize(360.0f, 180.0f);
    ImGui::SetNextWindowPos(ImVec2(
        (window.getSize().x - winSize.x) * 0.5f,
        (window.getSize().y - winSize.y) * 0.5f
    ), ImGuiCond_Always);
    ImGui::Begin("结算", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove);

    ImGui::Text("最终回合: %d", g_gameManager.getCurrentRound());
    ImGui::Text("玩家HP: %d", g_gameManager.getPlayerHP());
    ImGui::Separator();
    ImGui::Text("按下任意键以继续");

    ImGui::End();
    return nextState;
}

// ==================== 主函数 ====================
int main(){
    const auto& disp = DisplayConfig::g_Display.GetSettings();

    sf::State state = disp.fullscreen ? sf::State::Fullscreen : sf::State::Windowed;
    sf::RenderWindow window(sf::VideoMode({disp.width, disp.height}), disp.title, state);
    window.setFramerateLimit(60);

    // 初始化ImGui-SFML
    if (ImGui::SFML::Init(window)) {
        std::cout << "ImGui-SFML初始化成功" << std::endl;
    } else {
        std::cerr << "ImGui-SFML初始化失败" << std::endl;
        return -1;
    }

    // 从 DisplayConfig 获取字体路径与大小并加载字体
    ImGuiIO& io = ImGui::GetIO();
    std::string fontPath = disp.fontPath;
    float fontSize = static_cast<float>(disp.fontSize > 0 ? disp.fontSize : 18);

    if (!std::filesystem::exists(fontPath)) {
        std::filesystem::path p(fontPath);
        if (p.is_relative()) {
            std::filesystem::path tryPath = std::filesystem::path("lib") / p;
            if (std::filesystem::exists(tryPath)) {
                fontPath = tryPath.string();
            }
        }
    }

    ImFont* customFont = nullptr;
    if (std::filesystem::exists(fontPath)) {
        customFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        if (customFont) {
            io.FontDefault = customFont;
            if (!ImGui::SFML::UpdateFontTexture()) {
                std::cerr << "ImGui-SFML: UpdateFontTexture() failed" << std::endl;
            } else {
                std::cout << "字体加载成功：" << fontPath << " 大小=" << fontSize << std::endl;
            }
        } else {
            std::cerr << "字体加载失败 " << fontPath << std::endl;
        }
    } else {
        std::cerr << "字体文件不存在：" << fontPath << "，将使用 ImGui 默认字体" << std::endl;
    }

    // 界面状态
    AppState appstate = AppState::Start;

    sf::Clock deltaClock;
    float playingSaveAccumulator = 0.f;
    bool wasPlaying = false;

    std::vector<sf::Event> events;
    events.reserve(16);

    while (window.isOpen()) {
        window.clear();

        // 收集事件
        events.clear();
        while (auto eventOpt = window.pollEvent()) {
            sf::Event event = *eventOpt;
            if (event.is<sf::Event::Closed>()) {
                window.close();
            }

            // 监听窗口大小变化
            if (event.is<sf::Event::Resized>()) {
                auto* resized = event.getIf<sf::Event::Resized>();
                // 更新SFML视口以匹配新窗口大小
                sf::FloatRect visibleArea({0.f, 0.f}, {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)});
                window.setView(sf::View(visibleArea));
            }

            ImGui::SFML::ProcessEvent(window, event);
            events.push_back(event);
        }

        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        // 根据当前状态更新界面并渲染
        switch (appstate) {
            case AppState::Start:
                appstate = updateAndRenderStartScreen(window, events);
                break;
            case AppState::Setting:
                appstate = updateAndRenderSettingScreen(window, events);
                break;
            case AppState::Menu:
                appstate = updateAndRenderMenuScreen(window, events);
                break;
            case AppState::Playing:
                appstate = updateAndRenderPlayingScreen(window, events, dt.asSeconds());
                break;
            case AppState::Paused:
                appstate = updateAndRenderPausedScreen(window, events);
                break;
            case AppState::End:
                appstate = updateAndRenderEndScreen(window, events);
                break;
        }

        // 自动保存逻辑
        if (appstate == AppState::Playing) {
            if (!wasPlaying) {
                playingSaveAccumulator = 0.f;
                wasPlaying = true;
            }
            playingSaveAccumulator += dt.asSeconds();
            if (playingSaveAccumulator >= AUTO_SAVE_INTERVAL) {
                GameDataNS::g_GameData.Save();
                playingSaveAccumulator = 0.f;
            }
        } else {
            wasPlaying = false;
        }
        
        ImGui::SFML::Render(window);
        window.display();
    }

    GameDataNS::g_GameData.Save();

    ImGui::SFML::Shutdown();
    return 0;
}