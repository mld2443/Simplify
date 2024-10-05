#include "collapsible.h"

#include <GL/freeglut.h> // glut*, gl*
#include <cmath>         // tanf
#include <cstdlib>       // atoi


int g_windowWidth = 1440, g_windowHeight = 900;
int g_windowHandle = 0;
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


static void setPerspective(float fovx, float aspect, float near, float far) {
    constexpr float PI_360 = 3.1415926535897932 / 360.0;
    const float xmax = near * std::tan(fovx * PI_360),
                ymax = xmax / aspect,
                clip = -(far + near) / (far - near),
                div = (-2.0f * near * far) / (far - near);

    const float matrix[16] = { near/xmax, 0.0f,      0.0f, 0.0f,
                               0.0f,      near/ymax, 0.0f, 0.0f,
                               0.0f,      0.0f,      clip, -1.0f,
                               0.0f,      0.0f,      div,  0.0f };

    glMultMatrixf(matrix);
}

static void setOrthographic(float xmin, float ymin, float xmax, float ymax, float near = -1.0f, float far = 1.0f) {
    const float matrix[] = { 2.0f/(xmax - xmin), 0.0f,               0.0f,              (xmin + xmax)/(xmin - xmax),
                             0.0f,               2.0f/(ymax - ymin), 0.0f,              (ymin + ymax)/(ymin - ymax),
                             0.0f,               0.0f,               2.0f/(near - far), (near + far)/(near - far),
                             0.0f,               0.0f,               0.0f,              1.0f };

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

    { // Set up scene
        glEnable(GL_DEPTH);
        glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
        setPerspective(85.0f, double(g_windowWidth)/double(g_windowHeight), 0.0001, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(g_focus[0], g_focus[1], g_focus[2]);
    glMultMatrixf(g_modelView);

        // Models
    if (g_showVertices)
        g_shape->drawVertices();
    if (g_showEdges)
        g_shape->drawEdges();
    if (g_showFaces)
        g_shape->drawFaces();
    }

    // Submit
    glFlush();
    glutSwapBuffers();
}

static void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    g_windowWidth = width;
    g_windowHeight = height;
}

static void mouse(int button, int state, int x, int y) {
    y = g_windowHeight - y;

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
    y = g_windowHeight - y;

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
        glutDestroyWindow(g_windowHandle);
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
    glutInitWindowSize(g_windowWidth, g_windowHeight);
    g_windowHandle = glutCreateWindow("Manifold Surface Simplifier");

    // Set up our openGL specific parameters
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, g_windowWidth, g_windowHeight);

    glClearColor(0, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
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
