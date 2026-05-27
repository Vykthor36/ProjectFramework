#include <iostream>
#include <ctime>
#include <chrono>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using std::string;
using std::to_string;
using std::cout;
using std::endl;

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aOffset;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x + aOffset.x, aPos.y + aOffset.y, 0.0, 1.0);\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n"
    "}\n\0";

static const int WINDOW_SIZE = 750;
static const int PARTICLE_NB = 1;

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

        const int createRectangle()
        {         
            float vertices[] = {
                0.1f,  0.1f, 0.0f,  // Top right
                0.1f, -0.1f, 0.0f,  // Bottom right
                -0.1f, -0.1f, 0.0f,  // Bottom left
                -0.1f,  0.1f, 0.0f   // Top left 
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
        Particle() = default; // Empty constructor
        ~Particle() = default; // Destructor
        Particle(const float& x = 0.f, const float& y = 0.f, const float& xSpeed = 0.f, const float& ySpeed = 0.f) 
            : x(x), y(y), xSpeed(xSpeed), ySpeed(ySpeed) 
        { 
            cout << Utilities::getCurrentTime() << "New particle at position (" << x << ", " << y << ") and speed (" << xSpeed << ", " << ySpeed << ") !" << endl; 
        }

        Particle(const Particle&) = default; // Copy constructor
        Particle& operator=(const Particle&) = default; // Copy assignment operator

        void update(float dt)
        {
            // Euler's movement through time
            x += xSpeed * dt;
            y += ySpeed * dt;

            cout << Utilities::getCurrentTime() << "New particle position (" << x << ", " << y << ") !" << endl; 
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
    const int VAO = Utilities::OGL::createRectangle();

    // Particule generation & offset creation
    std::vector<Particle> particles;
    for (int i = 0; i < PARTICLE_NB; i++) particles.push_back(Particle(0, 0, .00025, .00058));
    std::vector<float> offsets;
    for (const Particle& p : particles) 
    {
        offsets.push_back(p.getX()); 
        offsets.push_back(p.getY());
    }

    unsigned int particlePosVBO;
    glGenBuffers(1, &particlePosVBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, particlePosVBO); // Every 'GL_ARRAY_BUFFER' operation next are linked to particlePosVBO...
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * PARTICLE_NB, offsets.data(), GL_DYNAMIC_DRAW); // ... like this one!
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
    glVertexAttribDivisor(1, 1); // One change of value per instance

    // Resetting the listening state of our buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Render loop initialization
    while (!glfwWindowShouldClose(window))
    {
        // Input handling
        Utilities::OGL::processInput(window);

        // Rendering commands
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Moving the particles
        offsets.clear();
        for (Particle& p: particles) 
        {
            p.update(0.05);
            offsets.push_back(p.getX());
            offsets.push_back(p.getY());
        }

        // Drawing our objects
        glUseProgram(shaderProg);
        glBindVertexArray(VAO); // Setting our VAO when starting to use it...
        glBindBuffer(GL_ARRAY_BUFFER, particlePosVBO);

        glBufferSubData(GL_ARRAY_BUFFER, 0, offsets.size() * sizeof(float), offsets.data()); 
        // ^ Can be used to replace specific parts in th buffer's memory
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