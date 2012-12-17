#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <omp.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#ifdef WIN32
#include "glew.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"
#include "glui.h"

#include "glslprogram.h"

// title of these windows:

const char *WINDOWTITLE = { "OpenGL Compute Shader Swarm Intelligence" };
const char *GLUITITLE   = { "Particle Controller" };

// random parameters:					

const float XMIN = 	{ -100.0 };
const float XMAX = 	{  100.0 };
const float YMIN = 	{ -100.0 };
const float YMAX = 	{  100.0 };
const float ZMIN = 	{ -100.0 };
const float ZMAX = 	{  100.0 };

const float VXMIN =	{   -100. };
const float VXMAX =	{    100. };
const float VYMIN =	{   -100. };
const float VYMAX =	{    100. };
const float VZMIN =	{   -100. };
const float VZMAX =	{    100. };


const int NUM_PARTICLES      = 1024*1024*32;
const int WORK_GROUP_SIZE    = 1024;

const int GLUITRUE  = { true  };
const int GLUIFALSE = { false };

#define ESCAPE		0x1b

const int INIT_WINDOW_SIZE = { 1000 };		// window size in pixels

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };
const float MINSCALE = { 0.001f };

const int LEFT   = { 4 };
const int MIDDLE = { 2 };
const int RIGHT  = { 1 };

enum Projections
{
	ORTHO,
	PERSP
};

enum ButtonVals
{
	GO,
	PAUSE,
	RESET,
	QUIT
};

const float BACKCOLOR[ ] = { 0., 0., 0., 0. };

const GLfloat AXES_COLOR[ ] = { 1., .5, 0. };
const GLfloat AXES_WIDTH   = { 3. };

//
// structs we will need later:

struct pos
{
	float x, y, z, w;	// positions
};

struct vel
{
	float vx, vy, vz, vw;	// velocities
};

struct color
{
	float r, g, b, a;	// colors
};

struct svel
{
	float svx, svy, svz, svw;	// velocities
};


// non-constant global variables:

int	ActiveButton;		// current button that is down
GLuint	AxesList;		// list to hold the axes
int	AxesOn;			// ON or OFF
GLUI *	Glui;			// instance of glui window
int	GluiWindow;		// the glut id for the glui window
int	MainWindow;		// window id for main graphics window
int	Paused;
GLfloat	RotMatrix[4][4];	// set by glui rotation widget
float	Scale, Scale2;		// scaling factors
//GLuint	SphereList;
int	WhichProjection;	// ORTHO or PERSP
int	Xmouse, Ymouse;		// mouse values
float	Xrot, Yrot;		// rotation angles in degrees
float	TransXYZ[3];		// set by glui translation widgets

double	ElapsedTime;
int		ShowPerformance;


GLuint  posSSbo;
GLuint  velSSbo;
GLuint  colSSbo;
GLuint svelSSbo;
GLSLProgram  *MyComputeShaderProgram;


float cm[3];
float mouseOpenGL[3];
float avel[3];
//float svel[3];

//
// function prototypes:
//

inline
float
SQR( float x )
{
	return x * x;
}

void	Animate( );
void	Axes( float );
void	Buttons( int );
void	Display( );
void	DoRasterString( float, float, float, char * );
void	DoStrokeString( float, float, float, float, char * );
void	InitGlui( );
void	InitGraphics( );
void	InitLists( );
void	Keyboard( unsigned char, int, int );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Quit( );
float	Ranf( float, float );
void	Reset( );
void	ResetParticles( );
void	Resize( int, int );
void	Traces( int );
void	Visibility( int );


//
// main Program:
//

int
main( int argc, char *argv[ ] )
{
	glutInit( &argc, argv );
	InitGraphics( );
	InitLists( );
	ResetParticles( );
	Reset( );
	InitGlui( );
	glutMainLoop( );
	return 0;
}

