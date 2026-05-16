#include "board.h"

namespace BoardNS {

Board::Board(int rows, int cols, int benchSize)
: rows_(rows)
, cols_(cols)
, benchSize_(benchSize)
, bench_(benchSize) {
    // 不能用 vector(rows, vector(cols)) 因为 unique_ptr 不可复制
    grid_.reserve(rows);
    for (int r = 0; r < rows; ++r) {
        grid_.emplace_back(cols);
    }
}

// --- 棋盘操作 ---

// 在棋盘指定位置放置单位，若位置越界或已有单位则失败
bool Board::placeOnBoard(const GridPos& pos, std::unique_ptr<Unit> unit) {
    if (!unit) return false;
    if (pos.row < 0 || pos.row >= rows_ || pos.col < 0 || pos.col >= cols_) return false;
    if (grid_[pos.row][pos.col]) return false; // 已有单位

    grid_[pos.row][pos.col] = std::move(unit);
    return true;
}

// 从棋盘指定位置移除并返回单位，若越界则返回空指针
std::unique_ptr<Unit> Board::removeFromBoard(const GridPos& pos) {
    if (pos.row < 0 || pos.row >= rows_ || pos.col < 0 || pos.col >= cols_) return nullptr;
    return std::move(grid_[pos.row][pos.col]);
}

// 获取棋盘指定位置的单位原始指针（不转移所有权），越界返回nullptr
Unit* Board::getUnitOnBoard(const GridPos& pos) const {
    if (pos.row < 0 || pos.row >= rows_ || pos.col < 0 || pos.col >= cols_) return nullptr;
    return grid_[pos.row][pos.col].get();
}

// 检查棋盘指定位置是否为空，越界返回false
bool Board::isBoardCellEmpty(const GridPos& pos) const {
    if (pos.row < 0 || pos.row >= rows_ || pos.col < 0 || pos.col >= cols_) return false;
    return grid_[pos.row][pos.col] == nullptr;
}

// --- Bench操作 ---

// 在Bench指定槽位放置单位，若槽位越界或已有单位则失败
bool Board::placeOnBench(int slot, std::unique_ptr<Unit> unit) {
    if (!unit) return false;
    if (slot < 0 || slot >= benchSize_) return false;
    if (bench_[slot]) return false;

    bench_[slot] = std::move(unit);
    return true;
}

// 从Bench指定槽位移除并返回单位，若越界则返回空指针
std::unique_ptr<Unit> Board::removeFromBench(int slot) {
    if (slot < 0 || slot >= benchSize_) return nullptr;
    return std::move(bench_[slot]);
}

// 获取Bench指定槽位的单位原始指针（不转移所有权）
Unit* Board::getUnitOnBench(int slot) const {
    if (slot < 0 || slot >= benchSize_) return nullptr;
    return bench_[slot].get();
}

// 检查Bench指定槽位是否为空
bool Board::isBenchSlotEmpty(int slot) const {
    if (slot < 0 || slot >= benchSize_) return false;
    return bench_[slot] == nullptr;
}

// 查找Bench上第一个空槽位的索引，没有空位则返回-1
int Board::findEmptyBenchSlot() const {
    for (int i = 0; i < benchSize_; ++i) {
        if (!bench_[i]) return i;
    }
    return -1;
}

// --- 查询 ---

// 获取棋盘上所有属于玩家的单位指针列表
std::vector<Unit*> Board::getPlayerUnitsOnBoard() const {
    std::vector<Unit*> result;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (grid_[r][c] && grid_[r][c]->owner() == Unit::Owner::PlayerCtrl) {
                result.push_back(grid_[r][c].get());
            }
        }
    }
    return result;
}

// 获取棋盘上所有属于敌方的单位指针列表
std::vector<Unit*> Board::getEnemyUnitsOnBoard() const {
    std::vector<Unit*> result;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (grid_[r][c] && grid_[r][c]->owner() == Unit::Owner::EnemyCtrl) {
                result.push_back(grid_[r][c].get());
            }
        }
    }
    return result;
}

// 获取棋盘上所有单位（不分阵营）
std::vector<Unit*> Board::getAllUnitsOnBoard() const {
    std::vector<Unit*> result;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (grid_[r][c]) {
                result.push_back(grid_[r][c].get());
            }
        }
    }
    return result;
}

// 获取Bench上所有单位的指针列表
std::vector<Unit*> Board::getBenchUnits() const {
    std::vector<Unit*> result;
    for (int i = 0; i < benchSize_; ++i) {
        if (bench_[i]) {
            result.push_back(bench_[i].get());
        }
    }
    return result;
}

// 统计棋盘上玩家单位的数量
int Board::getPlayerUnitCountOnBoard() const {
    int count = 0;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (grid_[r][c] && grid_[r][c]->owner() == Unit::Owner::PlayerCtrl) {
                ++count;
            }
        }
    }
    return count;
}

// 统计Bench上单位的数量
int Board::getBenchUnitCount() const {
    int count = 0;
    for (int i = 0; i < benchSize_; ++i) {
        if (bench_[i]) ++count;
    }
    return count;
}

// --- 清空 ---

// 清空棋盘上所有单位
void Board::clearBoard() {
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            grid_[r][c].reset();
        }
    }
}

// 清空Bench上所有单位
void Board::clearBench() {
    for (int i = 0; i < benchSize_; ++i) {
        bench_[i].reset();
    }
}

// 清空棋盘和Bench上所有单位
void Board::clearAll() {
    clearBoard();
    clearBench();
}

// --- 遍历 ---

// 对棋盘上每个非空格子执行回调函数，参数为格子坐标和单位指针
void Board::forEachBoardUnit(std::function<void(const GridPos&, Unit*)> func) const {
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (grid_[r][c]) {
                func(GridPos{r, c}, grid_[r][c].get());
            }
        }
    }
}

} // namespace BoardNS