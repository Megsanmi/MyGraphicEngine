#pragma once

#define NodeArray BulletNodeArray
#include <btBulletDynamicsCommon.h>
#undef NodeArray

#include <glm/glm.hpp>
#include <vector>
#include <iostream>

struct DebugLine
{
    glm::vec3 from;
    glm::vec3 to;
    glm::vec3 color;
};

class BulletDebugDrawer : public btIDebugDraw
{
public:
    std::vector<DebugLine> lines;
    int debugMode = btIDebugDraw::DBG_DrawWireframe |
        btIDebugDraw::DBG_DrawContactPoints;

    void drawLine(const btVector3& from,
        const btVector3& to,
        const btVector3& color) override
    {
        lines.push_back({
            { from.x(), from.y(), from.z() },
            { to.x(),   to.y(),   to.z()   },
            { color.x(), color.y(), color.z() }
            });
    }

    void drawContactPoint(const btVector3& point,
        const btVector3& normal,
        btScalar distance,
        int lifeTime,
        const btVector3& color) override
    {
        drawLine(point, point + normal * distance, color);
    }

    void reportErrorWarning(const char* warningString) override
    {
        std::cout << "[Bullet] " << warningString << std::endl;
    }

    void draw3dText(const btVector3&, const char*) override {}

    void setDebugMode(int mode) override
    {
        debugMode = mode;
    }

    int getDebugMode() const override
    {
        return debugMode;
    }

    void Clear()
    {
        lines.clear();
    }
};
