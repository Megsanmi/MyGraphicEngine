#pragma once

#include <glad/glad.h>
#include <string>
#include "glm/mat4x4.hpp"

namespace Renderer {

	class ShaderProgram {
	public:
		ShaderProgram(const std::string& vertexShader, const std::string& fragmentShader);
		~ShaderProgram();

		bool isCompiled() const { return m_isCompiled; }
		void use() const;

		ShaderProgram() = delete;
		ShaderProgram(ShaderProgram&) = delete;
		ShaderProgram& operator=(const ShaderProgram&) = delete;
		ShaderProgram& operator=(ShaderProgram&&) noexcept;
		ShaderProgram(ShaderProgram&& shaderProgram) noexcept;
		void setMatrix4(const char* name, const glm::mat4& matrix) const;
		void setVec3(const std::string& name, const glm::vec3& value);

		void setInt(const std::string& name, int value) {
			glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
		}
		
		void setBool(const std::string& name, bool value) {
			glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
		}
		
		GLuint m_ID = 0;
	private:
		bool createShader(const std::string& source, const GLenum shaderType, GLuint& shaderID);
		bool m_isCompiled = false;

	};
}