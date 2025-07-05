#include "Game.h"
#include "TetrominoData.h"
#include <algorithm>
#include <vector>
#include <random>

// Tetromino shapes data is now in TetrominoData.h

Game::Game() : board_(BOARD_HEIGHT, std::vector<TetrominoType>(BOARD_WIDTH, TetrominoType::EMPTY)),
               gameOver_(false), score_(0), lines_(0), level_(1), heldPiece_(TetrominoType::EMPTY), canHold_(true),
               dropTimer_(0.0), dropInterval_(1.0), lastActionWasRotation_(false) { // Start with 1 second interval
    // Initialize random number generator for the bag
    initializeTetrominoBag();
    spawnNewPiece();
}

Game::~Game() {}

void Game::update() {
    if (gameOver_) return;

    // Automatic drop based on level
    dropTimer_ += 1.0 / 60.0; // Assuming 60 FPS
    if (dropTimer_ >= dropInterval_) {
        softDrop();
        dropTimer_ = 0;
    }
}

void Game::move(int dx) {
    if (gameOver_) return;
    
    Tetromino newPiece = currentPiece_;
    newPiece.x += dx;
    
    if (isValid(newPiece)) {
        currentPiece_ = newPiece;
        updateGhostPiece();
        lastActionWasRotation_ = false;
    }
}

void Game::rotate() {
    if (gameOver_) return;
    
    Tetromino newPiece = currentPiece_;
    newPiece.rotation = (newPiece.rotation + 1) % 4;
    
    if (isValid(newPiece)) {
        currentPiece_ = newPiece;
        updateGhostPiece();
        lastActionWasRotation_ = true;
    }
}

void Game::rotateLeft() {
    if (gameOver_) return;
    
    Tetromino newPiece = currentPiece_;
    newPiece.rotation = (newPiece.rotation + 3) % 4; // +3 is same as -1 in mod 4
    
    if (isValid(newPiece)) {
        currentPiece_ = newPiece;
        updateGhostPiece();
        lastActionWasRotation_ = true;
    }
}

void Game::softDrop() {
    if (gameOver_) return;
    
    Tetromino newPiece = currentPiece_;
    newPiece.y++;
    
    if (isValid(newPiece)) {
        currentPiece_ = newPiece;
        updateGhostPiece();
        lastActionWasRotation_ = false;
    } else {
        // Piece can't move down anymore, lock it
        lockPiece();
        clearLines();
        spawnNewPiece();
        canHold_ = true; // Reset hold ability
    }
}

void Game::hardDrop() {
    if (gameOver_) return;
    
    while (isValid(currentPiece_)) {
        currentPiece_.y++;
    }
    currentPiece_.y--; // Move back to last valid position
    
    lockPiece();
    clearLines();
    spawnNewPiece();
    canHold_ = true;
}

void Game::hold() {
    if (gameOver_ || !canHold_) return;
    
    if (heldPiece_ == TetrominoType::EMPTY) {
        heldPiece_ = currentPiece_.type;
        spawnNewPiece();
    } else {
        TetrominoType temp = currentPiece_.type;
        currentPiece_.type = heldPiece_;
        currentPiece_.rotation = 0;
        currentPiece_.x = 3;
        currentPiece_.y = 0;
        heldPiece_ = temp;
        updateGhostPiece();
    }
    canHold_ = false;
}

void Game::reset() {
    board_ = std::vector<std::vector<TetrominoType>>(BOARD_HEIGHT, std::vector<TetrominoType>(BOARD_WIDTH, TetrominoType::EMPTY));
    gameOver_ = false;
    score_ = 0;
    lines_ = 0;
    level_ = 1;
    heldPiece_ = TetrominoType::EMPTY;
    canHold_ = true;
    dropTimer_ = 0.0;
    dropInterval_ = 1.0;
    lastActionWasRotation_ = false;
    initializeTetrominoBag();
    spawnNewPiece();
}

const std::vector<std::vector<TetrominoType>>& Game::getBoard() const {
    return board_;
}

const Tetromino& Game::getCurrentPiece() const {
    return currentPiece_;
}

const Tetromino& Game::getGhostPiece() const {
    return ghostPiece_;
}

TetrominoType Game::getHeldPiece() const {
    return heldPiece_;
}

const std::vector<TetrominoType>& Game::getNextQueue() const {
    return nextQueue_;
}

int Game::getScore() const {
    return score_;
}

int Game::getLines() const {
    return lines_;
}

int Game::getLevel() const {
    return level_;
}

double Game::getTime() const {
    // TODO: Implement timer
    return 0.0;
}

bool Game::isGameOver() const {
    return gameOver_;
}

void Game::initializeTetrominoBag() {
    // 7-bag random generator
    std::vector<TetrominoType> bag = {
        TetrominoType::I, TetrominoType::O, TetrominoType::T,
        TetrominoType::J, TetrominoType::L, TetrominoType::S, TetrominoType::Z
    };
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(bag.begin(), bag.end(), g);
    
    tetrominoBag_ = bag;
    
    // Fill initial next queue
    for (int i = 0; i < 6; ++i) {
        nextQueue_.push_back(getNextFromBag());
    }
}

