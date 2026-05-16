#include "game_renderer.h"
#include "DisplayConfig.h"
#include "GameData.h"
#include <algorithm>
#include <sstream>
#include <cstdint>

namespace RendererNS {

// 基础逻辑尺寸常量（不随窗口变化，用于计算缩放比例）
static constexpr float BASE_CELL_SIZE = 64.f;       // 每格基础像素大小
static constexpr float SIDE_PANEL_WIDTH = 260.f;    // 右侧ImGui面板预留宽度
static constexpr float TOP_MARGIN = 10.f;           // 顶部边距
static constexpr float BOTTOM_MARGIN = 10.f;        // 底部边距

GameRenderer::GameRenderer() {
}

// --- 布局计算 ---

// 根据窗口大小重新计算所有渲染布局参数
// 布局：[Bench竖列] [间距=1/4棋盘宽] [8x8棋盘]
// 核心算法：uniformScale = min(可用宽度/逻辑宽度, 可用高度/逻辑高度)，保持长宽比
void GameRenderer::updateLayout(const sf::RenderWindow& window) {
    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);

    layout_.availWidth = winW - SIDE_PANEL_WIDTH;
    layout_.availHeight = winH - TOP_MARGIN - BOTTOM_MARGIN;

    int rows = 8, cols = 8;
    float boardLogicalW = cols * BASE_CELL_SIZE;
    float boardLogicalH = rows * BASE_CELL_SIZE;

    // Bench竖排：宽度1格，高度=benchSize格
    int benchSize = GameDataNS::g_GameData.GetConfig().benchSize;
    float benchLogicalW = 1 * BASE_CELL_SIZE;
    float benchLogicalH = benchSize * BASE_CELL_SIZE;

    // 间距 = 1/4 棋盘宽度
    float gapLogicalW = boardLogicalW / 4.f;

    // 总逻辑尺寸 = bench宽 + 间距 + 棋盘宽
    float totalLogicalW = benchLogicalW + gapLogicalW + boardLogicalW;
    float totalLogicalH = std::max(boardLogicalH, benchLogicalH);

    float scaleX = layout_.availWidth / totalLogicalW;
    float scaleY = layout_.availHeight / totalLogicalH;
    layout_.uniformScale = std::min(scaleX, scaleY);
    layout_.uniformScale = std::clamp(layout_.uniformScale, 0.3f, 3.0f);

    layout_.cellSize = BASE_CELL_SIZE * layout_.uniformScale;
    layout_.boardPixelW = cols * layout_.cellSize;
    layout_.boardPixelH = rows * layout_.cellSize;
    layout_.benchCellSize = layout_.cellSize;
    layout_.benchPixelW = 1 * layout_.benchCellSize;
    layout_.benchPixelH = benchSize * layout_.benchCellSize;

    float gapPixelW = layout_.boardPixelW / 4.f;

    float actualTotalW = layout_.benchPixelW + gapPixelW + layout_.boardPixelW;
    float actualTotalH = std::max(layout_.boardPixelH, layout_.benchPixelH);

    float offsetX = (layout_.availWidth - actualTotalW) * 0.5f;
    float offsetY = (layout_.availHeight - actualTotalH) * 0.5f + TOP_MARGIN;

    // Bench在左侧
    layout_.benchOriginX = offsetX;
    layout_.benchOriginY = offsetY + (actualTotalH - layout_.benchPixelH) * 0.5f;

    // 棋盘在Bench右侧
    layout_.boardOriginX = layout_.benchOriginX + layout_.benchPixelW + gapPixelW;
    layout_.boardOriginY = offsetY + (actualTotalH - layout_.boardPixelH) * 0.5f;
}

// --- 渲染辅助 ---

