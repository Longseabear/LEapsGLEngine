#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ShaderManager.h>
#include <Color.h>
#include <Texture2D.h>
#include <Image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Camera.h>
#include <core/World.h>
#include <core/System.h>
#include <Events.h>
#include "ShaderPathConstant.h"
#include <FileSystem.h>
#include <Mesh.h>
#include <filesystem>
namespace fs = std::filesystem;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;


using Univ = LEapsGL::Universe;

LEapsGL::Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));

float deltaTime = 0.0f;	// 마지막 프레임과 현재 프레임 사이의 시간
float lastFrame = 0.0f; // 마지막 프레임의 시간

float lastX = SCR_WIDTH / 2, lastY = SCR_HEIGHT / 2;
bool firstMouse = true;
/*
    Components
*/

struct Position {
    using instance_type = glm::vec3; ///< output component type
};
struct Scale {
    using instance_type = glm::vec3; ///< output component type
};
struct ModelMatrix {
    using instance_type = glm::mat4;
};

class ModelMatrixCalcSystem : public LEapsGL::DefaultSystem {
    virtual void Update() override
    {
        auto& world = Univ::GetRelativeWorld<Position, Scale, ModelMatrix>();
        world.view<Position, Scale, ModelMatrix>().each([](auto& pos, auto& scale, auto& model) {
            model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, scale);
            });
    }
};

struct Camera {
    using container_type = LEapsGL::ContainerType::MemoryOptimized;
    Camera(const LEapsGL::Camera& c, bool _active) : camera(c), active(_active) {};

    LEapsGL::Camera camera;
    bool active;
};


