#include "Shader.h"
#include "AndroidOut.h"

Shader::Shader(const char *vertexSource, const char *fragmentSource) {
    bool vertexOk, fragmentOk, programOk;
    loaded_ = false;

    // 1. compile shaders
    GLuint vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSource, NULL);
    glCompileShader(vertex);
    vertexOk = checkCompileErrors(vertex, "VERTEX");

    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSource, NULL);
    glCompileShader(fragment);
    fragmentOk = checkCompileErrors(fragment, "FRAGMENT");

    if (vertexOk && fragmentOk) {
        // shader Program
        programId_ = glCreateProgram();
        glAttachShader(programId_, vertex);
        glAttachShader(programId_, fragment);
        glLinkProgram(programId_);
        programOk = checkCompileErrors(programId_, "PROGRAM");
        loaded_ = programOk;
    }

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (loaded_) {
        aout << "Shader loaded successfully. ID: " << programId_ << std::endl;
    } else {
        aout << "Shader failed to load." << std::endl;
    }
}

Shader::~Shader() {
    if (loaded_) {
        glDeleteProgram(programId_);
    }
}

void Shader::use() const {
    if (loaded_) {
        glUseProgram(programId_);
    }
}

void Shader::unuse() const {
    glUseProgram(0);
}

void Shader::setBool(const std::string &name, bool value) {
    if(loaded_) glUniform1i(glGetUniformLocation(programId_, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) {
    if(loaded_) glUniform1i(glGetUniformLocation(programId_, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) {
    if(loaded_) glUniform1f(glGetUniformLocation(programId_, name.c_str()), value);
}

void Shader::setVec2(const std::string &name, float x, float y) {
    if(loaded_) glUniform2f(glGetUniformLocation(programId_, name.c_str()), x, y);
}

void Shader::setVec3(const std::string &name, float x, float y, float z) {
    if(loaded_) glUniform3f(glGetUniformLocation(programId_, name.c_str()), x, y, z);
}

void Shader::setMat4(const std::string &name, const float* mat) {
    if(loaded_) glUniformMatrix4fv(glGetUniformLocation(programId_, name.c_str()), 1, GL_FALSE, mat);
}

bool Shader::checkCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            aout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            aout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
    }
    return true;
} 