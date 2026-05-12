#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "SimpleIni.h"
#include <iostream>
#include "unit.h"
#include "DisplayConfig.h"
#include "GameData.h"
#include <filesystem>

enum class AppState {
    Start,
    Setting,
    Menu,
    Playing,
    Paused,
    End
};


// 渲染与处理启动界面
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

// 渲染与处理设置界面
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

// 渲染与处理菜单界面
AppState updateAndRenderMenuScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
    AppState nextState = AppState::Menu;

    // 检测esc键（使用事件匹配而非实时状态查询）
    for (const auto& event : events) {
        if (event.is<sf::Event::KeyPressed>() 
            && event.getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape) {
            nextState = AppState::Start;
        }
    }

    // 创建返回键
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

    // 创建菜单
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
    // 无尽模式按钮
    if (ImGui::Button("无尽模式", ImVec2(200, 40))) {
        nextState = AppState::Playing; 
    }
    ImGui::End();

    return nextState;
}

// 渲染与处理游戏界面
AppState updateAndRenderPlayingScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
    AppState nextState = AppState::Playing;
    
    for (const auto& event : events) {
        if (event.is<sf::Event::KeyPressed>()
            && event.getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape) {
            nextState = AppState::Paused;
        }
    }

    ImGui::Begin("游戏测试面板", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::Button("击杀 +1", ImVec2(120, 30))) {
        GameDataNS::g_GameData.Config().killedCount += 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("结束并结算", ImVec2(120, 30))) {
        nextState = AppState::End;
    }
    ImGui::Text("当前杀敌数: %d", GameDataNS::g_GameData.GetConfig().killedCount);
    ImGui::End();

    return nextState;
}

// 渲染与处理暂停界面
AppState updateAndRenderPausedScreen(sf::RenderWindow& window, const std::vector<sf::Event>& /*events*/) {
    AppState nextState = AppState::Paused;

    // 居中显示暂停面板
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
        // 重置游戏数据（直接通过全局对象）
        GameDataNS::g_GameData.ResetConfig();
        nextState = AppState::Playing; // 重新开始后直接进入游戏（如需回到准备界面改为 AppState::Start）
    }

    if (ImGui::Button("退出", ImVec2(200, 40))) {
        nextState = AppState::Start;
    }

    ImGui::End();
    
    return nextState;
}

// 渲染与处理结束界面
AppState updateAndRenderEndScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
    AppState nextState = AppState::End;

    // 检测任意按键或鼠标按下以返回开始界面
    for (const auto& e : events) {
        if (e.is<sf::Event::KeyPressed>() || e.is<sf::Event::MouseButtonPressed>()) {
            nextState = AppState::Start;
            return nextState;
        }
    }

    // 居中显示结算面板
    const ImVec2 winSize(360.0f, 180.0f);
    ImGui::SetNextWindowPos(ImVec2(
        (window.getSize().x - winSize.x) * 0.5f,
        (window.getSize().y - winSize.y) * 0.5f
    ), ImGuiCond_Always);
    ImGui::Begin("结算", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove);

    const auto& gd = GameDataNS::g_GameData.GetConfig();
    ImGui::Text("最终等级: %d", gd.level);
    ImGui::Text("杀敌数: %d", gd.killedCount);
    ImGui::Separator();
    ImGui::Text("按下任意键以继续");

    ImGui::End();
    return nextState;
}

int main(){
    const auto& disp = DisplayConfig::g_Display.GetSettings();

    // 使用配置创建窗口
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

    // 若给定路径不存在，尝试在 lib 目录下寻找（容错）
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
    float playingSaveAccumulator = 0.f;                 // 累计在 Playing 状态下的时间（秒）
    bool wasPlaying = false;                           // 用于检测是否刚进入 Playing 状态

    // 事件容器在循环外创建，避免每帧重新分配内存
    std::vector<sf::Event> events;
    events.reserve(16);

    while (window.isOpen()) {
        // 清空屏幕
        window.clear();

        // 收集事件
        events.clear();
        while (auto eventOpt = window.pollEvent()) {
            sf::Event event = *eventOpt;
            if (event.is<sf::Event::Closed>()) {
                window.close();
            }
            ImGui::SFML::ProcessEvent(window, event);
            events.push_back(event); // 保存事件供后续处理
        }

        // 获取本帧时间增量并传入 ImGui-SFML
        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        // 根据当前状态更新界面并渲染
        switch (appstate) {
            case AppState::Start:
                // 启动界面
                appstate = updateAndRenderStartScreen(window, events);
                break;
            case AppState::Setting:
                appstate = updateAndRenderSettingScreen(window, events);
                break;
            case AppState::Menu:
                // 菜单界面
                appstate = updateAndRenderMenuScreen(window, events);
                break;
            case AppState::Playing:
                // 游戏界面
                appstate = updateAndRenderPlayingScreen(window, events);
                break;
            case AppState::Paused:
                // 暂停界面
                appstate = updateAndRenderPausedScreen(window, events);
                break;
            case AppState::End:
                // 结束界面
                appstate = updateAndRenderEndScreen(window, events);
        }

        // 自动保存逻辑：仅在 Playing 状态下计时，每隔 AUTO_SAVE_INTERVAL 秒保存一次
        if (appstate == AppState::Playing) {
            if (!wasPlaying) {
                // 刚进入 Playing：重置计时器
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

    // 退出前保存当前数据
    GameDataNS::g_GameData.Save();

    ImGui::SFML::Shutdown();
    return 0;
}