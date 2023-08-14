
#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <kissfft/kiss_fft.h>
#include <math.h>
#include <rtaudio/RtAudio.h>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <time.h>
#include <chrono>
#define VERT_LENGTH 4 //x,y,z,volume

#define FRAMES 1024
#define NUM_POINTS 32
#define NUM_TRIANGLES 3 * 4 * 8


using namespace std::chrono;
using namespace std;

GLFWwindow* window;
GLuint program, program2, program3, program4;


GLuint vao, vao2, vao3, vao4, vbo, vbo2, vbo3, vbo4;
float radius = 1.0f;
float radius2 = 1.25f;
RtAudio adc(RtAudio::Api::LINUX_PULSE);
GLfloat circleVertices[NUM_POINTS * VERT_LENGTH * 3];
GLfloat triangleVertices[NUM_TRIANGLES * VERT_LENGTH * 3 + NUM_TRIANGLES * 3];
GLfloat quadVertices[6 * 3];

kiss_fft_cfg cfg;



float rawdata[FRAMES];
float freqs[FRAMES];

float red_rawdata[NUM_POINTS];
float red_freqs[NUM_POINTS];
float last_freqs[NUM_POINTS];

double lastTime = glfwGetTime();
int nbFrames = 0;

struct Point {
    float x;
    float y;
};

milliseconds start;

void Render();

enum VisMode{
  SPECTRUM,
  ORIGINAL
};

VisMode mode = SPECTRUM;


int record(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData) {
  if (status){
    std::cout << "Stream overflow detected!" << std::endl;
    return 0;
    }
  
  //printf("%d \n", nBufferFrames);
  kiss_fft_cpx in[FRAMES] = {};
  for (unsigned int i = 0; i < nBufferFrames; i++) {
    in[i].r = ((float *)inputBuffer)[i];
    rawdata[i] = ((float *)inputBuffer)[i];
  }

  kiss_fft_cpx out[FRAMES] = {};
  kiss_fft(cfg, in, out);
  for (int i = 0; i < FRAMES; i++) {
    freqs[i] = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
  }

  int sample_group = FRAMES / NUM_POINTS;
  int fft_group = (FRAMES/4) / NUM_POINTS;
  for(int i = 0; i < NUM_POINTS; i++) {
    red_rawdata[i] = 0;
    red_freqs[i] = 0;
    for(int j = 0; j < sample_group; j++) {
      red_rawdata[i] += rawdata[i * sample_group + j];
    }
    for(int j = 0; j < fft_group; j++) {
      red_freqs[i] += freqs[i * fft_group + j + 5];
    }
    red_freqs[i] += last_freqs[i];
    red_freqs[i] /= 2.0;
  }

  for(int i = 0; i < NUM_POINTS; i++) {
    last_freqs[i] = red_freqs[i];
  }
  // Do something with the data in the "inputBuffer" buffer.
  return 0;
}

void getdevices() {
  // Get the list of device IDs
  std::vector<unsigned int> ids = adc.getDeviceIds();
  if (ids.size() == 0) {
    std::cout << "No devices found." << std::endl;
    exit(0);
  }

  // Scan through devices for various capabilities
  RtAudio::DeviceInfo info;
  for (unsigned int n = 0; n < ids.size(); n++) {

    info = adc.getDeviceInfo(ids[n]);

    // Print, for example, the name and maximum number of output channels for
    // each device
    std::cout << "device name = " << info.name << std::endl;
    std::cout << "device id = " << ids[n] << std::endl;
    std::cout << ": maximum input channels = " << info.inputChannels << std::endl;
    std::cout << ": maximum output channels = " << info.outputChannels << std::endl;
  }
}

static void set_camera(float cam_x, float cam_y, float cam_z, float target_z) {
    GLuint programs[4] = {program, program2, program3, program4};
    for (int i = 0; i < 4; i++) {
        glUseProgram(programs[i]);

        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::rotate(trans, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        GLint uniTrans = glGetUniformLocation(programs[i], "model");
        glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(trans));
        
        glm::mat4 view = glm::lookAt(
            glm::vec3(cam_x, cam_y, cam_z),
            glm::vec3(0.0f, 0.0f, target_z),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        GLint uniView = glGetUniformLocation(programs[i], "view");
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 3840.0f / 2160.0f, 1.0f, 10.0f);
        GLint uniProj = glGetUniformLocation(programs[i], "proj");
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_S) {
      mode = VisMode::SPECTRUM;
    } else if (key == GLFW_KEY_O) {
      mode = VisMode::ORIGINAL;
    } else if (key == GLFW_KEY_F) {
      set_camera(0.0f, -4.0f, 4.0f, -0.5f);
    } else if (key == GLFW_KEY_R) {
      set_camera(0.0f, -0.1f, 5.5f, 0.0f);
    }
}

