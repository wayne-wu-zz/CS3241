// CS3241Lab4.cpp : Defines the entry point for the console application.
//#include <cmath>
#include "math.h"
#include <iostream>
#include <fstream>
#include <algorithm>

/* Include header files depending on platform */
#ifdef _WIN32
	#include "GL\freeglut.h"
	#define M_PI 3.14159
#elif __APPLE__
	#include <OpenGL/gl.h>
	#include <GLUT/GLUT.h>
#endif

#define MAXPTNO 1000
#define NLINESEGMENT 32
#define NOBJECTONCURVE 8
#define H 0.01
#define SELECTRADIUS 5

using namespace std;

// Global variables that you can use
struct Point {
	int x,y;
	bool modified = false; // modified for continuity
	int x0, y0; // for storing previous position value
};

// Storage of control points
int nPt = 0;
Point ptList[MAXPTNO];

// Display options
bool displayControlPoints = true;
bool displayControlLines = true;
bool displayTangentVectors = false;
bool displayObjects = false;
bool C1Continuity = false;
bool animation = false;
bool shadow = false;

// State
bool draggingCV = false;
int draggingCVIdx = -1;

float time = 0;
int mouseX = -1;
int mouseY = -1;
	
void drawRightArrow()
{
	glColor3f(0,1,0);
	glBegin(GL_LINE_STRIP);
		glVertex2f(0,0);
		glVertex2f(100,0);
		glVertex2f(95,5);
		glVertex2f(100,0);
		glVertex2f(95,-5);
	glEnd();
}

Point cubicBezier(float t, Point& c0, Point& c1, Point& c2, Point& c3)
{
	float a = 1 - t;
	Point pt;
	pt.x = a*a*a*c0.x + 3*t*a*a*c1.x + 3*t*t*a*c2.x + t*t*t*c3.x;
	pt.y = a*a*a*c0.y + 3*t*a*a*c1.y + 3*t*t*a*c2.y + t*t*t*c3.y;
	return pt;
}

Point cubicBezierDiff(float t, Point& c0, Point& c1, Point& c2, Point& c3)
{
	float a = 1 - t;
	Point pt;
	pt.x = 3*a*a*(c1.x - c0.x) + 6*t*a*(c2.x - c1.x) + 3*t*t*(c3.x - c2.x);
	pt.y = 3*a*a*(c1.y - c0.y) + 6*t*a*(c2.y - c1.y) + 3*t*t*(c3.y - c2.y);
	return pt;
}

void drawLeaf(float freq)
{
	glBegin(GL_POLYGON);
	for (int i = 0; i < NLINESEGMENT; i++)
	{
		float theta = i * M_PI / NLINESEGMENT;
		float r = 20*cos(freq * theta);
		glVertex2f(r*cos(theta), r*sin(theta));
	}
	glEnd();
}

float getScale(float t)
{
	return max(1.0f, -5*t*t + 1.5f);
}

void drawObject(int i)
{
	if (animation)
	{
		float T = 1.0;                   // period
		float t0 = i*T / (NOBJECTONCURVE - 1);
		int n = time / T;                // interval
		float mint = min(abs(time - t0 - n*T), abs(time - t0 - (n + 1)*T));
		mint = min(mint, abs(time - t0 - (n - 1)*T));
		float scale = getScale(mint);
		glScalef(scale, scale, scale);
	}
	glColor4f(0.0, 0.0, 0.0, 0.7);
	if (!shadow)
		glColor3f(0.94, 0.32, 0.2);
	drawLeaf(2);
	if (!shadow)
		glColor3f(0.97, 0.59, 0.38);
	drawLeaf(4);
	if (!shadow)
		glColor3f(0.96, 0.48, 0.13);
	drawLeaf(6);
}