void
Animate( )
{
	double time0, time1;

	glutSetWindow( MainWindow );

	if( ShowPerformance )
	{
		glFinish( );
		time0 = omp_get_wtime( );
	}

	//mouse position in opengl coordinates
	GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY, winZ;
    GLdouble posX, posY, posZ;
 
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
 
    winX = (float)Xmouse;
    winY = (float)viewport[3] - (float)Ymouse;
    glReadPixels( Xmouse, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
 
    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

	mouseOpenGL[0] = posX;
	mouseOpenGL[1] = posY;
	mouseOpenGL[2] = posZ;
	

	MyComputeShaderProgram->Use( );

	//adding all uniforms for compute shader
	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_maxParticles"), NUM_PARTICLES);
	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_xMouse"), posX);
	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_yMouse"), posY);
	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_zMouse"), posZ);

	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_xcm"), cm[0]);
	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_ycm"), cm[1]);
	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_zcm"), cm[2]);

	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_xalign"), avel[0]);
	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_yalign"), avel[1]);
	glUniform1f(glGetUniformLocation(MyComputeShaderProgram->Program, "u_zalign"), avel[2]);

	glDispatchCompute( NUM_PARTICLES  / WORK_GROUP_SIZE, 1,  1 );
	CheckGlErrors( "glDispatchCompute" );
	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
	CheckGlErrors( "glMemoryBarrier" );

#ifdef NOTDEF
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, posSSbo );
	struct pos *points = (struct pos *) glMapBuffer( GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	fprintf( stderr, "\n(0x%08x) After:\n", points );
	for( int i = 0; i < 5; i++ )
	{
		fprintf( stderr, "%3d  %8.3f  %8.3f  %8.3f\n", i, points[ i ].x, points[ i ].y, points[ i ].z );
	}
	glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
#endif

	if( ShowPerformance )
	{
		glFinish( );
		time1 = omp_get_wtime( );
		ElapsedTime = time1 - time0;
		//fprintf( stderr, "ElapsedTime = %g\n", ElapsedTime );
	}

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}




//
// glui buttons callback:
//

void
Buttons( int id )
{
	switch( id )
	{
		case GO:
			GLUI_Master.set_glutIdleFunc( Animate );
			glutSetWindow( MainWindow );
			glutPostRedisplay( );
			break;

		case PAUSE:
			Paused = ! Paused;
			if( Paused )
				GLUI_Master.set_glutIdleFunc( NULL );
			else
				GLUI_Master.set_glutIdleFunc( Animate );
			break;

		case RESET:
			ResetParticles( );
			Reset( );
			GLUI_Master.set_glutIdleFunc( NULL );
			Glui->sync_live( );
			glutSetWindow( MainWindow );
			glutPostRedisplay( );
			break;

		case QUIT:
			Quit( );
			break;

		default:
			fprintf( stderr, "Don't know what to do with Button ID %d\n", id );
	}

}



//
// draw the complete scene:
//

void
Display( )
{
	glutSetWindow( MainWindow );
	glDrawBuffer( GL_BACK );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );
	glShadeModel( GL_FLAT );
	GLsizei vx = glutGet( GLUT_WINDOW_WIDTH );
	GLsizei vy = glutGet( GLUT_WINDOW_HEIGHT );
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = ( vx - v ) / 2;
	GLint yb = ( vy - v ) / 2;
	glViewport( xl, yb,  v, v );


	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	if( WhichProjection == ORTHO )
		glOrtho( -300., 300.,  -300., 300., 0.1, 2000. );
	else
		gluPerspective( 50., 1.,	0.1, 2000. );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
	gluLookAt( 0., -100., 800.,     0., -100., 0.,     0., 1., 0. );
	glTranslatef( (GLfloat)TransXYZ[0], (GLfloat)TransXYZ[1], -(GLfloat)TransXYZ[2] );
	glRotatef( (GLfloat)Yrot, 0., 1., 0. );
	glRotatef( (GLfloat)Xrot, 1., 0., 0. );
	glMultMatrixf( (const GLfloat *) RotMatrix );
	glScalef( (GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale );
	float scale2 = 1. + Scale2;		// because glui translation starts at 0.
	if( scale2 < MINSCALE )
		scale2 = MINSCALE;
	glScalef( (GLfloat)scale2, (GLfloat)scale2, (GLfloat)scale2 );

	glDisable( GL_FOG );

	if( AxesOn != GLUIFALSE )
		glCallList( AxesList );

	// ****************************************
	// Here is where you draw the current state of the particles:
	// ****************************************

	glBindBuffer( GL_ARRAY_BUFFER, posSSbo  );
	glVertexPointer( 4, GL_FLOAT, 0, (void *)0 );
	glEnableClientState( GL_VERTEX_ARRAY );

	glBindBuffer( GL_ARRAY_BUFFER, colSSbo );
	glColorPointer( 4, GL_FLOAT, 0, (void *)0 );
	glEnableClientState( GL_COLOR_ARRAY );

	glPointSize( 5. );
	glDrawArrays( GL_POINTS, 0, NUM_PARTICLES );
	glPointSize( 1. );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

//	glCallList( SphereList );

	if( ShowPerformance )
	{
		char str[128];
		sprintf( str, "%8.4f MegaParticles/Sec", (float)NUM_PARTICLES/ElapsedTime/1000000. );
		//glDisable( GL_DEPTH_TEST );
		//glMatrixMode( GL_PROJECTION );
		//glLoadIdentity( );
		//gluOrtho2D( 0., 100.,     0., 100. );
		//glMatrixMode( GL_MODELVIEW );
		//glLoadIdentity( );
		glColor3f( 1., 1., 1. );
		DoRasterString( -300., -400., 0., str );
	}

	glutSwapBuffers( );
	glFlush( );
}



//
// use glut to display a string of characters using a raster font:
//

void
DoRasterString( float x, float y, float z, char *s )
{
	char c;			// one character to print

	glRasterPos3f( (GLfloat)x, (GLfloat)y, (GLfloat)z );
	for( ; ( c = *s ) != '\0'; s++ )
	{
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, c );
	}
}