// 绘制血条：根据血量比例显示不同颜色（>60%绿色，>30%黄色，否则红色）
void GameRenderer::drawHealthBar(sf::RenderWindow& window, float centerX, float topY,
                                  float width, float height, int hp, int maxHp) {
    float ratio = (maxHp > 0) ? static_cast<float>(hp) / static_cast<float>(maxHp) : 0.f;
    ratio = std::clamp(ratio, 0.f, 1.f);

    sf::RectangleShape bg({width, height});
    bg.setPosition({centerX - width * 0.5f, topY});
    bg.setFillColor(sf::Color(60, 20, 20));
    window.draw(bg);

    sf::Color barColor;
    if (ratio > 0.6f) barColor = sf::Color(50, 200, 50);
    else if (ratio > 0.3f) barColor = sf::Color(200, 200, 50);
    else barColor = sf::Color(200, 50, 50);

    sf::RectangleShape fill({width * ratio, height});
    fill.setPosition({centerX - width * 0.5f, topY});
    fill.setFillColor(barColor);
    window.draw(fill);
}

// 在指定格子中心绘制单位：根据羁绊类型着色，敌方颜色偏暗，附带名称标签、血条和魔法条
void GameRenderer::drawUnitOnCell(sf::RenderWindow& window, Unit* unit,
                                   float centerX, float centerY, float cellSize) {
    if (!unit) return;

    float unitSize = cellSize * 0.7f;
    float halfUnit = unitSize * 0.5f;

    // 根据类型确定颜色
    sf::Color unitColor;
    if (unit->hasTrait("战士"))       unitColor = sf::Color(200, 80, 80);
    else if (unit->hasTrait("弓箭手")) unitColor = sf::Color(80, 200, 80);
    else if (unit->hasTrait("法师"))   unitColor = sf::Color(100, 100, 230);
    else if (unit->hasTrait("坦克"))   unitColor = sf::Color(200, 200, 80);
    else if (unit->hasTrait("刺客"))   unitColor = sf::Color(200, 80, 200);
    else unitColor = sf::Color(180, 180, 180);

    // 敌方单位颜色偏暗
    if (unit->owner() == Unit::Owner::EnemyCtrl) {
        unitColor.r = static_cast<std::uint8_t>(unitColor.r * 0.7f);
        unitColor.g = static_cast<std::uint8_t>(unitColor.g * 0.7f);
        unitColor.b = static_cast<std::uint8_t>(unitColor.b * 0.7f);
    }

    sf::RectangleShape unitShape({unitSize, unitSize});
    unitShape.setPosition({centerX - halfUnit, centerY - halfUnit});
    unitShape.setFillColor(unitColor);
    unitShape.setOutlineThickness(2.f);
    unitShape.setOutlineColor(unit->owner() == Unit::Owner::PlayerCtrl
        ? sf::Color(120, 180, 255) : sf::Color(255, 120, 120));
    window.draw(unitShape);

    // 绘制名称（使用SFML图形文字）
    float fontSize = cellSize * 0.22f;
    if (fontSize >= 8.f) {
        // 使用简单的矩形来表示名称区域（避免字体问题）
        // 实际文字通过ImGui覆盖层绘制，这里画一个小标签背景
        sf::RectangleShape nameLabel({unitSize, fontSize + 4.f});
        nameLabel.setPosition({centerX - halfUnit, centerY + halfUnit - fontSize - 4.f});
        nameLabel.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(nameLabel);
    }

    // 血条
    drawHealthBar(window, centerX, centerY - halfUnit - cellSize * 0.08f,
                  unitSize, cellSize * 0.06f, unit->getHP(), unit->getMaxHP());

    // Mana条
    if (unit->getMaxMana() > 0) {
        float manaBarY = centerY - halfUnit - cellSize * 0.08f + cellSize * 0.06f + 1.f;
        float manaRatio = static_cast<float>(unit->getMana()) / static_cast<float>(unit->getMaxMana());
        float barW = unitSize;
        float barH = cellSize * 0.04f;

        sf::RectangleShape manaBg({barW, barH});
        manaBg.setPosition({centerX - barW * 0.5f, manaBarY});
        manaBg.setFillColor(sf::Color(30, 30, 60));
        window.draw(manaBg);

        sf::RectangleShape manaFill({barW * manaRatio, barH});
        manaFill.setPosition({centerX - barW * 0.5f, manaBarY});
        manaFill.setFillColor(sf::Color(80, 80, 220));
        window.draw(manaFill);
    }
}

