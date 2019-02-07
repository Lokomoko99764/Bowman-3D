#include <stdio.h>
#include <stdlib.h>
#include <GL/freeglut.h>
#include "3dfunctions.h"

#define PI 3.14159265359f

int rightPressed = 0;
int leftPressed = 0;
int upPressed = 0;
int downPressed = 0;


GLdouble viewer[] = {0.0,0.0,-40.0,
                            0.0,0.0,0.0,
                            0.0,1.0,0.0
                    };

Vector ArrowVerts[17] ={   {0,0,0},        //arrowhead tip (0 to 8)
                        {4,2,0},{4,-2,0},{4.5,-1,0.5},{4.5,1,0.5},{2,0,0.4},{4.5,-1,-0.5},{4.5,1,-0.5},{2,0,-0.4},

                        {4,0,0.5},      //arrowShaft (9 to 16)
                        {4,0.5,0},{4,0,-0.5},{4,-0.5,0},{18,0,0.5},{18,0.5,0},{18,0,-0.5},{18,-0.5,0}

                        };

typedef struct{
    Off object;
    GLfloat centrePoint[3];
    GLfloat radius;
    GLfloat velocity[3];
    GLfloat mass;
    int halted;
} Object;

typedef float point3[3];
Object arrow;
int arrowFired = 0;
float strength = 0.085;
int count = 0;
int awarded = 0;

float startTime, timeModifier=1;

int windowSizeX = 1500, windowSizeY = 1000;
float mouseSensitivity = 0.2;
float angle = 0; //object angle
float horizontalAngle = 0, verticalAngle=0; //camera angles

float arrowVertAngle = 0, arrowHorAngle = 0;

int shot = 0;
int arrowHits = 0;
Vector direction; //camera looking direction

const float SCALE = 5;
const float TREESCALE = 15;

typedef struct{
    point3 centre; // The center point of the target
    GLfloat radius; // The radius of the biggest ring
    GLfloat segments; // The amount of segments of the target
    GLfloat width; // The width of the target
    GLfloat radDiff; // The multiplitive difference between the rings
    GLuint makeaTarget; //Required for GenLists
}Target;

typedef struct{
    point3 centre;
    point3 trunkCentre;
    GLfloat radius;
    GLfloat height;
    GLfloat segments;
    GLuint makeaTree;
}Tree;

Target target;

Tree tree;

// Makes the target's cylinder with given parameters
void makeTargetCylinder(GLfloat radius, GLfloat segments, GLfloat width, char colour){
    int i;

    GLUquadric *quadTarget = gluNewQuadric();
    GLUquadric *cap = gluNewQuadric();
    GLUquadric *border = gluNewQuadric();
    if(colour=='b'){
        glColor3f(0.255, 0.412, 0.882); //Blue colour
    }
    if(colour=='r'){
        glColor3f(0.863, 0.078, 0.235); //Red colour
    }
    if(colour=='y'){
        glColor3f(0.855, 0.647, 0.125); //Yellow colour
    }

    gluQuadricDrawStyle(quadTarget, GL_POLYGON); //Draws the outside of the target
    gluCylinder(quadTarget, radius, radius, width, segments, segments);

    if(target.radius/target.radDiff/target.radDiff == radius){ //Draws the caps of the smallest ring of the target
        gluQuadricDrawStyle(cap, GL_POLYGON);
        gluDisk(cap, 0, radius, segments, 1); // Front side
        glTranslatef(target.centre[0], target.centre[1], target.centre[2]+target.width);
        gluDisk(cap, 0, radius, segments, 1); // Back side
    } else{
        gluQuadricDrawStyle(cap, GL_POLYGON); //Draws the caps of the target
        gluDisk(cap, radius/target.radDiff, radius, segments, 1); // Front side
        glTranslatef(target.centre[0], target.centre[1], target.centre[2]+target.width);
        gluDisk(cap, radius/target.radDiff, radius, segments, 1); // Back side
    }

    glutSwapBuffers();
}

