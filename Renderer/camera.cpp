#include "camera.hpp"
#include <glm/trigonometric.hpp>
#include <GLFW/glfw3.h> 
#include <glm/gtc/constants.hpp>
#include <glm/vec3.hpp>
#include <iostream> 

namespace Renderer {
	Camera::Camera(const glm::vec3& position,
		const glm::vec3& rotation,
		const ProjectionMode projection_mode)
		: m_position(position)
		, m_rotation(rotation)
		, m_projection_mode(projection_mode)
	{
		update_veiw_matrix();
		update_projection_matrix();
	}

	void Camera::update_veiw_matrix()
{
    float rx = -glm::radians(m_rotation.x);
    float ry = -glm::radians(m_rotation.y);
    float rz = -glm::radians(m_rotation.z);

    glm::mat4 rotate_matrix_x(
        1, 0, 0, 0,
        0, cos(rx), sin(rx), 0,
        0, -sin(rx), cos(rx), 0,
        0, 0, 0, 1
    );

    glm::mat4 rotate_matrix_y(
        cos(ry), 0, -sin(ry), 0,
        0, 1, 0, 0,
        sin(ry), 0, cos(ry), 0,
        0, 0, 0, 1
    );

    glm::mat4 rotate_matrix_z(
        cos(rz), sin(rz), 0, 0,
        -sin(rz), cos(rz), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    glm::mat4 translate_matrix(
        1.f, 0, 0, 0,
        0, 1.f, 0, 0,
        0, 0, 1.f, 0,
        -m_position.x, -m_position.y, -m_position.z, 1.0f
    );

    m_view_matrix = rotate_matrix_x * rotate_matrix_y * rotate_matrix_z * translate_matrix;
}	

	void Camera::update_projection_matrix()
	{
		if (m_projection_mode == ProjectionMode::Orthographic)
		{
			float r = 1.f;
			float t = 1.f;
			float f = 100;
			float n = 0.1f;
			m_projection_matrix = glm::mat4(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, -(f + n) / (f - n), -1,
				0, 0, -2 * f * n / (f - n), 0
			);
			
		}
		else 
		{
			float r = 1.6f*1.5;
			float t = 0.9f*1.5;
			float f = 100;
			float n = 0.1f;
			m_projection_matrix = glm::mat4(
				1 / r, 0, 0, 0,
				0, 1 / t, 0, 0,
				0, 0, -2 / (f - n), -1,
				0, 0, -2 * f * n / (f - n), 0
			);

		};
	}
	
	void Camera::set_position(const glm::vec3& position)
	{
		m_position = position;
		update_veiw_matrix();
	}

	void Camera::set_rotation(const glm::vec3& rotation)
	{
		m_rotation = rotation;
		update_veiw_matrix();
	}

	void Camera::set_position_rotation(const glm::vec3& position, const glm::vec3& rotation)
	{
		m_position = position;
		m_rotation = rotation;
		update_veiw_matrix();
	}
	void Camera::set_projection_mode(const ProjectionMode projection_mode)
	{
		m_projection_mode = projection_mode;
		update_projection_matrix();
	}

	void Camera::process_input(GLFWwindow* window)
	{
		static double lastX = 0;
		static double lastY = 0;
		static bool firstMouse = true;
		float speed = 0.2f;
		float rot_speed = 0.1f;

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		double xoffset = lastX - xpos;
		double yoffset = lastY - ypos; // перевернутый Y
		xoffset *= rot_speed;
		yoffset *= rot_speed;

		lastX = xpos;
		lastY = ypos;

		

		// Обновляем вращение камеры
		m_rotation.y += (float)xoffset;
		m_rotation.x += (float)yoffset;

		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float yaw = glm::radians(m_rotation.y);
		float pitch = glm::radians(m_rotation.x);

		// Вектор направления
		glm::vec3 forward(
			-sin(yaw),
			0,
			-cos(yaw)
		);

		glm::vec3 right(
			sin(yaw - glm::half_pi<float>()),
			0,
			cos(yaw - glm::half_pi<float>())
		);

		glm::vec3 up(0, 0.5f, 0);

		
		
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_position += forward * speed;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_position -= forward * speed;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_position += right * speed;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_position -= right * speed;
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) m_position += up * speed;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) m_position -= up * speed;
		

		
		update_veiw_matrix();
	}
		

}