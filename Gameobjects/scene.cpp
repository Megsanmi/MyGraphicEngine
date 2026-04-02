#include "scene.hpp"

#define NodeArray BulletNodeArray
#include <btBulletDynamicsCommon.h>
#undef NodeArray
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "../Renderer/light.hpp"
#include "../Renderer/camera.hpp"
#include "RigidBody.hpp"
#include "Terrain.hpp"
#include "particle.hpp"
#include "../scripts/player.hpp"
#include "Animator.hpp"



Scene::Scene(int w, int h) : width(w), height(h)
{
	ShadowShader = new  Renderer::ShaderProgram(vsBuffer,fsBuffer);
	terrainShader = new  Renderer::ShaderProgram(textFromfile("Shaders/terrainShaderV.glsl"), textFromfile("Shaders/terrainShaderF.glsl"));
	GlobalShaderProgram =  new  Renderer::ShaderProgram(textFromfile("Shaders/vertex_shader.glsl"), textFromfile("Shaders/fragment_shader.glsl"));
	InitPhysics();
}

void Scene::shadowRender()
{
	GlobalShaderProgram->use();
	GlobalShaderProgram->setInt("lightCount", lights.size());

	for (unsigned int i = 0;i < lights.size(); i++)
	{
		lights[i]->Shadowmap->bindDepthTexture(lights[i]->Shadowmap->getDepthTex());
		GlobalShaderProgram->setMatrix4(("lightSpaceMatrices[" + std::to_string(i) + "]").c_str(), lights[i]->getLightSpaceMatrix());
		GlobalShaderProgram->setInt("shadowMaps[" + std::to_string(i) + "]", lights[i]->Shadowmap->getDepthTex());
		GlobalShaderProgram->setVec3("light_directions[" + std::to_string(i) + "]", lights[i]->lightDir);
		GlobalShaderProgram->setVec3("light_color", lights[i]->color);
		GlobalShaderProgram->setVec3("ambient_color", lights[i]->ambient);
	};

	for (auto& obj : objects)
	{

		if (auto light = obj->GetComponent<Light>())
		{
			obj->GetComponent<Light>()->drawShade();
		}

	}
}

GameObject* Scene::Addobject(const string name)
{
	auto obj = std::make_unique<GameObject>(name);
	GameObject* ptr = obj.get();

	obj->scene = this;
	obj->ID = IDs;
	IDs++;
	idToMap[obj->ID] = ptr;
	obj->modelCache = &modelCache;
	objects.push_back(std::move(obj));

	return ptr; 
}

void Scene::DeleteObject(int index)
{
	if (index < 0 || index >= objects.size()) return;
	auto object = objects[index].get();

	if (auto lightComp = object->GetComponent<Light>()) {
		lights.erase(std::remove(lights.begin(), lights.end(), lightComp), lights.end());
	}

	auto obj = std::remove_if(objects.begin(), objects.end(), [object](const std::unique_ptr<GameObject>& o) { return o.get() == object; });
	objects.erase(obj, objects.end());

}

void Scene::Update(float dt)
{
	// SET RENDER SETTINGS
	GlobalShaderProgram->setVec3("fogColor", glm::vec3(0.7f, 0.8f, 0.9f)); // например небо
	GlobalShaderProgram->setFloat("fogNear", fogNear);
	GlobalShaderProgram->setFloat("fogFar", fogFar);




	for (auto it = modelCache.begin(); it != modelCache.end(); )
	{
		if (it->second->ref == 0)
		{
			delete it->second;
			it = modelCache.erase(it); 
		}
		else
		{
			++it;
		}
	}

	for (auto& obj : objects)
	{
		if (auto rb = obj->GetComponent<RigidBody>())
		{
			rb->SyncTransformToPhysics();
         
		}
	}
	dynamicsWorld->stepSimulation(dt, 10);

	for (auto& obj : objects)
	{
		if (auto rb = obj->GetComponent<RigidBody>())
		{
			
			if (rb->body)
			{
				rb->SyncTransformFromPhysics();
				rb->body->activate();
			}
		}
		
	}

	
	for (auto& obj : objects)
	{
		if (auto animator = obj->GetComponent<Animator>())
		{
			animator->UpdateAnim(0.01f);
		}

		obj->Update(dt);
	}

	shadowRender();

	if(camera)
		camera->gameObject->GetComponent<Camera>()->UpdateCam(0.13f);
}