//Makes the target with three rings
void makeTarget(GLfloat radius, GLfloat segments, GLfloat width, GLfloat radDiff){
    int i;
    char colour;

    for(i=0;i<3;i++){
        if(i==0)
            colour='b';
        if(i==1)
            colour='r';
        if(i==2){
            colour='y';
        }
        glPushMatrix();
            glRotatef(180, 0, 1, 0);
            makeTargetCylinder(radius, segments, width, colour);
        glPopMatrix();
        radius /= radDiff;
    }
    glutSwapBuffers();
}

//Makes the tree's cylinder
void makeTreeCylinder(GLfloat height, GLfloat radius, GLfloat segments){
    GLUquadric *tree = gluNewQuadric();
    gluQuadricDrawStyle(tree, GL_POLYGON);
    glColor3f(0.64f, 0.16, 0.24f); //Brown colour
    glPushMatrix();
        glRotatef(-90, 1.0,0.0,0.0); //Rotate to upright
        gluCylinder(tree, radius,(0.6*radius), height, segments, segments); // Calculate height based on previous height (Each limb gets smaller)
    glPopMatrix();
    glutSwapBuffers();
}

//Makes the tree randomly recursive
void makeTree(GLfloat height, GLfloat radius, GLfloat segments){
    float angleX, angleZ;
    int a;
    makeTreeCylinder(height, radius, segments);
    glTranslatef(0.0, height, 0.0);
    height -= height*.2;
    radius -= radius*0.2;
    for(a= 0; a<3; a++){
        angleX = rand()%50+20;
        angleZ = rand()%50+20;
        if(angleX > 55){
            angleX = -(rand()%50+20);
        }
        if(angleZ > 55){
            angleZ = -(rand()%50+20);
        }
        if (height < 1 || radius < 0.01){
            glColor3f(0.0,0.75,0.0);
            glutSolidSphere(0.75,10,10);
        }
        else{
            glPushMatrix();
                glRotatef(angleX, 1, 0, 1);
                glRotatef(angleZ, 0, 1, 1);
                makeTree(height,radius, segments);
            glPopMatrix();
        }
    }
}