//
// use glut to display a string of characters using a stroke font:
//

void
DoStrokeString( float x, float y, float z, float ht, char *s )
{
	char c;			// one character to print

	glPushMatrix( );
		glTranslatef( (GLfloat)x, (GLfloat)y, (GLfloat)z );
		float sf = ht / ( 119.05 + 33.33 );
		glScalef( (GLfloat)sf, (GLfloat)sf, (GLfloat)sf );
		for( ; ( c = *s ) != '\0'; s++ )
		{
			glutStrokeCharacter( GLUT_STROKE_ROMAN, c );
		}
	glPopMatrix( );
}


//
// initialize the glui window:
//

void
InitGlui( )
{
	glutInitWindowPosition( INIT_WINDOW_SIZE + 50, 0 );
	Glui = GLUI_Master.create_glui( (char *) GLUITITLE );
	Glui->add_statictext( (char *) GLUITITLE );
	Glui->add_separator( );
	Glui->add_checkbox( "Axes",             &AxesOn );
	Glui->add_checkbox( "Perspective",      &WhichProjection );
	Glui->add_checkbox( "Show Performance", &ShowPerformance );

	GLUI_Panel *panel = Glui->add_panel( "Object Transformation" );

		GLUI_Rotation *rot = Glui->add_rotation_to_panel( panel, "Rotation", (float *) RotMatrix );
		rot->set_spin( 1.0 );

		Glui->add_column_to_panel( panel, GLUIFALSE );
		GLUI_Translation *scale = Glui->add_translation_to_panel( panel, "Scale",  GLUI_TRANSLATION_Y , &Scale2 );
		scale->set_speed( 0.01f );

		Glui->add_column_to_panel( panel, FALSE );
		GLUI_Translation *trans = Glui->add_translation_to_panel( panel, "Trans XY", GLUI_TRANSLATION_XY, &TransXYZ[0] );
		trans->set_speed( 1.1f );

		Glui->add_column_to_panel( panel, FALSE );
		trans = Glui->add_translation_to_panel( panel, "Trans Z",  GLUI_TRANSLATION_Z , &TransXYZ[2] );
		trans->set_speed( 1.1f );

	panel = Glui->add_panel( "", FALSE );
		Glui->add_button_to_panel( panel, "Go !", GO, (GLUI_Update_CB) Buttons );
		Glui->add_column_to_panel( panel, FALSE );
		Glui->add_button_to_panel( panel, "Pause", PAUSE, (GLUI_Update_CB) Buttons );
		Glui->add_column_to_panel( panel, FALSE );
		Glui->add_button_to_panel( panel, "Reset", RESET, (GLUI_Update_CB) Buttons );
		Glui->add_column_to_panel( panel, FALSE );
		Glui->add_button_to_panel( panel, "Quit", QUIT, (GLUI_Update_CB) Buttons );

	Glui->set_main_gfx_window( MainWindow );
	GLUI_Master.set_glutIdleFunc( NULL );
}



