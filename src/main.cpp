#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "SimpleIni.h"
#include <iostream>
#include "Config.h"

enum class AppState {
    Start,
    Menu,
    Playing,
    Paused,
    End
};

// 渲染与处理启动界面
AppState updateAndRenderStartScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
    AppState nextState = AppState::Start;

    ImGui::Begin("欢迎来到游戏", nullptr, 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove);
    ImGui::SetWindowPos(ImVec2(
        (window.getSize().x - ImGui::GetWindowWidth()) * 0.5f,
        (window.getSize().y - ImGui::GetWindowHeight()) * 0.4f
    ));

    if (ImGui::Button("开始游戏", ImVec2(200, 40))) {
        nextState = AppState::Menu;
    }
    if (ImGui::Button("设置", ImVec2(200, 40))) {
        // 设置按钮暂不处理
    }
    if (ImGui::Button("退出游戏", ImVec2(200, 40))) {
        window.close();
    }
    ImGui::End();

    return nextState;
}

// 渲染与处理菜单界面
AppState updateAndRenderMenuScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
    AppState nextState = AppState::Menu;

    // 检测esc键
    for (const auto& event : events) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)){
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
    ImGui::Begin("菜单", nullptr, 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground);
    ImGui::SetWindowPos(ImVec2(
        (window.getSize().x - ImGui::GetWindowWidth()) * 0.5f,
        (window.getSize().y - ImGui::GetWindowHeight()) * 0.5f
    ));

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
}

// 渲染与处理暂停界面
AppState updateAndRenderPausedScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
}

// 渲染与处理结束界面
AppState updateAndRenderEndScreen(sf::RenderWindow& window, const std::vector<sf::Event>& events) {
}

int main(){
    // 创建一个SFML窗口
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), WINDOW_TITLE);
    window.setFramerateLimit(60);

    // 初始化ImGui-SFML
    if (ImGui::SFML::Init(window)){
        std::cout << "ImGui-SFML初始化成功" << std::endl;
    } else {
        std::cerr << "ImGui-SFML初始化失败" << std::endl;
        return -1;
    }

    // 加载lib文件夹下的字体文件
    ImGuiIO& io = ImGui::GetIO();
    ImFont* customFont = io.Fonts->AddFontFromFileTTF(FONT_PATH, FONT_SIZE, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    if (customFont) {
        io.FontDefault = customFont;
        bool fontUpdated = ImGui::SFML::UpdateFontTexture(); // 构建字体集，避免断言错误
        if (!fontUpdated) {
            std::cerr << "ImGui-SFML: UpdateFontTexture() failed" << std::endl;
        } else {
            std::cout << "字体加载成功：" << FONT_PATH << std::endl;
        }
    } else {
        std::cerr << "字体加载失败" << std::endl;
    }

    // 界面状态
    AppState state = AppState::Start;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        // 清空屏幕
        window.clear();

        // 收集事件
        std::vector<sf::Event> events;
        while (auto eventOpt = window.pollEvent()) {
            sf::Event event = *eventOpt;
            if (event.is<sf::Event::Closed>()) {
                window.close();
            }
            ImGui::SFML::ProcessEvent(window, event);
            events.push_back(event); // 保存事件供后续处理
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // 根据当前状态更新界面并渲染
        switch (state) {
            case AppState::Start:
                // 启动界面
                state = updateAndRenderStartScreen(window, events);
                break;
            case AppState::Menu:
                // 菜单界面
                state = updateAndRenderMenuScreen(window, events);
                break;
            case AppState::Playing:
                // 游戏界面
                state = updateAndRenderPlayingScreen(window, events);
                break;
            case AppState::Paused:
                // 暂停界面
                state = updateAndRenderPausedScreen(window, events);
                break;
            case AppState::End:
                // 结束界面
                state = updateAndRenderEndScreen(window, events);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}