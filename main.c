
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glut.h>

#define VIEWHEIGHT 500.0
#define VIEWWIDTH 500.0
#define OFFHEIGHT 100
#define OFFWIDTH 100
#define CENTERX 350
#define CENTERY 350
//Emacs Lisp seems to be accurate only up to about 16 digits :(
//(insert (format "#define SQRT2 %.16f" (sqrt 2)))
#define SQRT2 1.4142135623730951
#define SQRTHALF 0.7071067811865476

bool circleDefined = false;

struct pt {
	GLint x;
	GLint y;
} linestart, lineend, wnddim;

struct {
	struct pt *points;
	int x, y;
	unsigned count;
} circle;

enum Region {
	CENTER = 0,
	LEFT = 1,
	RIGHT = 2,
	TOP = 8,
	BOTTOM = 4
};

bool ptCompare(struct pt lhs, struct pt rhs)
{
	if(lhs.x != rhs.x || lhs.y != rhs.y)
		return false;
	return true;
}

enum Region pointRegion(struct pt point);
void drawView(void);
void calcCircle(float radius);
void drawCircle();
void drawLine(void);
bool clipLine(struct pt *p1, struct pt *p2);
bool clipLine2(struct pt *p1, struct pt *p2);
void display(void);
void resize(GLsizei width, GLsizei height);
void mpress(int btn, int state, int x, int y);

int interpolateX(struct pt p1, struct pt p2, int p1x);
int interpolateY(struct pt p1, struct pt p2, int p1y);

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	drawView();
	glColor3f(1.0f, 0.0f, 0.0f);
	drawCircle();
	glFlush();
 	glutSwapBuffers();
}

enum Region pointRegion(struct pt point)
{
	enum Region reg = CENTER;
	if(point.x < OFFWIDTH)
		reg = LEFT;
	else if(point.x > OFFWIDTH + VIEWWIDTH)
		reg = RIGHT;
	if(point.y < OFFHEIGHT)
		reg |= BOTTOM;
	else if(point.y > OFFHEIGHT + VIEWHEIGHT)
		reg |= TOP;
	return reg;
}

float inCircle(float x, float y, float rad)
{
	return x * x + y * y - rad * rad;
}

void calcCircle(float radius)
{
	/* The number of points required is
	 * calculated with a right triangle.
	 * From the center to any of the diagonal
	 * points which are pi/4 of an arc away from
	 * a horizontal or vertical point,
	 * the x and y values are sqrt(2)/2 * r
	 * from the center.
	 * Since x always increments,
	 * this gives us a good approximation of
	 * the number of points needed to be filled.
	 * For safety, add a MAGIC NUMBER of extra points */
	float numPoints = radius * SQRT2 * 4 + 10;
	if(!(int)numPoints)
		return;
	if(circle.points) {
		free(circle.points);
		circle.points = NULL;
	}
	circle.count = (int)ceil(numPoints);
	circle.points = malloc(sizeof(struct pt[circle.count]));
	memset(circle.points, 0, sizeof(struct pt[circle.count]));
	int qArc = round(numPoints / 4),
		hArc = round(numPoints / 2),
		fArc = circle.count - 1;
	printf("circle.count: %d\n"
				 "qArc: %d\n"
				 "hArc: %d\n"
				 "fArc: %d\n\n",
				 circle.count,
				 qArc,
				 hArc,
				 fArc);
	int i = 0;
	float curx = 0, cury = radius;
	struct pt next;
	/* Calculate the circle points */
	while(i <= qArc / 2) {
		/* Start with the top right of the circle */
		next.x = curx;
		next.y = cury;
		circle.points[i] = next;
		/* The top left of the circle */
		next.x = -curx;
		next.y = cury;
		circle.points[fArc - i] = next;
		/* The bottom right */
		next.x = curx;
		next.y = -cury;
		circle.points[hArc - i] = next;
		/* The bottom left */
		next.x = -curx;
		next.y = -cury;
		circle.points[hArc + i] = next;

		/* The right top */
		next.x = cury;
		next.y = curx;
		circle.points[qArc - i] = next;
		/* The left top */
		next.x = -cury;
		next.y = curx;
		circle.points[hArc + qArc + i] = next;
		/* The right bottom */
		next.x = cury;
		next.y = -curx;
		circle.points[qArc + i] = next;
		/* The left bottom */
		next.x = -cury;
		next.y = -curx;
		circle.points[hArc + qArc - i] = next;

		/* Calculate the coordinates used for the next part of the circle */
		float old = inCircle(curx++, cury - 0.5, radius);
		if(old >= 1) {
			cury--;
		}
		else if(old >= 0) {
			cury -= 0.5;
		}
		i++;
	}
}