TetrominoType Game::getNextFromBag() {
    if (tetrominoBag_.empty()) {
        // Refill bag
        std::vector<TetrominoType> bag = {
            TetrominoType::I, TetrominoType::O, TetrominoType::T,
            TetrominoType::J, TetrominoType::L, TetrominoType::S, TetrominoType::Z
        };
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(bag.begin(), bag.end(), g);
        
        tetrominoBag_ = bag;
    }
    
    TetrominoType next = tetrominoBag_.back();
    tetrominoBag_.pop_back();
    return next;
}

void Game::spawnNewPiece() {
    if (nextQueue_.empty()) {
        initializeTetrominoBag();
    }
    
    currentPiece_.type = nextQueue_.front();
    nextQueue_.erase(nextQueue_.begin());
    nextQueue_.push_back(getNextFromBag());
    
    currentPiece_.rotation = 0;
    currentPiece_.x = 3; // Center of 10-wide board
    currentPiece_.y = 0; // Top of board
    
    updateGhostPiece();
    
    if (!isValid(currentPiece_)) {
        gameOver_ = true;
        // Check if any part of the locked piece is above the visible area
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < BOARD_WIDTH; ++x) {
                if (board_[y][x] != TetrominoType::EMPTY) {
                    gameOver_ = true;
                    return;
                }
            }
        }
    }
}

bool Game::isValid(const Tetromino& piece) const {
    if (piece.type == TetrominoType::EMPTY) return false;

    const auto& shape = tetrominoShapes[static_cast<int>(piece.type)][piece.rotation];
    for (const auto& mino : shape) {
        int boardX = piece.x + mino.x;
        int boardY = piece.y + mino.y;

        // Check board boundaries
        if (boardX < 0 || boardX >= BOARD_WIDTH || boardY < 0 || boardY >= BOARD_HEIGHT) {
            return false;
        }

        // Check for collision with existing pieces on the board
        if (board_[boardY][boardX] != TetrominoType::EMPTY) {
            return false;
        }
    }
    return true;
}

void Game::lockPiece() {
    if (currentPiece_.type == TetrominoType::EMPTY) return;

    const auto& shape = tetrominoShapes[static_cast<int>(currentPiece_.type)][currentPiece_.rotation];
    for (const auto& mino : shape) {
        int boardX = currentPiece_.x + mino.x;
        int boardY = currentPiece_.y + mino.y;

        if (boardY >= 0 && boardY < BOARD_HEIGHT && boardX >= 0 && boardX < BOARD_WIDTH) {
            board_[boardY][boardX] = currentPiece_.type;
        }
    }
}

void Game::clearLines() {
    int linesCleared = 0;
    bool tSpin = false;

    if (lastActionWasRotation_ && currentPiece_.type == TetrominoType::T) {
        int corners = 0;
        int x = currentPiece_.x;
        int y = currentPiece_.y;

        if (x > 0 && y > 0 && board_[y - 1][x - 1] != TetrominoType::EMPTY) corners++;
        if (x < BOARD_WIDTH - 1 && y > 0 && board_[y - 1][x + 1] != TetrominoType::EMPTY) corners++;
        if (x > 0 && y < BOARD_HEIGHT - 1 && board_[y + 1][x - 1] != TetrominoType::EMPTY) corners++;
        if (x < BOARD_WIDTH - 1 && y < BOARD_HEIGHT - 1 && board_[y + 1][x + 1] != TetrominoType::EMPTY) corners++;

        if (corners >= 3) {
            tSpin = true;
        }
    }

    for (int y = BOARD_HEIGHT - 1; y >= 0; --y) {
        bool fullLine = true;
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (board_[y][x] == TetrominoType::EMPTY) {
                fullLine = false;
                break;
            }
        }
        
        if (fullLine) {
            // Remove this line
            board_.erase(board_.begin() + y);
            // Add empty line at top
            board_.insert(board_.begin(), std::vector<TetrominoType>(BOARD_WIDTH, TetrominoType::EMPTY));
            linesCleared++;
            y++; // Check this line again since we shifted everything down
        }
    }
    
    if (linesCleared > 0 || tSpin) {
        lines_ += linesCleared;

        // Scoring based on lines cleared at once
        int baseScore = 0;
        if (tSpin) {
            switch (linesCleared) {
                case 0: baseScore = 400; break; // T-Spin Mini
                case 1: baseScore = 800; break; // T-Spin Single
                case 2: baseScore = 1200; break; // T-Spin Double
                case 3: baseScore = 1600; break; // T-Spin Triple
            }
        } else {
            switch (linesCleared) {
                case 1: baseScore = 100; break; // Single
                case 2: baseScore = 300; break; // Double
                case 3: baseScore = 500; break; // Triple
                case 4: baseScore = 800; break; // Tetris
            }
        }
        score_ += baseScore * level_;

        // Increase level every 10 lines
        if (lines_ / 10 >= level_) {
            level_++;
            // Decrease drop interval as level increases, making the game faster
            dropInterval_ = std::max(0.1, 1.0 - (level_ - 1) * 0.05);
        }
    }
}

void Game::updateGhostPiece() {
    ghostPiece_ = currentPiece_;
    while (isValid(ghostPiece_)) {
        ghostPiece_.y++;
    }
    ghostPiece_.y--; // Move it back to the last valid position
} 