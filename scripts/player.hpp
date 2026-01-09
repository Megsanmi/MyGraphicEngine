#pragma once
#include "../Gameobjects/GameObject.hpp"
#include "json.hpp"
#include "imgui/imgui.h"

using json = nlohmann::json;

class ExampleComponent : public Component
{
public:
    // =========================
    // ДАННЫЕ КОМПОНЕНТА
    // =========================

    float value = 0.0f;
    bool enabled = true;

    // =========================
    // КОНСТРУКТОР
    // =========================

    ExampleComponent() = default;
    ~ExampleComponent() override = default;

    // =========================
    // ЖИЗНЕННЫЙ ЦИКЛ
    // =========================

    void Start() override
    {
        // вызывается один раз (если реализуешь Start в Scene)
    }

    void OnEnable() override
    {
        // компонент добавлен или включён
    }

    void OnDisable() override
    {
        // компонент выключен или удаляется
    }

    void Update(float dt) override
    {
        if (!enabled) return;

        // логика каждый кадр
    }

    // =========================
    // СЕРИАЛИЗАЦИЯ
    // =========================

    json Serialize() override
    {
        return {
            { "type", "ExampleComponent" },
            { "value", value },
            { "enabled", enabled }
        };
    }

    void Deserialize(const json& j) override
    {
        if (j.contains("value"))   value = j["value"];
        if (j.contains("enabled")) enabled = j["enabled"];
    }

    // =========================
    // INSPECTOR (ImGui)
    // =========================

    void drawInspector() override
    {
        if (ImGui::CollapsingHeader("ExampleComponent"))
        {
            ImGui::Checkbox("Enabled", &enabled);
            ImGui::DragFloat("Value", &value, 0.1f);
        }
    }




};
