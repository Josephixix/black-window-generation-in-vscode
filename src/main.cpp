#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
using namespace std;

void framebuffer_size_callback(GLFWwindow* window,int width,int height){
    glViewport(0,0,width,height);
}

void checkShaderCompile(unsigned int shader){
    int success; char info[512];
    glGetShaderiv(shader,GL_COMPILE_STATUS,&success);
    if(!success){ glGetShaderInfoLog(shader,512,NULL,info); cout<<"SHADER ERROR\n"<<info<<endl;}
}

void checkProgramLink(unsigned int program){
    int success; char info[512];
    glGetProgramiv(program,GL_LINK_STATUS,&success);
    if(!success){ glGetProgramInfoLog(program,512,NULL,info); cout<<"PROGRAM LINK ERROR\n"<<info<<endl;}
}

struct Particle{
    float x, y;
    float vx, vy;
    float life; // remaining life
    float r, g, b;
};

vector<float> createCircle(float radius, int segments){
    vector<float> vertices;
    vertices.push_back(0); vertices.push_back(0); vertices.push_back(0);
    for(int i=0;i<=segments;i++){
        float angle=2.0f*3.1415926f*i/segments;
        vertices.push_back(radius*cos(angle));
        vertices.push_back(radius*sin(angle));
        vertices.push_back(0.0f);
    }
    return vertices;
}

int main(){
    if(!glfwInit()) return -1;
    GLFWwindow* window=glfwCreateWindow(800,800,"Supernova Explosion",NULL,NULL);
    if(!window){ glfwTerminate(); return -1;}
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    const char* vs = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        uniform vec2 offset;
        void main(){ gl_Position=vec4(aPos.x+offset.x, aPos.y+offset.y, aPos.z, 1.0); }
    )";
    const char* fs = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main(){ FragColor=vec4(color,1.0); }
    )";

    unsigned int vShader=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader,1,&vs,NULL); glCompileShader(vShader); checkShaderCompile(vShader);
    unsigned int fShader=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader,1,&fs,NULL); glCompileShader(fShader); checkShaderCompile(fShader);

    unsigned int shaderProgram=glCreateProgram();
    glAttachShader(shaderProgram,vShader); glAttachShader(shaderProgram,fShader);
    glLinkProgram(shaderProgram); checkProgramLink(shaderProgram);
    glDeleteShader(vShader); glDeleteShader(fShader);

    int segments=20;
    vector<float> circleVertices=createCircle(1.0f,segments);

    unsigned int VAO,VBO;
    glGenVertexArrays(1,&VAO); glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,circleVertices.size()*sizeof(float),circleVertices.data(),GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Create particles ---
    vector<Particle> particles;
    int particleCount=300;
    for(int i=0;i<particleCount;i++){
        float angle=((rand()%1000)/1000.0f)*2.0f*3.1415f;
        float speed=0.3f + ((rand()%100)/100.0f)*0.3f;
        particles.push_back({0.0f,0.0f, cos(angle)*speed, sin(angle)*speed, 1.0f, 1.0f, 0.8f, 0.2f});
    }

    float lastTime=glfwGetTime();

    while(!glfwWindowShouldClose(window)){
        float currentTime=glfwGetTime();
        float dt=currentTime-lastTime;
        lastTime=currentTime;

        glClearColor(0.0f,0.0f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        int colorLoc=glGetUniformLocation(shaderProgram,"color");
        int offsetLoc=glGetUniformLocation(shaderProgram,"offset");

        for(auto &p:particles){
            p.x += p.vx*dt;
            p.y += p.vy*dt;
            p.life -= dt*0.5f;

            if(p.life<0){ 
                // reset particle
                float angle=((rand()%1000)/1000.0f)*2.0f*3.1415f;
                float speed=0.3f + ((rand()%100)/100.0f)*0.3f;
                p.x=0; p.y=0; p.vx=cos(angle)*speed; p.vy=sin(angle)*speed; p.life=1.0f;
            }

            float ps=0.02f*(0.5f+p.life);
            vector<float> scaled;
            for(size_t k=0;k<circleVertices.size();k+=3){
                scaled.push_back(circleVertices[k]*ps);
                scaled.push_back(circleVertices[k+1]*ps);
                scaled.push_back(circleVertices[k+2]);
            }
            glBindBuffer(GL_ARRAY_BUFFER,VBO);
            glBufferSubData(GL_ARRAY_BUFFER,0,scaled.size()*sizeof(float),scaled.data());

            float r=1.0f;
            float g=0.5f + 0.5f*p.life;
            float b=0.0f;
            glUniform3f(colorLoc,r,g,b);
            glUniform2f(offsetLoc,p.x,p.y);
            glDrawArrays(GL_TRIANGLE_FAN,0,segments+2);
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