void drawBezierCurve(int start)
{
	Point c0 = ptList[start++];
	Point c1 = ptList[start++];
	Point c2 = ptList[start++];
	Point c3 = ptList[start++];

	glColor3f(0.61, 0.34, 0.03);
	glLineWidth(2.0);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i <= NLINESEGMENT; i++)
	{
		Point& pt = cubicBezier(static_cast<float>(i) / NLINESEGMENT, c0, c1, c2, c3);
		glVertex2f(pt.x, pt.y);
	}
	glEnd();

	if (displayTangentVectors || displayObjects)
	{
		for (int i = 0; i < NOBJECTONCURVE; i++)
		{
			float t = static_cast<float>(i) / (NOBJECTONCURVE-1);
			Point& vec = cubicBezierDiff(static_cast<float>(i) / (NOBJECTONCURVE-1), c0, c1, c2, c3);
			Point& pt = cubicBezier(t, c0, c1, c2, c3);

			glPushMatrix();
			glTranslatef(pt.x, pt.y, 0);
			glRotatef(atan2(vec.y, vec.x)*180/M_PI, 0, 0, 1);
			if (displayTangentVectors)
				drawRightArrow();
			if (displayObjects)
				drawObject(i);
			glPopMatrix(); 
		}
	}
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();

	if (C1Continuity)
	{
		// Change all second points
		int i = 3;
		while (i + 3 < nPt)
		{
			float dx = ptList[i].x - ptList[i - 1].x;
			float dy = ptList[i].y - ptList[i - 1].y;
			Point& pt = ptList[i + 1];
			if (!pt.modified)
			{
				// Store the unmodified point position to restore position
				pt.x0 = pt.x;
				pt.y0 = pt.y;
			}
			pt.x = ptList[i].x + dx;
			pt.y = ptList[i].y + dy;
			pt.modified = true;
			i += 3;
		}
	}
	else
	{
		// Restore all points' positions
		for (int i = 0; i < nPt; i++)
		{
			if (ptList[i].modified)
			{
				ptList[i].x = ptList[i].x0; 
				ptList[i].y = ptList[i].y0;
			}
			ptList[i].modified = false;
		}
	}

	// Draw Bezier Curve (Before Points)

	// Draw a shadow outline
	if (displayObjects)
	{
		glPushMatrix();
		glTranslatef(-3, -3, 0);
		shadow = true;
		for (int i = 0; i < nPt && i + 3 < nPt; i += 3)
		{
			drawBezierCurve(i);
		}
		shadow = false;
		glPopMatrix();
	}

	for (int i = 0; i < nPt && i + 3 < nPt; i += 3)
	{
		drawBezierCurve(i);
	}

	if(displayControlPoints)
	{
		glPointSize(5);
		glBegin(GL_POINTS);
		for(int i=0;i<nPt; i++)
		{
			ptList[i].modified ? glColor3f(1,0,0) : glColor3f(0,0,0);
			glVertex2d(ptList[i].x,ptList[i].y);
		}
		glEnd();
		glPointSize(1);
	}

	if(displayControlLines)
	{
		glColor3f(0,0.5,0);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < nPt; i++)
		{
			glVertex2d(ptList[i].x, ptList[i].y);
		}
		glEnd();
	}

	glPopMatrix();
	glutSwapBuffers ();
}

void reshape (int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,w,h,0);  
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
}

void init(void)
{
	glClearColor (1.0,1.0,1.0, 1.0);
	glEnable(GL_BLEND);
}

void readFile()
{

	std::ifstream file;
    file.open("savefile.txt");
	file >> nPt;

	if(nPt>MAXPTNO)
	{
		cout << "Error: File contains more than the maximum number of points." << endl;
		nPt = MAXPTNO;
	}

	for(int i=0;i<nPt;i++)
	{
		file >> ptList[i].x;
		file >> ptList[i].y;
	}
    file.close();// is not necessary because the destructor closes the open file by default
}

void writeFile()
{
	std::ofstream file;
    file.open("savefile.txt");
    file << nPt << endl;

	for(int i=0;i<nPt;i++)
	{
		file << ptList[i].x << " ";
		file << ptList[i].y << endl;
	}
    file.close(); // is not necessary because the destructor closes the open file by default
}

