#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
using namespace std;

const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 800;
const float PI = 3.14159265359f;

// ---------------- 3D Math ----------------
void perspective(float fov, float aspect, float near, float far, float* m)
{
    float tanFov = tan(fov / 2.0f);
    for(int i=0;i<16;i++) m[i]=0;
    m[0]=1/(aspect*tanFov); m[5]=1/tanFov; m[10]=-(far+near)/(far-near); m[11]=-1; m[14]=-(2*far*near)/(far-near);
}

void lookAt(float eyeX,float eyeY,float eyeZ,float centerX,float centerY,float centerZ,float* m)
{
    float f[3]={centerX-eyeX, centerY-eyeY, centerZ-eyeZ};
    float len = sqrt(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]);
    for(int i=0;i<3;i++) f[i]/=len;

    float up[3]={0,1,0};
    float s[3]={ f[1]*up[2]-f[2]*up[1], f[2]*up[0]-f[0]*up[2], f[0]*up[1]-f[1]*up[0] };
    len = sqrt(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
    for(int i=0;i<3;i++) s[i]/=len;

    float u[3]={ s[1]*f[2]-s[2]*f[1], s[2]*f[0]-s[0]*f[2], s[0]*f[1]-s[1]*f[0] };

    for(int i=0;i<16;i++) m[i]=0;
    m[0]=s[0]; m[4]=s[1]; m[8]=s[2];
    m[1]=u[0]; m[5]=u[1]; m[9]=u[2];
    m[2]=-f[0]; m[6]=-f[1]; m[10]=-f[2];
    m[15]=1;
    m[12]=-eyeX*s[0]-eyeY*s[1]-eyeZ*s[2];
    m[13]=-eyeX*u[0]-eyeY*u[1]-eyeZ*u[2];
    m[14]=eyeX*f[0]+eyeY*f[1]+eyeZ*f[2];
}

// ---------------- Sphere ----------------
vector<float> createSphere(float radius,int sectors,int stacks)
{
    vector<float> vertices;
    for(int i=0;i<stacks;i++){
        float stackAngle1 = PI/2 - i*PI/stacks;
        float stackAngle2 = PI/2 - (i+1)*PI/stacks;
        float xy1 = radius*cos(stackAngle1), z1=radius*sin(stackAngle1);
        float xy2 = radius*cos(stackAngle2), z2=radius*sin(stackAngle2);
        for(int j=0;j<sectors;j++){
            float sectorAngle1 = j*2*PI/sectors;
            float sectorAngle2 = (j+1)*2*PI/sectors;
            float x1=xy1*cos(sectorAngle1), y1=xy1*sin(sectorAngle1);
            float x2=xy2*cos(sectorAngle1), y2=xy2*sin(sectorAngle1);
            float x3=xy2*cos(sectorAngle2), y3=xy2*sin(sectorAngle2);
            float x4=xy1*cos(sectorAngle2), y4=xy1*sin(sectorAngle2);
            // Triangles
            vertices.insert(vertices.end(),{x1,y1,z1,x1/radius,y1/radius,z1/radius});
            vertices.insert(vertices.end(),{x2,y2,z2,x2/radius,y2/radius,z2/radius});
            vertices.insert(vertices.end(),{x3,y3,z2,x3/radius,y3/radius,z2/radius});
            vertices.insert(vertices.end(),{x1,y1,z1,x1/radius,y1/radius,z1/radius});
            vertices.insert(vertices.end(),{x3,y3,z2,x3/radius,y3/radius,z2/radius});
            vertices.insert(vertices.end(),{x4,y4,z1,x4/radius,y4/radius,z1/radius});
        }
    }
    return vertices;
}

// ---------------- Disk ----------------
vector<float> createDisk(float innerR,float outerR,int segments)
{
    vector<float> vertices;
    for(int i=0;i<segments;i++){
        float theta1 = 2*PI*i/segments;
        float theta2 = 2*PI*(i+1)/segments;
        float x1=innerR*cos(theta1), z1=innerR*sin(theta1);
        float x2=outerR*cos(theta1), z2=outerR*sin(theta1);
        float x3=outerR*cos(theta2), z3=outerR*sin(theta2);
        float x4=innerR*cos(theta2), z4=innerR*sin(theta2);
        // Triangles
        vertices.insert(vertices.end(),{x1,0,z1,0,1,0});
        vertices.insert(vertices.end(),{x2,0,z2,0,1,0});
        vertices.insert(vertices.end(),{x3,0,z3,0,1,0});
        vertices.insert(vertices.end(),{x1,0,z1,0,1,0});
        vertices.insert(vertices.end(),{x3,0,z3,0,1,0});
        vertices.insert(vertices.end(),{x4,0,z4,0,1,0});
    }
    return vertices;
}

struct Particle{ float angle,radius,height,speed; };