struct MeshRenderSystem : public LEapsGL::DefaultSystem {
    virtual void Update() override {
        Univ::View<Camera>().each([](auto& cam) {
            glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = glm::mat4(1.0f);
        proj = glm::perspective(glm::radians(camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

            });
    }
};

/*
    System
*/
using BaseSystem = LEapsGL::BaseSystem;


class CameraSystem : public BaseSystem,
    public LEapsGL::EventSubscriber<LEapsGL::event::MousePositionDeltaEvent>,
    public LEapsGL::EventSubscriber<LEapsGL::event::MouseScrollEvent> {
public:
    void Configure() {
        Univ::subscribe<LEapsGL::event::MousePositionDeltaEvent>(this);
        Univ::subscribe<LEapsGL::event::MouseScrollEvent>(this);
    }
    void Unconfigure() {
        Univ::unsubscribeAll(this);
    }
    void Start() {
    }
    void Update() {
        auto& glfw = LEapsGL::Context::getGlobalContext<LEapsGL::GLFWContext>();

        float cameraSpeed = 2.5f * deltaTime;
        if (glfwGetKey(glfw.getWindow(), GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(glfw.getWindow(), GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(glfw.getWindow(), GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(LEFT, deltaTime);
        if (glfwGetKey(glfw.getWindow(), GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(RIGHT, deltaTime);
    }

    virtual void receive(const LEapsGL::event::MousePositionDeltaEvent& event) override
    {
        camera.processMouseMovement(event.xoffset, event.yoffset);
    }

    virtual void receive(const LEapsGL::event::MouseScrollEvent& event) override
    {
        camera.processMouseScroll(static_cast<float>(event.yoffset));
    }

    LEapsGL::BaseWorld& world = LEapsGL::Context::getGlobalContext<LEapsGL::BaseWorld>();
};

class InputSystem : public LEapsGL::DefaultSystem {

    virtual void Update() {
        auto& glfw = LEapsGL::Context::getGlobalContext<LEapsGL::GLFWContext>();
        if (glfwGetKey(glfw.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(glfw.getWindow(), true);
    }
};

//*************************************//

double mouseX, mouseY;

float vertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};


glm::vec3 cubePositions[] = {
    glm::vec3(0.0f,  0.0f,  0.0f),
    glm::vec3(2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3(1.3f, -2.0f, -2.5f),
    glm::vec3(1.5f,  2.0f, -2.5f),
    glm::vec3(1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};
// positions of the point lights
glm::vec3 pointLightPositions[] = {
    glm::vec3(0.7f,  0.2f,  2.0f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -12.0f),
    glm::vec3(0.0f,  0.0f, -3.0f)
};
unsigned int indices[] = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
};

#define PRINTF_VEC3(vec) "(" << vec.x << ", " << vec.y << ", "<< vec.z << ")"


/* Setting shader
*/

//---------------------------
// Component
// Entity Component
// Position
// Model

int main() {
    // Engine 초기 설정
    auto& world = Univ::GetBaseWorld();

    auto cameraEntity = world.Create();
    world.emplace<Camera>(cameraEntity, Camera{ LEapsGL::Camera(glm::vec3(0.0f, 0.0f, 5.0f)), true });

    Univ::registerSystem(new CameraSystem());
    Univ::registerSystem(new InputSystem());
    Univ::registerSystem(new ModelMatrixCalcSystem());
    Univ::registerSystem(new MeshRenderSystem());

    //glm::vec4 vec(1.0f, 0.0f, 0.0f, 1.0f);
    //std::cout << PRINTF_VEC3(vec) << "\n";

    //glm::mat4 trans = glm::mat4(1.0f);
    //trans = glm::translate(trans, glm::vec3(1.0f, 1.0f, 0.0f));
    //vec = trans * vec;
    //std::cout << PRINTF_VEC3(vec) << "\n";

    std::cout << "GL Tuotrial Start" << std::endl;

    std::cout << LEapsGL::ReadFile(LEapsGL::lightingMappingVertexShader) << "\n";
    std::cout << LEapsGL::ReadFile(LEapsGL::flashLightFragentShaderPath) << "\n";

    auto& glfwContext = LEapsGL::Context::getGlobalContext<LEapsGL::GLFWContext>();
    glfwContext.init(SCR_WIDTH, SCR_HEIGHT, "Test");
    auto* window = glfwContext.getWindow();

    // 현재 쓰레드의 주 컨텍스트로 지정하겠다고 알려줌
    LEapsGL::FrameBufferSizeEventSystem::Activate();
    LEapsGL::MouseEventSystem::Activate();
    LEapsGL::ScrollEventSystem::Activate();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // depth test true
    glEnable(GL_DEPTH_TEST);


    //★★★ 일시 함수 호출 안되게 생성자 호출 후 chaning하기

    // VBO: 실제 데이터(버텍스 좌표, 텍스쳐 좌표, 노멀벡터 등을 관리)
    // VAO: (버텍스 속성 배열의 구성, 활성화 여부, 데이터 형식, 오프셋)
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Vetex array를 바인드한다. 현재 컨텍스트에서 Bind vertex array는 해당 VAO에서 동작하게 된다.
    glBindVertexArray(VAO);

    // 사용할 buffer object를 VBO id로 설정한다.
    // 현재 사용할 VBO 객체의 아이디를 GL_ARRAY_BUFFER의 ARRAY_BUFFER state
    // GL_ARRAY_BUFFER는 State의 종류이며, VBO는 그 스테이트에 할당할 내 opengl 객체의 id를 의미한다.
    // 바인드는 아무일도 하지 않는다. 단지 현재 state에 id를 채원허어준다.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // ---------------------------------------------------- //
    // Vertex Array Object가 짜새
    // Vertex array object에 vertext buffer object와 Element buffer object가 다 붙는다.
    // ---------------------------------------------------- //

    // CPU to GPU로 옮긴다. 데이터를 할당하고 복사한다.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //glGenBuffers(1, &EBO);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // gl vertex attribute 초기화. VAO에 이미 바인드 되어있기 때문에
    // attribute number, data size, none, stride, start pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal vector attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //    auto texture = LEapsGL::Texture2D(LEapsGL::Image::LoadImage("resources/textures/container2.png"));
    int img_update_idx = 0;

    auto texture_spec = LEapsGL::Texture2DFactory::from_blank("Test", 30, 30, 3, LEapsGL::ImageFormat{ GL_RGB, GL_UNSIGNED_BYTE });
    {
        auto& texture = LEapsGL::Proxy::assure(texture_spec);
        auto& img = texture.getImage();
        GLubyte* img_raws = static_cast<GLubyte*>(img.pixels.get());
        img_raws[0] = 0;
        img_raws[1] = 0;
        img_raws[2] = 0;

        texture.AllocateDefaultSetting();
        texture.Apply();
    }

    auto sepecular_spec = LEapsGL::Texture2DFactory::from_file("resources/textures/container2_specular.png");
    auto& specularMap = LEapsGL::Proxy::assure(sepecular_spec);
    specularMap.AllocateDefaultSetting();
    specularMap.Apply();

    auto& texture = LEapsGL::Proxy::assure(texture_spec);
    auto& img = texture.getImage();
    GLubyte* img_raws = static_cast<GLubyte*>(img.pixels.get());

    glActiveTexture(GL_TEXTURE0);
    texture.bind();

    glActiveTexture(GL_TEXTURE1);
    specularMap.bind();

    // LightSource Test
    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    auto& ShaderManager = LEapsGL::Context::getGlobalContext<LEapsGL::ShaderManager>();
    auto lightingShader = ShaderManager.GetGlobalProgramRequestor("LightShader");
    cout << lightingShader.getHash() << "\n";
    ShaderManager.setShaderProgram(
        lightingShader,
        {
            LEapsGL::ShaderObjectFactory::from_file(LEapsGL::lightVertextShaderPath, GL_VERTEX_SHADER),
            LEapsGL::ShaderObjectFactory::from_file(LEapsGL::lightFragmentShaderPath, GL_FRAGMENT_SHADER),
        }
    );
    ShaderManager.activateKeepMemory(lightingShader);


    auto shaderProgramObject = ShaderManager.GetGlobalProgramRequestor("ShaderObjectLighting");

    ShaderManager.setShaderProgram(
        shaderProgramObject,
        {
            LEapsGL::ShaderObjectFactory::from_file(LEapsGL::MeshVertexShader, GL_VERTEX_SHADER),
            LEapsGL::ShaderObjectFactory::from_file(LEapsGL::flashLightLibFragmentShaderPath, GL_FRAGMENT_SHADER),
            LEapsGL::ShaderObjectFactory::from_file(LEapsGL::MeshFragmmentShader, GL_FRAGMENT_SHADER),
        }
    );
    std::cout << LEapsGL::ReadFile(LEapsGL::MeshFragmmentShader) << "\n";




    LEapsGL::Model ourModel(LEapsGL::FileSystem::getPath("resources/objects/backpack/backpack.obj"));

    /*
        [        Game loop        ]
    */
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // state setting
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // stage using

        // DRAW
        float timeValue = glfwGetTime();
        float greenValue = (sin(timeValue) / 2.0f) + 0.5f;

        //float radius = 10.0f;
        //float camX = sin(glfwGetTime()) * radius;
        //float camZ = cos(glfwGetTime()) * radius;

        // 우리가 움직이고 싶은 방향과 반대의 방향으로 scene을 이동시키고 있다는 것을 알아두세요.
        // view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
        // view = glm::lookAt(glm::vec3(camX, 0.0f, camZ), // camera pos
        //                glm::vec3(0.0f, 0.0f, 0.0f), // target (look at)
        //                glm::vec3(0.0f, 1.0f, 0.0f)); // up vector
        glm::mat4 view = camera.getViewMatrix();
        //        cout << "cameraPos " << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << " cameraFront" << cameraFront.x << " " << cameraFront.y << " " << cameraFront.z << "\n";

        glm::mat4 proj = glm::mat4(1.0f);
        proj = glm::perspective(glm::radians(camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        //        glm::mat4 trans = glm::mat4(1.0f);
                //trans = glm::translate(trans, glm::vec3(1.0f, -0.5f, 0.0f));        
                //trans = glm::rotate(trans, (float)timeValue, glm::vec3(0.0f, 0.0f, 1.0f));

        glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
        glm::vec3 lightDirection(-0.2f, -1.0f, -0.3f);
        lightPos.x += sin(currentFrame);
        lightPos.y += cos(currentFrame);
        {
            auto& program = LEapsGL::Proxy::assure(lightingShader);
            program.use();
            program.SetUniform("projection", proj);
            program.SetUniform("view", proj);

            glm::mat4 model = glm::mat4(1.0f);

            model = glm::translate(model, lightPos);
            model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
            program.SetUniform("model", model);

            glBindVertexArray(lightVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        auto& program = LEapsGL::Proxy::assure(shaderProgramObject);
        program.use();
        program.SetUniform("view", view);
        program.SetUniform("projection", proj);

        glm::vec3 lightColor;
        lightColor.x = sin(glfwGetTime() * 2.0f);
        lightColor.y = sin(glfwGetTime() * 0.7f);
        lightColor.z = sin(glfwGetTime() * 1.3f);

        lightColor = glm::vec3(1.0, 1.0, 1.0);
        glBindVertexArray(VAO);
        for (int i = 0; i < 10; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i] * 5.0f);
            float angle = 20.0f * i;

            model = glm::rotate(model, (float)glfwGetTime() * (glm::float32)(i / 5.0) + glm::radians(angle), glm::vec3(0.5f, 1.0f, 0.0f));

            program.SetUniform("model", model);
            program.SetUniform("viewPos", camera.position);

            program.SetUniform("dirLight.direction", glm::vec3{ -0.2f, -1.0f, -0.3f });
            program.SetUniform("dirLight.ambient", glm::vec3{ 0.05f, 0.05f, 0.05f });
            program.SetUniform("dirLight.diffuse", glm::vec3{ 0.4f, 0.4f, 0.4f } *0.2f);
            program.SetUniform("dirLight.specular", glm::vec3{ 0.5f, 0.5f, 0.5f });

            for (int i = 0; i < 4; i++) {
                char buffer[32];
                sprintf(buffer, "pointLights[%d].position", i);
                program.SetUniform(buffer, pointLightPositions[i]);
                sprintf(buffer, "pointLights[%d].ambient", i);
                program.SetUniform(buffer, glm::vec3{ 0.05f, 0.05f, 0.05f });
                sprintf(buffer, "pointLights[%d].diffuse", i);
                program.SetUniform(buffer, glm::vec3{ 0.2f, 0.2f, 0.2f });
                sprintf(buffer, "pointLights[%d].specular", i);
                program.SetUniform(buffer, glm::vec3{ 1.0f, 1.0f, 1.0f });
                sprintf(buffer, "pointLights[%d].constant", i);
                program.SetUniform(buffer, 1.0f);
                sprintf(buffer, "pointLights[%d].linear", i);
                program.SetUniform(buffer, 0.09f);
                sprintf(buffer, "pointLights[%d].quadratic", i);
                program.SetUniform(buffer, 0.032f);
            }

            //ShaderManager.SetUniform("material.diffuse", 0);
            //ShaderManager.SetUniform("material.specular", 1);
            program.SetUniform("material.shininess", 32.0f);

            program.SetUniform("spotLight.position", camera.position);
            program.SetUniform("spotLight.direction", camera.front);
            program.SetUniform("spotLight.ambient", glm::vec3{ 0.0f, 0.0f, 0.0f });
            program.SetUniform("spotLight.diffuse", glm::vec3{ 0.5f, 0.5f, 0.5f });
            program.SetUniform("spotLight.specular", glm::vec3{ 1.0f, 1.0f, 1.0f });
            program.SetUniform("spotLight.constant", 1.0f);
            program.SetUniform("spotLight.linear", 0.09f);
            program.SetUniform("spotLight.quadratic", 0.032f);
            program.SetUniform("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
            program.SetUniform("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

            ourModel.Draw(program);
            //            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        {
            // Draw Light Source
            auto& program = LEapsGL::Proxy::assure(lightingShader);
            program.use();
            program.SetUniform("projection", proj);
            program.SetUniform("view", view);

            // we now draw as many light bulbs as we have point lights.
            glBindVertexArray(lightVAO);
            for (unsigned int i = 0; i < 4; i++)
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, pointLightPositions[i]);
                model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
                program.SetUniform("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        //        ShaderManager.memoryClear();

                // optional
                //ShaderManager.RelinkToShaderProgram(lightingShader);

                // optional
                //ShaderManager.AssignShaderObjectSourceCode(lightingShader);

        glfwSwapBuffers(window);
        glfwPollEvents();

        //--------------------
        //auto& texture = LEapsGL::Proxy::get(texture_spec);
        //texture.AllocateDefaultSetting();
        //texture.Apply();
        //glActiveTexture(GL_TEXTURE0);
        //texture.bind();

        int upval = 0;
        //if (img_raws[img_update_idx] == 0) upval = 255;
        //img_raws[img_update_idx] = upval;
        //img_raws[img_update_idx + 1] = upval;
        //img_raws[img_update_idx + 2] = upval;
        //img_update_idx = (img_update_idx + 3) % (img.width * img.height * img.nrChannels);

        Univ::Update();
    }

    glfwTerminate();
    return 0;
}