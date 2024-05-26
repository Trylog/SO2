//Michal Bernacki-Janson 264021 SO2 projekt etap 2
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <cmath>
#include <list>
#include <condition_variable>
#include <X11/Xlib.h>

#include "GL/freeglut.h"

using namespace std;

mutex mtx, mtx2;
condition_variable cv;
atomic<bool> aFlag;
atomic<bool> bFlag;
void cleanVector(int i);


template <typename T = float > class Coordinates {
public:
    T x, y;
    Coordinates(T x, T y) : x(x), y(y) {}
    Coordinates operator += (Coordinates const& obj) {
        this->x += obj.x;
        this->y += obj.y;
        return *this;
    }
    Coordinates operator -= (Coordinates const& obj) {
        this->x -= obj.x;
        this->y -= obj.y;
        return *this;
    }
};

struct Color{
    float r, g, b;
    Color(float r, float g, float b) : r(r), g(g), b(b) {}
};

struct Ball {
    Coordinates<> position;
    Coordinates<> speed;
    double radius;
    Color color;
    int bounces = 0;
    int id;
    bool visible = true;
    int timeStep;

    Ball(Coordinates<> position, Coordinates<> speed, int timeStep, double startRadius, Color color, int startId)
            : position(position), speed(speed), timeStep(timeStep), radius(startRadius), color(color), id(startId){    }

    void move() {
        bool flagA = true, flagB = true;
        while (bounces < 5 && !aFlag){
            position += speed;
            auto windowWidth = glutGet(GLUT_WINDOW_WIDTH);
            auto windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
            if (position.x - radius <= 0 || position.x + radius >= windowWidth) {
                speed.x *= -1;
                bounces++;
            }
            if (position.y - radius <= 0 || position.y + radius >= windowHeight) {
                speed.y *= -1;
                bounces++;
            }

            if ((speed.x < 0.0) && (position.x - radius <= 120) && (position.x + radius >= 100)) flagA = false;
            if ((speed.x > 0.0) && (position.x + radius >= 80) && (position.x - radius <= 100)) flagB = false;

            {
                unique_lock<mutex> lock(mtx2);
                cv.wait(lock, [&]{return ((bFlag && flagA) || (!bFlag && flagB));});
            }
            flagA = flagB = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(timeStep));
        }
        cout<<"id: "<<id<<endl;
        {
            visible = false;
            cout<<"stop"<<endl;
            cleanVector(id/*, this_thread::get_id()*/);
        }
    }

    void draw() {
        if(visible) {
            glColor3f(color.r, color.b, color.g);
            glBegin(GL_TRIANGLE_FAN);
            for (float angle = 0; angle < 2 * 3.14159; angle += 0.1f) {
                glVertex2f(position.x + radius * cos(angle), position.y + radius * sin(angle));
            }
            glEnd();
        }
    }
};

struct Strefa {
    double x, y;
    double width, height;
    double dy;
    double r, g, b;

    Strefa(float startX, float startY, float startWidth, float startHeight, float startDY, float startR, float startG, float startB)
            : x(startX), y(startY), dy(startDY), r(startR), g(startG), b(startB), width(startWidth), height(startHeight){}

    void move() {
        int step = 1;
        while(!aFlag) {
            y += step;
            if (y - height / 2 <= 0 || y + height / 2 >= glutGet(GLUT_WINDOW_HEIGHT)) {
                if((rand() % 2 == 0) && dy > 5){
                    dy * 0.8;
                    step = -step;
                } else {
                    dy * 1.2;
                    step = -step;
                }
                bFlag = !bFlag;
                cv.notify_all();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds((int)dy));
        }
    }

    void draw() {
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
        glVertex2f(x - width / 2, y - height / 2);
        glVertex2f(x + width / 2, y - height / 2);
        glVertex2f(x + width / 2, y + height / 2);
        glVertex2f(x - width / 2, y + height / 2);
        glEnd();
    }
};

list<Ball> balls;
vector<thread> ballThreads;
Strefa st(100.0, 100.0, 40, 40, 2, 0.2, 0.2, 0.2);
void cleanVector(int id/*, thread::id tId*/){
    mtx.lock();
    balls.erase(find_if(balls.begin(), balls.end(),[&](const Ball &k){return k.id == id;}));
    mtx.unlock();
}
void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto& ball : balls) {
        ball.draw();
    }
    st.draw();
    glutSwapBuffers();
}

void update(int value) {
    glutPostRedisplay();
    glutTimerFunc(50, update, 0);
}

void kulkiFactory(){
    int i = 0;
    while(!aFlag){
        mtx.lock();
        balls.emplace_back(Ball(Coordinates<>(320, 10), Coordinates<>(rand() % 3 - 1/*(rand() % 60 - 30) / 10*/, 1), rand() % 5 + 1, 10, Color(((float)(rand() % 10)) / 10, ((float)(rand() % 10)) / 10, ((float)(rand() % 10)) / 10), i++));
        ballThreads.emplace_back(&Ball::move, &balls.back());
        mtx.unlock();

        cout<<"threads: "<<ballThreads.size()<<" balls: "<<balls.size()<<endl;
        std::this_thread::sleep_for(std::chrono::milliseconds (2000));
    }

    for (auto& thread : ballThreads) {
        thread.join();
    }
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 32) {
        aFlag = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        glutLeaveMainLoop();
    }
}


int main(int argc, char** argv) {
    XInitThreads();
    aFlag = false;
    bFlag = true; //do góry
    srand(time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Bouncing Balls");

    thread t1(&Strefa::move, &st);
    thread t(kulkiFactory);

    glClearColor(1.0, 1.0, 1.0, 1.0);
    gluOrtho2D(0, 640, 0, 480);

    glutDisplayFunc(draw);
    glutTimerFunc(50, update, 0);
    glutKeyboardFunc(keyboard);

    glutMainLoop();

    t1.join();
    t.join();

    return 0;
}
//g++ -std=c++23 -o kulkiR kulki.cpp -lGL -lGLU -lglut -lX11