//
// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions
//

void
InitGraphics( )
{
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );

	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );
	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

	// setup the callback routines:

	glutSetWindow( MainWindow );
	glutDisplayFunc( Display );
	glutReshapeFunc( Resize );
	glutKeyboardFunc( Keyboard );
	glutMouseFunc( MouseButton );
	glutMotionFunc( MouseMotion );
	glutPassiveMotionFunc( MouseMotion );
	glutVisibilityFunc( Visibility );
	glutEntryFunc( NULL );
	glutSpecialFunc( NULL );
	glutSpaceballMotionFunc( NULL );
	glutSpaceballRotateFunc( NULL );
	glutSpaceballButtonFunc( NULL );
	glutButtonBoxFunc( NULL );
	glutDialsFunc( NULL );
	glutTabletMotionFunc( NULL );
	glutTabletButtonFunc( NULL );
	glutMenuStateFunc( NULL );
	glutTimerFunc( 0, NULL, 0 );

#ifdef WIN32
	GLenum err = glewInit();
	if( err != GLEW_OK )
	{
		fprintf( stderr, "glewInit Error\n" );
	}

#endif

	MyComputeShaderProgram = new GLSLProgram( );
	bool valid = MyComputeShaderProgram->Create( "particles.cs" );

	if( !valid )
		fprintf( stderr, "Compute Shader is invalid\n" );
	else
		fprintf( stderr, "Compute Shader created OK\n" );


	glGenBuffers( 1, &posSSbo);
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, posSSbo);
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct pos), NULL, GL_STATIC_DRAW );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

	glGenBuffers( 1, &velSSbo);
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, velSSbo);
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct vel), NULL, GL_STATIC_DRAW );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

	glGenBuffers( 1, &colSSbo);
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, colSSbo);
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct color), NULL, GL_STATIC_DRAW );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

	glGenBuffers( 1, &svelSSbo);
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, svelSSbo);
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct svel), NULL, GL_STATIC_DRAW );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

	// print some compute shader statistics:

	int cx, cy, cz;
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cx );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cy );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &cz );
	fprintf( stderr, "Max Compute Work Group Count = %5d, %5d, %5d\n", cx, cy, cz );

	int sx, sy, sz;
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sx );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sy );
	glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sz );
	fprintf( stderr, "Max Compute Work Group Size  = %5d, %5d, %5d\n", sx, sy, sz );

	fprintf( stderr, "My Compute Work Group Count  = %5d, %5d, %5d\n", NUM_PARTICLES/WORK_GROUP_SIZE, 1, 1 );
	fprintf( stderr, "My Compute Work Group Size   = %5d, %5d, %5d\n", WORK_GROUP_SIZE, 1, 1 );
}



//
// initialize the display lists that will not change:
//

void
InitLists( )
{
//	SphereList = glGenLists( 1 );
//	glNewList( SphereList, GL_COMPILE );
//		glColor3f( .9f, .9f, 0. );
//		glPushMatrix( );
//			glTranslatef( -100., -800., 0. );
//			glutWireSphere( 600., 100., 100. );
//		glPopMatrix( );
//	glEndList( );

	AxesList = glGenLists( 1 );
	glNewList( AxesList, GL_COMPILE );
		glColor3fv( AXES_COLOR );
		glLineWidth( AXES_WIDTH );
			Axes( 150. );
		glLineWidth( 1. );
	glEndList( );
}



//
// the keyboard callback:
//

void
Keyboard( unsigned char c, int x, int y )
{
	switch( c )
	{
		case 'o':
		case 'O':
			WhichProjection = ORTHO;
			break;

		case 'p':
		case 'P':
			WhichProjection = PERSP;
			break;

		case 'q':
		case 'Q':
		case ESCAPE:
			Buttons( QUIT );	// will not return here
			break;			// happy compiler

		default:
			fprintf( stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c );
	}
	Glui->sync_live( );
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}



//
// called when the mouse button transitions down or up:
//


