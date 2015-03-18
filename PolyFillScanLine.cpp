/*
@author:	TwinklingStar
@date:		2013/11/13
*/
#include <stdlib.h>
#include <GL/glut.h> 
#include <stdio.h>
#include <algorithm>
#include <math.h>


typedef unsigned long	DWORD;
typedef unsigned short	WORD;
typedef unsigned char	BYTE;
#define EDGE_TABLE_SIZE		1025

#define  RGB(r,g,b) ((DWORD)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
int g_inHeight;
int g_inWidth;
int g_inPixelArray[] = {1,2,4,8,16};
int g_inPixelIndx = 0;
unsigned char	g_ptrScreen[1080*1920*3];


typedef struct _Edge
{
	double	dbX;
	double	dbDelta;
	int		inMaxY;
	_Edge*	ptrNext;
}Edge;

Edge*	g_ptrEdgeTable[EDGE_TABLE_SIZE];
Edge*	g_ptrAELHead;

int		g_inMinY =	0x7fffffff;
int		g_inMaxY = -0x7fffffff;

typedef struct
{
	int		x;
	int		y;
}Vector;

#define GetMin(x,y) ((x)>(y)?(y):(x))
#define GetMax(x,y) ((x)>(y)?(x):(y))


void allocEdges()
{
	int i;
	for( i=0 ; i<EDGE_TABLE_SIZE ; i++ )
	{
		g_ptrEdgeTable[i] = new Edge;
		g_ptrEdgeTable[i]->ptrNext = NULL;
	}
	g_ptrAELHead	=	new Edge;
	g_ptrAELHead->ptrNext = NULL;
}

void deallocEdges()
{
	delete g_ptrAELHead;
	g_ptrAELHead	=	NULL;
	int i;
	for( i=0 ; i<EDGE_TABLE_SIZE ; i++ )
	{
		delete g_ptrEdgeTable[i];
		g_ptrEdgeTable[i] = NULL;
	}
}

#define  eps		0.00001

int ceilPixel(double xLeft)
{
	return (int)(xLeft + 1.0 - eps);
}

int floorPixel(double xRight)
{
	return (int)(xRight - eps);
}

void setPixel(int x,int y,DWORD color)
{
	int iStart = y*g_inPixelArray[g_inPixelIndx];
	int jStart = x*g_inPixelArray[g_inPixelIndx];
	int iEnd = iStart + g_inPixelArray[g_inPixelIndx];
	int jEnd = jStart + g_inPixelArray[g_inPixelIndx];
	if( iEnd>g_inHeight || jEnd>g_inWidth )
		return ;
	int i , j , lines;
	int w = (g_inWidth / g_inPixelArray[g_inPixelIndx]) * g_inPixelArray[g_inPixelIndx];
	for(i=iStart ; i<iEnd ; i++ )
	{
		lines = i*w*3;
		for( j=jStart; j<jEnd ; j++ )
		{
			*(g_ptrScreen + lines + j*3)	 = (color>>0)&0xff;
			*(g_ptrScreen + lines + j*3 + 1) = (color>>8)&0xff;
			*(g_ptrScreen + lines + j*3 + 2) = (color>>16)&0xff;
		}
	}
}

void insertEdgeList(Edge* ptrHead , Edge* ptrEdge)
{

	Edge* current = ptrHead ,*next = ptrHead->ptrNext;
	while( next && next->dbX > ptrEdge->dbX )
	{
		current = next;
		next = current->ptrNext;
	}
	current->ptrNext = ptrEdge;
	ptrEdge->ptrNext = next;

}

void insertAEL(Edge* ptrHeadList, Edge* ptrInsertList)
{
	if( !ptrInsertList->ptrNext )
		return ;

	Edge* curInsert = ptrInsertList->ptrNext,*nextInsert = NULL;
	Edge* current = ptrHeadList ,*next = current->ptrNext;
	while( curInsert!=NULL )
	{
		nextInsert = curInsert->ptrNext;
		while( next && next->dbX < curInsert->dbX )
		{
			current = next;
			next = current->ptrNext;
		}
		current->ptrNext = curInsert;
		curInsert->ptrNext = next;
		next = curInsert;
		curInsert = nextInsert;
	}
}

void initEdgeTable(Vector* ptrPolygon, int inNumPoly )
{
	int current = inNumPoly -1 , next = 0;
	Edge* edge = NULL;
	for(  ; next<inNumPoly ; current = next , next++ )
	{
		//不处理水平边
		if( ptrPolygon[current].y == ptrPolygon[next].y )
			continue;
		int minY , maxY , x;
		if( ptrPolygon[current].y > ptrPolygon[next].y)
		{
			maxY = ptrPolygon[current].y;
			minY = ptrPolygon[next].y;
			x	 = ptrPolygon[next].x;
		}
		else
		{
			maxY = ptrPolygon[next].y;
			minY = ptrPolygon[current].y;
			x	 = ptrPolygon[current].x;
		}
		if( minY<g_inMinY )
			g_inMinY = minY;
		if( maxY>g_inMaxY )
			g_inMaxY = maxY;

		edge = new Edge;
		edge->ptrNext	= NULL;
		edge->dbX		= x;
		edge->inMaxY	= maxY;
		edge->dbDelta	= (double)(ptrPolygon[current].x - ptrPolygon[next].x) / (double)(ptrPolygon[current].y - ptrPolygon[next].y);

		insertEdgeList(g_ptrEdgeTable[minY] , edge);
	}
}

