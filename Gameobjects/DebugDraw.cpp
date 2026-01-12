#include "DebugDraw.hpp"
#include <glad/glad.h> // Подключаем ТОЛЬКО ТУТ
#include <gtc/type_ptr.hpp>
#include <iostream>


MyDebugDrawer::MyDebugDrawer(Renderer::ShaderProgram& s) : shaderProgram(s)
{
    // Конструктор остается пустым, инициализацию лучше делать в init()
}

MyDebugDrawer::~MyDebugDrawer() {
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    
}

void MyDebugDrawer::init() {
     

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Атрибут позиции (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)offsetof(DebugVertex, pos));

    // Атрибут цвета (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)offsetof(DebugVertex, color));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void MyDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
    vertices.push_back({ glm::vec3(from.x(), from.y(), from.z()), glm::vec3(color.x(), color.y(), color.z()) });
    vertices.push_back({ glm::vec3(to.x(), to.y(), to.z()), glm::vec3(color.x(), color.y(), color.z()) });
}

void MyDebugDrawer::render(const glm::mat4& projection, const glm::mat4& view) {
    if (vertices.empty()) return;

    glUseProgram(shaderProgram.m_ID);

    // Передача матриц в шейдер
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram.m_ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram.m_ID, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Загружаем данные в GPU (GL_DYNAMIC_DRAW так как данные меняются каждый кадр)
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(DebugVertex), vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINES, 0, (GLsizei)vertices.size());

    glBindVertexArray(0);
    vertices.clear(); // Очищаем список после отрисовки
}

void MyDebugDrawer::reportErrorWarning(const char* warningString) {
    std::cerr << "[Bullet Debug]: " << warningString << std::endl;
}