static void resize(GLFWwindow* window, int width, int height){
	  glViewport(0,0,width,height);
    
    GLuint programs[4] = {program, program2, program3, program4};
    for (int i = 0; i < 4; i++) {
        glUseProgram(programs[i]);

      GLfloat uResolution[2] = { (float)width, (float)height };
      glUniform2fv(glGetUniformLocation(programs[i], "iResolution"), 1, uResolution);

      glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 1.0f, 10.0f);
      glUniformMatrix4fv(glGetUniformLocation(programs[i], "proj"), 1, GL_FALSE, glm::value_ptr(proj));
    }
}

    // Get the depth buffer value at this pixel.   
    

bool loadShaders(GLuint *program, std::vector<std::pair<GLenum, std::string>> shaders) {
  for (const auto &s : shaders) {
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

    glAttachShader(*program, shader);
  }

  glLinkProgram(*program);

  GLint linked = 0;
  glGetProgramiv(*program, GL_LINK_STATUS, &linked);

  if (!linked) {
    GLint length = 0;
    glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &length);

    if (length > 1) {
      std::string log(length, '\0');
      glGetProgramInfoLog(*program, length, &length, &log[0]);
      printf("Program link failed:\n%s", log.c_str());
    } else {
      printf("Program link failed.\n");
    }

    return false;
  }
  return true;
}

struct Point rotate(struct Point p, struct Point o, float theta) {
  return {cos(theta) * (p.x-o.x) - sin(theta) * (p.y-o.y) + o.x, sin(theta) * (p.x-o.x) + cos(theta) * (p.y-o.y) + o.y};
}

bool Initialize() {
  getdevices();

  start = duration_cast< milliseconds >(
      system_clock::now().time_since_epoch()
  );

  RtAudio::StreamParameters parameters;
  parameters.deviceId = adc.getDefaultInputDevice();
  //parameters.deviceId = 132;
  parameters.nChannels = 1;
  parameters.firstChannel = 0;
  unsigned int sampleRate = 48000;
  unsigned int bufferFrames = FRAMES;

  cfg = kiss_fft_alloc(FRAMES, 0, NULL, NULL);

  if (adc.openStream(NULL, &parameters, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &record)) {
    std::cout << '\n' << adc.getErrorText() << '\n' << std::endl;
    exit(0); // problem with device settings
  }

  // Stream is open ... now start it.
  if (adc.startStream()) {
    std::cout << adc.getErrorText() << std::endl;
  }

  // shaders
  std::stringstream vertex;
  vertex << std::ifstream("../vertex.glsl").rdbuf();
  std::stringstream fragment;
  fragment << std::ifstream("../fragment.glsl").rdbuf();
  std::vector<std::pair<GLenum, std::string>> shaders = {
      {GL_VERTEX_SHADER, vertex.str()},
      {GL_FRAGMENT_SHADER, fragment.str()},
  };
  std::stringstream vertex2;
  vertex2 << std::ifstream("../vertex2.glsl").rdbuf();
  std::stringstream fragment2;
  fragment2 << std::ifstream("../fragment2.glsl").rdbuf();
  std::vector<std::pair<GLenum, std::string>> shaders2 = {
      {GL_VERTEX_SHADER, vertex2.str()},
      {GL_FRAGMENT_SHADER, fragment2.str()},
  };

  std::stringstream vertex3;
  vertex3 << std::ifstream("../center_vertex.glsl").rdbuf();
  std::stringstream fragment3;
  fragment3 << std::ifstream("../center_fragment.glsl").rdbuf();
  std::vector<std::pair<GLenum, std::string>> shaders3 = {
      {GL_VERTEX_SHADER, vertex3.str()},
      {GL_FRAGMENT_SHADER, fragment3.str()},
  };

  std::stringstream vertex4;
  vertex4 << std::ifstream("../background_vertex.glsl").rdbuf();
  std::stringstream fragment4;
  fragment4 << std::ifstream("../background_fragment.glsl").rdbuf();
  std::vector<std::pair<GLenum, std::string>> shaders4 = {
      {GL_VERTEX_SHADER, vertex4.str()},
      {GL_FRAGMENT_SHADER, fragment4.str()},
  };
  printf("Creating programs \n");
  program = glCreateProgram();
  if (!loadShaders(&program, shaders)) {
    return false;
  }
  program2 = glCreateProgram();
  if (!loadShaders(&program2, shaders2)) {
    return false;
  }
  program3 = glCreateProgram();
  if (!loadShaders(&program3, shaders3)) {
    return false;
  }
  program4 = glCreateProgram();
  if (!loadShaders(&program4, shaders4)) {
    return false;
  }
  printf("Created programs \n");

  

  set_camera(0.0f, -0.1f, 5.5f, 0.0f);

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(circleVertices), circleVertices, GL_STATIC_DRAW);
  glUseProgram(program);
  glVertexAttribPointer(0, VERT_LENGTH, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (GLvoid*)(4 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);

  glGenVertexArrays(1, &vao2);
  glGenBuffers(1, &vbo2);
  glBindVertexArray(vao2);
  glBindBuffer(GL_ARRAY_BUFFER, vbo2);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);
  glUseProgram(program2);
  glVertexAttribPointer(0, VERT_LENGTH, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (GLvoid*)(4 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);

  glGenVertexArrays(1, &vao3);
  glGenBuffers(1, &vbo3);
  glBindVertexArray(vao3);
  glBindBuffer(GL_ARRAY_BUFFER, vbo3);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
  glUseProgram(program3);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);

  glGenVertexArrays(1, &vao4);
  glGenBuffers(1, &vbo4);
  glBindVertexArray(vao4);
  glBindBuffer(GL_ARRAY_BUFFER, vbo4);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
  glUseProgram(program4);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_POINT_SMOOTH);


  glLineWidth(5.0);
  
  printf("Init finished \n");
  return true;
}

