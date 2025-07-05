#ifndef PALIBRIX_SHADER_H
#define PALIBRIX_SHADER_H

#include <GLES3/gl3.h>
#include <string>

class Shader {
public:
    // constructor reads and builds the shader
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();

    // activate/deactivate the shader
    void use() const;
    void unuse() const;

    // utility uniform functions
    void setBool(const std::string &name, bool value);
    void setInt(const std::string &name, int value);
    void setFloat(const std::string &name, float value);
    void setVec2(const std::string &name, float x, float y);
    void setVec3(const std::string &name, float x, float y, float z);
    void setMat4(const std::string &name, const float* mat);

    bool isLoaded() const { return loaded_; }
    GLuint getProgram() const { return programId_; }

private:
    GLuint programId_;
    bool loaded_;
    bool checkCompileErrors(GLuint shader, std::string type);
};

#endif //PALIBRIX_SHADER_H 