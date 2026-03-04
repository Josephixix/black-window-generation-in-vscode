#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;
const float PI = 3.14159265359f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

struct Planet {
    float orbitRadius;
    float speed;
    float size;
    glm::vec3 color;
    float angle;
};

// Generate sphere vertices with triangle indices
void createSphere(float radius, int sectors, int stacks, vector<float> &vertices, vector<unsigned int> &indices)
{
    vertices.clear();
    indices.clear();

    for(int i = 0; i <= stacks; ++i)
    {
        float stackAngle = PI/2 - i * PI / stacks;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for(int j = 0; j <= sectors; ++j)
        {
            float sectorAngle = j * 2 * PI / sectors;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            // Vertex position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            // Normal for lighting
            glm::vec3 norm = glm::normalize(glm::vec3(x,y,z));
            vertices.push_back(norm.x);
            vertices.push_back(norm.y);
            vertices.push_back(norm.z);
        }
    }

    for(int i = 0; i < stacks; ++i)
    {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for(int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            if(i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1+1);
            }
            if(i != (stacks-1))
            {
                indices.push_back(k1+1);
                indices.push_back(k2);
                indices.push_back(k2+1);
            }
        }
    }
}

int main()
{
    // ---------------- GLFW ----------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"3D Solar System",NULL,NULL);
    if(!window){ glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ---------------- Shaders ----------------
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aNormal;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        out vec3 FragPos;
        out vec3 Normal;
        void main()
        {
            FragPos = vec3(model * vec4(aPos,1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * vec4(FragPos,1.0);
        }
    )";

    const char* fragmentShaderSrc = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 FragPos;
        in vec3 Normal;
        uniform vec3 objectColor;
        uniform vec3 lightColor;
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        void main()
        {
            // Ambient
            float ambientStrength = 0.2;
            vec3 ambient = ambientStrength * lightColor;

            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir),0.0);
            vec3 diffuse = diff * lightColor;

            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir),0.0),32);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 result = (ambient + diffuse + specular) * objectColor;
            FragColor = vec4(result,1.0);
        }
    )";

    auto compileShader = [](const char* src, GLenum type){
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader,1,&src,NULL);
        glCompileShader(shader);
        int success; char infoLog[512];
        glGetShaderiv(shader,GL_COMPILE_STATUS,&success);
        if(!success){
            glGetShaderInfoLog(shader,512,NULL,infoLog);
            cout << "Shader Compile Error: " << infoLog << endl;
        }
        return shader;
    };

    unsigned int vs = compileShader(vertexShaderSrc, GL_VERTEX_SHADER);
    unsigned int fs = compileShader(fragmentShaderSrc, GL_FRAGMENT_SHADER);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram,vs);
    glAttachShader(shaderProgram,fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // ---------------- Sphere ----------------
    vector<float> sphereVertices;
    vector<unsigned int> sphereIndices;
    createSphere(1.0f,36,18,sphereVertices,sphereIndices);

    unsigned int VAO,VBO,EBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sphereVertices.size()*sizeof(float),sphereVertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sphereIndices.size()*sizeof(unsigned int),sphereIndices.data(),GL_STATIC_DRAW);
    // position
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // ---------------- Planets ----------------
    vector<Planet> planets = {
        {3.0f,1.2f,0.3f, glm::vec3(1.0f,0.3f,0.2f),0.0f},
        {5.0f,0.8f,0.5f, glm::vec3(0.8f,1.0f,0.2f),0.0f},
        {7.0f,0.5f,0.6f, glm::vec3(0.2f,0.6f,1.0f),0.0f},
        {9.0f,0.35f,0.4f, glm::vec3(1.0f,0.4f,0.4f),0.0f}
    };

    // ---------------- Render Loop ----------------
    while(!glfwWindowShouldClose(window))
    {
        glClearColor(0.0f,0.0f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 view = glm::lookAt(glm::vec3(0,12,25), glm::vec3(0,0,0), glm::vec3(0,1,0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),(float)SCR_WIDTH/(float)SCR_HEIGHT,0.1f,100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(shaderProgram,"lightPos"),0.0f,0.0f,0.0f); // Sun position
        glUniform3f(glGetUniformLocation(shaderProgram,"lightColor"),1.0f,1.0f,1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram,"viewPos"),0.0f,12.0f,25.0f);

        glBindVertexArray(VAO);

        // Draw Sun
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model,glm::vec3(2.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
        glUniform3f(glGetUniformLocation(shaderProgram,"objectColor"),1.0f,0.6f,0.0f);
        glDrawElements(GL_TRIANGLES,sphereIndices.size(),GL_UNSIGNED_INT,0);

        // Draw planets
        for(auto &p: planets)
        {
            p.angle += p.speed*0.01f;
            float x = p.orbitRadius * cos(p.angle);
            float z = p.orbitRadius * sin(p.angle);

            model = glm::mat4(1.0f);
            model = glm::translate(model,glm::vec3(x,0.0f,z));
            model = glm::scale(model,glm::vec3(p.size));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shaderProgram,"objectColor"),1,glm::value_ptr(p.color));

            glDrawElements(GL_TRIANGLES,sphereIndices.size(),GL_UNSIGNED_INT,0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}