void keyboard (unsigned char key, int x, int y)
{
	switch (key) {
		case 'r':
		case 'R':
			readFile();
			animation = true;
			glClearColor(1.0, 0.82, 0.0, 1.0);
		break;

		case 'w':
		case 'W':
			writeFile();
		break;

		case 'T':
		case 't':
			displayTangentVectors = !displayTangentVectors;
		break;

		case 'o':
		case 'O':
			displayObjects = !displayObjects;
		break;

		case 'p':
		case 'P':
			displayControlPoints = !displayControlPoints;
		break;

		case 'L':
		case 'l':
			displayControlLines = !displayControlLines;
		break;

		case 'C':
		case 'c':
			C1Continuity = !C1Continuity;
		break;

		case 'e':
		case 'E':
			nPt = 0;
		break;
		
		case 127:
			nPt--; //undo
		break;

		case 27:
			exit(0);
		break;

		default:
		break;
	}

	glutPostRedisplay();
}

void findSelectedPoint(int x, int y)
{
	int min_idx = -1;
	float min_dist = -1;
	for (int i = 0; i < nPt; i++)
	{
		Point& pt = ptList[i];
		float dx = x - pt.x;
		float dy = y - pt.y;
		float dist = sqrt(dx*dx + dy*dy);
		if ((min_idx < 0 && dist < SELECTRADIUS) || (min_idx >= 0 && dist < min_dist))
		{
			min_idx = i;
			min_dist = dist;
		}
	}
	draggingCVIdx = min_idx;
}

void movePoint()
{
	if (draggingCVIdx >= 0)
	{
		Point& pt = ptList[draggingCVIdx];
		// If point is modified for C1 continuity, 
		// we don't allow dragging since the position 
		// is determined automatically
		if (!pt.modified)
		{
			pt.x = mouseX;
			pt.y = mouseY;
		}
	}
}

void idle(void)
{
	time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	if (draggingCV)
	{
		movePoint();
		glutPostRedisplay();
	}
	if (displayObjects)
	{
		glutPostRedisplay();
	}
}

void mouse_motion(int x, int y)
{
	mouseX = x;
	mouseY = y;
}

void mouse(int button, int state, int x, int y)
{
	/*button: GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON, or GLUT_RIGHT_BUTTON */
	/*state: GLUT_UP or GLUT_DOWN */
	enum
	{
		MOUSE_LEFT_BUTTON = 0,
		MOUSE_MIDDLE_BUTTON = 1,
		MOUSE_RIGHT_BUTTON = 2,
		MOUSE_SCROLL_UP = 3,
		MOUSE_SCROLL_DOWN = 4
	};
	if((button == MOUSE_LEFT_BUTTON)&&(state == GLUT_UP))
	{
		if(nPt==MAXPTNO)
		{
			cout << "Error: Exceeded the maximum number of points." << endl;
			return;
		}
		if (ptList[nPt].modified)
		{
			ptList[nPt].x0 = x;
			ptList[nPt].y0 = y;
		}
		ptList[nPt].x=x;
		ptList[nPt].y=y;
		
		nPt++;
	}

	// Drag control vertices (points)
	if (button == MOUSE_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			draggingCV = true;
			findSelectedPoint(x, y);
		}
		else if (state == GLUT_UP)
		{
			draggingCV = false;
		}
	}

	mouseX = x;
	mouseY = y;

	glutPostRedisplay();
}

int main(int argc, char **argv)
{
	cout<<"CS3241 Lab 4"<< endl<< endl;
	cout << "Left mouse click: Add a control point"<<endl;
	cout << "Right mouse click and drag: Move a control point" << endl;
	cout << "ESC: Quit" <<endl;
	cout << "P: Toggle displaying control points" <<endl;
	cout << "L: Toggle displaying control lines" <<endl;
	cout << "E: Erase all points (Clear)" << endl;
	cout << "Del: Erase the last point" << endl;
	cout << "C: Toggle C1 continuity" <<endl;	
	cout << "T: Toggle displaying tangent vectors" <<endl;
	cout << "O: Toggle displaying objects" <<endl;
	cout << "R: Read in control points from \"savefile.txt\"" <<endl;
	cout << "W: Write control points to \"savefile.txt\"" <<endl;
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize (600, 600);
	glutInitWindowPosition (50, 50);
	glutCreateWindow ("CS3241 Assignment 4");
	init ();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutMotionFunc(mouse_motion);
	glutIdleFunc(idle);
	glutMainLoop();

	return 0;
}