void init(){
    glClearColor(0.000, 0.749, 1.000,1.0);
    glColor3f(1.0,0.0,0.0);
    glEnable(GL_DEPTH_TEST);

    viewer[1] = 10;

    glutWarpPointer(windowSizeX/2,windowSizeY/2);
    glutSetCursor(GLUT_CURSOR_NONE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(60,1,0.1,20000.0);
    glMatrixMode(GL_MODELVIEW);

    arrow.centrePoint[0] = 0; arrow.centrePoint[1] = 5; arrow.centrePoint[2] = 0;
    arrow.velocity[0] = 0; arrow.velocity[1] = 0; arrow.velocity[2] = 0;
    arrow.radius = 1;
    arrow.halted = 1;

    //Initialising values
    target.radius = 4;      // base radius of target
    target.width = 2;       // width of target
    target.segments = 50;   // number of segments around the cylinder
    target.radDiff = 2;     // used to divide the base radius for relatively smaller rings
    target.makeaTarget=glGenLists(1);
    glNewList(target.makeaTarget, GL_COMPILE);
        makeTarget(target.radius, target.segments, target.width, target.radDiff);
    glEndList();
    target.centre[0] = 0; target.centre[1] = 30; target.centre[2] = -200; // Initial position of target

    //Initialising values
    tree.radius = 0.2;  // base radius of tree
    tree.height = 3;    // base height of tree
    tree.segments = 20; // number of segments around the cylinder
    tree.makeaTree=glGenLists(1);
    glNewList(tree.makeaTree, GL_COMPILE);
        makeTree(tree.height, tree.radius, tree.segments);
    glEndList();
    tree.centre[0] = 0; tree.centre[1] = 0; tree.centre[2] = 30;   // Initial position of tree
    tree.trunkCentre[0] = tree.centre[0]; tree.trunkCentre[1] = tree.centre[1]+(tree.height/2); tree.trunkCentre[2] = tree.centre[2]; // Calculate the middle of the trunk for later collision
}

float radHorizontal;
float radVertical;
void mousePassive(int x, int y){ //Triggered when the user's mouse points somewhere else in the window, with no mouse clicks
    int centreX = windowSizeX/2, centreY = windowSizeY/2;

    horizontalAngle += (centreX - x) * mouseSensitivity;
    verticalAngle += (centreY - y) * mouseSensitivity;
    radHorizontal = (horizontalAngle * PI / 180.0);
    radVertical = (verticalAngle * PI / 180.0);
    direction[0] = cos(radVertical) * sin(radHorizontal); direction[1] = sin(radVertical); direction[2] = cos(radVertical) * cos(radHorizontal);
    Vector right; right[0] = sin(radHorizontal - PI/2.0); right[1] = 0; right[2] = cos(radHorizontal - PI/2.0);
    Vector up;

    crossProductVec(&right, &direction, &up);

    viewer[3] = viewer[0] + direction[0]; viewer[4] = viewer[1] + direction[1]; viewer[5] = viewer[2] + direction[2];
    viewer[6] = up[0]; viewer[7] = up[1]; viewer[8] = up[2];


    if(x!=centreX || y != centreY)
        glutWarpPointer(centreX, centreY); //Set mouse back to centre if it's moved

    glutPostRedisplay();
}

void drawArrow()
{
    glColor3f(0.5,0.5,0.5);
    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[0]);
        glVertex3fv(ArrowVerts[5]);
        glVertex3fv(ArrowVerts[1]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[1]);
        glVertex3fv(ArrowVerts[5]);
        glVertex3fv(ArrowVerts[4]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[0]);
        glVertex3fv(ArrowVerts[1]);
        glVertex3fv(ArrowVerts[8]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[8]);
        glVertex3fv(ArrowVerts[1]);
        glVertex3fv(ArrowVerts[7]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[5]);
        glVertex3fv(ArrowVerts[3]);
        glVertex3fv(ArrowVerts[4]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[8]);
        glVertex3fv(ArrowVerts[7]);
        glVertex3fv(ArrowVerts[6]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[7]);
        glVertex3fv(ArrowVerts[4]);
        glVertex3fv(ArrowVerts[3]);
        glVertex3fv(ArrowVerts[6]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[1]);
        glVertex3fv(ArrowVerts[4]);
        glVertex3fv(ArrowVerts[7]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[2]);
        glVertex3fv(ArrowVerts[6]);
        glVertex3fv(ArrowVerts[3]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[0]);
        glVertex3fv(ArrowVerts[2]);
        glVertex3fv(ArrowVerts[5]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[2]);
        glVertex3fv(ArrowVerts[3]);
        glVertex3fv(ArrowVerts[5]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[2]);
        glVertex3fv(ArrowVerts[0]);
        glVertex3fv(ArrowVerts[8]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[2]);
        glVertex3fv(ArrowVerts[8]);
        glVertex3fv(ArrowVerts[6]);
    glEnd();

    glColor3f(0.545, 0.271, 0.075);

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[9]);
        glVertex3fv(ArrowVerts[10]);
        glVertex3fv(ArrowVerts[11]);
        glVertex3fv(ArrowVerts[12]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[11]);
        glVertex3fv(ArrowVerts[12]);
        glVertex3fv(ArrowVerts[14]);
        glVertex3fv(ArrowVerts[15]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[16]);
        glVertex3fv(ArrowVerts[15]);
        glVertex3fv(ArrowVerts[14]);
        glVertex3fv(ArrowVerts[13]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[12]);
        glVertex3fv(ArrowVerts[11]);
        glVertex3fv(ArrowVerts[15]);
        glVertex3fv(ArrowVerts[16]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[9]);
        glVertex3fv(ArrowVerts[12]);
        glVertex3fv(ArrowVerts[16]);
        glVertex3fv(ArrowVerts[13]);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3fv(ArrowVerts[10]);
        glVertex3fv(ArrowVerts[9]);
        glVertex3fv(ArrowVerts[13]);
        glVertex3fv(ArrowVerts[14]);
    glEnd();
}


void followCamera()
{
    //move bow to match viewer position
    glTranslatef(viewer[0], viewer[1], viewer[2]);
    //rotate bow to match viewer:
    glRotatef(horizontalAngle, 0,1,0);
    glRotatef(verticalAngle, -1,0,0);
}

