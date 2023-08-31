
#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
//#include <GL/glx.h>
#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <fstream>
#ifdef _WIN32
#include <kiss_fft.h>
#elif
#include <kissfft/kiss_fft.h>
#endif
#include <math.h>
#include <rtaudio/RtAudio.h>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <time.h>
#include <chrono>
#include <numeric>
#include <tuple>

#define VERT_LENGTH 4 //x,y,z,volume
#define SHADER_PATH "../../../"
#define FRAMES 1024
#define NUM_POINTS 32
#define NUM_TRIANGLES 3 * 4 * 8
#define COARSE_NUM 8

using namespace std::chrono;
using namespace std;

GLFWwindow* window;
GLuint program, program2, program3, program4, program5;


GLuint vao, vao2, vao3, vao4, vao5, vbo, vbo2, vbo3, vbo4, vbo5;
float radius = 1.0f;
float radius2 = 1.4f;

GLfloat bary_a[3] = {1.0f, 0.0f, 0.0f};
GLfloat bary_b[3] = {0.0f, 1.0f, 0.0f};
GLfloat bary_c[3] = {0.0f, 0.0f, 1.0f};

RtAudio adc(RtAudio::Api::LINUX_PULSE);
GLfloat circleVertices[NUM_POINTS * VERT_LENGTH * 3];
GLfloat triangleVertices[NUM_TRIANGLES * VERT_LENGTH * 3 + NUM_TRIANGLES * 3];
GLfloat quadVertices[6 * 5];

kiss_fft_cfg cfg;

float zoom = 6.0;


float rawdata[FRAMES];
float freqs[FRAMES];

float red_rawdata[NUM_POINTS];
float red_freqs[NUM_POINTS];

float coarse_freqs[COARSE_NUM];

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
  ORIGINAL,
  PIXEL
};

enum BackgroundMode{
  SPIRAL,
  RINGS,
  ARPEGGIO,
};

VisMode mode = PIXEL;
BackgroundMode backgroundMode = ARPEGGIO;

float sensitivity = 0.5;

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
    float last_freq = red_freqs[i];
    red_freqs[i] = 0;
    for(int j = 0; j < sample_group; j++) {
      red_rawdata[i] += rawdata[i * sample_group + j];
    }
    for(int j = 0; j < fft_group; j++) {
      red_freqs[i] += freqs[i * fft_group + j + 5];
    }
    red_freqs[i] /= fft_group;
    red_freqs[i] += last_freq;
    red_freqs[i] /= 2.0;
  }
  
  int coarse_group = NUM_POINTS / COARSE_NUM;
  for(int i = 0; i < COARSE_NUM; i++) {
    float last_freq = coarse_freqs[i];
    coarse_freqs[i] = 0;
    for(int j = 0; j < coarse_group; j++) {
      coarse_freqs[i] += red_freqs[i * coarse_group + j];
    }
    coarse_freqs[i] /= coarse_group;
    coarse_freqs[i] += last_freq;
    coarse_freqs[i] /= 2.0;
  }
  // Do something with the data in the "inputBuffer" buffer.
  return 0;
}

void getdevices() {
  // Get the list of device IDs
    
    #ifdef _WIN32
        std::vector<unsigned int> ids(adc.getDeviceCount());
        std::iota(ids.begin(), ids.end(), 0);
    #elif
        std::vector<unsigned int> ids = adc.getDeviceIds();
    #endif
  
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
    GLuint programs[5] = {program, program2, program3, program4, program5};
    for (int i = 0; i < 5; i++) {
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

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 3840.0f / 2160.0f, 0.1f, 40.0f);
        GLint uniProj = glGetUniformLocation(programs[i], "proj");
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
      mode = VisMode::SPECTRUM;
    } else if (key == GLFW_KEY_O && action == GLFW_PRESS) {
      mode = VisMode::ORIGINAL;
    } else if (key == GLFW_KEY_P && action == GLFW_PRESS) {
      mode = VisMode::PIXEL;
    } else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9 && action == GLFW_PRESS) {
      backgroundMode = (BackgroundMode)(key - GLFW_KEY_0);
    } else if (key == GLFW_KEY_F && action == GLFW_PRESS) {
      set_camera(0.0f, -4.0f, 4.0f, -0.5f);
    } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
      set_camera(0.0f, -0.1f, 6.2f, 0.0f);
    } else if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS) {
      set_camera(0.0f, -0.1f, zoom -= 1.0, 0.0f);
    } else if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS) {
      set_camera(0.0f, -0.1f, zoom += 1.0, 0.0f);
    }
}

static void resize(GLFWwindow* window, int width, int height){
	  glViewport(0,0,width,height);
    
    GLuint programs[5] = {program, program2, program3, program4, program5};
    for (int i = 0; i < 5; i++) {
        glUseProgram(programs[i]);

      GLfloat uResolution[2] = { (float)width, (float)height };
      glUniform2fv(glGetUniformLocation(programs[i], "iResolution"), 1, uResolution);

      glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 1.0f, 10.0f);
      glUniformMatrix4fv(glGetUniformLocation(programs[i], "proj"), 1, GL_FALSE, glm::value_ptr(proj));
    }
}

