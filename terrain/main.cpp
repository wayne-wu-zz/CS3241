// CS3241Lab3.cpp : Defines the entry point for the console application.
//#include <cmath>
#include "math.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <string>
#include <time.h>
#include <unordered_map>
#include "FastNoise.h"

#ifdef _WIN32
#include <Windows.h>
#include "GL\freeglut.h"
#define M_PI 3.141592654
#elif __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/GLUT.h>
#endif

#define POLY_RES 50
#define INV_RES 1.0/POLY_RES
#define SPHERE_RES 20
#define INV_SPHERE_RES 1.0/SPHERE_RES
#define TERRAIN_COLS 80
#define TERRAIN_ROWS 50
#define TERRAIN_SCL 0.2
#define BRANCH_RES 5
#define TREES_SPACE_MAX 20

using namespace std;

// global variable
float m_Time;
bool m_Smooth = false;
bool m_Highlight = false;
bool m_Pause = false;
bool m_addTrees = false;
int m_treesSpace = TREES_SPACE_MAX; // 20 = no trees

GLfloat angle = 0;   /* in degrees */
GLfloat angle2 = 0;   /* in degrees */
GLfloat zoom = 1.0;
GLfloat field_of_view = 40.0;
GLfloat x_translation = 0.0;
GLfloat near_plane = 1.0;
GLfloat far_plane = 80.0;
GLfloat clipping_step = 0.5;

int mouseButton = 0;
int moving, startx, starty;

#define NO_OBJECT 4;
int current_object = 0;

// TERRAIN 
struct Point; 
struct Prim;
typedef shared_ptr<Point> PointPtr;
typedef vector<PointPtr> Points;
typedef shared_ptr<Prim> PrimPtr;
typedef vector<PrimPtr> Prims;

FastNoise noise;
PointPtr heightMap[TERRAIN_COLS][TERRAIN_ROWS];
Prims primitives;

float offset = 0;

struct Point {
	GLfloat pos[3]; // position
	GLfloat n[3];	// point normal
	vector<int> primsIdx;
	bool n_calc;
};

struct Prim {
	GLfloat n[3]; // face normal
	Points pts; // must be in the right order
};

////////////////////////////////////////////////////////////////////////
//UTILITIES

inline float randBetween(float low, float hi)
{
	return low + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (hi - low));
}

