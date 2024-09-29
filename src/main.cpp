#include "collapsible.h"

#include <GL/freeglut.h> // glut*, gl*
#include <cmath>         // tanf
#include <cstdlib>       // atoi


int WINDOW_WIDTH = 1440, WINDOW_HEIGHT = 900;
int g_window = 0;
bool g_simplified = false;
unsigned g_target = 0u;
const char *g_fileName;
Collapsible *g_shape;
bool g_showFaces = true, g_showEdges = false, g_showVertices = false;
bool g_toggle = false;

// mouse state
int g_prevX = 0, g_prevY = 0;
bool g_leftPressed = false, g_rightPressed = false, g_middlePressed = false;

// view state
float g_modelView[16] = { 1, 0, 0, 0,
                          0, 1, 0, 0,
                          0, 0, 1, 0,
                          0, 0, 0, 1 };
float g_focus[3] = { 0, 0, 0 };


static void setPerspective(float fovx, float aspect, float znear, float zfar) {
    constexpr float PI_360 = 3.1415926535897932 / 360.0;
    const float xmax = znear * std::tan(fovx * PI_360);
    const float ymax = xmax / aspect;
    const float zclip = -(zfar + znear) / (zfar - znear);
    const float shear = (-2.0f * znear * zfar) / (zfar - znear);

    float matrix[16] = { znear/xmax, 0.0f,       0.0f,  0.0f,
                         0.0f,       znear/ymax, 0.0f,  0.0f,
                         0.0f,       0.0f,       zclip, -1.0f,
                         0.0f,       0.0f,       shear, 0.0f };

    glMultMatrixf(matrix);
}

static void resetViewMatrix() {
    for (int i = 0; i < 16; ++i) {
        g_modelView[i] = i % 5 ? 0.0f : 1.0f;
    }
    const auto offset = -g_shape->getAABBCentroid();
    g_modelView[12] = offset.x;
    g_modelView[13] = offset.y;
    g_modelView[14] = offset.z;

    g_focus[0] = g_focus[1] = 0.0f;
    g_focus[2] = -g_shape->getAABBSizes().max();
}

////////////////////
// GLUT CALLBACKS //
////////////////////
static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up viewing matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    setPerspective(85.0f, double(WINDOW_WIDTH)/double(WINDOW_HEIGHT), 0.0001, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Camera
    glTranslatef(g_focus[0], g_focus[1], g_focus[2]);
    glMultMatrixf(g_modelView);

    if (g_showVertices)
        g_shape->drawVertices();
    if (g_showEdges)
        g_shape->drawEdges();
    if (g_showFaces)
        g_shape->drawFaces();

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
        g_prevX = x;
        g_prevY = y;
    }

    if (button == GLUT_LEFT_BUTTON) {
        g_leftPressed = state == GLUT_DOWN;
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        g_rightPressed = state == GLUT_DOWN;
    }
    else if (button == GLUT_MIDDLE_BUTTON) {
        g_middlePressed = state == GLUT_DOWN;
    }
}

static void motion(int x, int y) {
    y = WINDOW_HEIGHT - y;

    const float dx = (x - g_prevX);
    const float dy = (y - g_prevY);

    // rotate the scene
    if (g_leftPressed) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(dx, 0, 1, 0);
        glRotatef(dy, -1, 0, 0);
        glMultMatrixf(g_modelView);
        glGetFloatv(GL_MODELVIEW_MATRIX, g_modelView);
    } else if (g_middlePressed) {
        g_focus[0] += 0.005 * dx;
        g_focus[1] += 0.005 * dy;
    } else if (g_rightPressed) {
        g_focus[2] += 0.01 * dy;
    }

    // Store previous mouse positions
    g_prevX = x;
    g_prevY = y;

    glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y) {
    switch(key)
    {
    case ' ':
        if (g_simplified) {
            delete g_shape;
            g_shape = new Collapsible(g_fileName);
        } else {
            g_shape->simplify(g_target);
        }
        g_simplified = !g_simplified;
        break;

    case 'p':
        g_showVertices = !g_showVertices;
        break;
    case 'e':
        g_showEdges = !g_showEdges;
        break;
    case 'f':
        g_showFaces = !g_showFaces;
        break;

    case 9: //tab
        g_toggle = !g_toggle;
        break;

    case 13: //return
        return;

    case 8: //backspace
        resetViewMatrix();
        break;

    case 127: //delete
        return;

    case 114: //leftctrl
        return;

    case 115: //rightctrl
        return;

    case 116: //leftalt
        return;

    case 117: //rightalt
        return;

    case 27: //escape
        glutDestroyWindow(g_window);
        return;

    default:
        return;
    }

    glutPostRedisplay();
}

static void specialkey(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
        g_focus[1] -= 0.05;
        glutPostRedisplay();
        break;

    case GLUT_KEY_DOWN:
        g_focus[1] += 0.05;
        glutPostRedisplay();
        break;

    case GLUT_KEY_LEFT:
        g_focus[0] += 0.05;
        glutPostRedisplay();
        break;

    case GLUT_KEY_RIGHT:
        g_focus[0] -= 0.05;
        glutPostRedisplay();
        break;

    default:
        break;
    }
}

//////////
// MAIN //
//////////
int main(int argc, char **argv) {
    // Load the model
    g_fileName = "...";
    g_shape = new Collapsible(g_fileName);
    g_target = 2000u;

    // Prepare the window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    g_window = glutCreateWindow("Manifold Surface Simplifier");

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

    // Initialize the glut callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialkey);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    // Kick off the main loop, return when window closes
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    glutMainLoop();

    // Clean up upon exit
    if (g_shape) {
        delete g_shape;
        g_shape = nullptr;
    }
    return 0;
}