void updateAEL(Edge* ptrHead,int inCurrentY)
{
	Edge* remove = NULL;
	Edge* current = ptrHead ,*next = ptrHead->ptrNext;
	while( next )
	{
		if( next->inMaxY<=inCurrentY )
		{
			remove = next;
			next = next->ptrNext;
			current->ptrNext = next;
			delete remove;
		}
		else
		{
			current = next;
			current->dbX += current->dbDelta;
			next = current->ptrNext;
		}
	}
}

void fillAELScanLine(Edge* ptrHead,int inY , DWORD inColor)
{
	Edge* current = NULL , *next = ptrHead->ptrNext;

	do
	{
		current = next;
		next	= current->ptrNext;
		int x		= ceilPixel(current->dbX);
		int endX	= floorPixel(next->dbX);
		for( ; x <= endX ; x++ )
			setPixel(x,inY,inColor);
		current = next;
		next = current->ptrNext;
	}while(next);
}

void scanLineFill(Vector* ptrPolygon, int inNumPoly,DWORD inColor)
{
	allocEdges();
	initEdgeTable(ptrPolygon,inNumPoly);
	for(int y=g_inMinY ; y<g_inMaxY ; y++ )
	{
		insertAEL(g_ptrAELHead,g_ptrEdgeTable[y]);
		fillAELScanLine(g_ptrAELHead,y,inColor);
		updateAEL(g_ptrAELHead,y+1);
	}
	deallocEdges();
}


void setBackground( int inW, int inH)
{
	int i , j ,lines;

	for( i=0 ; i<inH ; i ++ )
	{
		lines = i*inW*3;
		for( j=0 ; j<inW ; j ++ )
		{
			if( (j/g_inPixelArray[g_inPixelIndx] + i/g_inPixelArray[g_inPixelIndx])%2==0 )
			{
				*(g_ptrScreen + lines + j*3) = 0;
				*(g_ptrScreen + lines + j*3 + 1) = 0;
				*(g_ptrScreen + lines + j*3 + 2) = 0;
			}
			else
			{
				*(g_ptrScreen + lines + j*3) = 128;
				*(g_ptrScreen + lines + j*3 + 1) = 128;
				*(g_ptrScreen + lines + j*3 + 2) = 128;
			}
		}
	}
}

void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	int gridH = (g_inHeight / g_inPixelArray[g_inPixelIndx]) * g_inPixelArray[g_inPixelIndx];
	int gridW = (g_inWidth / g_inPixelArray[g_inPixelIndx]) * g_inPixelArray[g_inPixelIndx];

	setBackground(gridW,gridH);

	Vector polygon[12];
	polygon[0].x = 52;	polygon[0].y = 30;
	polygon[1].x = 74;	polygon[1].y = 43;
	polygon[2].x = 60;	polygon[2].y = 60;
	polygon[3].x = 38;	polygon[3].y = 60;
	polygon[4].x = 30;	polygon[4].y = 50;
	polygon[5].x = 10;	polygon[5].y = 50;
	polygon[6].x = 10;	polygon[6].y = 28;
	polygon[7].x = 22;	polygon[7].y = 41;
	polygon[8].x = 33;	polygon[8].y = 10;
	polygon[9].x = 50;	polygon[9].y = 10;
	polygon[10].x = 39;	polygon[10].y = 30;
	scanLineFill(polygon,11,RGB(255,0,0));

	glDrawPixels(gridW,gridH,GL_RGB,GL_UNSIGNED_BYTE,g_ptrScreen);

	glutSwapBuffers();
	glFlush();
}

void myMouseButton(int button, int state, int x, int y)
{
	if( button==GLUT_LEFT_BUTTON && state==GLUT_DOWN )
	{

	}
	else if( button==GLUT_LEFT_BUTTON && state==GLUT_UP )
	{

	}

}

void init()
{
	glClearColor(0.0f,0.0f,0.0f,0.0f); 
}

void myKeys(unsigned char key, int x, int y) 
{
	switch(key) 
	{
	case 'u':
	case 'U':
		if( g_inPixelIndx<4 )
			g_inPixelIndx += 1;
		break;
	case 'l':
	case 'L':
		if( g_inPixelIndx>0 )
			g_inPixelIndx -= 1;
		break;
	}
	myDisplay();
}

void myReshape(GLsizei w,GLsizei h)
{
	g_inHeight = h;
	g_inWidth = w;
	glViewport(0,0,w,h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,w,0,h);
	glMatrixMode(GL_MODELVIEW);
}


int main(int argc,char ** argv)
{
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(500,600);

	glutCreateWindow("Brensenham");

	init();
	glutReshapeFunc(myReshape);
	glutDisplayFunc(myDisplay);
	glutKeyboardFunc(myKeys);
	glutMouseFunc(myMouseButton);
	glutMainLoop();

	return(0);
}