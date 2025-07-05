#ifndef PALIBRIX_GAME_H
#define PALIBRIX_GAME_H

#include <vector>
#include <cstdint>
#include <random>

constexpr int BOARD_WIDTH = 10;
constexpr int BOARD_HEIGHT = 22; // Standard Tetris is 20 rows visible, with 2 hidden rows above.

struct Mino {
    int x, y;
};

enum class TetrominoType {
    I, O, T, J, L, S, Z, EMPTY
};

struct Tetromino {
    TetrominoType type;
    int rotation;
    int x, y; // Position of the top-left corner of the bounding box
};

class Game {
public:
    Game();
    ~Game();

    void update(); // Main game logic tick

    // Game Actions
    void move(int dx);
    void rotate();
    void rotateLeft();
    void softDrop();
    void hardDrop();
    void hold();
    void reset();

    // Getters for rendering
    const std::vector<std::vector<TetrominoType>>& getBoard() const;
    const Tetromino& getCurrentPiece() const;
    const Tetromino& getGhostPiece() const;
    TetrominoType getHeldPiece() const;
    const std::vector<TetrominoType>& getNextQueue() const;

    // Game State
    int getScore() const;
    int getLines() const;
    int getLevel() const;
    double getTime() const; // Game time in seconds
    bool isGameOver() const;

    // Sound-related status
    bool wasPieceLocked() const;
    void clearPieceLockedFlag();
    bool wereLinesCleared() const;
    void clearLinesClearedFlag();

private:
    void spawnNewPiece();
    bool isValid(const Tetromino& piece) const;
    void lockPiece();
    void clearLines();
    void updateGhostPiece();
    void initializeTetrominoBag();
    TetrominoType getNextFromBag();

    std::vector<std::vector<TetrominoType>> board_;
    Tetromino currentPiece_;
    Tetromino ghostPiece_;

    std::vector<TetrominoType> tetrominoBag_;
    std::vector<TetrominoType> nextQueue_;
    
    TetrominoType heldPiece_;
    bool canHold_;
    bool lastActionWasRotation_;

    int score_;
    int lines_;
    int level_;
    double dropTimer_;
    double dropInterval_;
    // a timer will be needed
    bool gameOver_;

    bool pieceLocked; // Flag to indicate a piece was just locked
    bool linesClearedFlag; // Flag to indicate lines were just cleared

    std::mt19937 rng;
    std::uniform_int_distribution<int> dist;
};

#endif //PALIBRIX_GAME_H 