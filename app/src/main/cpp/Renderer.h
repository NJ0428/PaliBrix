#ifndef PALIBRIX_RENDERER_H
#define PALIBRIX_RENDERER_H

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <memory>

#include "Shader.h"
#include "Game.h"

class Renderer {
public:
    Renderer();
    virtual ~Renderer();

    void initRenderer();
    void updateRenderArea(int width, int height);
    void render(const Game& game);

private:
    void drawBoard(const Game& game);
    void drawPiece(const Tetromino& piece, bool isGhost);
    void drawUI(const Game& game);
    void drawBlock(int x, int y, TetrominoType type, bool isGhost = false);
    void drawBackground();
    void drawBorder();

    int width_;
    int height_;

    std::unique_ptr<Shader> blockShader_;
    GLuint vao_;
    GLuint vbo_;
};

#endif //PALIBRIX_RENDERER_H