void Scene::InitPhysics()
{
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfig);
	broadphase = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(
		dispatcher, broadphase, solver, collisionConfig
	);

	broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
	
	dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

	debugDrawer = new MyDebugDrawer(*GlobalShaderProgram);
	debugDrawer->init();
	dynamicsWorld->setDebugDrawer(debugDrawer);
}

json Scene::SaveScene()
{
	json scene;
	scene["IDs"] = IDs;

	for (auto& obj : objects) {
		json jObj;
		jObj["name"] = obj->name;
		jObj["ID"] = obj->ID;
		jObj["parentID"] = obj->parentID;
		
		for (auto& c : obj->components)
		{
			json jComp = c->Serialize();

			if(!jComp.is_null())
				jObj["components"].push_back(jComp);
		};

		scene["objects"].push_back(jObj);
	};

	json renderSettings;
	renderSettings["fogFar"] = fogFar;
	renderSettings["fogNear"] = fogNear;


	scene["renderSettings"] = renderSettings;

	ofstream ofs("scene.json");
	ofs << scene.dump();
	return scene;
}

void Scene::LoadScene() {
	clear();

	InitPhysics();
	
	ifstream file("scene.json");
	json j;
	file >> j;

	IDs = j["IDs"];

	for (auto& obj : j["objects"]) {

		
		objects.push_back(LoadGameObject(obj));
		 
		
	}

	for (auto& objJson : j["objects"]) {
		if (objJson.contains("parentID")) {
			int currentID = objJson["ID"];
			int pID = objJson["parentID"];

			if (pID != -1) { // Если есть родитель
				GameObject* child = idToMap[currentID];
				GameObject* parent = idToMap[pID];

				if (child && parent) {
					parent->AddChild(child); // Устанавливаем связь
				}
			}
		}
	}

	if (j.contains("renderSettings")) fogFar = j["renderSettings"]["fogFar"];
	if (j.contains("renderSettings")) fogNear = j["renderSettings"]["fogNear"];
}

void Scene::clear()
{
	IDs = 0;
	idToMap.clear();

	if (dynamicsWorld)
	{
		for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);

			if (body && body->getMotionState())
				delete body->getMotionState();

			dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}
	}

	lights.clear();
	objects.clear();

	
}

std::unique_ptr<GameObject> Scene::LoadGameObject(const json& j)
{
	string name = j["name"];

	auto obj = std::make_unique<GameObject>(name);
	obj->ID = j["ID"];
	obj->modelCache = &modelCache;
	if(j.contains("parentID")) obj->parentID = j["parentID"];
	obj->scene = this;

	for (auto& c : j["components"]) {
		string type = c["type"];

		
		if (type == "Transform")
		{
			Transform* t = obj->GetComponent<Transform>(); //создается по умолчанию
			t->Deserialize(c);
			t->OnEnable();
		}

		else if (type == "MeshRenderer" && !obj->GetComponent<MeshRenderer>())
		{
			MeshRenderer* mr = obj->AddComponent<MeshRenderer>(c["path"], *terrainShader);;
			mr->Deserialize(c);
		}

		else if (type == "Light")
		{
			Light* L = obj->AddComponent<Light>(objects, *GlobalShaderProgram);
			L->Deserialize(c);
		}
		else if (type == "Terrain")
		{
			Terrain* T = obj->AddComponent<Terrain>(*GlobalShaderProgram);
			T->Deserialize(c);
			T->OnEnable();
		}

		else if (type == "Collider" && !obj->GetComponent<Collider>())
		{
			Collider* T = obj->AddComponent<Collider>();
			T->Deserialize(c);

		}
		else if (type == "CharacterController" && !obj->GetComponent<CharacterController>())
		{
			CharacterController* T = obj->AddComponent<CharacterController>();
			T->Deserialize(c);

		}

		else if (type == "RigidBody")
		{
			RigidBody* T = obj->AddComponent<RigidBody>();
			T->Deserialize(c);
		}
		else if (type == "Camera")
		{
			Camera* T = obj->AddComponent<Camera>(*GlobalShaderProgram);
			T->Deserialize(c);
			T->OnEnable();
		}
		else if (type == "ParticleSystem")
		{
			ParticleSystem* T = obj->AddComponent<ParticleSystem>(*GlobalShaderProgram);
			T->Deserialize(c);
			T->OnEnable();
		}
		else if (type == "Animator")
		{
			Animator* T = obj->AddComponent<Animator>();
			T->Deserialize(c);
			T->OnEnable();
		}
	}
	if (obj->GetComponent<Camera>())
		camera = obj->GetComponent<Transform>();

	idToMap[obj->ID] = obj.get();
	return obj;
}