void cross3(GLdouble v1[3], GLdouble v2[3], GLdouble n[3])
{
	// Manual cross product
	n[0] = v1[1] * v2[2] - v1[2] * v2[1];
	n[1] = v1[2] * v2[0] - v1[0] * v2[2];
	n[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void cross3(GLfloat v1[3], GLfloat v2[3], GLfloat n[3])
{
	// Manual cross product
	n[0] = v1[1] * v2[2] - v1[2] * v2[1];
	n[1] = v1[2] * v2[0] - v1[0] * v2[2];
	n[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void normalize(GLfloat n[3])
{
	GLfloat mag = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
	n[0] /= mag;
	n[1] /= mag;
	n[2] /= mag;
}

////////////////////////////////////////////////////////////////////////////////////
// LIGHTING

void setupLighting()
{
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);

	// Lights, material properties
	GLfloat	ambientProperties[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	GLfloat	diffuseProperties[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat	specularProperties[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat lightPosition[] = { -100.0f, 100.0f, 100.0f, 1.0f };

	glClearDepth(1.0);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientProperties);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseProperties);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularProperties);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 0.0);

	// Default : lighting
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
}

////////////////////////////////////////////////////////////////////
// SPHERE 

void drawSphereVertex(double r, int res, int i, int j)
{
	float inv = 1.0 / res;
	GLdouble pos[3]; // x y z
	pos[0] = r*sin(i*M_PI*inv)*cos(j*M_PI*inv);
	pos[1] = r*cos(i*M_PI*inv)*cos(j*M_PI*inv);
	pos[2] = r*sin(j*M_PI*inv);

	// In this case since the sphere is centered at (0,0,0), 
	// the normal is simply (x-0, y-0, z-0) = (x, y, z).
	// Normalization is not needed since NORMALIZE is enabled
	if (m_Smooth)
		glNormal3dv(pos);
	glVertex3dv(pos);
}

void drawSphere(double r, int res)
{
	int i, j;
	for (i = 0; i < res; i++)
		for (j = 0; j<2*res; j++)
		{
			glBegin(GL_POLYGON);
			if (!m_Smooth)
				glNormal3d(sin((i + 0.5)*M_PI/res)*cos((j + 0.5)*M_PI/res), cos((i + 0.5)*M_PI/res)*cos((j + 0.5)*M_PI/res), sin((j + 0.5)*M_PI/res));
			drawSphereVertex(r, res, i, j);
			drawSphereVertex(r, res, i + 1, j);
			drawSphereVertex(r, res, i + 1, j + 1);
			drawSphereVertex(r, res, i, j + 1);
			glEnd();
		}
}

void drawSphere(void)
{
	// Sphere Color (Red-ish Matted Material)
	float no_mat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float mat_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float mat_diffuse[] = { 0.7f, 0.2f, 0.3f, 1.0f };
	float mat_emission[] = { 0.3f, 0.2f, 0.2f, 0.0f };
	float mat_specular[] = { 0.5f, 0.5f, 0.5f, 0.0f };
	float no_shininess = 0.0f;
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	if (m_Highlight)
	{
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);
	}
	else {
		glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
		glMaterialf(GL_FRONT, GL_SHININESS, no_shininess);
	}

	drawSphere(1, SPHERE_RES);
}

//////////////////////////////////////////////////////////////////////////////////////////
// TWISTED TORUS 

int color_offset = 0;

void getRainbowColor(int j, GLfloat color[4])
{
	// Get the specific color in the rainbow spectrum based on j

	float theta = j * 2.0 * M_PI * INV_RES;
	float range = 2 * M_PI / 6.0;
	float max = 0.9;
	float slope = max / range;

	if (theta >= 0 && theta < range)
	{
		color[0] = max;
		color[1] = slope*theta;
		color[2] = 0.0;
	}
	else if (theta >= range && theta < range * 2)
	{
		color[0] = -slope*theta;
		color[1] = max;
		color[2] = 0.0;
	}
	else if (theta >= 2 * range && theta < range * 3)
	{
		color[0] = 0.0;
		color[1] = max;
		color[2] = slope*theta;
	}
	else if (theta >= 3 * range && theta < range * 4)
	{
		color[0] = 0.0;
		color[1] = -slope*theta;
		color[2] = max;
	}
	else if (theta >= 4 * range && theta < range * 5)
	{
		color[0] = slope*theta;
		color[1] = 0.0;
		color[2] = max;
	}
	else if (theta >= 5 * range && theta <= range * 6)
	{
		color[0] = max;
		color[1] = 0.0;
		color[2] = -slope*theta;
	}

	color[3] = 1.0;
}

void twistedTorusCartesian(double c, double a, float i, float j, GLdouble pos[3])
{
	const float twistness = 5;
	const float amplitude = 0.2;
	const float turns = 2;

	double theta = j * 2 * M_PI * INV_RES;
	double phi = i * 2 * M_PI * INV_RES;

	double r = a + amplitude*sin(twistness * phi);
	double ang = phi + M_PI * 2 * sin(turns * theta);

	double tx = r * cos(ang) + c;
	pos[0] = tx * cos(theta);
	pos[1] = tx * sin(theta);
	pos[2] = r * sin(ang);
}

void twistedTorusNormal(double c, double a, int i, int j, GLdouble pos[3], GLdouble n[3])
{
	// Calculates the normal of the twisted torus
	// using finite differrence approximation

	float h = 0.001;
	float h_inv = 1.0 / h;

	GLdouble pos_xf[3], pos_xb[3], pos_yf[3], pos_yb[3];
	twistedTorusCartesian(c, a, i + h, j, pos_xf);
	twistedTorusCartesian(c, a, i - h, j, pos_xb);
	twistedTorusCartesian(c, a, i, j + h, pos_yf);
	twistedTorusCartesian(c, a, i, j - h, pos_yb);

	GLdouble dx[3], dy[3];
	for (int it = 0; it < 3; it++)
	{
		dy[it] = (pos_xf[it] - pos_xb[it])*h_inv*0.5;
		dx[it] = (pos_yf[it] - pos_yb[it])*h_inv*0.5;
	}

	cross3(dx, dy, n);
}

void drawTwistedTorusVertex(double c, double a, int i, int j)
{
	GLdouble pos[3], n[3];
	twistedTorusCartesian(c, a, i, j, pos);

	float mat[4];
	getRainbowColor((j+color_offset)%POLY_RES, mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat);
	
	if (m_Smooth)
	{
		// Smooth shading: each vertex has a calculated normal
		twistedTorusNormal(c, a, i, j, pos, n);
		glNormal3dv(n);
	}
	glVertex3dv(pos);
}

void drawTwistedTorus(double c, double a)
{
	float no_mat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float mat_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float mat_diffuse[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	float mat_emission[] = { 0.2f, 0.2f, 0.2f, 0.0f };
	float mat_specular[] = { 0.5f, 0.5f, 0.5f, 0.0f };
	float no_shininess = 0.0f;
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	if (m_Highlight)
	{
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);
	}
	else {
		glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
		glMaterialf(GL_FRONT, GL_SHININESS, no_shininess);
	}

	for (int i = 0; i < POLY_RES; i++)
	{
		for (int j = 0; j < POLY_RES; j++)
		{
			glBegin(GL_POLYGON);
			
			if (!m_Smooth)
			{
				// Flat shading based on center normal
				GLdouble pos[3], n[3];
				twistedTorusCartesian(c, a, i + 0.5, j + 0.5, pos);
				twistedTorusNormal(c, a, i + 0.5, j + 0.5, pos, n);
				glNormal3dv(n);
			}

			drawTwistedTorusVertex(c, a, i, j);
			drawTwistedTorusVertex(c, a, i + 1, j);
			drawTwistedTorusVertex(c, a, i + 1, j + 1);
			drawTwistedTorusVertex(c, a, i, j + 1);

			glEnd();
		}
	}

	color_offset++;
}

////////////////////////////////////////////////////////////////////////////////////////
// TREE

// Tree Object
struct Tree {
	string rule;
	float y_angle;
	float z_angle;
	float height;
	GLfloat color[4];
	GLfloat pos[3];
	GLfloat start_pos[3];
	float startTime;

	Tree(string& sentence) {
		rule = sentence;
		y_angle = randBetween(20, 40);
		z_angle = randBetween(10, 120);
		height = randBetween(0.7, 1);
	}
};

typedef unordered_map<char, string> RulesMap;

RulesMap rules = {{'F', "FF+[+F-F-F]-[-F+F+F]"}};
string axiom = "F";
string tree_sentence;
vector<Tree> trees;

void drawCylinderVertex(float r, float l, float i, float j)
{
	float theta = i * 2 * M_PI / BRANCH_RES;
	float z = j * l / BRANCH_RES;
	float x = r*cos(theta);
	float y = r*sin(theta);
	if (m_Smooth)
		glNormal3d(x, y, 0);
	glVertex3d(x, y, z);
}

void drawCylinder(float r, float z)
{
	for (int i = 0; i < BRANCH_RES; i++)
	{
		for (int j = 0; j < BRANCH_RES; j++)
		{
			glBegin(GL_POLYGON);
			if (!m_Smooth)
			{
				float theta = (i + 0.5) * 2 * M_PI / BRANCH_RES;
				glNormal3d(r*cos(theta), r*sin(theta), 0);
			}
			drawCylinderVertex(r, z, i, j);
			drawCylinderVertex(r, z, i + 1, j);
			drawCylinderVertex(r, z, i + 1, j + 1);
			drawCylinderVertex(r, z, i, j + 1);
			glEnd();
		}
	}
}

void drawBranch(float r, float z)
{
	// Tree Material (Brown Color)
	float no_mat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float mat_ambient[] = { 0.3f, 0.2f, 0.1f, 1.0f };
	float mat_diffuse[] = { 0.55f, 0.27f, 0.1f, 1.0f };
	float mat_emission[] = { 0.2f, 0.2f, 0.2f, 0.0f };
	float mat_specular[] = { 0.2f, 0.2f, 0.2f, 0.0f };
	float no_shininess = 0.0f;
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	if (m_Highlight)
	{
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialf(GL_FRONT, GL_SHININESS, 30.0f);
	}
	else {
		glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
		glMaterialf(GL_FRONT, GL_SHININESS, no_shininess);
	}

	//draw stick
	drawCylinder(r, z);
}

void drawLeavesBunch(float size)
{
	// Leaf Material (Green Color)
	float no_mat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float mat_ambient[] = { 0.1f, 0.3f, 0.1f, 1.0f };
	float mat_diffuse[] = { 0.2f, 0.5f, 0.1f, 1.0f };
	float mat_specular[] = { 0.2f, 0.3f, 0.2f, 0.0f };
	float no_shininess = 0.0f;
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	if (m_Highlight)
	{
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialf(GL_FRONT, GL_SHININESS, 30.0f);
	}
	else {
		glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
		glMaterialf(GL_FRONT, GL_SHININESS, no_shininess);
	}
	drawSphere(size, 6);
}

void lsystemGenerate(string& sentence, int iteration) {
	if (iteration == 0)
		return;

	string newSentence;
	for (int i = 0; i < sentence.length(); i++)
	{
		RulesMap::iterator it = rules.find(sentence[i]);
		if (it == rules.end())
			newSentence.push_back(sentence[i]);
		else
			newSentence.append(it->second);
	}
	sentence = newSentence;
	lsystemGenerate(sentence, iteration - 1);
}

void lsystemDraw(Tree& tree, float size) {
	float r = 0.1;
	float z = tree.height;
	
	glPushMatrix();
	glTranslatef(tree.pos[0], tree.pos[1], tree.pos[2]);
	glScalef(size, size, size);
	for (int i = 0; i < tree.rule.length(); i++)
	{
		char ch = tree.rule[i];
		switch (ch) {
			case 'F':
				if (r < 0.01)
					break;
				drawBranch(r, z);
				glTranslatef(0, 0, z);
				glScalef(0.8, 0.8, 0.8);
				break;
			case '+':
				glRotatef(tree.y_angle, 0, 1, 0);
				glRotatef(tree.z_angle, 0, 0, 1);
				break;
			case '-':
				glRotatef(-tree.y_angle, 0, 1, 0);
				glRotatef(tree.z_angle, 0, 0, 1);
				break;
			case '[':
				glPushMatrix();
				break;
			case ']':
				drawLeavesBunch(2.2);
				glPopMatrix();
				break;
			default: 
				break;
		}
	}
	glPopMatrix();
}

void initTrees(void) {
	tree_sentence = axiom;
	lsystemGenerate(tree_sentence, 2);

	for (int i = -1; i < 2; i++)
	{
		for (int j = -1; j < 2; j++)
		{
			Tree t(tree_sentence);
			t.pos[0] = i*2;
			t.pos[1] = j*2; 
			t.pos[2] = 0;
			trees.push_back(t);
		}
	}
}

void drawTree(void) {
	for (vector<Tree>::iterator it = trees.begin(); it != trees.end(); ++it)
	{
		lsystemDraw(*it, 1.0);
	}
}

/////////////////////////////////////////////////////////////////////////
//TERRAIN
vector<Tree> terrain_trees;

int last_timestep;
int current_timestep;

void recalculateNormal(Point& pt)
{
	// Calculate the point normal by averaging
	// all the surface normals connecting to the points

	if (pt.n_calc)
		return;

	fill(begin(pt.n), end(pt.n), 0);
	for (vector<int>::iterator it = pt.primsIdx.begin(); it != pt.primsIdx.end(); ++it)
	{
		PrimPtr prim = primitives[*it];
		pt.n[0] += prim->n[0];
		pt.n[1] += prim->n[1];
		pt.n[2] += prim->n[2];
	}
	//normalize(pt.n); not needed
	pt.n_calc = true;
}

void recalculateNormal(Prim& prim)
{
	// Calculate the surface normal of the primitive 
	// based on the points of the face.

	if (prim.pts.size() < 3)
		return;

	const GLfloat* p1 = prim.pts[0]->pos;
	const GLfloat* p2 = prim.pts[1]->pos;
	const GLfloat* p3 = prim.pts[2]->pos;

	if (!p1 || !p2 || !p3)
		return;

	GLfloat v1[3], v2[3];
	v1[0] = p2[0] - p1[0];
	v1[1] = p2[1] - p1[1];
	v1[2] = p2[2] - p1[2];
	v2[0] = p3[0] - p1[0];
	v2[1] = p3[1] - p1[1];
	v2[2] = p3[2] - p1[2];

	cross3(v1, v2, prim.n);
	normalize(prim.n);
}

void initTerrain(void)
{
	// Initialize the polygonal mesh for terrain

	noise.SetNoiseType(FastNoise::Perlin);
	noise.SetFrequency(0.08);

	// Initialize pointers
	for (int i = 0; i < TERRAIN_ROWS; i++)
	{
		for (int j = 0; j < TERRAIN_COLS; j++)
			heightMap[j][i] = make_shared<Point>();
	}

	// Initialize geometry data
	for (int i = 0; i < TERRAIN_ROWS; i++)
	{
		for (int j = 0; j < TERRAIN_COLS; j++)
		{
			PointPtr pt1 = heightMap[j][i];
			pt1->pos[0] = j*TERRAIN_SCL; // fixed
			pt1->pos[1] = i*TERRAIN_SCL; // fixed
			pt1->pos[2] = noise.GetNoise(j, i) * 1.5;

			if (j + 1 >= TERRAIN_COLS || i + 1 >= TERRAIN_ROWS)
				continue;

			PointPtr pt2 = heightMap[j + 1][i];
			PointPtr pt3 = heightMap[j + 1][i + 1];
			PointPtr pt4 = heightMap[j][i + 1];

			PrimPtr prim1 = make_shared<Prim>();
			prim1->pts.push_back(pt1);
			prim1->pts.push_back(pt2);
			prim1->pts.push_back(pt4);
			primitives.push_back(prim1);

			int primIdx = primitives.size() - 1; 
			pt1->primsIdx.push_back(primIdx);
			pt2->primsIdx.push_back(primIdx);
			pt4->primsIdx.push_back(primIdx);

			PrimPtr prim2 = make_shared<Prim>();
			prim2->pts.push_back(pt2);
			prim2->pts.push_back(pt3);
			prim2->pts.push_back(pt4);
			primitives.push_back(prim2);
			primIdx = primitives.size() - 1;
			pt2->primsIdx.push_back(primIdx);
			pt3->primsIdx.push_back(primIdx);
			pt4->primsIdx.push_back(primIdx);
		}
	}

	cout << "Number of Primitives: " << primitives.size() << endl;
}

void drawTerrain(void)
{	
	current_timestep = static_cast<int>(m_Time * 30);


	if (!m_Pause)
	{
		for (int i = 0; i < TERRAIN_ROWS; i++)
		{
			for (int j = 0; j < TERRAIN_COLS; j++)
			{
				// Get the new z value for each point from Perlin Noise
				PointPtr pt = heightMap[j][i];
				pt->pos[2] = noise.GetNoise(j, i + current_timestep) * 1.5;
				pt->n_calc = false;
			}
		}

		if (m_Smooth)
		{
			// Recalculate the surface normal for all prims
			for (Prims::iterator it = primitives.begin(); it != primitives.end(); ++it)
				recalculateNormal(**it);
		}

		int delay = current_timestep - last_timestep;
		m_addTrees = m_treesSpace < 20;
		if (m_addTrees && delay >= m_treesSpace)
		{
			// Add trees
			int startIdx = randBetween(max(TERRAIN_ROWS - (delay + 1), 0), m_treesSpace);
			for (int i = startIdx; i < TERRAIN_ROWS; i += m_treesSpace)
			{
				for (int j = randBetween(0, m_treesSpace); j < TERRAIN_COLS; j += m_treesSpace)
				{
					PointPtr pt = heightMap[j][i];
					float val = noise.GetNoise(j, i + current_timestep);
					if (val > 0)
					{
						Tree t(tree_sentence);
						t.pos[0] = t.start_pos[0] = pt->pos[0];
						t.pos[1] = t.start_pos[1] = pt->pos[1];
						t.pos[2] = t.start_pos[2] = pt->pos[2];
						t.startTime = m_Time;
						terrain_trees.push_back(t);
					}
				}
				delay -= m_treesSpace;
			}
			last_timestep = current_timestep - delay; // leftover delay
		}
	}
	
	// Terrain Material (Light Green Color)
	float no_mat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float mat_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float mat_diffuse[] = { 0.68f, 0.9f, 0.184f, 1.0f };
	float mat_specular[] = { 0.2f, 0.2f, 0.2f, 0.0f };
	float no_shininess = 0.0f;
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	if (m_Highlight)
	{
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialf(GL_FRONT, GL_SHININESS, 30.0f);
	}
	else {
		glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
		glMaterialf(GL_FRONT, GL_SHININESS, no_shininess);
	}

	glPushMatrix();
	glTranslatef(-TERRAIN_COLS*TERRAIN_SCL*0.5, -TERRAIN_ROWS*TERRAIN_SCL*0.5, 0);

	// Render terrain
	for (int y = 0; y < TERRAIN_ROWS-1; y++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < TERRAIN_COLS-1; x++)
		{
			PointPtr pt = heightMap[x][y];
			PointPtr adj_pt = heightMap[x][y + 1];

			if (m_Smooth)
			{
				recalculateNormal(*pt);
				recalculateNormal(*adj_pt); //TODO: Redundant computation
				glNormal3fv(pt->n);
				glVertex3fv(pt->pos);
				glNormal3fv(adj_pt->n);
				glVertex3fv(adj_pt->pos);
			}
			else
			{
				GLdouble v1[3], v2[3], n[3];
				v1[0] = 0;
				v1[1] = TERRAIN_SCL;
				v1[2] = heightMap[x][y + 1]->pos[2] - pt->pos[2];
				v2[0] = TERRAIN_SCL;
				v2[1] = 0;
				v2[2] = heightMap[x + 1][y]->pos[2] - pt->pos[2];
				cross3(v2, v1, n);
				glNormal3dv(n);
				glVertex3fv(pt->pos);
				glVertex3fv(adj_pt->pos);
			}
		}
		glEnd();
	}

	// Render Trees
	if (m_addTrees)
		for (vector<Tree>::iterator it = terrain_trees.begin(); it != terrain_trees.end();)
		{
			it->pos[1] = it->start_pos[1] - static_cast<int>((m_Time-it->startTime)*30)*TERRAIN_SCL;
			if (it->pos[1] < 0)
			{
				it = terrain_trees.erase(it);
				continue;
			}
			lsystemDraw(*it, 0.5);
			++it;
		}

	glPopMatrix();
}

