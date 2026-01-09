#include <glad/glad.h>



#include <GLFW/glfw3.h> 
#include <iostream> 
#include <fstream>
#include <sstream>

#include "Renderer/ShaderProgram.hpp"
#include "Renderer/camera.hpp"
#include "Renderer/light.hpp"
#include "Renderer/ShadowMap.hpp"

#include <glm/vec3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/trigonometric.hpp>


#include "Gameobjects/Mesh.hpp"
#include "Gameobjects/GameObject.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Gameobjects/scene.hpp"
#include "Gameobjects/sprite.hpp"
#include "Gameobjects/particle.hpp"

#include "Gameobjects/RigidBody.hpp"
#include "Gameobjects/Terrain.hpp"


//Настройки окна
int WIDTH = 1920;
int HEIGHT = 1024;
float window_color[3] = {0.f,0.f,0.f };
float light_color[3] = { 0.6f,0.6f,0.6f };
float rlight_dir[3] = { 1.0f, -1.0f, -1.0f };

//Настройки камеры
bool perspective_camera = true;
bool perspective_light = true;



std::string vertex_shader = "NO VERTEX SHADER";
std::string fragment_shader = "NO FRAGMENT SHADER";

static char Objpath[128] = "assets/cirno.obj";
static char nameobj[128] = "Object";
static char texturepath[128] = "default.jpg";

int main() {
    // Чтение шейдера из .txt
    std::ifstream vertex_shader_path("Shaders/vertex_shader.glsl");

    if (vertex_shader_path.is_open()) {
        std::stringstream buffer;
        buffer << vertex_shader_path.rdbuf();
        vertex_shader = buffer.str();
        vertex_shader_path.close(); 
    }
    else {
        std::cerr << "Cant open vertex_shader.glsl!" << std::endl;
    }


    std::ifstream fragment_shader_path("Shaders/fragment_shader.glsl");

    if (fragment_shader_path.is_open()) {
        std::stringstream buffer;
        buffer << fragment_shader_path.rdbuf();
        fragment_shader = buffer.str();
        fragment_shader_path.close(); 
    }
    else {
        std::cerr << "cant open file fragment_shader.glsl!" << std::endl;
    }

    // 1. Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Error init GLFW!" << std::endl;
        return -1;
    }

    // 2. Создание окна
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "window", NULL, NULL);
    if (!window) {
        std::cerr << "Error init window!" << std::endl;
        glfwTerminate(); // Освобождаем ресурсы GLFW
        return -1;
    }

    // 3. Создание и установка текущего контекста OpenGL
    glfwMakeContextCurrent(window);
    

    if (!gladLoadGL()) {
        std::cerr << "Error init GLAD!" << std::endl;
        return -1;
    }
 
    glEnable(GL_DEPTH_TEST);


    // Компиляция шейдеров
    std::string fragmentShader(fragment_shader);
    Renderer::ShaderProgram shaderProgram(vertex_shader, fragmentShader);

    if (!shaderProgram.isCompiled())
    {
        std::cerr << "cant create shader" << std::endl;
        return  -1;
    };
    
    //IMgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    

    ImGui::StyleColorsDark();

    // Подключаем GLFW + OpenGL3 backend
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    //вертикаьная синхр 
    glfwSwapInterval(1);

    Scene scene(WIDTH, HEIGHT, shaderProgram);
    scene.window = window;
    scene.LoadScene();
    
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    while (!glfwWindowShouldClose(window)) {
        
        //очистка окнна
        glClearColor(window_color[0],window_color[1], window_color[2], 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        

        
        

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        double currentTime = glfwGetTime();
        nbFrames++;

        if (currentTime - lastTime >= 1.0) { 
            std::cout << "FPS: " << nbFrames << std::endl;
            nbFrames = 0;
            lastTime += 1.0;
        }

        // Тут окна
        
        static int selectedObjectIndex = -1;
        if (ImGui::BeginMainMenuBar())
        {

            if (ImGui::BeginMenu("settings"))
            {
                ImGui::Text("Background color:");
                ImGui::ColorEdit3("##background_color", window_color);
                ImGui::Checkbox("perspective ",&perspective_camera);
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Add"))
            {
                ImGui::Text("File name:");
                ImGui::InputText("##Path", Objpath, IM_ARRAYSIZE(Objpath));
                ImGui::Text("Object name:");
                ImGui::InputText("##gile_name", nameobj, IM_ARRAYSIZE(nameobj));
               
                if (ImGui::Button("Add object"))
                {
                    auto obj = scene.Addobject(nameobj);
                    obj->AddComponent<MeshRenderer>(Objpath, shaderProgram);

                    std::cout << "Created object: " << Objpath << std::endl;

                }

                if (ImGui::Button("Save scene"))
                {
                    scene.SaveScene();
                }

                if (ImGui::Button("Load scene"))
                {
                    scene.LoadScene();
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
        if (ImGui::Begin("Scene: "))
        {
            
            for (int i = 0; i < scene.objects.size(); ++i)
            {
                ImGui::PushID(i);  
                if (ImGui::Selectable(scene.objects[i]->name.c_str(), selectedObjectIndex == i))
                {
                    selectedObjectIndex = i;
                }
                ImGui::PopID();
            }            
        } 

        ImGui::End();
        
        ImGui::Begin("Inspector");  
      ;
        if (selectedObjectIndex >= 0)
        {

            auto& obj = scene.objects[selectedObjectIndex];

            //ImGui::InputText("Object: %s", &obj->name);
            if (ImGui::Button("Delete object"))
            {
                scene.DeleteObject(selectedObjectIndex);
            }
            ImGui::Separator();

            obj->drawInspector();

        }

        ImGui::End();

        
      
        scene.Update(0.013);
        
        
        
        
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            GameObject* obj = scene.Addobject("Cube");
            obj->AddComponent<MeshRenderer>("assets/Cube.obj", shaderProgram);

            obj->GetComponent<Transform>()->position = vec3(30, 40, 40);
            obj->AddComponent<RigidBody>(1.f);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        glfwPollEvents();
        glfwSwapBuffers(window); 
    }

    // 5. Очистка и завершение
    glfwDestroyWindow(window); // Уничтожаем окно
    glfwTerminate(); // Освобождаем все ресурсы GLFW
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}