// --- 渲染 ---

// 渲染棋盘网格：左半部分为玩家半场（绿色调），右半部分为敌方半场（红色调），中间有竖直分隔线
void GameRenderer::renderBoard(sf::RenderWindow& window, const BoardNS::Board& board) {
    int rows = board.getRows();
    int cols = board.getCols();
    float cs = layout_.cellSize;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            sf::RectangleShape cell({cs, cs});
            cell.setPosition({layout_.boardOriginX + c * cs, layout_.boardOriginY + r * cs});

            // 左半部分（col 0-3）为玩家半场，右半部分（col 4-7）为敌方半场
            sf::Color base = (c < cols / 2) ? playerSideColor_ : enemySideColor_;
            if ((r + c) % 2 == 0) {
                cell.setFillColor(sf::Color(
                    std::min(base.r + 10, 255),
                    std::min(base.g + 10, 255),
                    std::min(base.b + 10, 255)));
            } else {
                cell.setFillColor(base);
            }

            cell.setOutlineThickness(1.f);
            cell.setOutlineColor(boardLineColor_);
            window.draw(cell);
        }
    }

    // 中间竖直分隔线
    sf::RectangleShape divider({2.f, layout_.boardPixelH});
    divider.setPosition({layout_.boardOriginX + (cols / 2) * cs, layout_.boardOriginY});
    divider.setFillColor(sf::Color(200, 200, 200, 150));
    window.draw(divider);
}

// 渲染Bench区域：竖排在棋盘左侧，带深色背景框
void GameRenderer::renderBench(sf::RenderWindow& window, const BoardNS::Board& board) {
    int benchSize = board.getBenchSize();
    float cs = layout_.benchCellSize;

    // 背景框（竖向）
    sf::RectangleShape benchBg({cs + 10.f * layout_.uniformScale,
                                 layout_.benchPixelH + 20.f * layout_.uniformScale});
    benchBg.setPosition({layout_.benchOriginX - 5.f * layout_.uniformScale,
                          layout_.benchOriginY - 10.f * layout_.uniformScale});
    benchBg.setFillColor(sf::Color(30, 40, 30, 200));
    benchBg.setOutlineThickness(1.f);
    benchBg.setOutlineColor(benchLineColor_);
    window.draw(benchBg);

    // 竖排格子：每个格子从上到下排列
    for (int i = 0; i < benchSize; ++i) {
        sf::RectangleShape cell({cs, cs});
        cell.setPosition({layout_.benchOriginX, layout_.benchOriginY + i * cs});
        cell.setFillColor(benchFillColor_);
        cell.setOutlineThickness(1.f);
        cell.setOutlineColor(benchLineColor_);
        window.draw(cell);
    }
}