////////////////////////////////////////////////////////////////////////
//OPENGL CALLBACKS

void display(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(field_of_view, 1.0, near_plane, far_plane);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glRotatef(angle2, 1.0, 0.0, 0.0);
	glRotatef(angle, 0.0, 1.0, 0.0);
	glScalef(zoom, zoom, zoom);
	
	switch (current_object) {
	case 0:
		drawSphere();
		break;
	case 1:
		drawTwistedTorus(2, 0.5);
		break;
	case 2:
		drawTree();
		break;
	case 3:
		drawTerrain();
		break;
	default:
		break;
	};
	glPopMatrix();
	glutSwapBuffers();
}

void resetCamera(){
	zoom = 1;
	angle = 0;  
	angle2 = 0;  
	field_of_view = 40;
	x_translation = 0;
	near_plane = 1.0;
	far_plane = 80.0;
	
	//NOTE: Projection is reset in the display function,
	//since it is run on every draw call

	// Reset camera position and direction
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(8, -8, 8, 0, 0, 0, 0, 0, 1);
	return;
}

void setCameraBestAngle() {
	zoom = 1;
	angle = 0;
	angle2 = 0;
	field_of_view = 40;
	near_plane = 1.0;
	x_translation = 0;
	far_plane = 80.0;

	// Set camera position and direction for optimal viewing 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, -8, 4, 0, 3, 0, 0, 0, 1);
	return;
}

