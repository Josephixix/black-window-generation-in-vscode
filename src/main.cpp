#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;
const float PI = 3.14159265359f;

////////////////////////////////////////////////////////////
// Sphere generator with normals
////////////////////////////////////////////////////////////
vector<float> createSphere(float radius, int sectors, int stacks)
{
    vector<float> vertices;

    for(int i=0;i<stacks;i++)
    {
        float stack1 = PI/2 - i * PI/stacks;
        float stack2 = PI/2 - (i+1) * PI/stacks;

        float xy1 = radius*cos(stack1);
        float z1  = radius*sin(stack1);
        float xy2 = radius*cos(stack2);
        float z2  = radius*sin(stack2);

        for(int j=0;j<sectors;j++)
        {
            float sec1 = j*2*PI/sectors;
            float sec2 = (j+1)*2*PI/sectors;

            float x1 = xy1*cos(sec1), y1 = xy1*sin(sec1);
            float x2 = xy2*cos(sec1), y2 = xy2*sin(sec1);
            float x3 = xy2*cos(sec2), y3 = xy2*sin(sec2);
            float x4 = xy1*cos(sec2), y4 = xy1*sin(sec2);

            // Triangle 1
            vertices.insert(vertices.end(), {x1,y1,z1, x1/radius,y1/radius,z1/radius});
            vertices.insert(vertices.end(), {x2,y2,z2, x2/radius,y2/radius,z2/radius});
            vertices.insert(vertices.end(), {x3,y3,z2, x3/radius,y3/radius,z2/radius});
            // Triangle 2
            vertices.insert(vertices.end(), {x1,y1,z1, x1/radius,y1/radius,z1/radius});
            vertices.insert(vertices.end(), {x3,y3,z2, x3/radius,y3/radius,z2/radius});
            vertices.insert(vertices.end(), {x4,y4,z1, x4/radius,y4/radius,z1/radius});
        }
    }

    return vertices;
}

////////////////////////////////////////////////////////////
// Perspective Matrix
////////////////////////////////////////////////////////////
void perspective(float fov, float aspect, float near, float far, float* m)
{
    float tanHalf = tan(fov/2.0f);
    for(int i=0;i<16;i++) m[i]=0;
    m[0] = 1.0f/(aspect*tanHalf);
    m[5] = 1.0f/tanHalf;
    m[10] = -(far+near)/(far-near);
    m[11] = -1.0f;
    m[14] = -(2*far*near)/(far-near);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"3D Solid Solar System",NULL,NULL);
    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);

    ////////////////////////////////////////////////////////////
    // Shaders
    ////////////////////////////////////////////////////////////
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aNormal;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        out vec3 FragPos;
        out vec3 Normal;

        void main()
        {
            FragPos = vec3(model * vec4(aPos,1.0));
            Normal = mat3(model) * aNormal;
            gl_Position = projection * view * vec4(FragPos,1.0);
        }
    )";

    const char* fragmentShaderSrc = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 Normal;
        in vec3 FragPos;

        uniform vec3 objectColor;

        void main()
        {
            vec3 lightPos = vec3(0,0,0);
            vec3 lightColor = vec3(1.0);
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm,lightDir),0.0);
            vec3 result = (0.15 + diff) * objectColor;
            FragColor = vec4(result,1.0);
        }
    )";

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSrc,NULL);
    glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragmentShaderSrc,NULL);
    glCompileShader(fs);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram,vs);
    glAttachShader(shaderProgram,fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs); glDeleteShader(fs);

    ////////////////////////////////////////////////////////////
    // Sphere VAO/VBO
    ////////////////////////////////////////////////////////////
    vector<float> sphere = createSphere(1.0f,36,18);
    unsigned int VAO,VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sphere.size()*sizeof(float),sphere.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    ////////////////////////////////////////////////////////////
    // Stars
    ////////////////////////////////////////////////////////////
    vector<float> stars;
    for(int i=0;i<500;i++)
        stars.push_back((rand()%200-100)/5.0f), stars.push_back((rand()%200-100)/5.0f), stars.push_back((rand()%200-100)/5.0f);

    unsigned int starVAO,starVBO;
    glGenVertexArrays(1,&starVAO);
    glGenBuffers(1,&starVBO);
    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER,starVBO);
    glBufferData(GL_ARRAY_BUFFER,stars.size()*sizeof(float),stars.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    ////////////////////////////////////////////////////////////
    // Matrices
    ////////////////////////////////////////////////////////////
    float projection[16];
    perspective(45.0f*PI/180.0f,(float)SCR_WIDTH/SCR_HEIGHT,0.1f,500.0f,projection);

    float view[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,-50,1
    };

    ////////////////////////////////////////////////////////////
    // Planet settings
    ////////////////////////////////////////////////////////////
    float orbitRadius[9] = {3,5,7,9,12,15,18,21,24};
    float size[9] = {1.5,0.2,0.3,0.3,0.6,0.5,0.45,0.4,0.35};
    float speed[9] = {0,4,3.5,3,2,1.5,1.2,1,0.8};
    float color[9][3] = {
        {1,0.8,0}, // Sun
        {0.8,0.3,0.1}, // Mercury
        {0.9,0.6,0.2}, // Venus
        {0.2,0.5,1}, // Earth
        {0.9,0.4,0.2}, // Mars
        {1.0,0.9,0.3}, // Jupiter
        {0.9,0.7,0.5}, // Saturn
        {0.3,0.6,1}, // Uranus
        {0.2,0.3,0.9} // Neptune
    };

    ////////////////////////////////////////////////////////////
    // Render loop
    ////////////////////////////////////////////////////////////
    while(!glfwWindowShouldClose(window))
    {
        float time = glfwGetTime();
        glClearColor(0,0,0.05f,1); 
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE,projection);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE,view);

        ////////////////////////////////////////////////////////
        // Stars
        ////////////////////////////////////////////////////////
        glUniform3f(glGetUniformLocation(shaderProgram,"objectColor"),1,1,1);
        float identity[16]={
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,identity);
        glBindVertexArray(starVAO);
        glDrawArrays(GL_POINTS,0,stars.size()/3);

        ////////////////////////////////////////////////////////
        // Planets
        ////////////////////////////////////////////////////////
        glBindVertexArray(VAO);

        for(int i=0;i<9;i++)
        {
            float angle = time*speed[i];
            float x = orbitRadius[i]*cos(angle);
            float z = orbitRadius[i]*sin(angle);

            float model[16] = {
                size[i],0,0,0,
                0,size[i],0,0,
                0,0,size[i],0,
                x,0,z,1
            };

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,model);
            glUniform3f(glGetUniformLocation(shaderProgram,"objectColor"),color[i][0],color[i][1],color[i][2]);

            glDrawArrays(GL_TRIANGLES,0,sphere.size()/6);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}