void
MouseButton( int button, int state, int x, int y )
{
	int b;			// LEFT, MIDDLE, or RIGHT
	
	switch( button )
	{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;

		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		default:
			b = 0;
			fprintf( stderr, "Unknown mouse button: %d\n", button );
	}


	// button down sets the bit, up clears the bit:

	if( state == GLUT_DOWN )
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}
}



//
// called when the mouse moves while a button is down:
//

void
MouseMotion( int x, int y )
{
	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if( ActiveButton & LEFT )
	{
			Xrot += ( ANGFACT*dy );
			Yrot += ( ANGFACT*dx );
	}


	if( ActiveButton & RIGHT )
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:

		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}


	if( ActiveButton & MIDDLE )
	{
		TransXYZ[0] += dx;
		TransXYZ[1] -= dy;
	}


	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}







//
// reset the transformations and the colors:
//
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene
//

void
Reset( )
{
	ActiveButton = 0;
	AxesOn = GLUIFALSE;
	Paused = GLUIFALSE;
	Scale  = 1.0;
	Scale2 = 0.0;		// because add 1. to it in Display( )
	ShowPerformance = GLUIFALSE;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
	TransXYZ[0] = TransXYZ[1] = TransXYZ[2] = 0.;

	                  RotMatrix[0][1] = RotMatrix[0][2] = RotMatrix[0][3] = 0.;
	RotMatrix[1][0]                   = RotMatrix[1][2] = RotMatrix[1][3] = 0.;
	RotMatrix[2][0] = RotMatrix[2][1]                   = RotMatrix[2][3] = 0.;
	RotMatrix[3][0] = RotMatrix[3][1] = RotMatrix[3][3]                   = 0.;
	RotMatrix[0][0] = RotMatrix[1][1] = RotMatrix[2][2] = RotMatrix[3][3] = 1.;
}


void
ResetParticles( )
{
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, posSSbo );
	//struct pos *points = (struct pos *) glMapBuffer( GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY );
	struct pos *points = (struct pos *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct pos), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
	for( int i = 0; i < NUM_PARTICLES; i++ )
	{
		points[ i ].x = Ranf( XMIN, XMAX );
		points[ i ].y = Ranf( YMIN, YMAX );
		points[ i ].z = Ranf( ZMIN, ZMAX );
		points[ i ].w = 1.;
	}

	cm[0] = 0; cm[1] = 0; cm[2] = 0;
	for( int i = 0; i < NUM_PARTICLES; i++ )
	{
		cm[0] += points[i].x;
		cm[1] += points[i].y;
		cm[2] += points[i].z;
	}

	avel[0] = 0; avel[1] = 0; avel[2] = 0;
    for(int i = 0; i < NUM_PARTICLES; i++)
    {
		avel[0] += mouseOpenGL[0] - points[i].x;
        avel[1] += mouseOpenGL[1] - points[i].y;
        avel[2] += mouseOpenGL[2] - points[i].z;        
    }

    avel[0] = avel[0] / NUM_PARTICLES;
	avel[1] = avel[1] / NUM_PARTICLES;
	avel[2] = avel[2] / NUM_PARTICLES;


	glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

	glBindBuffer( GL_SHADER_STORAGE_BUFFER, velSSbo );
	//struct vel *vels = (struct vel *) glMapBuffer( GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY );
	struct vel *vels = (struct vel *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct vel), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
	for( int i = 0; i < NUM_PARTICLES; i++ )
	{
		vels[ i ].vx = Ranf( VXMIN, VXMAX );
		vels[ i ].vy = Ranf( VYMIN, VYMAX );
		vels[ i ].vz = Ranf( VZMIN, VZMAX );
		vels[ i ].vw = 0.;
	}
	glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
	
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, colSSbo );
	//struct color *colors = (struct color *) glMapBuffer( GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY );
	struct color *colors = (struct color *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct color), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
	for( int i = 0; i < NUM_PARTICLES; i++ )
	{
		colors[ i ].r = Ranf( .3f, 1. );
		colors[ i ].g = Ranf( .3f, 1. );
		colors[ i ].b = Ranf( .3f, 1. );
		colors[ i ].a = 1.;
	}
	glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

	//glBindBuffer( GL_SHADER_STORAGE_BUFFER, svelSSbo );
	////struct vel *svels = (struct svel *) glMapBuffer( GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY );
	//struct svel *svels = (struct svel *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct svel), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
	//float dist[3];
	//
	//for( int i = 0; i < NUM_PARTICLES; i++ )
	//{
	//	svels[ i ].svx = 0.;
	//	svels[ i ].svy = 0.;
	//	svels[ i ].svz = 0.;
	//	svels[ i ].svw = 0.;
	//	dist[0] = 0; dist[1] = 0; dist[2] = 0;
	//	
	//	for( int j = 0; j < NUM_PARTICLES; j++ )
	//	{
	//		if(i != j)
	//		{
	//			dist[0] = points[i].x - points[j].x;
	//			dist[1] = points[i].y - points[j].y;
	//			dist[2] = points[i].z - points[j].z;
	//		}
	//	
	//		if(dist[0] != 0 && dist[1] != 0 && dist[2] != 0)
	//		{
	//			svels[ i ].svx += dist[0] / (dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2]);
	//			svels[ i ].svy += dist[1] / (dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2]);
	//			svels[ i ].svz += dist[2] / (dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2]);
	//			svels[ i ].svw = 0.;	
	//		}
	//	}
	//}

	//glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
	//glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
	
	//glBindBufferBase( GL_SHADER_STORAGE_BUFFER,  7,  svelSSbo);
	

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER,  4,  posSSbo );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER,  5,  velSSbo );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER,  6,  colSSbo );
	
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
	