// 渲染棋盘和Bench上所有单位
void GameRenderer::renderUnits(sf::RenderWindow& window, const BoardNS::Board& board) {
    float cs = layout_.cellSize;

    board.forEachBoardUnit([&](const BoardNS::GridPos& pos, Unit* unit) {
        if (unit->isInCombat()) {
            // 战斗模式：使用单位自身的像素位置
            sf::Vector2f pixelPos = unit->getPosition();
            drawUnitOnCell(window, unit, pixelPos.x, pixelPos.y, cs);

            // 死亡单位半透明
            if (!unit->isAlive()) {
                sf::RectangleShape deadOverlay({cs * 0.7f, cs * 0.7f});
                deadOverlay.setOrigin({cs * 0.35f, cs * 0.35f});
                deadOverlay.setPosition(pixelPos);
                deadOverlay.setFillColor(sf::Color(0, 0, 0, 150));
                window.draw(deadOverlay);
            }
        } else {
            // 非战斗模式：使用格子坐标计算像素位置
            float cx = layout_.boardOriginX + pos.col * cs + cs * 0.5f;
            float cy = layout_.boardOriginY + pos.row * cs + cs * 0.5f;
            drawUnitOnCell(window, unit, cx, cy, cs);
        }
    });

    float bcs = layout_.benchCellSize;
    for (int i = 0; i < board.getBenchSize(); ++i) {
        Unit* unit = board.getUnitOnBench(i);
        if (unit) {
            // 竖排：X固定，Y随槽位递增
            float cx = layout_.benchOriginX + bcs * 0.5f;
            float cy = layout_.benchOriginY + i * bcs + bcs * 0.5f;
            drawUnitOnCell(window, unit, cx, cy, bcs);
        }
    }
}

// 依次渲染棋盘网格、Bench区域、所有单位
void GameRenderer::renderAll(sf::RenderWindow& window, const BoardNS::Board& board) {
    renderBoard(window, board);
    renderBench(window, board);
    renderUnits(window, board);
}

// 渲染正在被拖拽的单位：半透明黄色方块跟随鼠标位置
void GameRenderer::renderDraggedUnit(sf::RenderWindow& window, Unit* unit, const sf::Vector2f& mousePos) {
    if (!unit) return;
    float cs = layout_.cellSize;
    float unitSize = cs * 0.7f;

    sf::RectangleShape dragShape({unitSize, unitSize});
    dragShape.setOrigin({unitSize * 0.5f, unitSize * 0.5f});
    dragShape.setPosition(mousePos);
    dragShape.setFillColor(sf::Color(255, 255, 100, 180));
    dragShape.setOutlineThickness(2.f);
    dragShape.setOutlineColor(sf::Color(255, 255, 200));
    window.draw(dragShape);
}

// 渲染单位悬浮提示框：显示名称、HP、ATK、Range、Mana、羁绊、所属方等信息
void GameRenderer::renderUnitTooltip(sf::RenderWindow& window, Unit* unit, const sf::Vector2f& mousePos) {
    if (!unit) return;

    float scale = layout_.uniformScale;
    float tooltipW = 180.f * scale;
    float lineH = 20.f * scale;
    float padding = 8.f * scale;

    std::vector<std::string> lines;
    lines.push_back(unit->getUnitName());
    lines.push_back("HP: " + std::to_string(unit->getHP()) + "/" + std::to_string(unit->getMaxHP()));
    lines.push_back("ATK: " + std::to_string(unit->getATK()));
    lines.push_back("Range: " + std::to_string(static_cast<int>(unit->getRange())));
    if (unit->getMaxMana() > 0) {
        lines.push_back("Mana: " + std::to_string(unit->getMana()) + "/" + std::to_string(unit->getMaxMana()));
    }

    std::string traitsStr = "Traits: ";
    bool first = true;
    for (const auto& t : unit->getTraits()) {
        if (!first) traitsStr += ", ";
        traitsStr += t;
        first = false;
    }
    if (!unit->getTraits().empty()) lines.push_back(traitsStr);

    lines.push_back(unit->owner() == Unit::Owner::PlayerCtrl ? "[Player]" :
                    unit->owner() == Unit::Owner::EnemyCtrl ? "[Enemy]" : "[Neutral]");

    float tooltipH = lines.size() * lineH + padding * 2.f;

    float tx = mousePos.x + 15.f;
    float ty = mousePos.y + 15.f;
    if (tx + tooltipW > static_cast<float>(window.getSize().x)) {
        tx = mousePos.x - tooltipW - 15.f;
    }
    if (ty + tooltipH > static_cast<float>(window.getSize().y)) {
        ty = mousePos.y - tooltipH - 15.f;
    }

    sf::RectangleShape bg({tooltipW, tooltipH});
    bg.setPosition({tx, ty});
    bg.setFillColor(sf::Color(20, 20, 30, 230));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(150, 150, 180));
    window.draw(bg);

    // 用小矩形模拟文字行（实际文字通过ImGui绘制）
    float fontSize = 14.f * scale;
    if (fontSize < 8.f) fontSize = 8.f;
    for (size_t i = 0; i < lines.size(); ++i) {
        sf::RectangleShape lineBg({tooltipW - padding * 2.f, fontSize});
        lineBg.setPosition({tx + padding, ty + padding + i * lineH});
        lineBg.setFillColor(i == 0 ? sf::Color(80, 70, 30, 200) : sf::Color(50, 50, 60, 150));
        window.draw(lineBg);
    }
}

