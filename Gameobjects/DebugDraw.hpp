#pragma once

#define NodeArray BulletNodeArray
#include <btBulletDynamicsCommon.h>
#undef NodeArray
#include <vector>
#include <glm/glm.hpp>
#include "../Renderer/ShaderProgram.hpp"

// Структура для хранения данных вершины
struct DebugVertex {
    glm::vec3 pos;
    glm::vec3 color;
};

class MyDebugDrawer : public btIDebugDraw {
public:
    MyDebugDrawer(Renderer::ShaderProgram& s);
    ~MyDebugDrawer();

    // Метод инициализации шейдеров (нужно вызвать один раз после создания окна)
    void init();

    // Основной метод Bullet для рисования линий
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;

    // Метод для финальной отрисовки накопленного буфера
    void render(const glm::mat4& projection, const glm::mat4& view);

    // Обязательные методы интерфейса btIDebugDraw
    void setDebugMode(int debugMode) override { m_debugMode = debugMode; }
    int getDebugMode() const override { return m_debugMode; }

    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {}
    void reportErrorWarning(const char* warningString) override;
    void draw3dText(const btVector3& location, const char* textString) override {}

private:
    unsigned int VAO = 0, VBO = 0;
    Renderer::ShaderProgram& shaderProgram;
    int m_debugMode = DBG_DrawWireframe;
    std::vector<DebugVertex> vertices;

    // Вспомогательная функция для компиляции шейдеров
;
};