#ifdef NOTDEF
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, posSSbo );
	points = (struct pos *) glMapBuffer( GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY );
	fprintf( stderr, "\n(0x%08x) Just created:\n", points );
	for( int i = 0; i < 5; i++ )
	{
		fprintf( stderr, "%3d  %8.3f  %8.3f  %8.3f\n", i, points[ i ].x, points[ i ].y, points[ i ].z );
	}
	glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
#endif
}



void
Resize( int width, int height )
{
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
Visibility ( int state )
{
	if( state == GLUT_VISIBLE )
	{
		glutSetWindow( MainWindow );
		glutPostRedisplay( );
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}







// the stroke characters 'X' 'Y' 'Z' :

static float xx[ ] = {
		0., 1., 0., 1.
	      };

static float xy[ ] = {
		-.5, .5, .5, -.5
	      };

static int xorder[ ] = {
		1, 2, -3, 4
		};


static float yx[ ] = {
		0., 0., -.5, .5
	      };

static float yy[ ] = {
		0., .6f, 1., 1.
	      };

static int yorder[ ] = {
		1, 2, 3, -2, 4
		};


static float zx[ ] = {
		1., 0., 1., 0., .25, .75
	      };

static float zy[ ] = {
		.5, .5, -.5, -.5, 0., 0.
	      };

static int zorder[ ] = {
		1, 2, 3, 4, -5, 6
		};


// fraction of the length to use as height of the characters:

#define LENFRAC		0.10


// fraction of length to use as start location of the characters:

#define BASEFRAC	1.10


//
//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)
//

void
Axes( float length )
{
	glBegin( GL_LINE_STRIP );
		glVertex3f( length, 0., 0. );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., length, 0. );
	glEnd( );
	glBegin( GL_LINE_STRIP );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., 0., length );
	glEnd( );

	float fact = LENFRAC * length;
	float base = BASEFRAC * length;

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 4; i++ )
		{
			int j = xorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( base + fact*xx[j], fact*xy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 5; i++ )
		{
			int j = yorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( fact*yx[j], base + fact*yy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 6; i++ )
		{
			int j = zorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( 0.0, fact*zy[j], base + fact*zx[j] );
		}
	glEnd( );

}


//
// exit gracefully:
//

void
Quit( )
{
	Glui->close( );
	glutSetWindow( MainWindow );
	glFinish( );
	glutDestroyWindow( MainWindow );

	exit( 0 );
}




#define TOP	2147483647.		// 2^31 - 1	

float
Ranf( float low, float high )
{
	long random( );		// returns integer 0 - TOP

	float r = (float)rand( );
	return(   low  +  r * ( high - low ) / (float)RAND_MAX   );
}
