#include "Renderer.h"
#include "AndroidOut.h"
#include "TetrominoData.h"
#include <vector>

// Helper to create an orthographic projection matrix
void createOrthoMatrix(float* mat, float left, float right, float bottom, float top, float near, float far) {
    mat[0] = 2.0f / (right - left);
    mat[1] = 0.0f;
    mat[2] = 0.0f;
    mat[3] = 0.0f;

    mat[4] = 0.0f;
    mat[5] = 2.0f / (top - bottom);
    mat[6] = 0.0f;
    mat[7] = 0.0f;

    mat[8] = 0.0f;
    mat[9] = 0.0f;
    mat[10] = -2.0f / (far - near);
    mat[11] = 0.0f;

    mat[12] = -(right + left) / (right - left);
    mat[13] = -(top + bottom) / (top - bottom);
    mat[14] = -(far + near) / (far - near);
    mat[15] = 1.0f;
}

// Simple vertex shader for drawing colored blocks
const char* VERTEX_SHADER =
    "#version 300 es\n"
    "layout(location = 0) in vec2 aPosition;\n"
    "uniform mat4 uProjection;\n"
    "uniform vec2 uOffset;\n"
    "uniform vec2 uScale;\n"
    "void main() {\n"
    "    gl_Position = uProjection * vec4((aPosition * uScale) + uOffset, 0.0, 1.0);\n"
    "}\n";

// Simple fragment shader for drawing colored blocks
const char* FRAGMENT_SHADER =
    "#version 300 es\n"
    "precision mediump float;\n"
    "out vec4 FragColor;\n"
    "uniform vec3 uColor;\n"
    "uniform float uAlpha;\n"
    "void main() {\n"
    "    FragColor = vec4(uColor, uAlpha);\n"
    "}\n";


Renderer::Renderer() : width_(0), height_(0), vao_(0), vbo_(0) {}

Renderer::~Renderer() {
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
    }
}

void Renderer::initRenderer() {
    aout << "Initializing Renderer..." << std::endl;

    // Clean up previous resources if they exist
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (blockShader_) {
        blockShader_.reset();
    }

    blockShader_ = std::make_unique<Shader>(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!blockShader_->isLoaded()) {
        aout << "Failed to load block shader" << std::endl;
        return;
    }

    // A single 1x1 quad (0,0) to (1,1)
    const std::vector<float> vertices = {
        0.0f, 1.0f,  // Top-left
        1.0f, 0.0f,  // Bottom-right
        0.0f, 0.0f,  // Bottom-left
        0.0f, 1.0f,  // Top-left
        1.0f, 1.0f,  // Top-right
        1.0f, 0.0f   // Bottom-right
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.05f, 0.1f, 0.15f, 1.0f); // Dark blue background
    aout << "Renderer Initialized" << std::endl;
}

void Renderer::updateRenderArea(int width, int height) {
    width_ = width;
    height_ = height;
    glViewport(0, 0, width_, height_);
}

void Renderer::render(const Game& game) {
    glClear(GL_COLOR_BUFFER_BIT);

    if (!blockShader_ || !blockShader_->isLoaded()) {
        return;
    }
    
    // Adjusted coordinate system - make game area wider to show UI elements
    float gameAreaWidth = 20.0f; // Increased width for UI
    float gameAreaHeight = 26.0f; // Increased height to show top rows
    float projection[16];
    createOrthoMatrix(projection, 0.0f, gameAreaWidth, gameAreaHeight, 0.0f, -1.0f, 1.0f);

    blockShader_->use();
    blockShader_->setMat4("uProjection", projection);
    
    // Draw game board background (dark gray rectangle)
    drawBackground();
    
    // Draw locked pieces on the board
    drawBoard(game);
    
    // Draw ghost piece (semi-transparent)
    drawPiece(game.getGhostPiece(), true);
    
    // Draw current falling piece
    drawPiece(game.getCurrentPiece(), false);
    
    // Draw board border
    drawBorder();
    
    // Draw UI elements
    drawUI(game);
}