void drawBow(float drawLength) //4 is about the highest drawLength should go
{
    //rotate and translate bow to actual correct position:
    followCamera();
    //translate bow to correct position if player were standing at origin
    glTranslatef(-5.0, 0.0, 0.0);
    glColor3f(0.64, 0.26, 0.16); //brown
    point3 firstCoord, currCoord, prevCoord;
    float bowWidthFactor, bowWidth;
    GLfloat bowWid;


    int i, j;
    for(i = -60; i < 60; i++) //loop through angles for drawing bow, from -60 radians to 60 radians
    {
        //get current width for bow based on angle:
        bowWidthFactor =  60 - abs(i);
        bowWidth = 0.5 * sqrt(bowWidthFactor)/sqrt(120);
        bowWid = (GLfloat) bowWidth;

        //get position for bow at current angle
        currCoord[1] = sin(i/100.0) * 10;
        currCoord[2] = cos(i/100.0) * 10;


        if(i == -60)
        {
            firstCoord[0] = currCoord[0]; //the first position is stored here to be used later
            firstCoord[1] = currCoord[1];
            firstCoord[2] = currCoord[2];
        }
        else
        {
            //draw a polygon between the last coordinate and the current one, with the corners widened based on bowWid
            glBegin(GL_POLYGON);
                glVertex3f(currCoord[0]+bowWid, currCoord[1], currCoord[2]);
                glVertex3f(currCoord[0]-bowWid, currCoord[1], currCoord[2]);
                glVertex3f(prevCoord[0]-bowWid, prevCoord[1], prevCoord[2]);
                glVertex3f(prevCoord[0]+bowWid, prevCoord[1], prevCoord[2]);
            glEnd();
        }

        //keep track of the previous coordinate to use next loop
        prevCoord[0] = currCoord[0];
        prevCoord[1] = currCoord[1];
        prevCoord[2] = currCoord[2];
    }

    glLineWidth(10); //draw a thick line
    glColor3f(1.0, 1.0, 1.0); //draw it in white
    //draw a line between the first coordinate at the bottom, the arrow's position in the middle, and the last coordinate at the top
    glBegin(GL_LINE_STRIP);
        glVertex3fv(firstCoord);
        glVertex3f(firstCoord[0], 0, firstCoord[2]-drawLength); //this is the center of the bow, but with the z value adjusted based on the draw length
        glVertex3fv(currCoord);
    glEnd();

    glLoadIdentity();
    return;
}

void display(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(viewer[0], viewer[1], viewer[2],
                viewer[3], viewer[4], viewer[5],
                    viewer[6], viewer[7], viewer[8]);

    glPushMatrix();
        glTranslatef(arrow.centrePoint[0], arrow.centrePoint[1], arrow.centrePoint[2]); //Moves the arrow to where it is in 3d coordinates
        glRotatef(-arrowVertAngle*cos(arrowHorAngle), 1, 0, 0); //These two just make sure it rotates accordingly around Z and X (ie arrowhead points in the direction of flight)
        glRotatef(arrowVertAngle*sin(arrowHorAngle), 0, 0, 1); //Rotates the arrow according to its vertical relative to its horizontal velocity
        glRotatef(arrowHorAngle * 180.0 / PI, 0,1,0); //rotates arrow in the horizontal direction it's flying (it will already be pointing up or down at this point according to velocity).
        glRotatef(90, 0,1,0); //Positions arrow so its pointing down the positive Z axis
        drawArrow();
    glPopMatrix();

    glPushMatrix();
        drawBow(count/5.0);
    glPopMatrix();
    glPushMatrix();
        drawGrass();
    glPopMatrix();
    glPushMatrix();
        glScalef(10,15,10);
        glTranslatef(-10,0,-5);
        drawHouse();
    glPopMatrix();

        glPushMatrix();
        glTranslatef(target.centre[0], target.centre[1], target.centre[2]);
        glScalef(SCALE, SCALE, SCALE);
        glCallList(target.makeaTarget);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(tree.centre[0], tree.centre[1], tree.centre[2]);
        glScalef(TREESCALE, TREESCALE, TREESCALE);
        glCallList(tree.makeaTree);
    glPopMatrix();

    glPushMatrix();
        glColor3f(0,0,0);
        DisplayScore();
    glPopMatrix();

    glutSwapBuffers();
}

 void moveLeft(){
    GLfloat viewVecs[3] = { viewer[3] - viewer[0],
                            viewer[4] - viewer[1],
                            viewer[5] - viewer[2] };
    Vector right;
    Vector upVec = { viewer[6], viewer[7], viewer[8] };

    crossProductVec(&viewVecs, &upVec, &right);

    float magnitude = sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);

    right[0] /= magnitude;
    right[1] /= magnitude;
    right[2] /= magnitude;

    viewer[0] -= right[0];
    viewer[1] -= right[1];
    viewer[2] -= right[2];

    viewer[3] -= right[0];
    viewer[4] -= right[1];
    viewer[5] -= right[2];
}

 void moveRight(){
    Vector viewVecs = {     viewer[3] - viewer[0],
                            viewer[4] - viewer[1],
                            viewer[5] - viewer[2] };
    Vector right;
    Vector upVec = { viewer[6], viewer[7], viewer[8] };

    crossProductVec(&viewVecs, &upVec, &right);

    float magnitude = sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);

    right[0] /= magnitude;
    right[1] /= magnitude;
    right[2] /= magnitude;

    viewer[0] += right[0];
    viewer[1] += right[1];
    viewer[2] += right[2];

    viewer[3] += right[0];
    viewer[4] += right[1];
    viewer[5] += right[2];
}

