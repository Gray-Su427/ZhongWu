#pragma once
#include <SFML/Graphics.hpp>
#include "board.h"
#include "game_phase.h"
#include <string>

namespace RendererNS {

// 渲染布局信息（根据窗口大小自动计算，保持长宽比不变）
struct LayoutInfo {
    // 棋盘区域
    float boardOriginX = 0.f;   // 棋盘左上角X坐标（像素）
    float boardOriginY = 0.f;   // 棋盘左上角Y坐标（像素）
    float cellSize = 64.f;      // 每格实际渲染像素大小（= 基础尺寸 × uniformScale）
    float boardPixelW = 0.f;    // 棋盘总宽度（像素）
    float boardPixelH = 0.f;    // 棋盘总高度（像素）

    // Bench区域（竖排在棋盘左侧）
    float benchOriginX = 0.f;   // Bench左上角X坐标（像素）
    float benchOriginY = 0.f;   // Bench左上角Y坐标（像素）
    float benchCellSize = 64.f; // Bench每格像素大小
    float benchPixelW = 0.f;    // Bench总宽度（像素，= 1格宽）
    float benchPixelH = 0.f;    // Bench总高度（像素，= benchSize * cellSize）

    // 统一缩放因子（保持棋盘长宽比，取宽高缩放的较小值）
    float uniformScale = 1.0f;

    // 可用渲染区域（减去右侧UI面板和边距后的区域）
    float availWidth = 0.f;
    float availHeight = 0.f;
};

// 游戏渲染器：负责棋盘、Bench、单位的SFML绘制，以及屏幕坐标与逻辑坐标的转换
class GameRenderer {
public:
    GameRenderer();

    // 根据当前窗口大小重新计算布局（每帧调用，响应窗口缩放）
    void updateLayout(const sf::RenderWindow& window);

    // --- 渲染 ---
    // 渲染棋盘网格（区分玩家/敌方半场底色）
    void renderBoard(sf::RenderWindow& window, const BoardNS::Board& board);
    // 渲染Bench区域（带背景框）
    void renderBench(sf::RenderWindow& window, const BoardNS::Board& board);
    // 渲染棋盘和Bench上所有单位（含血条、名称标签、魔法条）
    void renderUnits(sf::RenderWindow& window, const BoardNS::Board& board);
    // 依次渲染棋盘、Bench、所有单位
    void renderAll(sf::RenderWindow& window, const BoardNS::Board& board);
    // 渲染正在被拖拽的单位（半透明黄色方块跟随鼠标）
    void renderDraggedUnit(sf::RenderWindow& window, Unit* unit, const sf::Vector2f& mousePos);
    // 渲染单位悬浮提示框（显示详细属性信息）
    void renderUnitTooltip(sf::RenderWindow& window, Unit* unit, const sf::Vector2f& mousePos);

    // --- 坐标转换 ---
    // 屏幕像素坐标 → 棋盘格子坐标（不在棋盘上返回 {-1,-1}）
    BoardNS::GridPos screenToBoardCell(const sf::Vector2f& screenPos) const;
    // 屏幕像素坐标 → Bench槽位索引（不在Bench上返回 -1）
    int screenToBenchSlot(const sf::Vector2f& screenPos) const;
    // 棋盘格子坐标 → 屏幕像素坐标（格子中心点）
    sf::Vector2f boardCellToScreen(const BoardNS::GridPos& pos) const;
    // Bench槽位索引 → 屏幕像素坐标（格子中心点）
    sf::Vector2f benchSlotToScreen(int slot) const;

    // --- 查询 ---
    // 获取当前布局信息（只读）
    const LayoutInfo& getLayout() const { return layout_; }

    // --- 命中检测 ---
    // 检测屏幕坐标是否命中某个单位，返回其位置信息（棋盘格子或Bench槽位）
    BoardNS::Placement hitTest(const sf::Vector2f& screenPos, const BoardNS::Board& board) const;

private:
    LayoutInfo layout_;     // 当前布局信息

    // 渲染辅助方法
    // 绘制网格（通用，用于棋盘和Bench）
    void drawGrid(sf::RenderWindow& window, float originX, float originY,
                  int rows, int cols, float cellSize,
                  const sf::Color& lineColor, const sf::Color& fillColor);
    // 在指定位置绘制单位（彩色方块+名称标签+血条+魔法条）
    void drawUnitOnCell(sf::RenderWindow& window, Unit* unit,
                        float centerX, float centerY, float cellSize);
    // 绘制血条（根据血量比例变色：绿→黄→红）
    void drawHealthBar(sf::RenderWindow& window, float centerX, float topY,
                       float width, float height, int hp, int maxHp);

    // 颜色方案
    sf::Color boardLineColor_   = sf::Color(80, 80, 80);   // 棋盘网格线颜色
    sf::Color boardFillColor_   = sf::Color(45, 45, 55);   // 棋盘格子填充色
    sf::Color boardAltColor_    = sf::Color(55, 55, 65);   // 棋盘交替格颜色
    sf::Color benchLineColor_   = sf::Color(80, 80, 80);   // Bench网格线颜色
    sf::Color benchFillColor_   = sf::Color(40, 50, 40);   // Bench格子填充色
    sf::Color playerSideColor_  = sf::Color(35, 55, 35);   // 玩家半场底色（绿色调）
    sf::Color enemySideColor_   = sf::Color(55, 35, 35);   // 敌方半场底色（红色调）
};

} // namespace RendererNS