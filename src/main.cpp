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

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec4 aParticleData;\n"
    "\n"
    "uniform float uTime;"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x + aParticleData.x + aParticleData.z * uTime, aPos.y + aParticleData.y + aParticleData.w * uTime, 0.0, 1.0);\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n"
    "}\n\0";

static const int WINDOW_SIZE = 750;
static const int PARTICLE_NB = 1000;

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); For macOS only

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
    int vertShader = Utilities::OGL::compileVertexShader();
    int fragShader = Utilities::OGL::compileFragmentShader();
    const int shaderProg = Utilities::OGL::createShaderProgram(vertShader, fragShader);
    GLint uTime = glGetUniformLocation(shaderProg, "uTime");
    const int VAO = Utilities::OGL::createSquare(0.1);

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

    unsigned int particlePosVBO;
    glGenBuffers(1, &particlePosVBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, particlePosVBO); // Every 'GL_ARRAY_BUFFER' operation next are linked to particlePosVBO...
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * PARTICLE_NB, gpuData.data(), GL_DYNAMIC_DRAW); // ... like this one!
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glVertexAttribDivisor(1, 1); // One change of value per instance

    // Resetting the listening state of our buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Render loop initialization
    float firstTime = glfwGetTime();
    float lastTime = firstTime;
    while (!glfwWindowShouldClose(window))
    {
        // Getting our time variables
        float currentTime = glfwGetTime();
        //float dt = currentTime - lastTime;
        float ut = currentTime - firstTime;
        lastTime = currentTime;
        glUniform1f(uTime, ut);

        // Input handling
        Utilities::OGL::processInput(window);

        // Rendering commands
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Drawing our objects
        glUseProgram(shaderProg);

        glBindVertexArray(VAO); // Setting our VAO when starting to use it...
        glBindBuffer(GL_ARRAY_BUFFER, particlePosVBO);

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, particles.size()); // With EBO and instancing

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0); // ... and un-setting it when we are done.

        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    // Properly closing GLFW
    glfwTerminate();

    return 0;
}