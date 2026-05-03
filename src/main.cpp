#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>
#include "Config.h"

enum class AppState {
    Start,
    Menu,
    Playing,
    Paused,
    End
};

void displayStartScreen() {
    // 显示启动界面
}

void displayMenuScreen() {
    // 显示菜单界面
}

void displayPlayingScreen() {
    // 显示游戏界面
}

void displayPausedScreen() {
    // 显示暂停界面
}

void displayEndScreen() {
    // 显示结束界面
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

    // 设置界面状态
    AppState appState = AppState::Start;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        // 事件处理
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            ImGui::SFML::ProcessEvent(window, *event);
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // ImGui示例窗口
        ImGui::Begin("Hello, ImGui!");
        ImGui::Text("这是一个基本的图形化窗口示例。");
        ImGui::End();

        switch (appState) {
            case AppState::Start:
                // 启动界面
                displayStartScreen();
                break;
            case AppState::Menu:
                // 菜单界面
                displayMenuScreen();
                break;
            case AppState::Playing:
                // 游戏界面
                displayPlayingScreen();
                break;
            case AppState::Paused:
                // 暂停界面
                displayPausedScreen();
                break;
            case AppState::End:
                // 结束界面
                displayEndScreen();
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}