int main(){
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window=glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"3D Black Hole",NULL,NULL);
    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    const char* vs=R"(
    #version 330 core
    layout(location=0) in vec3 aPos;
    layout(location=1) in vec3 aNormal;
    uniform mat4 model; uniform mat4 view; uniform mat4 projection;
    out vec3 FragPos; out vec3 Normal;
    void main(){
        FragPos=vec3(model*vec4(aPos,1.0));
        Normal=mat3(model)*aNormal;
        gl_Position=projection*view*vec4(FragPos,1.0);
    }
    )";

    const char* fs=R"(
    #version 330 core
    in vec3 FragPos; in vec3 Normal;
    out vec4 FragColor;
    uniform vec3 objectColor;
    void main(){
        vec3 lightPos=vec3(0,0,0);
        vec3 norm=normalize(Normal);
        vec3 lightDir=normalize(lightPos-FragPos);
        float diff=max(dot(norm,lightDir),0.0);
        vec3 color=(0.15+diff)*objectColor;
        FragColor=vec4(color,1.0);
    }
    )";

    unsigned int vShader=glCreateShader(GL_VERTEX_SHADER); glShaderSource(vShader,1,&vs,NULL); glCompileShader(vShader);
    unsigned int fShader=glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fShader,1,&fs,NULL); glCompileShader(fShader);
    unsigned int shaderProgram=glCreateProgram(); glAttachShader(shaderProgram,vShader); glAttachShader(shaderProgram,fShader); glLinkProgram(shaderProgram);
    glDeleteShader(vShader); glDeleteShader(fShader);

    // Black hole sphere
    vector<float> sphere=createSphere(0.5f,36,18);
    unsigned int sphereVAO,sphereVBO;
    glGenVertexArrays(1,&sphereVAO); glGenBuffers(1,&sphereVBO);
    glBindVertexArray(sphereVAO); glBindBuffer(GL_ARRAY_BUFFER,sphereVBO);
    glBufferData(GL_ARRAY_BUFFER,sphere.size()*sizeof(float),sphere.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

    // Disk
    vector<float> disk=createDisk(0.6f,1.5f,120);
    unsigned int diskVAO,diskVBO;
    glGenVertexArrays(1,&diskVAO); glGenBuffers(1,&diskVBO);
    glBindVertexArray(diskVAO); glBindBuffer(GL_ARRAY_BUFFER,diskVBO);
    glBufferData(GL_ARRAY_BUFFER,disk.size()*sizeof(float),disk.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

    // Particles
    vector<Particle> particles;
    for(int i=0;i<400;i++){
        float r=0.6f+((rand()%100)/100.0f)*0.9f;
        float a=((rand()%1000)/1000.0f)*2.0f*PI;
        float h=((rand()%100)/100.0f)*0.05f;
        float s=0.5f+((rand()%100)/100.0f);
        particles.push_back({a,r,h,s});
    }

    // Matrices
    float projection[16]; perspective(45*PI/180.0f,(float)SCR_WIDTH/SCR_HEIGHT,0.1f,50.0f,projection);

    while(!glfwWindowShouldClose(window)){
        float time=(float)glfwGetTime();
        float camX=sin(time*0.1f)*5, camZ=cos(time*0.1f)*5;
        float view[16]; lookAt(camX,1.5f,camZ,0,0,0,view);

        glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE,projection);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE,view);

        float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};

        // Black hole
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,identity);
        glUniform3f(glGetUniformLocation(shaderProgram,"objectColor"),0.02f,0.02f,0.02f);
        glBindVertexArray(sphereVAO); glDrawArrays(GL_TRIANGLES,0,sphere.size()/6);

        // Rotating disk
        float angle=time*1.5f, c=cos(angle), s=sin(angle);
        float diskModel[16]={c,0,s,0,0,1,0,0,-s,0,c,0,0,0,0,1};
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,diskModel);
        glUniform3f(glGetUniformLocation(shaderProgram,"objectColor"),1.0f,0.5f,0.1f);
        glBindVertexArray(diskVAO); glDrawArrays(GL_TRIANGLES,0,disk.size()/6);

        // Particles
        for(auto &p:particles){
            p.angle+=p.speed*0.01f; p.radius-=0.0008f;
            if(p.radius<0.5f) {p.radius=1.4f; p.angle=((rand()%1000)/1000.0f)*2*PI;}
            float px=p.radius*cos(p.angle), py=p.height, pz=p.radius*sin(p.angle);
            float particleModel[16]={0.02f,0,0,0,0,0.02f,0,0,0,0,0.02f,0,px,py,pz,1};
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,particleModel);
            glUniform3f(glGetUniformLocation(shaderProgram,"objectColor"),1.0f,0.6f,0.2f);
            glBindVertexArray(sphereVAO); glDrawArrays(GL_TRIANGLES,0,sphere.size()/6);
        }

        glfwSwapBuffers(window); glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}