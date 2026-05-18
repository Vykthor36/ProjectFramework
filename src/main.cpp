#include <iostream>
#include <ctime>
#include <chrono>

using std::string;
using std::to_string;
using std::cout;
using std::endl;

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
};

int main() 
{
    Particle p(0, 0, 1, 0);

    for (int i = 0; i < 75; i++)
    {
        p.update(1.0f);
    }

    return 0;
}