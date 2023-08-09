#include <cstdio>
#include <GL/glew.h>
#include "platform_linux_xlib.h"


#include <fstream>
#include <kissfft/kiss_fft.h>
#include <math.h>
#include <rtaudio/RtAudio.h>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define NUM_SEGMENTS 400

GLuint program;
GLuint vao, vbo;
float radius = 0.5f;
RtAudio adc(RtAudio::Api::LINUX_PULSE);
GLfloat circleVertices[NUM_SEGMENTS * 2 + 2];

kiss_fft_cfg cfg;

float rawdata[1024];
float freqs[1024];

int record(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData) {
  if (status){
    std::cout << "Stream overflow detected!" << std::endl;
    return 0;
    }

  //printf("%d \n", nBufferFrames);
  kiss_fft_cpx in[1024] = {};
  for (unsigned int i = 0; i < nBufferFrames; i++) {
    in[i].r = ((float *)inputBuffer)[i];
    rawdata[i] = ((float *)inputBuffer)[i];
  }

  kiss_fft_cpx out[1024] = {};
  kiss_fft(cfg, in, out);
  for (int i = 0; i < 1024; i++) {
    freqs[i] = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
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
    std::cout << ": maximum input channels = " << info.inputChannels << std::endl;
    std::cout << ": maximum output channels = " << info.outputChannels << std::endl;
  }
}

bool Initialize() {
  getdevices();

  RtAudio::StreamParameters parameters;
  parameters.deviceId = adc.getDefaultInputDevice();
  parameters.nChannels = 1;
  parameters.firstChannel = 0;
  unsigned int sampleRate = 44100;
  unsigned int bufferFrames = 1024;

  if (adc.openStream(NULL, &parameters, RTAUDIO_SINT16, sampleRate,
                     &bufferFrames, &record)) {
    std::cout << '\n' << adc.getErrorText() << '\n' << std::endl;
    exit(0); // problem with device settings
  }

  // Stream is open ... now start it.
  if (adc.startStream()) {
    std::cout << adc.getErrorText() << std::endl;
  }

  cfg = kiss_fft_alloc(1024, 0, NULL, NULL);

  // shaders
  std::ifstream v("../vertex.glsl");
  std::stringstream vertex;
  vertex << v.rdbuf();
  std::ifstream f("../fragment.glsl");
  std::stringstream fragment;
  fragment << f.rdbuf();

  std::vector<std::pair<GLenum, std::string>> shaders = {
      {GL_VERTEX_SHADER, vertex.str()},
      {GL_FRAGMENT_SHADER, fragment.str()},
  };
  glewInit();
  program = glCreateProgram();

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
  if (true) {
    for (int i = 0; i <= NUM_SEGMENTS; i++) {
      circleVertices[i * 2] = i * 0.005 - 0.9;
      circleVertices[i * 2 + 1] = rawdata[i];
    }
  } else {
    for (int i = 0; i <= NUM_SEGMENTS; i++) {
      float theta = 2.0f * 3.1415926f * float(i) / float(NUM_SEGMENTS);
      float x = radius * cosf(theta);
      float y = radius * sinf(theta);

      circleVertices[i * 2] = x;
      circleVertices[i * 2 + 1] = y;
    }
  }

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(circleVertices), circleVertices,GL_STATIC_DRAW);
  glUseProgram(program);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);

  return true;
}

void Render() {

  if (false) {
    for (int i = 0; i <= NUM_SEGMENTS; i++) {
      circleVertices[i * 2] = i * 0.005 - 0.9;
      float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
      circleVertices[i * 2 + 1] = r;
    }
  } else {
    for (int i = 0; i <= NUM_SEGMENTS; i++) {
      float theta = 2.0f * 3.1415926f * float(i) / float(NUM_SEGMENTS);

      float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) + 0.5;

      float x = radius * r * cosf(theta);
      float y = radius * r * sinf(theta);

      circleVertices[i * 2] = x;
      circleVertices[i * 2 + 1] = y;
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(circleVertices), circleVertices,GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glClear(GL_COLOR_BUFFER_BIT);

  // Use the shader program
  glUseProgram(program);

  // Set the circle color uniform
  glUniform3f(glGetUniformLocation(program, "circleColor"), 1.0f, 0.0f, 0.0f);

  // Bind vertex array object (VAO)
  glBindVertexArray(vao);

  // Draw the circle as a line loop
  glDrawArrays(GL_LINE_LOOP, 0, NUM_SEGMENTS + 1);
  glDrawArrays(GL_POINTS, 0, NUM_SEGMENTS);

  // Unbind VAO and shader
  glBindVertexArray(0);
  glUseProgram(0);
}

int main() {
  const unsigned int client_width = 2048;
  const unsigned int client_height = 2048;

  platform_window_t *window =
      platform::create_window("Triangle", client_width, client_height);
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

  if (adc.isStreamRunning())
    adc.stopStream();

  platform::destroy_window(window);

  delete window;

  return 0;
}