void drawHouse()
{
    glColor3f(0.627, 0.322, 0.176);
    glBegin(GL_POLYGON); //Right side
        glVertex3f(1,-1,1);
        glVertex3f(1,1,1);
        glVertex3f(1,1,-1);
        glVertex3f(1,-1,-1);
    glEnd();

    glBegin(GL_POLYGON); //Left side
        glVertex3f(-1,-1,1);
        glVertex3f(-1,1,1);
        glVertex3f(-1,1,-1);
        glVertex3f(-1,-1,-1);
    glEnd();

    glBegin(GL_POLYGON); //Top
        glVertex3f(1,1,1);
        glVertex3f(1,1,-1);
        glVertex3f(-1,1,-1);
        glVertex3f(-1,1,1);
    glEnd();

    glBegin(GL_POLYGON); //Bottom
        glVertex3f(1,-1,1);
        glVertex3f(1,-1,-1);
        glVertex3f(-1,-1,-1);
        glVertex3f(-1,-1,1);
    glEnd();

    glBegin(GL_POLYGON); //Back wall
        glVertex3f(1,-1,-1);
        glVertex3f(1,1,-1);
        glVertex3f(-1,1,-1);
        glVertex3f(-1,-1,-1);
    glEnd();

    glBegin(GL_POLYGON); //Roof front
        glVertex3f(1,1,-1);
        glVertex3f(-1,1,-1);
        glVertex3f(0,1.75,-1);
    glEnd();

    glBegin(GL_POLYGON); //Roof back
        glVertex3f(1,1,1);
        glVertex3f(-1,1,1);
        glVertex3f(0,1.75,1);
    glEnd();

    glBegin(GL_POLYGON); //roof rectangle right
        glVertex3f(1,1,1);
        glVertex3f(1,1,-1);
        glVertex3f(0,1.75,-1);
        glVertex3f(0,1.75,1);
    glEnd();

    glBegin(GL_POLYGON); //roof rectangle left
        glVertex3f(-1,1,1);
        glVertex3f(-1,1,-1);
        glVertex3f(0,1.75,-1);
        glVertex3f(0,1.75,1);
    glEnd();

    glBegin(GL_POLYGON); //front right rectangle
        glVertex3f(1,-1,1);
        glVertex3f(1,1,1);
        glVertex3f(0.5,1,1);
        glVertex3f(0.5,-1,1);
    glEnd();

    glBegin(GL_POLYGON); //front left rectangle
        glVertex3f(-1,-1,1);
        glVertex3f(-1,1,1);
        glVertex3f(-0.5,1,1);
        glVertex3f(-0.5,-1,1);
    glEnd();

    glBegin(GL_POLYGON); //front top rectangle
        glColor3f(0.545, 0.271, 0.075);
        glVertex3f(0.5,0.25,1);
        glVertex3f(0.5,1,1);
        glVertex3f(-0.5,1,1);
        glVertex3f(-0.5,0.25,1);
        glColor3f(1.0,0.0,0.0);
    glEnd();
}