GLuint loadTexture(const char * imagepath){
    int w;
    int h;
    int comp;
    unsigned char* image = stbi_load(imagepath, &w, &h, &comp, STBI_rgb_alpha);

    if(image == nullptr) {
      throw(std::string("Failed to load texture"));
    }

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Read the file, call glTexImage2D with the right parameters
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);


    // Nice trilinear filtering.
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //glBindTexture(GL_TEXTURE_2D, 0);

    // Return the ID of the texture we just created
    return textureID;
}

    // Get the depth buffer value at this pixel.   
std::vector<std::tuple<GLenum, std::string, std::string>> specifyShaders(std::string vertex_path, std::string fragment_path) {
  std::stringstream vertex;
  vertex << std::ifstream(SHADER_PATH + vertex_path).rdbuf();
  std::stringstream fragment;
  fragment << std::ifstream(SHADER_PATH + fragment_path).rdbuf();
  return {
      std::make_tuple(GL_VERTEX_SHADER, vertex.str(), vertex_path),
      std::make_tuple(GL_FRAGMENT_SHADER, fragment.str(), fragment_path),
  };
}

bool loadShaders(GLuint *program, std::vector<std::tuple<GLenum, std::string, std::string>> shaders) {
  for (const auto &s : shaders) {
    GLenum type = std::get<0>(s);
    const std::string &source = std::get<1>(s);

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
        printf("Shader (%s) compile failed:\n%s\n", source.c_str(), log.c_str());
      } else {
        printf("Shader (%s) compile failed.\n", source.c_str());
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
      printf("Program (%s) link failed:\n%s", std::get<2>(shaders[0]).c_str(), log.c_str());
    } else {
      printf("Program (%s) link failed.\n", std::get<2>(shaders[0]).c_str());
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

#ifdef _WIN32
  bool failure = false;
  try
  {
      adc.openStream(NULL, &parameters, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &record);
      adc.startStream();
  }
  catch (const RtAudioError e)
  {
      std::cout << '\n' << e.getMessage() << '\n' << std::endl;
  }
#elif
  if (adc.openStream(NULL, &parameters, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &record)) {
      std::cout << '\n' << adc.getErrorText() << '\n' << std::endl;
      exit(0); // problem with device settings
  }
  // Stream is open ... now start it.
  if (adc.startStream()) {
      std::cout << adc.getErrorText() << std::endl;
  }
#endif



  // shaders
  printf("Creating programs \n");
  program = glCreateProgram();
  if (!loadShaders(&program, specifyShaders("vertex.glsl", "fragment.glsl"))) {
    return false;
  }
  program2 = glCreateProgram();
  if (!loadShaders(&program2, specifyShaders("vertex2.glsl", "fragment2.glsl"))) {
    return false;
  }
  program3 = glCreateProgram();
  if (!loadShaders(&program3, specifyShaders("center_vertex.glsl", "center_fragment.glsl"))) {
    return false;
  }
  program4 = glCreateProgram();
  if (!loadShaders(&program4, specifyShaders("background_vertex.glsl", "background_fragment.glsl"))) {
    return false;
  }
  program5 = glCreateProgram();
  if (!loadShaders(&program5, specifyShaders("foreground_vertex.glsl", "foreground_fragment.glsl"))) {
    return false;
  }
  printf("Created programs \n");

  

  set_camera(0.0f, -0.1f, 6.2f, 0.0f);

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

  std::vector<float> triangle_vertices;

  for (int angle = 0; angle < 8; angle++) {
    float theta = (M_PI_4) * angle;
    struct Point o1 = { -1.2f, 0.0};
    struct Point o2 = { 0.0, 0.0 };
    struct Point o3 = { -2.0, 0.0 };

    int draws = 4;

    if(angle % 2 == 0) {
      draws = 1;
    }

    for (int j = 0; j < draws; j++) {
      float beta = -(M_PI_2) * j;
      
      for (int i = 4; i > 0; i--) {
          struct Point fo = o1;
          float fr = beta;
          int s = i;
          if (i == 4){
            if (j != 0) {
              continue;
            }
            fo = o3;
            fr = M_PI;
            s = 1;
          }

          struct Point p1 = rotate({ -2.0f + s * 0.05f, 0.25f + s * 0.1f}, fo, fr);
          struct Point p2 = rotate({ -2.0f + s * 0.05f, -0.25f - s * 0.1f}, fo, fr);
          struct Point p3 = rotate({ -1.75f + s * 0.15f, 0.0}, fo, fr);

          p1 = rotate(p1, o2, theta);
          p2 = rotate(p2, o2, theta);
          p3 = rotate(p3, o2, theta);

          int f = 3 - s;

          float a[4] = {p1.x, p1.y, 0.1f * f,0.0};
          float b[4] = {p2.x, p2.y, 0.1f * f,0.0};
          float c[4] = {p3.x, p3.y, 0.1f * f,0.0};
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


  float x = 4.0;
  GLfloat quads[5*6] = {-x,-x,0.0,0.0,0.0, -x,x,0.0,0.0,1.0, x,x,0.0,1.0,1.0, -x,-x,0.0,0.0,0.0, x,-x,0.0,1.0,0.0, x,x,0.0,1.0,1.0, };
  glBindBuffer(GL_ARRAY_BUFFER, vbo3);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quads), quads, GL_STATIC_DRAW);
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

  x = 10.0;
  GLfloat quads2[5*6] = {-x,-x,0.0,0.0,0.0, -x,x,0.0,0.0,1.0, x,x,0.0,1.0,1.0, -x,-x,0.0,0.0,0.0, x,-x,0.0,1.0,0.0, x,x,0.0,1.0,1.0, };
  glBindBuffer(GL_ARRAY_BUFFER, vbo4);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quads2), quads2, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0); 

  glBindVertexArray(0);
  glUseProgram(0);


  glGenVertexArrays(1, &vao5);
  glGenBuffers(1, &vbo5);
  glBindVertexArray(vao5);
  glBindBuffer(GL_ARRAY_BUFFER, vbo5);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
  glUseProgram(program5);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  x = 5.0;
  GLfloat quads3[5*6] = {-x,-x,0.0,0.0,0.0, -x,x,0.0,0.0,1.0, x,x,0.0,1.0,1.0, -x,-x,0.0,0.0,0.0, x,-x,0.0,1.0,0.0, x,x,0.0,1.0,1.0, };
  glBindBuffer(GL_ARRAY_BUFFER, vbo5);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quads3), quads3, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0); 

  glBindVertexArray(0);

  loadTexture((SHADER_PATH + std::string("dust.png")).c_str());

  glUseProgram(0);


  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_MULTISAMPLE);
  glEnable( GL_TEXTURE_2D );

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


  std::vector<float> vertices;

  for (int i = 0; i < NUM_POINTS; i++) {
      float theta1 = 2.0f * M_PI * float(i - 0.1) / float(NUM_POINTS);
      float theta2 = 2.0f * M_PI * float(i + 1) / float(NUM_POINTS);
      float theta3 = 2.0f * M_PI * float(i + 0.5) / float(NUM_POINTS);

      float theta_corr = 2.0f * M_PI * float(i + 0.5) / float(NUM_POINTS);

      float sound = coarse_freqs[0] * sensitivity * 0.1 + 1.0;
      float inner_radius = radius * sound;
      if (mode == SPECTRUM) {
        sound = red_freqs[i] * 0.5 + 1.0;
        inner_radius = radius;
      }
      float x1 = inner_radius * cosf(theta1);
      float y1 = inner_radius * sinf(theta1);
      float x2 = sound * radius2 * cosf(theta_corr);
      float y2 = sound * radius2 * sinf(theta_corr);
      float x3 = inner_radius * cosf(theta2);
      float y3 = inner_radius * sinf(theta2);
      float x4 = inner_radius * cosf(theta3);
      float y4 = inner_radius * sinf(theta3);

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

  milliseconds ms = duration_cast< milliseconds >(
      system_clock::now().time_since_epoch()
  );
  float time = (double)((double)ms.count() - (double)start.count()) / 1000.0f;

  

  //Background
  glUseProgram(program4);
  glBindVertexArray(vao4);
  glUniform1f(glGetUniformLocation(program4, "time"), (float)time);
  glUniform1i(glGetUniformLocation(program4, "mode"), (int)backgroundMode);
  glUniform1f(glGetUniformLocation(program4, "vol"), (float)coarse_freqs[0] * sensitivity * 0.02);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  glUseProgram(0);

  if (mode != PIXEL){
    //Triangles
    glUseProgram(program2);
    glBindVertexArray(vao2);
    glDrawArrays(GL_TRIANGLES, 0, NUM_TRIANGLES * 3);
    glUniform1f(glGetUniformLocation(program2, "vol"), (float)coarse_freqs[COARSE_NUM - 1] * sensitivity * 0.3);
    glBindVertexArray(0);
    glUseProgram(0);

    //Center
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glUseProgram(program3);
    glUniform1f(glGetUniformLocation(program3, "time"), (float)time);
    if (mode == SPECTRUM) {
      glUniform1f(glGetUniformLocation(program3, "vol"), 0.0);
    } else {
      glUniform1f(glGetUniformLocation(program3, "vol"), (float)coarse_freqs[0] * sensitivity * 0.1);
    }
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

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
  glEnable(GL_BLEND);

  glUseProgram(program5);
  glBindVertexArray(vao5);
  glDrawArrays(GL_TRIANGLES, 0, NUM_POINTS * 6);
  glBindVertexArray(0);
  glUseProgram(0);
  glDisable(GL_BLEND);
}

int main() {
  const unsigned int client_width = 3840;
  const unsigned int client_height = 2160;

  if (!glfwInit())
      exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_SAMPLES, 4);
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
