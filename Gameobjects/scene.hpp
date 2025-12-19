#pragma once
#include "GameObject.hpp"
#include "json.hpp"
#include <fstream>
#include "../Renderer/light.hpp"

using namespace std;
using namespace nlohmann;
class Scene {
public:
	Renderer::ShaderProgram& GlobalShaderProgram;//глобальный шейдер для сцены(по надобности можно сделать несколько шейдеров)
	
	std::vector<std::unique_ptr<GameObject>> objects;//нужно создавать именно массив ссылок а не объектов
	std::vector<Light*> lights;
	int width = 1920;
	int height = 1024;
	Scene(int w,int h,Renderer::ShaderProgram& shaderProgram) : GlobalShaderProgram(shaderProgram), width(w), height(h)
	{
		
	}

	void shadowRender()
	{
		for (int i = 0;i < lights.size(); i++)
		{

			lights[i]->Shadowmap.bindDepthTexture(lights[i]->Shadowmap.getDepthTex());
			GlobalShaderProgram.setMatrix4(("lightSpaceMatrices[" + std::to_string(i) + "]").c_str(), lights[i]->getLightSpaceMatrix());
			GlobalShaderProgram.setInt("shadowMaps[" + std::to_string(i) + "]", lights[i]->Shadowmap.getDepthTex());
			GlobalShaderProgram.setVec3("light_directions[" + std::to_string(i) + "]", lights[i]->lightDir);
			GlobalShaderProgram.setVec3("light_color", lights[i]->color);
			GlobalShaderProgram.setVec3("ambient_color", lights[i]->ambient);
			GlobalShaderProgram.setInt("lightCount", lights.size());
	
		};
	}
	GameObject* Addobject(const string name) 
	{
		auto obj = std::make_unique<GameObject>(name);
		GameObject* ptr = obj.get();
		obj->scene = this;
		objects.push_back(std::move(obj));

		return ptr; //Возврат ссылки на объект позволит сразу после создания назначить ему компоненты
	}
	
	void DeleteObject(int index) 
	{
		if (index < 0 || index >= objects.size()) return;
		auto object = objects[index].get();

		if (auto lightComp = object->GetComponent<Light>()) {
			lights.erase(std::remove(lights.begin(), lights.end(), lightComp), lights.end());
		}
		
		auto obj = std::remove_if(objects.begin(),objects.end(), [object](const std::unique_ptr<GameObject>& o) { return o.get() == object; });
		objects.erase(obj, objects.end());

		
	}

	void Update(float dt)
	{
		shadowRender();
		for(auto& obj: objects)
		{
			
			if (auto light = obj->GetComponent<Light>())
			{
				obj->GetComponent<Light>()->drawShade();
			}

		}

		for(auto& obj: objects)
		{
			obj->Update(dt);

		}
	}

	//сохранение всех объектов сцены в файл
	json SaveScene()
	{
		json scene;
		for (auto& obj : objects) {
			json jObj;
			jObj["name"] = obj->name;

			for (auto& c : obj->components)
			{
				json jComp = c->Serialize();

				jObj["components"].push_back(jComp);
			};

			scene["objects"].push_back(jObj);
		}; 
		ofstream ofs("scene.json");
		ofs << scene.dump();
		return scene;
	}


	//здесь загрузка сцены из json файла
	void LoadScene(){
		ifstream file("scene.json");
		json j;
		file >> j;

		std::vector<std::unique_ptr<GameObject>> scene;

		for (auto& obj : j["objects"]) {
			objects.push_back(LoadGameObject(obj)); //
		}

	}

	//Загрузка объекта и всех его компонентов 
	std::unique_ptr<GameObject> LoadGameObject(const json& j)
	{
		
		string name = j["name"];
		
		//создаем ссылку на объект

		auto obj = std::make_unique<GameObject>(name);

		for (auto& c : j["components"]) {
			string type = c["type"];

			//загрузка и присваивание каждого компонента по имени
			if (type == "Transform" )
			{
				Transform* t = obj->GetComponent<Transform>(); //создается по умолчанию
				t->Deserialize(c);
				t->OnEnable();
			}

			else if (type == "MeshRenderer" && !obj->GetComponent<MeshRenderer>())
			{
				MeshRenderer* mr = obj->AddComponent<MeshRenderer>(c["path"], GlobalShaderProgram);
				mr->Deserialize(c);
				mr->OnEnable();
			}
			else if (type == "Light")
			{
				Light* L = obj->AddComponent<Light>(width, height, objects, GlobalShaderProgram);
				L->Deserialize(c);
				L->OnEnable();
			}
			else if (type == "Terrain")
			{
				Terrain* T = obj->AddComponent<Terrain>(GlobalShaderProgram);
				T->Deserialize(c);
				T->OnEnable();
			}
		}
		return obj;
	}
};

