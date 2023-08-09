#include <cstdio>

#include <GL/glew.h>
#include "platform_linux_xlib.h"

#include <set>
#include <utility>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

GLuint program;
GLuint vb;

float vertices[] = { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f };
//float vertices[] = {-1.0f, 1.0f, -1.0f, -1.0f,1.0f, -1.0f };

bool Initialize()
{
    // shaders
    std::ifstream v("../vertex.glsl");
    std::stringstream vertex;
    vertex << v.rdbuf();
    std::ifstream f("../fragment.glsl");
    std::stringstream fragment;
    fragment << f.rdbuf();

    std::vector<std::pair<GLenum, std::string>> shaders = {
        {
            GL_VERTEX_SHADER,
            vertex.str()
        },
        {
            GL_FRAGMENT_SHADER,
            fragment.str()
        },
    };
    glewInit();
    program = glCreateProgram();

    for (const auto &s: shaders) {
        GLenum type = s.first;
        const std::string &source = s.second;

        const GLchar *src = source.c_str();

        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);

        glCompileShader(shader);

        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

            if (length > 1) {
                std::string log(length, '\0');
                glGetShaderInfoLog(shader, length, &length, &log[0]);
                printf("Shader compile failed:\n%s\n", log.c_str());
            } else {
                printf("Shader compile failed.\n");
            }

            return false;
        }

        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        if (length > 1) {
            std::string log(length, '\0');
            glGetProgramInfoLog(program, length, &length, &log[0]);
            printf("Program link failed:\n%s", log.c_str());
        } else {
            printf("Program link failed.\n");
        }

        return false;
    }

    // vertex buffer

    

    
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // initial state
    glUseProgram(program);
    
    GLfloat uResolution[2] = { 512, 512 };
    glUniform4fv(glGetUniformLocation(program, "uResolution"), 2, &uResolution[0]);

    GLfloat aRadius = 0.1;
    glUniform4fv(glGetUniformLocation(program, "aRadius"), 1, &aRadius);

    GLfloat aColor[4] = { 1, 1, 0, 1 };
    glUniform4fv(glGetUniformLocation(program, "aColor"), 4, &aColor[0]);
    
    GLint position = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(position);
    
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    

    return true;
}

void Render()
{
    glEnable( GL_DEPTH_TEST );
    glClearColor( 0.0, 0.0, 0.0, 1.0 );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glUseProgram(program);

    GLfloat uResolution[2] = { 512, 512 };
    glUniform4fv(glGetUniformLocation(program, "uResolution"), 2, &uResolution[0]);

    GLfloat aRadius = 50.0;
    glUniform4fv(glGetUniformLocation(program, "aRadius"), 1, &aRadius);

    GLfloat aColor[4] = { 1, 1, 0, 1 };
    glUniform4fv(glGetUniformLocation(program, "aColor"), 4, &aColor[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vb);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLint position = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(position);
    
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_POINTS, 0, 5);
    //glDrawArrays(GL_TRIANGLES, 0, 5);
	glUseProgram(0);
}

int main()
{
    const unsigned int client_width = 512;
    const unsigned int client_height = 512;

    platform_window_t *window = platform::create_window("Triangle", client_width, client_height);
    if (!window) {
        printf("Failed to create window.\n");
        return 1;
    }

    printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));

    if (!Initialize()) {
        printf("Scene initialization failed.\n");
        return 1;
    }

    while (true) {
        bool quit = platform::handle_events(window);
        if (quit) {
            break;
        }

        Render();

        platform::swap(window);
    }

    platform::destroy_window(window);

    delete window;

    return 0;
}
