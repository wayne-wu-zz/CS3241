// CS3241Lab2.cpp : Defines the entry point for the console application.
#include <cmath>
#include <iostream>
#include <vector>
#include <memory>
#include <time.h>

/* Include header files depending on platform */
#ifdef _WIN32
    #include "GL\freeglut.h"
	#define M_PI 3.14159
#elif __APPLE__
	#include <OpenGL/gl.h>
	#include <GLUT/GLUT.h>
#endif

using namespace std;

#define numStars 300
#define numPlanets 9
#define FADE 0.02

float alpha = 0.0, k = 1;
float tx = 0.0, ty = 0.0;
float e_time = 0.0;
int MB_i = 0;
bool motionBlur = true;
bool freeze = false;

struct Star
{
	GLfloat color[3] = { 1.0, 1.0, 1.0 }; // white by default
	GLfloat pos[2];
	float size = 1.0;
	float time_shift = 0.0;
	float life = 0.5;
};

struct Planet;
typedef shared_ptr<Planet> PlanetPtr;
typedef vector<PlanetPtr> Planets;

struct Planet
{
	float angularSpeed;
	float r;
	float size;
	int res = 30; //resolution
	GLfloat color[3]; 
	Planets orbits; //planets that are orbiting around this planet
};

Planets planets;
PlanetPtr sun;
Star starList[numStars];


inline float randBetween(float low, float hi)
{
	return low + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (hi - low));
}

/*
Add one orbiting planet to the given planet
*/
void addOrbit(Planet &planet)
{
	PlanetPtr p = make_shared<Planet>();
	p->angularSpeed = randBetween(-M_PI, M_PI);
	p->r = randBetween(2 * planet.size, 5 * planet.size);
	p->size = randBetween(planet.size*0.5, planet.size*0.85);
	p->color[0] = randBetween(0.0, 0.5);
	p->color[1] = randBetween(0.0, 0.5);
	p->color[2] = randBetween(0.0, 0.5);
	planet.orbits.push_back(p);
	planets.push_back(p);
}


