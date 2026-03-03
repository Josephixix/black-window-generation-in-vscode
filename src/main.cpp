#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void checkShaderCompile(unsigned int shader) {
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << endl;
    }
}

void checkProgramLink(unsigned int program) {
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cout << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }
}

vector<float> createCircle(float radius, int segments) {
    vector<float> vertices;
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    for(int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.1415926f * i / segments;
        vertices.push_back(radius * cos(angle));
        vertices.push_back(radius * sin(angle));
        vertices.push_back(0.0f);
    }
    return vertices;
}

struct Planet {
    float orbitRadius;
    float speed;
    float size;
    float color[3];
    float angle;
};

int main() {
    if(!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(900, 700, "Cool Solar System", NULL, NULL);
    if(!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // --- Shaders ---
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        uniform vec2 offset;
        void main() { gl_Position = vec4(aPos.x + offset.x, aPos.y + offset.y, aPos.z, 1.0); }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() { FragColor = vec4(color,1.0); }
    )";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vertexShaderSource,NULL);
    glCompileShader(vertexShader);
    checkShaderCompile(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fragmentShaderSource,NULL);
    glCompileShader(fragmentShader);
    checkShaderCompile(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram,vertexShader);
    glAttachShader(shaderProgram,fragmentShader);
    glLinkProgram(shaderProgram);
    checkProgramLink(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int circleSegments = 60;
    vector<float> circleVertices = createCircle(1.0f, circleSegments);

    unsigned int VAO,VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,circleVertices.size()*sizeof(float),circleVertices.data(),GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindVertexArray(0);

    vector<Planet> planets = {
        {0.25f, 1.2f, 0.06f, {1.0f,0.3f,0.2f},0.0f}, // Mercury
        {0.4f, 0.8f, 0.09f, {0.8f,1.0f,0.2f},0.0f},  // Venus
        {0.55f, 0.5f, 0.1f, {0.2f,0.6f,1.0f},0.0f},  // Earth
        {0.7f, 0.35f, 0.08f, {1.0f,1.0f,0.4f},0.0f}   // Mars
    };

    float lastTime = glfwGetTime();

    // --- Render loop ---
    while(!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // update angles
        for(auto &p: planets) {
            p.angle += p.speed * deltaTime;
            if(p.angle>2.0f*3.1415926f) p.angle -= 2.0f*3.1415926f;
        }

        glClearColor(0.0f,0.0f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        int colorLoc = glGetUniformLocation(shaderProgram,"color");
        int offsetLoc = glGetUniformLocation(shaderProgram,"offset");

        // --- Draw Sun ---
        float sunScale = 0.15f;
        vector<float> scaledSun;
        for(size_t i=0;i<circleVertices.size();i+=3){
            scaledSun.push_back(circleVertices[i]*sunScale);
            scaledSun.push_back(circleVertices[i+1]*sunScale);
            scaledSun.push_back(circleVertices[i+2]);
        }
        glBindBuffer(GL_ARRAY_BUFFER,VBO);
        glBufferSubData(GL_ARRAY_BUFFER,0,scaledSun.size()*sizeof(float),scaledSun.data());
        glUniform3f(colorLoc,1.0f,0.5f,0.0f);
        glUniform2f(offsetLoc,0.0f,0.0f);
        glDrawArrays(GL_TRIANGLE_FAN,0,circleSegments+2);

        // --- Draw planets and orbits ---
        for(auto &p: planets){
            // Draw orbit as line loop
            vector<float> orbit;
            for(int i=0;i<=circleSegments;i++){
                float angle = 2.0f*3.1415926f*i/circleSegments;
                orbit.push_back(p.orbitRadius*cos(angle));
                orbit.push_back(p.orbitRadius*sin(angle));
                orbit.push_back(0.0f);
            }
            glBindBuffer(GL_ARRAY_BUFFER,VBO);
            glBufferSubData(GL_ARRAY_BUFFER,0,orbit.size()*sizeof(float),orbit.data());
            glUniform3f(colorLoc,0.5f,0.5f,0.5f);
            glUniform2f(offsetLoc,0.0f,0.0f);
            glDrawArrays(GL_LINE_LOOP,0,circleSegments+1);

            // Draw planet
            vector<float> planetVertices;
            for(size_t i=0;i<circleVertices.size();i+=3){
                planetVertices.push_back(circleVertices[i]*p.size);
                planetVertices.push_back(circleVertices[i+1]*p.size);
                planetVertices.push_back(circleVertices[i+2]);
            }
            glBindBuffer(GL_ARRAY_BUFFER,VBO);
            glBufferSubData(GL_ARRAY_BUFFER,0,planetVertices.size()*sizeof(float),planetVertices.data());
            float x = p.orbitRadius*cos(p.angle);
            float y = p.orbitRadius*sin(p.angle);
            glUniform3f(colorLoc,p.color[0],p.color[1],p.color[2]);
            glUniform2f(offsetLoc,x,y);
            glDrawArrays(GL_TRIANGLE_FAN,0,circleSegments+2);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}