void keyboard(unsigned char key, int x, int y)
{//add additional commands here to change Field of View and movement
	switch (key) {
	case 'p':
	case 'P':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 'w':
	case 'W':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 'v':
	case 'V':
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	case 's':
	case 'S':
		m_Smooth = !m_Smooth;
		break;
	case 'h':
	case 'H':
		m_Highlight = !m_Highlight;
		break;

	case 'n':
		near_plane = max(near_plane - clipping_step / zoom, 1);
		break;

	case 'N':
		near_plane = min(near_plane + clipping_step / zoom, far_plane);
		break;

	case 'f':
		far_plane = max(far_plane - clipping_step / zoom, near_plane);
		break;

	case 'F':
		far_plane += clipping_step / zoom;
		break;

	case 'o':
		field_of_view -= 2;
		break;

	case 'O':
		field_of_view += 2;
		break;

	case 'r':
		resetCamera();
		break;

	case 'R':
		setCameraBestAngle();
		break;

	case 't':
		// Reduce number of trees
		m_treesSpace = max(4, m_treesSpace - 2);
		break;

	case 'T':
		m_treesSpace = min(20, m_treesSpace + 2);
		break;

	case '1':
	case '2':
	case '3':
	case '4':
		current_object = key - '1';
		break;

	case 27:
		exit(0);
		break;

	default:
		break;
	}

	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN) {
		mouseButton = button;
		moving = 1;
		startx = x;
		starty = y;
	}
	if (state == GLUT_UP) {
		mouseButton = button;
		moving = 0;
	}
}

