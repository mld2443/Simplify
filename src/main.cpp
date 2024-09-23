//
//  main.cpp
//  Simplify
//
//  Created by Matthew Dillard on 11/17/15.
//

#include <GL/glut.h>
#include <cstdlib>

#include "manifold.h"
#include "fileio.h"

int WINDOW_WIDTH = 1440, WINDOW_HEIGHT = 900;
int window = 0;
bool drawcontrol = false;
bool executed = false;
unsigned int target;
char* file;

// mouse state
int prevX = 0, prevY = 0;
bool leftPressed = false, rightPressed = false, middlePressed = false;

// view state
float rotMat[] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
float focus[] = {0,0,0};

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up viewing matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, float(WINDOW_WIDTH)/WINDOW_HEIGHT, .0001, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Camera
    glTranslatef(focus[0], focus[1], focus[2]);
    glMultMatrixf(rotMat);

    m.draw(drawcontrol);

    glFlush();
    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
}

void mouse(int button, int state, int x, int y) {
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

void motion(int x, int y) {
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
        glMultMatrixf(rotMat);
        glGetFloatv(GL_MODELVIEW_MATRIX, rotMat);
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

void keyboard(unsigned char key, int x, int y) {
    switch(key)
    {
        case 9: //tab
            drawcontrol = !drawcontrol;
            glutPostRedisplay();
            break;

        case 13: //return
            break;

        case 8: //delete
            break;

        case 127: //backspace
            break;

        case ' ':
            if (!executed)
                m.simplify(target);
            else
                load(file);
            executed = !executed;
            glutPostRedisplay();
            break;

        case 27: //escape
            glutDestroyWindow(window);
            exit(0);
            break;

        default:
            break;
    }
}

void specialkey(int key, int x, int y) {
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
            break;
    }
}

void init(char *filename, const unsigned int t) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glClearColor(0, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    glEnable(GL_CULL_FACE);

    load(filename);
    file = filename;
    target = t;

    float dx = xhigh-xlow, dy = yhigh - ylow, dz = zhigh - zlow;

    if (dx >= dy && dx >= dz) {
        focus[2] -= (4*dx)/5;
    }
    else if (dy >= dz) {
        focus[2] -= (4*dy)/5;
    }
    else {
        focus[2] -= (4*dz)/5;
    }
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    window = glutCreateWindow("CSCE 645 - Matthew Dillard");
    //glutFullScreen();

    init(argv[1], atoi(argv[2]));

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialkey);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    // main loop
    glutMainLoop();
}