void drawGrass(){
    glBegin(GL_POLYGON); //square of grass
        glColor3f(0.133, 0.545, 0.133);
        glVertex3f(500,-1,-500);
        glVertex3f(500,-1,500);
        glVertex3f(-500,-1,500);
        glVertex3f(-500,1,-500);

        glColor3f(1.0,0.0,0.0);
    glEnd();
}

void keyDown(unsigned char key, int x, int y)
{
    static int i, j;

    switch(key){

        case 27:
            exit(0);
            break;
        case 'R':
        case 'r':
            arrow.halted = 1;
            arrow.velocity[0] = 0;
            arrow.velocity[1] = 0;
            arrow.velocity[2] = 0;
            shot = 0;
            count = 0;
            strength = 0.085;
            break;
        case 'D':
        case 'd':
            rightPressed = 1;
            break;
        case 'A':
        case 'a':
            leftPressed = 1;
            break;
        case 'W':
        case 'w':
            upPressed = 1;
            break;
        case 'S':
        case 's':
            downPressed = 1;
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

void keyUp(unsigned char key, int x, int y)
{
    static int i, j;

    switch(key)
    {
        case 'D':
        case 'd':
            rightPressed = 0;
            break;
        case 'A':
        case 'a':
            leftPressed = 0;
            break;
        case 'W':
        case 'w':
            upPressed = 0;
            break;
        case 'S':
        case 's':
            downPressed = 0;
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

 void moveForward(){
    Vector viewVecs = { viewer[3] - viewer[0],
                            viewer[4] - viewer[1],
                            viewer[5] - viewer[2] };

    float magnitude = sqrt(viewVecs[0] * viewVecs[0] + viewVecs[1] * viewVecs[1] + viewVecs[2] * viewVecs[2]);

    viewVecs[0] /= magnitude;
    viewVecs[2] /= magnitude;

    viewer[0] += viewVecs[0];
    viewer[2] += viewVecs[2];
    viewer[3] += viewVecs[0];
    viewer[5] += viewVecs[2];
}

 void moveBack(){
    Vector viewVecs = { viewer[3] - viewer[0],
                            viewer[4] - viewer[1],
                            viewer[5] - viewer[2] };

    float magnitude = sqrt(viewVecs[0] * viewVecs[0] + viewVecs[1] * viewVecs[1] + viewVecs[2] * viewVecs[2]);

    viewVecs[0] /= magnitude;
    viewVecs[2] /= magnitude;

    viewer[0] -= viewVecs[0];
    viewer[2] -= viewVecs[2];
    viewer[3] -= viewVecs[0];
    viewer[5] -= viewVecs[2];
}

void skeyDown(int key, int x, int y)
{
    switch(key)
    {
    case GLUT_KEY_RIGHT:
        rightPressed = 1;
        break;
    case GLUT_KEY_LEFT:
        leftPressed = 1;
        break;
    case GLUT_KEY_UP:
        upPressed = 1;
        break;
    case GLUT_KEY_DOWN:
        downPressed = 1;
        break;
    default:
        break;
    }
    glutPostRedisplay();
}

void skeyUp(int key, int x, int y)
{
    switch(key)
    {
    case GLUT_KEY_RIGHT:
        rightPressed = 0;
        break;
    case GLUT_KEY_LEFT:
        leftPressed = 0;
        break;
    case GLUT_KEY_UP:
        upPressed = 0;
        break;
    case GLUT_KEY_DOWN:
        downPressed = 0;
        break;
    default:
        break;
    }
    glutPostRedisplay();
}

void mouseWheel(int wheel, int direction, int x, int y)
{
    if(wheel == 0 && shot == 0)
    {
        switch(direction)
        {
            case -1:
                if(count != 20)
                {
                    strength=strength*1.5;
                    count++;
                }
                break;
            case 1:
                if(count != 0)
                {
                    count--;
                }
            default:
                break;
        }
    }
}

void mouse(int button, int state, int x, int y)
{
    switch(button)
    {
        case GLUT_LEFT_BUTTON:
            if(state == GLUT_DOWN && !shot)
            {
                shoot();
            }
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

void DisplayScore(){
    int i;

    int length = snprintf( NULL, 0, "%d", arrowHits ); //Length of score number string
    char* scoreString = malloc( length + 1 ); //ALlocates memory to score number string
    snprintf( scoreString, length + 1, "%d", arrowHits ); //Converts number to string

    glDisable(GL_TEXTURE_2D); //Matrix/camera converted to orthographic 2d
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, windowSizeX, 0.0, windowSizeY);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glRasterPos2i(windowSizeX-105, 10); //Positions the score text
    char s[] = "Score: ";
    void * font = GLUT_BITMAP_TIMES_ROMAN_24; //font

    for (i = 0; i < 7; i++)
    {
      char c = s[i];
      glColor3f(1.0, 1.0, 1.0);
      glutBitmapCharacter(font, c); //Prints string to screen
    }
    for(i=0;i < length; i++){
        char c = scoreString[i];
        glColor3f(1.0, 1.0, 1.0);
        glutBitmapCharacter(font, c); //Prints score string to screen
    }

    glLineWidth(1);
    glBegin(GL_LINES);
        glColor3f(1,1,1);
        glVertex2f((windowSizeX/2.0)-7, windowSizeY/2.0);
        glVertex2f((windowSizeX/2.0)+6, windowSizeY/2.0);
    glEnd();

    glBegin(GL_LINES);
        glColor3f(1,1,1);
        glVertex2f(windowSizeX/2.0, (windowSizeY/2.0-7));
        glVertex2f(windowSizeX/2.0, (windowSizeY/2.0+6));
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_TEXTURE_2D); //Matrix set back to 3d etc.

    free(scoreString); //Frees memory allocated to score string
}

 void shoot()
{
    arrow.centrePoint[1] = viewer[1];
    shot = 1;
    count = 0;
    awarded = 0;

    drawArrow();
    arrow.halted = 0;
    arrow.velocity[0] += sin(horizontalAngle*PI/180.0) * cos(verticalAngle*PI/180.0) * strength;
    arrow.velocity[1] += sin(verticalAngle*PI/180.0) * strength;
    arrow.velocity[2] += cos(horizontalAngle*PI/180.0) * cos(verticalAngle*PI/180.0) * strength;

}

void reshape(int w, int h){
    glViewport(0,0,w,h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLfloat newAspect = (GLfloat) w / (GLfloat) h;
    gluPerspective(60,newAspect,0.1,20000.0);

    glMatrixMode(GL_MODELVIEW);
}

void targetCollision(Target *targetobj){
    GLfloat dX = arrow.centrePoint[0]-targetobj->centre[0];
    GLfloat dY = arrow.centrePoint[1]-targetobj->centre[1];
    GLfloat dZ = arrow.centrePoint[2]-targetobj->centre[2];

    GLfloat distToTarget = sqrt(dX*dX + dY*dY + dZ*dZ); // Distance from the arrow to the target

    if(!awarded)
    {
        if(distToTarget <= targetobj->radius*SCALE)
        {
            if(arrow.centrePoint[2] <= targetobj->centre[2]+(targetobj->width/5))
            {
                if(distToTarget <= ((targetobj->radius/targetobj->radDiff)/targetobj->radDiff)*SCALE)
                {
                    arrowHits += 3;
                    awarded = 1;
                    arrow.velocity[0] = 0; arrow.velocity[1] = 0; arrow.velocity[2] = 0;
                    arrow.halted=1;
                }
                else
                {
                    if(distToTarget <= targetobj->radius/targetobj->radDiff*SCALE)
                    {
                        arrowHits += 2;
                        awarded = 1;
                        arrow.velocity[0] = 0; arrow.velocity[1] = 0; arrow.velocity[2] = 0;
                        arrow.halted=1;
                    }
                    else
                    {
                        arrowHits += 1;
                        awarded = 1;
                        arrow.velocity[0] = 0; arrow.velocity[1] = 0; arrow.velocity[2] = 0;
                        arrow.halted=1;

                    }
                }
            }
        }
    }
}

 void treeCollision(Tree *treeobj){
    GLfloat diX = arrow.centrePoint[0]-treeobj->trunkCentre[0];
    GLfloat diY = arrow.centrePoint[1]-treeobj->trunkCentre[1];
    GLfloat diZ = arrow.centrePoint[2]-treeobj->trunkCentre[2];

    GLfloat distToTrunk = sqrt(diX*diX + diY*diY + diZ*diZ); // Distance from the tree to target

    if(distToTrunk <= (treeobj->height*SCALE)/2)
    {
        /****************************************************************/
        //bounding box to find collision of tree
        if((arrow.centrePoint[0] >= treeobj->centre[0]-treeobj->radius*TREESCALE/*min x*/ && arrow.centrePoint[0] <= treeobj->centre[0]+treeobj->radius*TREESCALE /*max x*/)
            && ((arrow.centrePoint[2] >= treeobj->centre[2]-treeobj->radius*TREESCALE/*min z*/ && arrow.centrePoint[2] <= treeobj->centre[2]+treeobj->radius*TREESCALE)/*max z*/)
            ){
            // Stops the arrow
            arrow.velocity[0] = 0; arrow.velocity[1] = 0; arrow.velocity[2] = 0;
            arrow.halted=1;
        }
    }
}

 void groundCollision(){
    if(arrow.centrePoint[1] - arrow.radius <= 0){
        arrow.centrePoint[1] = arrow.radius;
        arrow.velocity[0] = 0; arrow.velocity[1] = 0; arrow.velocity[2] = 0;
        arrow.halted = 1;
    }
}

 void applyVelocity(Object *obj, float time){
    obj->centrePoint[0] += obj->velocity[0] * (time);
    obj->centrePoint[1] += obj->velocity[1] * (time);
    obj->centrePoint[2] += obj->velocity[2] * (time);
}

 void applyAcceleration(Object *obj, float time, GLfloat acc[3]){
    obj->velocity[0] += acc[0] * (time);
    obj->velocity[1] += acc[1] * (time);
    obj->velocity[2] += acc[2] * (time);
}

void animate(int x){
    static GLfloat gravity[3] = {0, -98, 0};

    glutTimerFunc(33,animate,0);

    int currTime = glutGet(GLUT_ELAPSED_TIME);
    float difference = ((currTime - startTime)/1000.f) * timeModifier;
    angle = (int) angle % 360;

    if(rightPressed)
    {
        moveRight();
    }
    if(leftPressed)
    {
        moveLeft();
    }
    if(upPressed)
    {
        moveForward();
    }
    if(downPressed)
    {
        moveBack();
    }
    if(!shot)
    {
        arrowVertAngle = verticalAngle;
        arrowHorAngle = radHorizontal;

        arrow.centrePoint[0] = viewer[0];
        arrow.centrePoint[1] = viewer[1]+10000;
        arrow.centrePoint[2] = viewer[2];
    }

    if(!arrow.halted)
    {
        applyVelocity(&arrow, difference);
        applyAcceleration(&arrow, difference, gravity);
        arrowVertAngle = asin(arrow.velocity[1] / sqrt(arrow.velocity[0]*arrow.velocity[0] + arrow.velocity[1]*arrow.velocity[1] + arrow.velocity[2]*arrow.velocity[2])) * 180.0 / PI;
        float arrowVelX = arrow.velocity[0];
        float arrowVelZ = arrow.velocity[2];
        float inverter = 0;
        if(arrowVelZ < 0){ inverter = PI; }
        arrowHorAngle = atan(arrowVelX / arrowVelZ) + inverter;
    }

    groundCollision();
    treeCollision(&tree);
    targetCollision(&target);


    startTime = currTime;
    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowSizeX,windowSizeY);
    glutInitWindowPosition(0,0);
    glutCreateWindow("BowMan3D");
    glutFullScreen();
    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutReshapeFunc(reshape);
    glutTimerFunc(33,animate, 0);
    glutPassiveMotionFunc(mousePassive);
    glutSpecialFunc(skeyDown);
    glutSpecialUpFunc(skeyUp);
    glutMouseWheelFunc(mouseWheel);
    glutMouseFunc(mouse);


    startTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