void Renderer::drawBackground() {
    blockShader_->setVec3("uColor", 0.1f, 0.1f, 0.1f); // Dark gray
    blockShader_->setFloat("uAlpha", 1.0f);
    blockShader_->setVec2("uOffset", 4.0f, 3.0f); // Moved down from y=1.0f to y=3.0f
    blockShader_->setVec2("uScale", 10.0f, 22.0f); // Full 22 rows height
    
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::drawNextQueue(const Game& game) {
    const auto& nextQueue = game.getNextQueue();
    
    // Draw background for next queue area
    blockShader_->setVec3("uColor", 0.2f, 0.2f, 0.2f); // Darker gray
    blockShader_->setFloat("uAlpha", 0.8f);
    blockShader_->setVec2("uOffset", 15.0f, 3.0f);
    blockShader_->setVec2("uScale", 3.0f, 18.0f);
    
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Draw "NEXT" label background
    blockShader_->setVec3("uColor", 0.3f, 0.3f, 0.3f);
    blockShader_->setFloat("uAlpha", 1.0f);
    blockShader_->setVec2("uOffset", 15.0f, 3.0f);
    blockShader_->setVec2("uScale", 3.0f, 1.0f);
    
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Draw each piece in the next queue
    for (size_t i = 0; i < nextQueue.size() && i < 6; ++i) {
        float yOffset = 4.5f + i * 2.8f;
        drawPreviewPiece(nextQueue[i], 15.5f, yOffset, 0.4f);
    }
}

void Renderer::drawHoldPiece(const Game& game) {
    TetrominoType heldPiece = game.getHeldPiece();
    
    // Draw background for hold area
    blockShader_->setVec3("uColor", 0.2f, 0.2f, 0.2f); // Darker gray
    blockShader_->setFloat("uAlpha", 0.8f);
    blockShader_->setVec2("uOffset", 0.5f, 3.0f);
    blockShader_->setVec2("uScale", 3.0f, 4.0f);
    
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Draw "HOLD" label background
    blockShader_->setVec3("uColor", 0.3f, 0.3f, 0.3f);
    blockShader_->setFloat("uAlpha", 1.0f);
    blockShader_->setVec2("uOffset", 0.5f, 3.0f);
    blockShader_->setVec2("uScale", 3.0f, 1.0f);
    
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Draw held piece if it exists
    if (heldPiece != TetrominoType::EMPTY) {
        drawPreviewPiece(heldPiece, 1.0f, 4.5f, 0.4f);
    }
}

void Renderer::drawPreviewPiece(TetrominoType type, float x, float y, float scale) {
    if (type == TetrominoType::EMPTY) return;
    
    const auto& shape = tetrominoShapes[static_cast<int>(type)][0]; // Always use rotation 0 for preview
    
    // Calculate piece bounds for centering
    int minX = 4, maxX = -1, minY = 4, maxY = -1;
    for (const auto& mino : shape) {
        minX = std::min(minX, mino.x);
        maxX = std::max(maxX, mino.x);
        minY = std::min(minY, mino.y);
        maxY = std::max(maxY, mino.y);
    }
    
    // Center the piece in the preview area
    float centerOffsetX = -(maxX + minX) * scale * 0.5f;
    float centerOffsetY = -(maxY + minY) * scale * 0.5f;
    
    // Set color based on piece type
    float r=1, g=1, b=1;
    switch (type) {
        case TetrominoType::I: r=0.0f; g=1.0f; b=1.0f; break; // Cyan
        case TetrominoType::O: r=1.0f; g=1.0f; b=0.0f; break; // Yellow
        case TetrominoType::T: r=0.6f; g=0.2f; b=0.8f; break; // Purple
        case TetrominoType::J: r=0.0f; g=0.3f; b=1.0f; break; // Blue
        case TetrominoType::L: r=1.0f; g=0.5f; b=0.0f; break; // Orange
        case TetrominoType::S: r=0.0f; g=0.8f; b=0.0f; break; // Green
        case TetrominoType::Z: r=1.0f; g=0.0f; b=0.0f; break; // Red
        default: return;
    }
    
    blockShader_->setVec3("uColor", r, g, b);
    blockShader_->setFloat("uAlpha", 1.0f);
    
    // Draw each mino of the piece
    for (const auto& mino : shape) {
        float blockX = x + centerOffsetX + mino.x * scale;
        float blockY = y + centerOffsetY + mino.y * scale;
        
        blockShader_->setVec2("uOffset", blockX, blockY);
        blockShader_->setVec2("uScale", scale * 0.9f, scale * 0.9f);
        
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
}

void Renderer::drawBorder() {
    blockShader_->setVec3("uColor", 0.7f, 0.7f, 0.7f); // Light gray
    blockShader_->setFloat("uAlpha", 1.0f);
    blockShader_->setVec2("uScale", 1.0f, 1.0f);
    
    // Left border - adjusted position
    blockShader_->setVec2("uOffset", 3.8f, 3.0f);
    blockShader_->setVec2("uScale", 0.2f, 22.0f);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Right border - adjusted position  
    blockShader_->setVec2("uOffset", 14.0f, 3.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Bottom border - adjusted position
    blockShader_->setVec2("uOffset", 3.8f, 25.0f);
    blockShader_->setVec2("uScale", 10.4f, 0.2f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindVertexArray(0);
}

void Renderer::drawBoard(const Game& game) {
    const auto& board = game.getBoard();
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (board[y][x] != TetrominoType::EMPTY) {
                drawBlock(x, y, board[y][x], false);
            }
        }
    }
}

// This is defined in Game.cpp, need to include Game.h or move it
// extern const std::vector<std::vector<std::vector<Mino>>> tetrominoShapes;

void Renderer::drawPiece(const Tetromino& piece, bool isGhost) {
    if (piece.type == TetrominoType::EMPTY) return;
    
    const auto& shape = tetrominoShapes[static_cast<int>(piece.type)][piece.rotation];
    for (const auto& mino : shape) {
        drawBlock(piece.x + mino.x, piece.y + mino.y, piece.type, isGhost);
    }
}

void Renderer::drawUI(const Game& game) {
    // Draw next queue (6 pieces)
    drawNextQueue(game);
    
    // Draw hold piece
    drawHoldPiece(game);
}

void Renderer::drawBlock(int x, int y, TetrominoType type, bool isGhost) {
    float r=1, g=1, b=1; // Default to white
    switch (type) {
        case TetrominoType::I: r=0.0f; g=1.0f; b=1.0f; break; // Cyan
        case TetrominoType::O: r=1.0f; g=1.0f; b=0.0f; break; // Yellow
        case TetrominoType::T: r=0.6f; g=0.2f; b=0.8f; break; // Purple
        case TetrominoType::J: r=0.0f; g=0.3f; b=1.0f; break; // Blue
        case TetrominoType::L: r=1.0f; g=0.5f; b=0.0f; break; // Orange
        case TetrominoType::S: r=0.0f; g=0.8f; b=0.0f; break; // Green
        case TetrominoType::Z: r=1.0f; g=0.0f; b=0.0f; break; // Red
        default: return;
    }

    blockShader_->setVec3("uColor", r, g, b);
    blockShader_->setFloat("uAlpha", isGhost ? 0.3f : 1.0f); // Ghost pieces are transparent
    
    float boardOffsetX = 4.0f;
    float boardOffsetY = 3.0f; // Moved down from y=1.0f to y=3.0f
    blockShader_->setVec2("uOffset", boardOffsetX + x, boardOffsetY + y);
    blockShader_->setVec2("uScale", 0.9f, 0.9f); // Make blocks slightly smaller for visible grid
    
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
} 