// --- 坐标转换 ---

// 屏幕像素坐标 → 棋盘格子坐标（row, col），不在棋盘范围内返回 {-1, -1}
BoardNS::GridPos GameRenderer::screenToBoardCell(const sf::Vector2f& screenPos) const {
    float cs = layout_.cellSize;
    float relX = screenPos.x - layout_.boardOriginX;
    float relY = screenPos.y - layout_.boardOriginY;

    int col = static_cast<int>(relX / cs);
    int row = static_cast<int>(relY / cs);

    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return {-1, -1};
    }
    return {row, col};
}

// 屏幕像素坐标 → Bench槽位索引（竖排），不在Bench范围内返回 -1
int GameRenderer::screenToBenchSlot(const sf::Vector2f& screenPos) const {
    float cs = layout_.benchCellSize;
    float relX = screenPos.x - layout_.benchOriginX;
    float relY = screenPos.y - layout_.benchOriginY;

    // 竖排：X范围=1格宽，Y范围=benchSize格高
    int benchSize = GameDataNS::g_GameData.GetConfig().benchSize;
    if (relX < 0 || relX >= cs || relY < 0) {
        return -1;
    }
    int slot = static_cast<int>(relY / cs);
    if (slot < 0 || slot >= benchSize) {
        return -1;
    }
    return slot;
}

// 棋盘格子坐标 → 屏幕像素坐标（返回格子中心点）
sf::Vector2f GameRenderer::boardCellToScreen(const BoardNS::GridPos& pos) const {
    float cs = layout_.cellSize;
    return {
        layout_.boardOriginX + pos.col * cs + cs * 0.5f,
        layout_.boardOriginY + pos.row * cs + cs * 0.5f
    };
}

// Bench槽位索引 → 屏幕像素坐标（竖排，返回格子中心点）
sf::Vector2f GameRenderer::benchSlotToScreen(int slot) const {
    float cs = layout_.benchCellSize;
    return {
        layout_.benchOriginX + cs * 0.5f,
        layout_.benchOriginY + slot * cs + cs * 0.5f
    };
}

// --- 命中检测 ---

// 检测屏幕坐标是否命中某个单位，优先检测Bench，再检测棋盘
// 返回 Placement 结构体表示命中的位置（类型+坐标/槽位）
BoardNS::Placement GameRenderer::hitTest(const sf::Vector2f& screenPos, const BoardNS::Board& board) const {
    int slot = screenToBenchSlot(screenPos);
    if (slot >= 0 && board.getUnitOnBench(slot)) {
        BoardNS::Placement p;
        p.type = BoardNS::Placement::Bench;
        p.benchSlot.index = slot;
        return p;
    }

    BoardNS::GridPos cell = screenToBoardCell(screenPos);
    if (cell.isValid() && board.getUnitOnBoard(cell)) {
        BoardNS::Placement p;
        p.type = BoardNS::Placement::Board;
        p.gridPos = cell;
        return p;
    }

    return {};
}

} // namespace RendererNS