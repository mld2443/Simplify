#include "collapsible.h"

#include <GL/glut.h>
#include <cstdlib>
#ifndef NDEBUG
#include <iostream>
#endif


int WINDOW_WIDTH = 1440, WINDOW_HEIGHT = 900;
int window = 0;
bool executed = false;
unsigned target;
const char* file;
Manifold *shape;

// mouse state
int prevX = 0, prevY = 0;
bool leftPressed = false, rightPressed = false, middlePressed = false;

// view state
float modelView[16] = { 1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1 };
float focus[3] = { 0, 0, 0 };

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up viewing matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, float(WINDOW_WIDTH)/WINDOW_HEIGHT, .0001, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Camera
    glTranslatef(focus[0], focus[1], focus[2]);
    glMultMatrixf(modelView);

    shape->draw();

    glFlush();
    glutSwapBuffers();
}

static void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
}

static void mouse(int button, int state, int x, int y) {
    y = WINDOW_HEIGHT - y;

    // Mouse state that should always be stored on pressing
    if (state == GLUT_DOWN) {
        prevX = x;
        prevY = y;
    }

    if (button == GLUT_LEFT_BUTTON) {
        leftPressed = state == GLUT_DOWN;
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        rightPressed = state == GLUT_DOWN;
    }
    else if (button == GLUT_MIDDLE_BUTTON) {
        middlePressed = state == GLUT_DOWN;
    }
}

static void motion(int x, int y) {
    y = WINDOW_HEIGHT - y;

    float dx = (x - prevX);
    float dy = (y - prevY);

    // rotate the scene
    if (leftPressed)
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(dx, 0, 1, 0);
        glRotatef(dy, -1, 0, 0);
        glMultMatrixf(modelView);
        glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    }
    else if (middlePressed)
    {
        focus[0] += 0.005 * dx;
        focus[1] += 0.005 * dy;
    }
    else if (rightPressed)
    {
        focus[2] += 0.01 * dy;
    }

    // Store previous mouse positions
    prevX = x;
    prevY = y;

    glutPostRedisplay();
}

static void resetViewMatrix() {
    for (int i = 0; i < 16; ++i) {
        modelView[i] = i % 5 ? 0.0f : 1.0f;
    }

    const auto offset = -shape->getAABBCentroid();
    modelView[12] = offset.x;
    modelView[13] = offset.y;
    modelView[14] = offset.z;
}

static void keyboard(unsigned char key, int x, int y) {
    switch(key)
    {
        case 9: //tab
            break;

        case 13: //return
            break;

        case 8: //backspace
            resetViewMatrix();
            glutPostRedisplay();
            break;

        case 127: //delete
            break;

        case ' ':
            // if (!executed)
            //     shape->simplify(target);
            // else {
            //     delete shape;
            //     shape = new Manifold(file);
            // }
            // executed = !executed;
            // glutPostRedisplay();
            break;

        case 27: //escape
            glutDestroyWindow(window);
            exit(0);
            break;

        default:
#ifndef NDEBUG
            std::cout << "Unknown key code #" << key << std::endl;
#endif
            break;
    }
}

static void specialkey(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:
            focus[1] -= 0.05;
            glutPostRedisplay();
            break;

        case GLUT_KEY_DOWN:
            focus[1] += 0.05;
            glutPostRedisplay();
            break;

        case GLUT_KEY_LEFT:
            focus[0] += 0.05;
            glutPostRedisplay();
            break;

        case GLUT_KEY_RIGHT:
            focus[0] -= 0.05;
            glutPostRedisplay();
            break;

        default:
#ifndef NDEBUG
            std::cout << "Unknown key code #" << key << std::endl;
#endif
            break;
    }
}

int main(int argc, char **argv) {
    // Load the model
    file = "...";
    shape = new Manifold(file);
    target = 2000u;

    // Prepare the window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    window = glutCreateWindow("Manifold Surface Simplifier");

    // Set up our openGL specific parameters
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glClearColor(0, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);

    // Center the model in viewspace and zoom in/out so it takes up most of the screen
    resetViewMatrix();

    const v3f deltas = shape->getAABBSizes();

    if (deltas.x >= deltas.y && deltas.x >= deltas.z)
        focus[2] -= (4.0f * deltas.x) / 5.0f;
    else if (deltas.y >= deltas.z)
        focus[2] -= (4.0f * deltas.y) / 5.0f;
    else
        focus[2] -= (4.0f * deltas.z) / 5.0f;

    // Initialize the glut callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialkey);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    // Kick off the main loop
    glutMainLoop();
}