void reshape (int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-10, 10, -10, 10, -10, 10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void initStars(void)
{
	for (int i = 0; i < numStars; i++)
	{
		Star &s = starList[i];
		s.pos[0] = randBetween(-10, 10);
		s.pos[1] = randBetween(-10, 10);
		s.size = randBetween(0.05, 0.1);
		s.color[0] = randBetween(0.7, 1.0);
		s.color[1] = randBetween(0.7, 1.0);
		s.color[2] = randBetween(0.7, 1.0);
		s.time_shift = randBetween(0, M_PI);
		s.life = randBetween(0.5, 1.5);
	}
}

void initSun(void)
{
	sun = make_shared<Planet>();
	sun->color[0] = 1.0;
	sun->color[1] = 0.62;
	sun->color[2] = 0.19;
	sun->r = 0.0;
	sun->size = 2.0;
	addOrbit(*sun);
	addOrbit(*sun);
	addOrbit(*sun);
	planets.push_back(sun);
}

void init(void)
{
	srand(time(NULL));
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor (0.0, 0.0, 0.0, 0.0);
	glClearAccum (0.0, 0.0, 0.0, 0.0);
	glClear(GL_DEPTH_BUFFER_BIT);
	glShadeModel (GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	initStars();
	initSun();
}

void drawStar(const Star &s)
{
	glPushMatrix();
	glTranslatef(s.pos[0], s.pos[1], s.pos[2]);
	glColor4f(s.color[0], s.color[1], s.color[2], sin(s.life*e_time+s.time_shift));
	glBegin(GL_POLYGON);
	for (int i = 0; i < 10; i++)
	{
		float x = s.size*cos(i*2*M_PI/10);
		float y = s.size*sin(i*2*M_PI/10);
		glVertex2f(x, y);
	}
	glEnd();
	glPopMatrix();
}

void drawPlanet(const Planet &p, float t)
{
	float x = p.r*sin(p.angularSpeed*t);
	float y = p.r*cos(p.angularSpeed*t);

	glPushAttrib(GL_CURRENT_BIT);
	glColor4f(p.color[0], p.color[1], p.color[2], 1.0);

	// drawOrbitLine
	glBegin(GL_LINES);
	glVertex2f(0, 0);
	glVertex2f(x, y);
	glEnd();

	// drawPlanet
	glPushMatrix();
	glTranslatef(x, y, 0.0);

	for (Planets::const_iterator it = p.orbits.begin(); it != p.orbits.end(); ++it)
	{
		drawPlanet(**it, t);
	}

	glBegin(GL_POLYGON);
	for (int i = 0; i < p.res; i++)
	{
		float fade = FADE*i;
		glColor3f(p.color[0] + fade, p.color[1] + fade, p.color[2] + fade);
		float x = p.size*cos(i*2*M_PI/p.res);
		float y = p.size*sin(i*2*M_PI/p.res);
		glVertex2f(x, y);
	}
	glEnd();
	glPopMatrix();
	glPopAttrib();
}

void display(void)
{
	//glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//make color buffer = previous accumulated buffer * 0.5
	if (motionBlur)
		glAccum(GL_RETURN, 0.5f);

	glClear(GL_ACCUM_BUFFER_BIT);
	
	glPushMatrix();

	//controls transformation
	glScalef(k, k, k);	
	glTranslatef(tx, ty, 0);	
	glRotatef(alpha, 0, 0, 1);

	//drawStars
	for (int i = 0; i < numStars; i++)
	{
		drawStar(starList[i]);
	}

	glEnable(GL_LINE_SMOOTH);
	glLineWidth(2.0);

	drawPlanet(*sun, e_time);

	glPopMatrix();

	// store current buffer to accum buffer to be used later
	glAccum(GL_LOAD, 1.0f);

	// swap buffer: display the color buffer
	glutSwapBuffers();

	glFlush ();
}

void idle()
{
	//update animation here
	if (!freeze)
	{
		e_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	}
	glutPostRedisplay();	//after updating, draw the screen again
}

void onClick(int x, int y)
{
	int n = static_cast<int> (rand() % planets.size());
	addOrbit(*planets[n]); //add orbit to random planet
}

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		onClick(x, y);
	}
}

void keyboard (unsigned char key, int x, int y)
{
	//keys to control scaling - k
	//keys to control rotation - alpha
	//keys to control translation - tx, ty
	switch (key) {

		case 'a':
			alpha+=10;
			glutPostRedisplay();
		break;

		case 'd':
			alpha-=10;
			glutPostRedisplay();
		break;

		case 'q':
			k+=0.1;
			glutPostRedisplay();
		break;

		case 'e':
			if(k>0.1)
				k-=0.1;
			glutPostRedisplay();
		break;

		case 'z':
			tx-=0.1;
			glutPostRedisplay();
		break;

		case 'c':
			tx+=0.1;
			glutPostRedisplay();
		break;

		case 's':
			ty-=0.1;
			glutPostRedisplay();
		break;

		case 'w':
			ty+=0.1;
			glutPostRedisplay();
		break;

		case 'm':
			motionBlur = !motionBlur;
			glutPostRedisplay();
		break;
		
		case ' ':
			freeze = !freeze;
			glutPostRedisplay();
		break;

		case 27:
			exit(0);
		break;

		default:
		break;
	}
}

int main(int argc, char **argv)
{
	cout<<"CS3241 Lab 2\n\n";
	cout<<"+++++CONTROL BUTTONS+++++++\n\n";
	cout<<"Scale Up/Down: Q/E\n";
	cout<<"Rotate Clockwise/Counter-clockwise: A/D\n";
	cout<<"Move Up/Down: W/S\n";
	cout<<"Move Left/Right: Z/C\n";
	cout<<"Enable/Disable Motion Blur: M\n";
	cout<<"Freeze: Space\n";
	cout<<"ESC: Quit\n";

	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize (600, 600);
	glutInitWindowPosition (50, 50);
	glutCreateWindow (argv[0]);
	init ();
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);	
	glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutMainLoop();

	return 0;
}
