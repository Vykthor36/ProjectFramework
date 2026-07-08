#include <iostream>
#include <ctime>
#include <chrono>
#include <vector>
#include <array>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using std::string;
using std::to_string;
using std::cout;
using std::endl;

const char* vertexShaderSource = "#version 430 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (std430, binding = 0) buffer Particles\n"
    "{\n"
    "   vec4 particleData[];\n"
    "};\n"
    "\n"
    "void main()\n"
    "{\n"
    "   vec4 particle = particleData[gl_InstanceID];\n"
    "   gl_Position = vec4(aPos.x + particle.x, aPos.y + particle.y, 0.0, 1.0);\n"
    "}\0";

const char* fragmentShaderSource = "#version 430 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n"
    "}\n\0";

const char* computeShaderSource = "#version 430 core\n"
    "\n"
    "layout (local_size_x = 256) in;\n"
    "\n"
    "struct Particle\n"
    "{\n"
    "   float x;\n"
    "   float y;\n"
    "   float xSpeed;\n"
    "   float ySpeed;\n"
    "};\n"
    "\n"
    "layout(std430, binding = 0) buffer Particles\n"
    "{\n"
    "   Particle particles[];\n"
    "};\n"
    "\n"
    "uniform float dTime;\n"
    "void main()\n"
    "{\n"
    "   uint id = gl_GlobalInvocationID.x;\n"
    "   particles[id].x += particles[id].xSpeed * dTime;\n"
    "   particles[id].y += particles[id].ySpeed * dTime;\n"
    "}\n\0";

static const int WINDOW_SIZE = 750;
static const int PARTICLE_NB = 500000;
static const float PARTICLE_SIZE = 0.005;

namespace Utilities
{
    // Not thread-safe yet, careful!
    string getCurrentTime()
    {
        using namespace std::chrono;

        // #1: Left part (hours, minutes and seconds)
        auto nowChrono = system_clock::now();

        time_t nowCTime = system_clock::to_time_t(nowChrono); 
        struct tm *localTime = localtime(&nowCTime); 

        char res[9 + 1]; 
        strftime(res, 10, "%H:%M:%S:", localTime);
        
        // #2: Right part (milliseconds only) 
        // Convert the current time to time since epoch 
        auto duration = nowChrono.time_since_epoch(); 
        
        // Convert duration to milliseconds 
        auto ms = duration_cast<milliseconds>(duration).count() % 1000;

        // Fixing the accuracy of smaller values by adding missing 0 on the left
        char msBuffer[3 + 1];
        sprintf(msBuffer, "%03d", (int) ms);
        
        // #3: Merge the two parts together and return it 
        return "[" + string(res) + msBuffer + "]: ";
    }

    float getRandomNb(const float& start, const float& end)
    {
        // Generate a random number between start and end
        //int randomNum = rand() % (end - start + 1) + start; // Only for integers
        float randomNum = ((float) rand() / RAND_MAX) * (end - start) + start;

        return randomNum;
    }

    namespace OGL
    {
        void framebufferSizeCallback(GLFWwindow* window, int width, int height)
        {
            glViewport(0, 0, width, height);
        }

        void processInput(GLFWwindow* window)
        {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                glfwSetWindowShouldClose(window, true);
            }
                
            if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }

            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
        }

        // From zestedesavoir.com
        void printWorkGroupsCapabilities() {
            int workgroup_count[3];
            int workgroup_size[3];
            int workgroup_invocations;

            glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workgroup_count[0]);
            glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workgroup_count[1]);
            glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workgroup_count[2]);

            printf ("Max. size of the workgroups:\n\tx:%u\n\ty:%u\n\tz:%u\n",
            workgroup_size[0], workgroup_size[1], workgroup_size[2]);

            glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workgroup_size[0]);
            glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workgroup_size[1]);
            glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workgroup_size[2]);

            printf ("Max. number of local invocation:\n\tx:%u\n\ty:%u\n\tz:%u\n",
            workgroup_size[0], workgroup_size[1], workgroup_size[2]);

            glGetIntegerv (GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workgroup_invocations);
            printf ("Max. number of workgroups invocations\n\t%u\n", workgroup_invocations);
            }

        int compileVertexShader()
        {
            unsigned int vertexShader;
            vertexShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
            glCompileShader(vertexShader);

            int  success;
            char infoLog[512];
            glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

                return 0;
            }

            return vertexShader;
        }

        int compileFragmentShader()
        {
            unsigned int fragmentShader;
            fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
            glCompileShader(fragmentShader);

            int  success;
            char infoLog[512];
            glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

                return 0;
            }

            return fragmentShader;
        }

        int compileAndCreateComputeShader()
        {
            // Compiling the shader itself...
            unsigned int computeShader;
            computeShader = glCreateShader(GL_COMPUTE_SHADER);
            glShaderSource(computeShader, 1, &computeShaderSource, NULL);
            glCompileShader(computeShader);

            int  success;
            char infoLog[512];
            glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;

                return 0;
            }

            // Creating the associated program...
            unsigned int shaderProgram;
            shaderProgram = glCreateProgram();

            glAttachShader(shaderProgram, computeShader);
            glLinkProgram(shaderProgram);

            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
            if (!success) 
            {
                glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n" << infoLog << std::endl;

                return 0;
            }
            
            glDeleteShader(computeShader);

            return shaderProgram;
        }

        int createShaderProgram(const int& vertexShader, const int& fragmentShader)
        {
            unsigned int shaderProgram;
            shaderProgram = glCreateProgram();

            glAttachShader(shaderProgram, vertexShader);
            glAttachShader(shaderProgram, fragmentShader);
            glLinkProgram(shaderProgram);

            int  success;
            char infoLog[512];
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
            if (!success) 
            {
                glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n" << infoLog << std::endl;

                return 0;
            }
            
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            return shaderProgram;
        }

        const int createSquare(const float& size)
        {   
            const float pos = size / 2;

            float vertices[] = {
                pos,  pos, 0.0f,  // Top right
                pos, -pos, 0.0f,  // Bottom right
                -pos, -pos, 0.0f,  // Bottom left
                -pos,  pos, 0.0f   // Top left 
            };

            unsigned int indices[] = {
                0, 1, 3,   // First triangle
                1, 2, 3    // Second triangle
            };

            unsigned int VBO, VAO, EBO;
            glGenBuffers(1, &VBO); glGenVertexArrays(1, &VAO); glGenBuffers(1, &EBO);

            // Binding "Vertex Array Object"
            glBindVertexArray(VAO);

            // Copying our vertices array into a vertex buffer for OpenGL
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            // Copying our index array into a element buffer for OpenGL
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            // Setting the vertex attributes pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            return VAO;
        }
    };
};

