#include "ShaderManager.h"

using namespace LEapsGL;

static char infoLog[1024];

#define PRINT_NAME(name) (#name)
const char* LEapsGL::GetShaderTypeName(GLenum type) {
	switch (type)
	{
	case GL_VERTEX_SHADER:
		return PRINT_NAME(GL_VERTEX_SHADER);
	case GL_FRAGMENT_SHADER:
		return PRINT_NAME(GL_FRAGMENT_SHADER);
	default:
		break;
	}
	return "Unknown Shader Type";
}

std::string LEapsGL::ReadFile(const char* filePath)
{
	std::string content;
	std::ifstream fileStream(filePath, std::ios::in);

	if (!fileStream.is_open()) {
		std::cout << "Could not read file " << filePath << ". File does not exist." << std::endl;
		return "";
	}

	std::stringstream buffer;
	buffer << fileStream.rdbuf();
	fileStream.close();

	return buffer.str();
}

unsigned int LEapsGL::ShaderProgram::getShaderIndex(GLuint shaderType)
{
	for (int i = 0; i < registeredShader.size(); i++) {
		if (registeredShader[i].shaderType == shaderType) return i;
	}
	return -1;
}

ShaderProgram& LEapsGL::ShaderProgram::MakeProgram()
{
	int state;

	for (auto shaderInfo : registeredShader) {
		glAttachShader(id, shaderInfo.shaderId);
	}
	glLinkProgram(id);
	glGetProgramiv(id, GL_LINK_STATUS, &state);
	if (!state) {
		glGetProgramInfoLog(id, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	return *this;
}

ShaderProgram& LEapsGL::ShaderProgram::DeleteShaderAll() {
	for (auto shaderInfo : registeredShader) {
		glDeleteShader(shaderInfo.shaderId);
	}
	return *this;
}

GLuint LEapsGL::ShaderProgram::getID() {
	return id;
}
void LEapsGL::ShaderProgram::setID(GLuint id) {
	this->id = id;
}
void LEapsGL::ShaderProgram::use()
{
	glUseProgram(id);
}
void LEapsGL::ShaderProgram::setBool(const std::string& name, bool value) const
{
	glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
}
void LEapsGL::ShaderProgram::setInt(const std::string& name, int value) const
{
	glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}
void LEapsGL::ShaderProgram::setFloat(const std::string& name, float value) const
{
	glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}
void LEapsGL::ShaderProgram::setMat4fv(const std::string& name, glm::mat4 value) const
{
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}
void LEapsGL::ShaderProgram::setVec3fv(const std::string& name, glm::vec3 value) {
    glUniform3fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
}
void LEapsGL::ShaderProgram::setVec4fv(const std::string& name, glm::vec4 value) {
    glUniform4fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
}
void LEapsGL::ShaderProgram::setFloat(const std::string& name, vector<float> v) const
{
	size_t vSize = v.size();
	switch (vSize)
	{
	case 1:
		glUniform1f(glGetUniformLocation(id, name.c_str()), v[0]);
		return;
	case 2:
		glUniform2f(glGetUniformLocation(id, name.c_str()), v[0], v[1]);
		return;
	case 3:
		glUniform3f(glGetUniformLocation(id, name.c_str()), v[0], v[1], v[2]);
		return;
	case 4:
		glUniform4f(glGetUniformLocation(id, name.c_str()), v[0], v[1], v[2], v[3]);
		return;
	default:
		break;
	}
}

void LEapsGL::ShaderProgram::setColor(const std::string& name, const Color& color, const GLuint type) const
{
	switch (type)
	{
	case GL_RGBA:
		glUniform4f(glGetUniformLocation(id, name.c_str()), color.r, color.g, color.b, color.a);
		return;
	case GL_RGB:
		glUniform3f(glGetUniformLocation(id, name.c_str()), color.r, color.g, color.b);
		return;
	default:
		std::cout << "ERROR::SHADER::PROGRAM::ColorSettingError\n";
		break;
	}
}

ShaderProgram& LEapsGL::ShaderProgram::CompileFromFile(GLuint shaderType, const char* path)
{	
	return LEapsGL::ShaderProgram::CompileFromSourceCode(shaderType, LEapsGL::ReadFile(path).c_str());
}
ShaderProgram& LEapsGL::ShaderProgram::CompileFromSourceCode(GLuint shaderType, const char* sourceCode)
{
	int state;
	int idx = getShaderIndex(shaderType);
	if (idx != -1) {
		glDeleteShader(registeredShader[idx].shaderId);
	}
	else {
		registeredShader.push_back({ shaderType, 0 });
		idx = registeredShader.size() - 1;
	}

	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &sourceCode, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &state);
	if (!state)
	{
		glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
		std::cerr << "Compile fail!! COMPILATION_FAILED " << GetShaderTypeName(shaderType) << " \n" << infoLog << std::endl;
	}
	registeredShader[idx].shaderId = shader;
	return *this;
}