void drawCircle()
{
	if(circle.points) {
		glBegin(GL_POINTS);
		/* Draw the center of the circle */
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex2i(circle.x, circle.y);
		/* Draw the circle itself */
		glColor3f(0.0f, 0.0f, 1.0f);
		int i;
		for(i = 0; i < circle.count; i++) {
			/* We need to clip the circle's line,
			 * but we don't want to modify the circle
			 * itself. */
			struct pt tmp[2];
			tmp[0] = circle.points[i];
			if(i + 1 < circle.count)
				tmp[1] = circle.points[i + 1];
			else
				tmp[1] = circle.points[i - 1];
			tmp[0].x += circle.x;
			tmp[0].y += circle.y;
			tmp[1].x += circle.x;
			tmp[1].y += circle.y;
			if(clipLine2(&tmp[0], &tmp[1])) {
				glVertex2i(tmp[0].x,
									 tmp[0].y);
			}
		}
		glEnd();
	}
}

bool clipLine2(struct pt *p1, struct pt *p2)
{
	enum Region reg1 = pointRegion(*p1),
		reg2 = pointRegion(*p2);
	if(reg1 & reg2) {
		return false;
	}
	if(!reg1 && !reg2)
		return true;
	if(p1->x < OFFWIDTH) {
		p1->y = interpolateX(*p1, *p2, OFFWIDTH);
		p1->x = OFFWIDTH;
	}
	else if(p1->x > OFFWIDTH + VIEWWIDTH) {
		p1->y = interpolateX(*p1, *p2, OFFWIDTH + VIEWWIDTH);
		p1->x = OFFWIDTH + VIEWWIDTH;
	}
	if(p1->y < OFFHEIGHT) {
		p1->x = interpolateY(*p1, *p2, OFFHEIGHT);
		p1->y = OFFHEIGHT;
	}
	else if(p1->y > OFFHEIGHT + VIEWHEIGHT) {
		p1->x = interpolateY(*p1, *p2, OFFHEIGHT + VIEWHEIGHT);
		p1->y = OFFHEIGHT + VIEWHEIGHT;
	}

	if(p2->x < OFFWIDTH) {
		p2->y = interpolateX(*p1, *p2, OFFWIDTH);
		p2->x = OFFWIDTH;
	}
	else if(p2->x > OFFWIDTH + VIEWWIDTH) {
		p2->y = interpolateX(*p1, *p2, OFFWIDTH + VIEWWIDTH);
		p2->x = OFFWIDTH + VIEWWIDTH;
	}
	if(p2->y < OFFHEIGHT) {
		p2->x = interpolateY(*p1, *p2, OFFHEIGHT);
		p2->y = OFFHEIGHT;
	}
	else if(p2->y > OFFHEIGHT + VIEWHEIGHT) {
		p2->x = interpolateY(*p1, *p2, OFFHEIGHT + VIEWHEIGHT);
		p2->y = OFFHEIGHT + VIEWHEIGHT;
	}
	reg1 = pointRegion(*p1);
	reg2 = pointRegion(*p2);
	if(reg1 & reg2) {
		return false;
	}
	return true;
}

int interpolateX(struct pt p1, struct pt p2, int newX)
{
	float dx = p1.x - p2.x,
		dy = p1.y - p2.y;
	return (int)dy / dx * (newX - p2.x) + p2.y;
}

int interpolateY(struct pt p1, struct pt p2, int newY)
{
	float dx = p1.x - p2.x,
		dy = p1.y - p2.y;
	return (int)dx / dy * (newY - p2.y) + p2.x;
}

void mpress(int btn, int state, int x, int y)
{
	y = wnddim.y - y;
	if(x < OFFWIDTH || x > OFFWIDTH + VIEWWIDTH ||
		 y < OFFHEIGHT || y > OFFHEIGHT + VIEWHEIGHT)	{
		printf("Error:\n\tx: %d\n\ty: %d\n", x, y);
	}
	else {
		if(btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			circle.x = x;
			circle.y = y;
			circleDefined = true;
		}
		else if(btn == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
			if(circleDefined) {
				float tmpx = x - circle.x,
					tmpy = y - circle.y;
				float radius = sqrt(tmpx * tmpx + tmpy * tmpy);
				calcCircle(radius);
				glutPostRedisplay();
				circleDefined = false;
			}
			else {
				if(circle.points) {
					free(circle.points);
					circle.points = NULL;
					circle.count = 0;
				}
				glutPostRedisplay();
			}
		}
	}
}

void drawView(void)
{
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_POINTS);
	struct pt pos;
	for(pos.x = 100; pos.x < 600; pos.x++)
		for(pos.y = 100; pos.y < 600; pos.y++) {
			glVertex2i(pos.x, pos.y);
		}
	glEnd();
}

void resize(GLsizei width, GLsizei height)
{
	wnddim.x = width;
	wnddim.y = height;
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, 0.0,
					height, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
}

void keypress(unsigned char key, int x, int y)
{
	key = tolower(key);
	switch(key) {
	case 'q':
		exit(0);
	}
}

int main(int argc, char **argv)
{
	memset(&linestart, 0, sizeof(linestart));
	memset(&lineend, 0, sizeof(lineend));
	memset(&wnddim, 0, sizeof(wnddim));
	memset(&circle, 0, sizeof(circle));
	calcCircle(1000);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 10);
	glutInitWindowSize(700, 700);
	glutCreateWindow("Program 1");
	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutMouseFunc(mpress);
	glutKeyboardFunc(keypress);
	glutMainLoop();
  return 0;
}
