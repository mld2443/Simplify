#include "collapsible.h"

#include <GL/freeglut.h> // glut*, gl*
#include <cmath>         // tan


int windowWidth = 1440, windowHeight = 900;
int windowHandle = 0;
bool simplified = false;
unsigned target = 0u;
const char *fileName;
Collapsible *shape;
bool showFaces = true, showEdges = false, showVertices = false;
bool toggle = false;

// mouse state
int prevX = 0, prevY = 0;
bool leftPressed = false, rightPressed = false, middlePressed = false;

// view state
float modelView[16] = { 1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1 };
float focus[3] = { 0, 0, 0 };


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
        ::modelView[i] = i % 5 ? 0.0f : 1.0f;
    }
    const auto offset = -::shape->getAABBCentroid();
    ::modelView[12] = offset.x;
    ::modelView[13] = offset.y;
    ::modelView[14] = offset.z;

    ::focus[0] = ::focus[1] = 0.0f;
    ::focus[2] = -::shape->getAABBSizes().max();
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
        ::setPerspective(85.0f, double(::windowWidth)/double(::windowHeight), 0.0001, 100.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glTranslatef(::focus[0], ::focus[1], ::focus[2]);
        glMultMatrixf(::modelView);

        // Models
        if (::showVertices)
            ::shape->drawVertices();
        if (::showEdges)
            ::shape->drawEdges();
        if (::showFaces)
            ::shape->drawFaces();
    }

    // Submit
    glFlush();
    glutSwapBuffers();
}

static void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    ::windowWidth = width;
    ::windowHeight = height;
}

static void mouse(int button, int state, int x, int y) {
    y = ::windowHeight - y;

    // Mouse state that should always be stored on pressing
    if (state == GLUT_DOWN) {
        ::prevX = x;
        ::prevY = y;
    }

    if (button == GLUT_LEFT_BUTTON) {
        ::leftPressed = state == GLUT_DOWN;
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        ::rightPressed = state == GLUT_DOWN;
    }
    else if (button == GLUT_MIDDLE_BUTTON) {
        ::middlePressed = state == GLUT_DOWN;
    }
}

static void motion(int x, int y) {
    y = ::windowHeight - y;

    const float dx = (x - ::prevX);
    const float dy = (y - ::prevY);

    // rotate the scene
    if (::leftPressed) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(dx, 0, 1, 0);
        glRotatef(dy, -1, 0, 0);
        glMultMatrixf(::modelView);
        glGetFloatv(GL_MODELVIEW_MATRIX, ::modelView);
    } else if (::middlePressed) {
        ::focus[0] += 0.005 * dx;
        ::focus[1] += 0.005 * dy;
    } else if (::rightPressed) {
        ::focus[2] += 0.01 * dy;
    }

    // Store previous mouse positions
    ::prevX = x;
    ::prevY = y;

    glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y) {
    switch(key) {
    case ' ':
        if (::simplified) {
            delete ::shape;
            ::shape = new Collapsible(::fileName);
        } else {
            ::shape->simplify(::target);
        }
        ::simplified = !::simplified;
        break;

    case 'p':
        ::showVertices = !::showVertices;
        break;
    case 'e':
        ::showEdges = !::showEdges;
        break;
    case 'f':
        ::showFaces = !::showFaces;
        break;

    case 9: //tab
        ::toggle = !::toggle;
        break;

    case 13: //return
        return;

    case 8: //backspace
        ::resetViewMatrix();
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
        glutDestroyWindow(::windowHandle);
        return;

    default:
        return;
    }

    glutPostRedisplay();
}

static void specialkey(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
        ::focus[1] -= 0.05;
        glutPostRedisplay();
        break;

    case GLUT_KEY_DOWN:
        ::focus[1] += 0.05;
        glutPostRedisplay();
        break;

    case GLUT_KEY_LEFT:
        ::focus[0] += 0.05;
        glutPostRedisplay();
        break;

    case GLUT_KEY_RIGHT:
        ::focus[0] -= 0.05;
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
    ::fileName = "...";
    ::shape = new Collapsible(::fileName);
    ::target = 800u;

    // Prepare the window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(::windowWidth, ::windowHeight);
    ::windowHandle = glutCreateWindow("Manifold Surface Simplifier");

    // Set up our openGL specific parameters
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, ::windowWidth, ::windowHeight);

    glClearColor(0, 0, 0, 1);
    glEnable(GL_LIGHT0);
    glEnable(GL_CULL_FACE);

    // Center the model in viewspace and zoom in/out so it takes up most of the screen
    ::resetViewMatrix();

    // Initialize the glut callbacks
    glutDisplayFunc(::display);
    glutReshapeFunc(::reshape);
    glutKeyboardFunc(::keyboard);
    glutSpecialFunc(::specialkey);
    glutMouseFunc(::mouse);
    glutMotionFunc(::motion);

    // Kick off the main loop, return when window closes
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    glutMainLoop();

    // Clean up upon exit
    if (::shape) {
        delete ::shape;
        ::shape = nullptr;
    }

    return 0;
}
