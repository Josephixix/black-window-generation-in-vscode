#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
using namespace std;

void framebuffer_size_callback(GLFWwindow* window,int width,int height){
    glViewport(0,0,width,height);
}

void checkShaderCompile(unsigned int shader){
    int success;
    char infoLog[512];
    glGetShaderiv(shader,GL_COMPILE_STATUS,&success);
    if(!success){
        glGetShaderInfoLog(shader,512,NULL,infoLog);
        cout<<"SHADER ERROR:\n"<<infoLog<<endl;
    }
}

void checkProgramLink(unsigned int program){
    int success;
    char infoLog[512];
    glGetProgramiv(program,GL_LINK_STATUS,&success);
    if(!success){
        glGetProgramInfoLog(program,512,NULL,infoLog);
        cout<<"PROGRAM LINK ERROR:\n"<<infoLog<<endl;
    }
}

vector<float> createCircle(float radius,int segments){
    vector<float> vertices;
    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
    for(int i=0;i<=segments;i++){
        float angle=2.0f*3.1415926f*i/segments;
        vertices.push_back(radius*cos(angle));
        vertices.push_back(radius*sin(angle));
        vertices.push_back(0.0f);
    }
    return vertices;
}

struct Particle{
    float angle;
    float radius;
    float speed;
};

int main(){
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window=glfwCreateWindow(800,800,"Black Hole",NULL,NULL);
    if(!window){ glfwTerminate(); return -1;}
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        uniform vec2 offset;
        void main(){gl_Position=vec4(aPos.x+offset.x,aPos.y+offset.y,aPos.z,1.0);}
    )";
    const char* fragShaderSrc = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main(){FragColor=vec4(color,1.0);}
    )";

    unsigned int vs=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSrc,NULL); glCompileShader(vs); checkShaderCompile(vs);
    unsigned int fs=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragShaderSrc,NULL); glCompileShader(fs); checkShaderCompile(fs);

    unsigned int shaderProgram=glCreateProgram();
    glAttachShader(shaderProgram,vs); glAttachShader(shaderProgram,fs);
    glLinkProgram(shaderProgram); checkProgramLink(shaderProgram);
    glDeleteShader(vs); glDeleteShader(fs);

    int segments=60;
    vector<float> circleVertices=createCircle(1.0f,segments);

    unsigned int VAO,VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,circleVertices.size()*sizeof(float),circleVertices.data(),GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Create particles ---
    vector<Particle> particles;
    for(int i=0;i<200;i++){
        float r=0.25f+((rand()%100)/100.0f)*0.3f;
        float a=((rand()%1000)/1000.0f)*2.0f*3.1415f;
        float s=0.5f+((rand()%100)/100.0f)*1.0f;
        particles.push_back({a,r,s});
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

        // --- Draw black hole glow (layers) ---
        for(int i=5;i>0;i--){
            float scale=0.12f+i*0.02f;
            vector<float> scaled;
            for(size_t k=0;k<circleVertices.size();k+=3){
                scaled.push_back(circleVertices[k]*scale);
                scaled.push_back(circleVertices[k+1]*scale);
                scaled.push_back(circleVertices[k+2]);
            }
            glBindBuffer(GL_ARRAY_BUFFER,VBO);
            glBufferSubData(GL_ARRAY_BUFFER,0,scaled.size()*sizeof(float),scaled.data());
            float intensity=0.2f+0.15f*i;
            glUniform3f(colorLoc,intensity,intensity*0.1f,0.0f);
            glUniform2f(offsetLoc,0.0f,0.0f);
            glDrawArrays(GL_TRIANGLE_FAN,0,segments+2);
        }

        // --- Draw black hole center ---
        vector<float> bhVertices;
        float bhScale=0.12f;
        for(size_t i=0;i<circleVertices.size();i+=3){
            bhVertices.push_back(circleVertices[i]*bhScale);
            bhVertices.push_back(circleVertices[i+1]*bhScale);
            bhVertices.push_back(circleVertices[i+2]);
        }
        glBindBuffer(GL_ARRAY_BUFFER,VBO);
        glBufferSubData(GL_ARRAY_BUFFER,0,bhVertices.size()*sizeof(float),bhVertices.data());
        glUniform3f(colorLoc,0.0f,0.0f,0.0f);
        glUniform2f(offsetLoc,0.0f,0.0f);
        glDrawArrays(GL_TRIANGLE_FAN,0,segments+2);

        // --- Draw particles (accretion disk) ---
        for(auto &p:particles){
            p.angle += 1.5f*dt; // spin faster
            p.radius -= 0.2f*dt; // spiral inward
            if(p.radius<0.12f) p.radius=0.55f; // reset to outer edge

            vector<float> scaled;
            float ps=0.015f;
            for(size_t k=0;k<circleVertices.size();k+=3){
                scaled.push_back(circleVertices[k]*ps);
                scaled.push_back(circleVertices[k+1]*ps);
                scaled.push_back(circleVertices[k+2]);
            }
            glBindBuffer(GL_ARRAY_BUFFER,VBO);
            glBufferSubData(GL_ARRAY_BUFFER,0,scaled.size()*sizeof(float),scaled.data());

            float x=p.radius*cos(p.angle);
            float y=p.radius*sin(p.angle);

            // gradient color: brighter near center
            float intensity=1.0f-(p.radius-0.12f)/0.43f;
            glUniform3f(colorLoc,1.0f,intensity*0.5f,0.0f);
            glUniform2f(offsetLoc,x,y);
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