class Particle
{
    private:
        float x;
        float y;
        float xSpeed;
        float ySpeed;

    public:
        //Particle() = default; // Empty constructor
        ~Particle() = default; // Destructor
        Particle(const float& x = 0.f, const float& y = 0.f, const float& xSpeed = 0.f, const float& ySpeed = 0.f) 
            : x(x), y(y), xSpeed(xSpeed), ySpeed(ySpeed) 
        { 
            //cout << Utilities::getCurrentTime() << "New particle at position (" << x << ", " << y << ") and speed (" << xSpeed << ", " << ySpeed << ") !" << endl; 
        }

        Particle(const Particle&) = default; // Copy constructor
        Particle& operator=(const Particle&) = default; // Copy assignment operator

        void update(float dt)
        {
            // Euler's movement through time
            x += xSpeed * dt;
            y += ySpeed * dt;

            //cout << Utilities::getCurrentTime() << "New particle position (" << x << ", " << y << ") !" << endl; 
        }

        std::array<float, 4> getGPUData() const
        {
            return {x, y, xSpeed, ySpeed};
        }

        float getX() const
        {
            return x;
        }

        float getY() const
        {
            return y;
        }            
};

int main() 
{
    /* UTILITIES */
    srand(time(0)); // Needed to make sure that random numbers are distinct from each other

    /* FRONTEND */
    // GLFW initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); for macOS only

    // GLFW window initialization
    GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "Project Framework", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); 

    // GLAD initialization
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }

    // Setting additional parameters
    glViewport(0, 0, WINDOW_SIZE, WINDOW_SIZE);
    glfwSetFramebufferSizeCallback(window, Utilities::OGL::framebufferSizeCallback);

    // Shadering initialization
    const int computeShaderProg = Utilities::OGL::compileAndCreateComputeShader();
    GLint dTime = glGetUniformLocation(computeShaderProg, "dTime");
    int vertShader = Utilities::OGL::compileVertexShader();
    int fragShader = Utilities::OGL::compileFragmentShader();
    const int shaderProg = Utilities::OGL::createShaderProgram(vertShader, fragShader);
    const int VAO = Utilities::OGL::createSquare(PARTICLE_SIZE);

    // Particule generation & offset creation
    std::vector<Particle> particles(PARTICLE_NB);
    for (int i = 0; i < PARTICLE_NB; i++) particles[i] = Particle(Utilities::getRandomNb(-.25, .25), 
                                                                        Utilities::getRandomNb(-.25, .25),
                                                                        Utilities::getRandomNb(-1, 1), 
                                                                        Utilities::getRandomNb(-1, 1));
    std::vector<float> gpuData(PARTICLE_NB * 4);
    for (int i = 0; i < PARTICLE_NB; i++) 
    {
        const auto pData = particles[i].getGPUData();
        gpuData[i * 4] = pData[0];
        gpuData[i * 4 + 1] = pData[1];
        gpuData[i * 4 + 2] = pData[2];
        gpuData[i * 4 + 3] = pData[3];
    }

    // Init. of our SSBO
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        gpuData.size() * sizeof(float),
        gpuData.data(),
        GL_DYNAMIC_DRAW
    );

    // ...
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    // Render loop initialization
    float firstTime = glfwGetTime();
    float lastTime = firstTime;
    while (!glfwWindowShouldClose(window))
    {
        // Getting our time variables
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        //float ut = currentTime - firstTime;
        lastTime = currentTime;

        // Input handling
        Utilities::OGL::processInput(window);

        // Rendering commands
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Computing our particles on the GPU through the compute shader
        glUseProgram(computeShaderProg);
        glUniform1f(dTime, dt);
        glDispatchCompute((PARTICLE_NB + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Drawing our objects
        glUseProgram(shaderProg);

        glBindVertexArray(VAO); // Setting our VAO when starting to use it...
        //glBindBuffer(GL_ARRAY_BUFFER, particlePosVBO); We are now using the SSBO for that!

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, PARTICLE_NB); // With EBO or SSBO and instancing

        //glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0); // ... and un-setting it when we are done.

        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    // Properly closing GLFW
    glfwTerminate();

    return 0;
}