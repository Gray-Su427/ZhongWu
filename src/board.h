#pragma once
#include "unit.h"
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace BoardNS {

// 棋盘上一个格子的坐标
struct GridPos {
    int row = -1;  // 行 (0-based)
    int col = -1;  // 列 (0-based)

    bool isValid() const { return row >= 0 && col >= 0; }
    bool operator==(const GridPos& o) const { return row == o.row && col == o.col; }
    bool operator!=(const GridPos& o) const { return !(*this == o); }
};

// Bench上的槽位索引
struct BenchSlot {
    int index = -1;  // 0-based

    bool isValid() const { return index >= 0; }
};

// 放置位置：可以是棋盘格子或Bench槽位
struct Placement {
    enum Type { None, Board, Bench };
    Type type = None;
    GridPos gridPos;
    BenchSlot benchSlot;

    bool isValid() const { return type != None; }
};

class Board {
public:
    // 默认8×8棋盘，8格Bench
    Board(int rows = 8, int cols = 8, int benchSize = 8);

    // --- 棋盘操作 ---
    int getRows() const { return rows_; }
    int getCols() const { return cols_; }
    int getBenchSize() const { return benchSize_; }

    // 在棋盘指定位置放置单位（返回是否成功）
    bool placeOnBoard(const GridPos& pos, std::unique_ptr<Unit> unit);
    // 从棋盘移除单位（返回被移除的单位，如果有的话）
    std::unique_ptr<Unit> removeFromBoard(const GridPos& pos);
    // 获取棋盘指定位置的单位（不转移所有权）
    Unit* getUnitOnBoard(const GridPos& pos) const;
    // 检查棋盘指定位置是否有单位
    bool isBoardCellEmpty(const GridPos& pos) const;

    // --- Bench操作 ---
    // 在Bench指定槽位放置单位（返回是否成功）
    bool placeOnBench(int slot, std::unique_ptr<Unit> unit);
    // 从Bench移除单位（返回被移除的单位，如果有的话）
    std::unique_ptr<Unit> removeFromBench(int slot);
    // 获取Bench指定槽位的单位（不转移所有权）
    Unit* getUnitOnBench(int slot) const;
    // 检查Bench指定槽位是否为空
    bool isBenchSlotEmpty(int slot) const;
    // 找到Bench上第一个空槽位，返回索引（没有则返回-1）
    int findEmptyBenchSlot() const;

    // --- 查询 ---
    // 获取棋盘上所有玩家单位
    std::vector<Unit*> getPlayerUnitsOnBoard() const;
    // 获取棋盘上所有敌方单位
    std::vector<Unit*> getEnemyUnitsOnBoard() const;
    // 获取棋盘上所有单位（不分阵营）
    std::vector<Unit*> getAllUnitsOnBoard() const;
    // 获取Bench上所有单位
    std::vector<Unit*> getBenchUnits() const;
    // 获取棋盘上玩家单位数量
    int getPlayerUnitCountOnBoard() const;
    // 获取Bench上单位数量
    int getBenchUnitCount() const;

    // --- 清空 ---
    void clearBoard();
    void clearBench();
    void clearAll();

    // --- 遍历 ---
    // 遍历棋盘上所有非空格子
    void forEachBoardUnit(std::function<void(const GridPos&, Unit*)> func) const;

private:
    int rows_;
    int cols_;
    int benchSize_;

    // 棋盘网格：rows_ × cols_，每个格子可能持有或为空
    std::vector<std::vector<std::unique_ptr<Unit>>> grid_;
    // Bench槽位
    std::vector<std::unique_ptr<Unit>> bench_;
};

} // namespace BoardNS