float cur = 0.0;

void Render() {
  glClear(GL_COLOR_BUFFER_BIT);

  double currentTime = glfwGetTime();
  nbFrames++;
  if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1 sec ago
      // printf and reset timer
      printf("%f ms/frame\n", 1000.0/double(nbFrames));
      nbFrames = 0;
      lastTime += 1.0;
  }
  GLfloat bary_a[3] = {1.0f, 0.0f, 0.0f};
  GLfloat bary_b[3] = {0.0f, 1.0f, 0.0f};
  GLfloat bary_c[3] = {0.0f, 0.0f, 1.0f};


  std::vector<float> vertices;

  for (int i = 0; i < NUM_POINTS; i++) {
      float theta1 = 2.0f * M_PI * float(i - 0.1) / float(NUM_POINTS);
      float theta2 = 2.0f * M_PI * float(i + 1) / float(NUM_POINTS);
      float theta3 = 2.0f * M_PI * float(i + 0.5) / float(NUM_POINTS);

      float theta_corr = 2.0f * M_PI * float(i + 0.5) / float(NUM_POINTS);

      float sound = red_freqs[5] * 0.01 + 1.0;
      if (mode == SPECTRUM) {
        sound = red_freqs[i] * 0.05 + 1.0;
      }
      float x1 = radius * cosf(theta1);
      float y1 = radius * sinf(theta1);
      float x2 = sound * radius2 * cosf(theta_corr);
      float y2 = sound * radius2 * sinf(theta_corr);
      float x3 = radius * cosf(theta2);
      float y3 = radius * sinf(theta2);
      float x4 = radius * cosf(theta3);
      float y4 = radius * sinf(theta3);

      float a[4] = {x1,y1,0.0,0.0};
      float b[4] = {x2,y2,0.1,0.0};
      float c[4] = {x3,y3,0.0,0.0};
      float d[4] = {x4,y4,0.0,0.0};
      vertices.insert(vertices.end(), std::begin(a), std::end(a));
      vertices.insert(vertices.end(), std::begin(bary_a), std::end(bary_a));
      vertices.insert(vertices.end(), std::begin(b), std::end(b));
      vertices.insert(vertices.end(), std::begin(bary_b), std::end(bary_b));
      vertices.insert(vertices.end(), std::begin(d), std::end(d));
      vertices.insert(vertices.end(), std::begin(bary_c), std::end(bary_c));
      vertices.insert(vertices.end(), std::begin(d), std::end(d));
      vertices.insert(vertices.end(), std::begin(bary_a), std::end(bary_a));
      vertices.insert(vertices.end(), std::begin(b), std::end(b));
      vertices.insert(vertices.end(), std::begin(bary_b), std::end(bary_b));
      vertices.insert(vertices.end(), std::begin(c), std::end(c));
      vertices.insert(vertices.end(), std::begin(bary_c), std::end(bary_c));
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);



  std::vector<float> triangle_vertices;

  for (int angle = 0; angle < 8; angle++) {
    float theta = (M_PI_4) * angle;
    struct Point o1 = { -1.2f, 0.0};
    struct Point o2 = { 0.0, 0.0 };

    int draws = 4;

    if(angle % 2 == 0) {
      draws = 1;
    }

    for (int j = 0; j < draws; j++) {
      float beta = -(M_PI_2) * j;
      
      for (int i = 3; i > 0; i--) {
          struct Point p1 = rotate({ -2.0f + i * 0.05f, 0.25f + i * 0.1f}, o1, beta);
          struct Point p2 = rotate({ -2.0f + i * 0.05f, -0.25f - i * 0.1f}, o1, beta);
          struct Point p3 = rotate({ -1.75f + i * 0.15f, 0.0}, o1, beta);

          p1 = rotate(p1, o2, theta);
          p2 = rotate(p2, o2, theta);
          p3 = rotate(p3, o2, theta);

          int f = 3 - i;
          float sound = 0.0;

          if (mode == SPECTRUM) {
            sound = red_freqs[0] * 0.001 * f;
          }

          float a[4] = {p1.x, p1.y, 0.1f * f + sound,0.0};
          float b[4] = {p2.x, p2.y, 0.1f * f + sound,0.0};
          float c[4] = {p3.x, p3.y, 0.1f * f + sound,0.0};
          triangle_vertices.insert(triangle_vertices.end(), std::begin(a), std::end(a));
          triangle_vertices.insert(triangle_vertices.end(), std::begin(bary_a), std::end(bary_a));
          triangle_vertices.insert(triangle_vertices.end(), std::begin(b), std::end(b));
          triangle_vertices.insert(triangle_vertices.end(), std::begin(bary_b), std::end(bary_b));
          triangle_vertices.insert(triangle_vertices.end(), std::begin(c), std::end(c));
          triangle_vertices.insert(triangle_vertices.end(), std::begin(bary_c), std::end(bary_c));
      }
    }
  }


  glBindBuffer(GL_ARRAY_BUFFER, vbo2);
  glBufferData(GL_ARRAY_BUFFER, triangle_vertices.size() * sizeof(float), &triangle_vertices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  float x = 2.0;
  GLfloat quads[5*6] = {-x,-x,0.0,0.0,0.0, -x,x,0.0,0.0,1.0, x,x,0.0,1.0,1.0, -x,-x,0.0,0.0,0.0, x,-x,0.0,1.0,0.0, x,x,0.0,1.0,1.0, };
  glBindBuffer(GL_ARRAY_BUFFER, vbo3);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quads), quads, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  x = 10.0;
  GLfloat quads2[5*6] = {-x,-x,0.0,0.0,0.0, -x,x,0.0,0.0,1.0, x,x,0.0,1.0,1.0, -x,-x,0.0,0.0,0.0, x,-x,0.0,1.0,0.0, x,x,0.0,1.0,1.0, };
  glBindBuffer(GL_ARRAY_BUFFER, vbo4);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quads2), quads2, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);


  milliseconds ms = duration_cast< milliseconds >(
      system_clock::now().time_since_epoch()
  );
  float time = (double)((double)ms.count() - (double)start.count()) / 1000.0f;

  //Background
  glUseProgram(program4);
  glBindVertexArray(vao4);
  glUniform1f(glGetUniformLocation(program4, "time"), (float)time);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  glUseProgram(0);

  //Triangles
  glUseProgram(program2);
  glBindVertexArray(vao2);
  glDrawArrays(GL_TRIANGLES, 0, NUM_TRIANGLES * 3);
  glUniform1f(glGetUniformLocation(program3, "vol"), (float)red_freqs[5] * 0.01);
  glBindVertexArray(0);
  glUseProgram(0);

  //Center
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glUseProgram(program3);
  glUniform1f(glGetUniformLocation(program3, "time"), (float)time);
  glUniform1f(glGetUniformLocation(program3, "vol"), (float)red_freqs[5] * 0.01);
  glBindVertexArray(vao3);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  glUseProgram(0);


  glDisable(GL_BLEND);


  glUseProgram(program);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, NUM_POINTS * 6);
  glBindVertexArray(0);
  glUseProgram(0);
}

int main() {
  const unsigned int client_width = 3840;
  const unsigned int client_height = 2160;

  if (!glfwInit())
      exit(EXIT_FAILURE);


  //window = glfwCreateWindow(client_width, client_height, "My Title", NULL, NULL);
  window = glfwCreateWindow(client_width, client_height, "My Title", glfwGetPrimaryMonitor(), nullptr);
  if (!window) {
    printf("Failed to create window.\n");
    return 1;
  }
  //glClearColor(0.0,0.0,0.0,0.0);
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetFramebufferSizeCallback(window, resize);
  glewInit();

  printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
  printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
  printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));

  if (!Initialize()) {
    printf("Scene initialization failed.\n");
    return 1;
  }

  while (!glfwWindowShouldClose(window)) {
    Render();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  if (adc.isStreamRunning()){
    adc.stopStream();
  }

  glfwDestroyWindow(window);

  return 0;
}