void motion(int x, int y)
{
	if (moving) {
		if (mouseButton == GLUT_LEFT_BUTTON)
		{
			angle = angle + (x - startx);
			angle2 = angle2 + (y - starty);
		}
		else zoom += ((y - starty)*0.01);
		startx = x;
		starty = y;
		glutPostRedisplay();
	}

}

void idle()
{
	m_Time = glutGet(GLUT_ELAPSED_TIME)/1000.0;
	glutPostRedisplay();
}


int main(int argc, char **argv)
{
	cout << "CS3241 Lab 3" << endl << endl;

	cout << "1-4: Draw different objects" << endl;
	cout << "S: Toggle Smooth Shading" << endl;
	cout << "H: Toggle Highlight" << endl;
	cout << "W: Draw Wireframe" << endl;
	cout << "P: Draw Polygon" << endl;
	cout << "V: Draw Vertices" << endl;
	cout << "n, N: Reduce or increase the distance of the near plane from the camera" << endl;
	cout << "f, F: Reduce or increase the distance of the far plane from the camera" << endl;
	cout << "o, O: Reduce or increase the distance of the povy plane from the camera" << endl;
	cout << "r: Reset camera to the initial parameters when the program starts" << endl;
	cout << "R: Change camera to another setting that is has the best viewing angle for your object" << endl;
	cout << endl;
	cout << "======Special Control======" << endl;
	cout << "t, T: Reduce or increase the number of trees generated" << endl;
	cout << "============================" << endl;
	cout << "ESC: Quit" << endl << endl;


	cout << "Left mouse click and drag: rotate the object" << endl;
	cout << "Right mouse click and drag: zooming" << endl;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("CS3241 Assignment 3");
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glutDisplayFunc(display);
	glMatrixMode(GL_PROJECTION);
	glutIdleFunc(idle);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	setupLighting();
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	srand(time(NULL));
	initTrees();
	initTerrain();

	resetCamera();

	glutMainLoop();

	return 0;
}