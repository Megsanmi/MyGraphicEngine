#pragma once

#include <glm/vec3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include "../Gameobjects/scene.hpp"

struct GLFWwindow;

class Camera : public Component
{
public:
	
	bool perspective_camera = true;
	bool escape = false;

	Transform* T;

	enum class ProjectionMode
	{
		Perspective,
		Orthographic
	};
	Camera(Renderer::ShaderProgram& s, const ProjectionMode m_projection_mode = ProjectionMode::Perspective);

	void UpdateCam(float dt);

	void OnEnable();
	void OnDisable();
	

	void drawInspector()
	{
		if (ImGui::CollapsingHeader("Camera"))
		{
			ImGui::Checkbox("enabled", &enabled);
			ImGui::DragFloat("speed", &speed, 0.05);
		}
	}
	json Serialize() override;
	void Deserialize(const json& j) override;

	void set_position(const glm::vec3& position);
	void set_rotation(const glm::vec3& rotation);
	void set_position_rotation(const glm::vec3& position, const glm::vec3& rotation);
	glm::mat4 get_view_matrix() const { return  m_view_matrix; }
	glm::mat4 get_projection_matrix() const { return m_projection_matrix; }


	void set_projection_mode(const ProjectionMode projection_mode);
	void process_input(GLFWwindow* window);



private:
	void update_veiw_matrix();
	void update_projection_matrix();
	float speed = 1.f;
		
	Renderer::ShaderProgram& shader;
	ProjectionMode m_projection_mode;
	glm::mat4 m_view_matrix;
	glm::mat4 m_projection_matrix;
};
