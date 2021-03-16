#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#include "glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"
#define	 GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "vertexbufferobject.h"
#include "glslprogram.h"

//	The left mouse button does rotation
//	The middle mouse button does scaling
//	The user interface allows:
//		1. The axes to be turned on and off
//		2. The color of the axes to be changed
//		3. Debugging to be turned on and off
//		4. Depth cueing to be turned on and off
//		5. The projection to be changed
//		6. The transformations to be reset
//		7. The program to quit
//
//	Author:			Colin Van Overschelde

// NOTE: There are a lot of good reasons to use const variables instead
// of #define's.  However, Visual C++ does not allow a const variable
// to be used as an array size or as the case in a switch( ) statement.  So in
// the following, all constants are const variables except those which need to
// be array sizes or cases in switch( ) statements.  Those are #defines.

// title of these windows:
const char *WINDOWTITLE = { "CS457 Final Project -- Colin Van Overschelde" };
const char *GLUITITLE   = { "User Interface Window" };

// what the glui package defines as true and false:
const int GLUITRUE  = { true  };
const int GLUIFALSE = { false };

// convert degrees to radians:
const float D2R = (float)M_PI / 180.f;

// the escape key:
#define ESCAPE		0x1b

// initial window size:
const int INIT_WINDOW_SIZE = { 600 };

// size of the 3d box:
const float BOXSIZE = { 2.f };

// multiplication factors for input interaction:
//  (these are known from previous experience)
const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };

// minimum allowable scale factor:
const float MINSCALE = { 0.05f };

// scroll wheel button values:
const int SCROLL_WHEEL_UP   = { 3 };
const int SCROLL_WHEEL_DOWN = { 4 };

// equivalent mouse movement when we click a the scroll wheel:
const float SCROLL_WHEEL_CLICK_FACTOR = { 5. };

// active mouse buttons (or them together):
const int LEFT   = { 4 };
const int MIDDLE = { 2 };
const int RIGHT  = { 1 };

// which projection:
enum Projections
{
	ORTHO,
	PERSP
};

// which button:
enum ButtonVals
{
	RESET,
	QUIT
};

// window background color (rgba):
const GLfloat BACKCOLOR[ ] = { 0., 0., 0., 1. };

// line width for the axes:
const GLfloat AXES_WIDTH   = { 3. };

struct Vert {
	float  V[3];
	float  N[3];
	float ST[2];
};

struct Pos {
	float x, y, z, w;		// positions
};

struct Vel {
	float vX, vY, vZ, vW;		// Velocities
};

struct Col {
	float r, g, b, a;		// Colors
};

// should we turn the shadows on?
//#define ENABLE_SHADOWS

// non-constant global variables:
int		ActiveButton;			// current button that is down
GLuint	AxesList;				// list to hold the axes
int		AxesOn;					// != 0 means to draw the axes
int		DebugOn;				// != 0 means to print debugging info
int		MainWindow;				// window id for main graphics window
float	Scale;					// scaling factor
int		WhichProjection;		// ORTHO or PERSP
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees
// Beam Objects
VertexBufferObject* BeamVBO;
GLSLProgram*		BeamShader;
GLuint				NoiseTexture;
GLuint				NoiseMask;
GLSLProgram*		BeamMainParticles;
GLuint				PosSSBO;
GLuint				VelSSBO;
GLuint				ColSSBO;
VertexBufferObject* ParticleVBO;
GLSLProgram*		ParticleShader;
GLuint				ParticleTexture;
GLSLProgram*		WhooshShader;
GLuint				WhooshTexture;
#define NUM_PARTICLES		64 * 64
#define WORK_GROUP_SIZE		128
// Animation Timers
const int   MS_IN_STAR_ANIMATION = 1000;
const int   MS_IN_BUMP_ANIMATION = 10000;
const int	MS_IN_BULB_ANIMATION = 10000;
const int	MS_IN_SPIN_ANIMATION = 2500;
float		BumpTime;
float		BulbTime;
float		SpinTime;
float		StarTime;

// function prototypes:
void	Animate( );
void	Display( );
void	DoAxesMenu( int );
void	DoMainMenu( int );
void	DoProjectMenu( int );
void	DoRasterString( float, float, float, char * );
void	DoStrokeString( float, float, float, float, char * );
float	ElapsedSeconds( );
void	InitGraphics( );
void	InitLists( );
void	InitMenus( );
void	Keyboard( unsigned char, int, int );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Reset( );
void	Resize( int, int );
void	Visibility( int );
// Utility Functions
void			Axes( float );
unsigned char*	BmpToTexture( char *, int *, int * );
unsigned char*  ReadTexture2D(char*, int*, int*);
void			HsvRgb( float[3], float [3] );
int				ReadInt( FILE * );
short			ReadShort( FILE * );
// Transformation Functions
void			Cross(float[3], float[3], float[3]);
float			Dot(float [3], float [3]);
float			Unit(float [3], float [3]);
float			SmoothStep(float, float, float);
// Geometry Functions
void			CreateBeam(int, float, float, int, int, int, int);
void			SetupParticleBuffer();

// main program:
int
main( int argc, char *argv[ ] )
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)
	glutInit( &argc, argv );

	// setup all the graphics stuff:
	InitGraphics( );

	// create the display structures that will not change:
	InitLists( );

	// init all the global variables used by Display( ):
	// this will also post a redisplay
	Reset( );

	// setup all the user interface stuff:
	InitMenus( );

	// draw the scene once and wait for some interaction:
	// (this will never return)
	glutSetWindow( MainWindow );
	glutMainLoop( );

	// glutMainLoop( ) never returns
	// this line is here to make the compiler happy:
	return 0;
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display( ) from here -- let glutMainLoop( ) do it
void
Animate( )
{
	// put animation stuff in here -- change some global variables
	// for Display( ) to find:
	int ms = glutGet(GLUT_ELAPSED_TIME);
	int BumpMs = ms % MS_IN_BUMP_ANIMATION;
	int BulbMs = ms % MS_IN_BULB_ANIMATION;
	int SpinMs = ms % MS_IN_SPIN_ANIMATION;
	int StarMs = ms % MS_IN_STAR_ANIMATION;

	BumpTime = (float)BumpMs / (float)MS_IN_BUMP_ANIMATION;
	BulbTime = (float)BulbMs / (float)MS_IN_BULB_ANIMATION;
	SpinTime = (float)SpinMs / (float)MS_IN_SPIN_ANIMATION;
	StarTime = (float)StarMs / (float)MS_IN_STAR_ANIMATION;

	// Run Compute Shaders
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, PosSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, VelSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ColSSBO);

	BeamMainParticles->Use();
	BeamMainParticles->DispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);

	// force a call to Display( ) next time it is convenient:
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// draw the complete scene:
void
Display( )
{
	if( DebugOn != 0 )
	{
		fprintf( stderr, "Display\n" );
	}

	// set which window we want to do the graphics into:
	glutSetWindow( MainWindow );

	// erase the background:
	glDrawBuffer( GL_BACK );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glEnable( GL_DEPTH_TEST );

	// specify shading to be flat:
	glShadeModel( GL_FLAT );

	// set the viewport to a square centered in the window:
	GLsizei vx = glutGet( GLUT_WINDOW_WIDTH );
	GLsizei vy = glutGet( GLUT_WINDOW_HEIGHT );
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = ( vx - v ) / 2;
	GLint yb = ( vy - v ) / 2;
	glViewport( xl, yb,  v, v );

	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	// USE gluOrtho2D( ) IF YOU ARE DOING 2D !
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );

	// Setup Projection matrix
	glm::mat4 projection;

	if (WhichProjection == ORTHO)
		projection = glm::ortho(-3., 3., -3., 3., 0.1, 1000.);
	else
		projection = glm::perspective(D2R * 90., 1., 0.1, 1000.);

	// apply the projection matrix:
	glMultMatrixf(glm::value_ptr(projection));

	// place the objects into the scene:
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );


	// set the eye position, look-at position, and up-vector:
	glm::vec3 eye(0., 0., 3.);
	glm::vec3 look(0., 0., 0.);
	glm::vec3 up(0., 1., 0.);
	glm::mat4 modelView = glm::lookAt(eye, look, up);


	// rotate the scene:
	modelView = glm::rotate(modelView, D2R * Yrot, glm::vec3(0., 1., 0.));
	modelView = glm::rotate(modelView, D2R * Xrot, glm::vec3(1., 0., 0.));


	// uniformly scale the scene:
	if( Scale < MINSCALE )
		Scale = MINSCALE;
	modelView = glm::scale(modelView, glm::vec3(Scale, Scale, Scale));

	// Apply the modelview matrix
	glMultMatrixf(glm::value_ptr(modelView));

	/*
	// set the fog parameters:
	if( DepthCueOn != 0 )
	{
		glFogi( GL_FOG_MODE, FOGMODE );
		glFogfv( GL_FOG_COLOR, FOGCOLOR );
		glFogf( GL_FOG_DENSITY, FOGDENSITY );
		glFogf( GL_FOG_START, FOGSTART );
		glFogf( GL_FOG_END, FOGEND );
		glEnable( GL_FOG );
	}
	else
	{
		glDisable( GL_FOG );
	}
	*/

	// possibly draw the axes:
	if( AxesOn != 0 )
	{
		glColor3f( 1., 1., 1. );
		glCallList( AxesList );
	}

	// since we are using glScalef( ), be sure normals get unitized:
	glEnable( GL_NORMALIZE );

	// draw the current object:
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, NoiseTexture);
	
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, NoiseMask);

	BeamShader->Use();
	glm::vec3 lightPos = { -1., 5., 5. };
	BeamShader->SetUniformVariable("uLightPos", lightPos);
	BeamShader->SetUniformVariable("uBumpTime", BumpTime);
	BeamShader->SetUniformVariable("uBulbTime", BulbTime);
	BeamShader->SetUniformVariable("uSpinTime", SpinTime);
	BeamShader->SetUniformVariable("uNoise", 8);
	BeamShader->SetUniformVariable("uNoiseMask", 9);
	BeamShader->SetUniformVariable("uIsWhoosh", 0);
	BeamVBO->Draw();
	BeamShader->Use(0);

	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, WhooshTexture);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	WhooshShader->Use();
	WhooshShader->SetUniformVariable("uLightPos", lightPos);
	WhooshShader->SetUniformVariable("uBumpTime", BumpTime);
	WhooshShader->SetUniformVariable("uBulbTime", BulbTime);
	WhooshShader->SetUniformVariable("uSpinTime", SpinTime);
	WhooshShader->SetUniformVariable("uStarTime", StarTime);
	WhooshShader->SetUniformVariable("uIsWhoosh", 1);
	WhooshShader->SetUniformVariable("uNoise", 8);
	WhooshShader->SetUniformVariable("uWhoosh", 9);
	BeamVBO->Draw();
	WhooshShader->Use(0);

	ParticleShader->Use();
	//ParticleShader->SetAttributeVariable("aVertex", &PosSSBO, GL_VERTEX);
	//for (int i = 0; i < NUM_PARTICLES; i++) {
		glBindBuffer(GL_ARRAY_BUFFER, PosSSBO);
		glVertexPointer(4, GL_FLOAT, 0, (void*)0);
		glEnableClientState(GL_VERTEX_ARRAY);
		glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
		glDisableClientState(GL_VERTEX_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		//ParticleVBO->Draw();
	//}
	ParticleShader->Use(0);

	/*
	// draw some gratuitous text that just rotates on top of the scene:
	glDisable( GL_DEPTH_TEST );
	glColor3f( 0., 1., 1. );
	DoRasterString( 0., 1., 0., (char *)"Text That Moves" );

	// draw some gratuitous text that is fixed on the screen:
	//
	// the projection matrix is reset to define a scene whose
	// world coordinate system goes from 0-100 in each axis
	//
	// this is called "percent units", and is just a convenience
	//
	// the modelview matrix is reset to identity as we don't
	// want to transform these coordinates
	glDisable( GL_DEPTH_TEST );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluOrtho2D( 0., 100.,     0., 100. );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
	glColor3f( 1., 1., 1. );
	DoRasterString( 5., 5., 0., (char *)"Text That Doesn't" );
	*/

	// swap the double-buffered framebuffers:
	glutSwapBuffers( );

	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !
	glFlush( );
}


void
DoAxesMenu( int id )
{
	AxesOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

// main menu callback:
void
DoMainMenu( int id )
{
	switch( id )
	{
		case RESET:
			Reset( );
			break;

		case QUIT:
			// gracefully close out the graphics:
			// gracefully close the graphics window:
			// gracefully exit the program:
			glutSetWindow( MainWindow );
			glFinish( );
			glutDestroyWindow( MainWindow );
			exit( 0 );
			break;

		default:
			fprintf( stderr, "Don't know what to do with Main Menu ID %d\n", id );
	}

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoProjectMenu( int id )
{
	WhichProjection = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

/*
void
DoShadowsMenu(int id)
{
	ShadowsOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}
*/

// use glut to display a string of characters using a raster font:
void
DoRasterString( float x, float y, float z, char *s )
{
	glRasterPos3f( (GLfloat)x, (GLfloat)y, (GLfloat)z );

	char c;			// one character to print
	for( ; ( c = *s ) != '\0'; s++ )
	{
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, c );
	}
}


// use glut to display a string of characters using a stroke font:
void
DoStrokeString( float x, float y, float z, float ht, char *s )
{
	glPushMatrix( );
		glTranslatef( (GLfloat)x, (GLfloat)y, (GLfloat)z );
		float sf = ht / ( 119.05f + 33.33f );
		glScalef( (GLfloat)sf, (GLfloat)sf, (GLfloat)sf );
		char c;			// one character to print
		for( ; ( c = *s ) != '\0'; s++ )
		{
			glutStrokeCharacter( GLUT_STROKE_ROMAN, c );
		}
	glPopMatrix( );
}


// return the number of seconds since the start of the program:
float
ElapsedSeconds( )
{
	// get # of milliseconds since the start of the program:
	int ms = glutGet( GLUT_ELAPSED_TIME );

	// convert it to seconds:
	return (float)ms / 1000.f;
}


// initialize the glui window:
void
InitMenus( )
{
	glutSetWindow( MainWindow );

	int axesmenu = glutCreateMenu( DoAxesMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int projmenu = glutCreateMenu( DoProjectMenu );
	glutAddMenuEntry( "Orthographic",  ORTHO );
	glutAddMenuEntry( "Perspective",   PERSP );

	glutAddMenuEntry( "Reset",         RESET );
	//glutAddSubMenu(   "Debug",         debugmenu);
	glutAddMenuEntry( "Quit",          QUIT );

	// attach the pop-up menu to the right mouse button:
	glutAttachMenu( GLUT_RIGHT_BUTTON );
}



// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions
void
InitGraphics( )
{
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	// set the initial window configuration:
	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );
	
	// open the window and set its title:
	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );

	// set the framebuffer clear values:
	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

	// setup the callback functions:
	// DisplayFunc -- redraw the window
	// ReshapeFunc -- handle the user resizing the window
	// KeyboardFunc -- handle a keyboard input
	// MouseFunc -- handle the mouse button going down or up
	// MotionFunc -- handle the mouse moving with a button down
	// PassiveMotionFunc -- handle the mouse moving with a button up
	// VisibilityFunc -- handle a change in window visibility
	// EntryFunc	-- handle the cursor entering or leaving the window
	// SpecialFunc -- handle special keys on the keyboard
	// SpaceballMotionFunc -- handle spaceball translation
	// SpaceballRotateFunc -- handle spaceball rotation
	// SpaceballButtonFunc -- handle spaceball button hits
	// ButtonBoxFunc -- handle button box hits
	// DialsFunc -- handle dial rotations
	// TabletMotionFunc -- handle digitizing tablet motion
	// TabletButtonFunc -- handle digitizing tablet button hits
	// MenuStateFunc -- declare when a pop-up menu is in use
	// TimerFunc -- trigger something to happen a certain time from now
	// IdleFunc -- what to do when nothing else is going on

	glutSetWindow( MainWindow );
	glutDisplayFunc( Display );
	glutReshapeFunc( Resize );
	glutKeyboardFunc( Keyboard );
	glutMouseFunc( MouseButton );
	glutMotionFunc( MouseMotion );
	glutPassiveMotionFunc( NULL );
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
	glutTimerFunc( -1, NULL, 0 );
	glutIdleFunc( Animate );

	// init glew (a window must be open to do this):
#ifdef WIN32
	GLenum err = glewInit( );
	if( err != GLEW_OK )
	{
		fprintf( stderr, "glewInit Error\n" );
	}
	else
		fprintf( stderr, "GLEW initialized OK\n" );
	fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

	
	int numS, numT;// , numP;
	//unsigned char* NoiseTextureArray = ReadTexture2D("noise2d.064.tex", &numS, &numT);
	unsigned char* NoiseTextureArray = BmpToTexture("Noise.bmp", &numS, &numT);
	if (NoiseTextureArray == NULL) {
		printf("Error loading Noise Texture\n");
	}
	glGenTextures(1, &NoiseTexture);
	glBindTexture(GL_TEXTURE_2D, NoiseTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, numS, numT, 0, GL_RGB, GL_UNSIGNED_BYTE, NoiseTextureArray);

	int width, height;
	unsigned char* NoiseMaskArray = BmpToTexture("noise1.bmp", &width, &height);
	if (NoiseTextureArray == NULL) {
		printf("Error loading Noise Texture\n");
	}
	glGenTextures(1, &NoiseMask);
	glBindTexture(GL_TEXTURE_2D, NoiseMask);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NoiseMaskArray);

	BeamShader = new GLSLProgram();
	bool valid = BeamShader->Create("beam.vert", "beam.frag");
	if (!valid) {
		printf("Error loading shader\n");
	}

	BeamMainParticles = new GLSLProgram();
	valid = BeamMainParticles->Create("mainParticles.cs");
	if (!valid) {
		printf("Error loading Computer Shader\n");
	}
	SetupParticleBuffer();

	ParticleShader = new GLSLProgram();
	valid = ParticleShader->Create("particle.vert", "particle.frag");
	if (!valid) {
		printf("Error loading Computer Shader\n");
	}

	WhooshShader = new GLSLProgram();
	valid = WhooshShader->Create("beam.vert", "whoosh.frag");
	if (!valid) {
		printf("Error loading Computer Shader\n");
	}

	unsigned char* WhooshArray = BmpToTexture("woosh.bmp", &width, &height);
	if (NoiseTextureArray == NULL) {
		printf("Error loading Noise Texture\n");
	}
	glGenTextures(1, &WhooshTexture);
	glBindTexture(GL_TEXTURE_2D, WhooshTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, WhooshArray);
}


// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )
void
InitLists( )
{
	glutSetWindow( MainWindow );

	BeamVBO = new VertexBufferObject();
	BeamVBO->CollapseCommonVertices(true);
	float radius = 0.3;
	// Create the Beam Vertices
	BeamVBO->glBegin(GL_TRIANGLE_STRIP);
		CreateBeam(1, radius, 8., 0, 30, 0, 70);
	BeamVBO->glEnd();

	ParticleVBO = new VertexBufferObject();
	ParticleVBO->CollapseCommonVertices(false);
	ParticleVBO->glBegin(GL_QUADS);
		ParticleVBO->glTexCoord2f(0., 0.);
		ParticleVBO->glVertex3f(-.5, -.5, 0.);

		ParticleVBO->glTexCoord2f(1., 0.);
		ParticleVBO->glVertex3f(.5, -.5, 0.);

		ParticleVBO->glTexCoord2f(1., 1.);
		ParticleVBO->glVertex3f(.5, .5, 0.);

		ParticleVBO->glTexCoord2f(0., 1.);
		ParticleVBO->glVertex3f(-.5, .5, 0.);
	ParticleVBO->glEnd();

	// create the axes:
	AxesList = glGenLists( 1 );
	glNewList( AxesList, GL_COMPILE );
		glLineWidth( AXES_WIDTH );
			Axes( 1.5 );
		glLineWidth( 1. );
	glEndList( );
}


// the keyboard callback:
void
Keyboard( unsigned char c, int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Keyboard: '%c' (0x%0x)\n", c, c );

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
			DoMainMenu( QUIT );	// will not return here
			break;				// happy compiler

		default:
			fprintf( stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c );
	}

	// force a call to Display( ):
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// called when the mouse button transitions down or up:
void
MouseButton( int button, int state, int x, int y )
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if( DebugOn != 0 )
		fprintf( stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y );

	
	// get the proper button bit mask:
	switch( button )
	{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;

		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		case SCROLL_WHEEL_UP:
			Scale += SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
			// keep object from turning inside-out or disappearing:
			if (Scale < MINSCALE)
				Scale = MINSCALE;
			break;

		case SCROLL_WHEEL_DOWN:
			Scale -= SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
			// keep object from turning inside-out or disappearing:
			if (Scale < MINSCALE)
				Scale = MINSCALE;
			break;

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

	glutSetWindow(MainWindow);
	glutPostRedisplay();

}


// called when the mouse moves while a button is down:
void
MouseMotion( int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "MouseMotion: %d, %d\n", x, y );


	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if( ( ActiveButton & LEFT ) != 0 )
	{
		Xrot += ( ANGFACT*dy );
		Yrot += ( ANGFACT*dx );
	}


	if( ( ActiveButton & MIDDLE ) != 0 )
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:
		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene
void
Reset( )
{
	ActiveButton = 0;
	AxesOn = 1;
	DebugOn = 0;
	Scale  = 1.0;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
}


// called when user resizes the window:
void
Resize( int width, int height )
{
	if( DebugOn != 0 )
		fprintf( stderr, "ReSize: %d, %d\n", width, height );

	// don't really need to do anything since window size is
	// checked each time in Display( ):
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// handle a change to the window's visibility:
void
Visibility ( int state )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Visibility: %d\n", state );

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



///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


// the stroke characters 'X' 'Y' 'Z' :
static float xx[ ] = {
		0.f, 1.f, 0.f, 1.f
	      };

static float xy[ ] = {
		-.5f, .5f, .5f, -.5f
	      };

static int xorder[ ] = {
		1, 2, -3, 4
		};

static float yx[ ] = {
		0.f, 0.f, -.5f, .5f
	      };

static float yy[ ] = {
		0.f, .6f, 1.f, 1.f
	      };

static int yorder[ ] = {
		1, 2, 3, -2, 4
		};

static float zx[ ] = {
		1.f, 0.f, 1.f, 0.f, .25f, .75f
	      };

static float zy[ ] = {
		.5f, .5f, -.5f, -.5f, 0.f, 0.f
	      };

static int zorder[ ] = {
		1, 2, 3, 4, -5, 6
		};

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)
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

struct bmfh
{
	short bfType;
	int bfSize;
	short bfReserved1;
	short bfReserved2;
	int bfOffBits;
} FileHeader;

struct bmih
{
	int biSize;
	int biWidth;
	int biHeight;
	short biPlanes;
	short biBitCount;
	int biCompression;
	int biSizeImage;
	int biXPelsPerMeter;
	int biYPelsPerMeter;
	int biClrUsed;
	int biClrImportant;
} InfoHeader;

const int birgb = { 0 };

// read a BMP file into a Texture:
unsigned char *
BmpToTexture( char *filename, int *width, int *height )
{
	FILE *fp = fopen( filename, "rb" );
	if( fp == NULL )
	{
		fprintf( stderr, "Cannot open Bmp file '%s'\n", filename );
		return NULL;
	}

	FileHeader.bfType = ReadShort( fp );


	// if bfType is not 0x4d42, the file is not a bmp:
	if( FileHeader.bfType != 0x4d42 )
	{
		fprintf( stderr, "File '%s' is the wrong type of file: 0x%0x\n", filename, FileHeader.bfType );
		fclose( fp );
		return NULL;
	}

	FileHeader.bfSize = ReadInt( fp );
	FileHeader.bfReserved1 = ReadShort( fp );
	FileHeader.bfReserved2 = ReadShort( fp );
	FileHeader.bfOffBits = ReadInt( fp );

	InfoHeader.biSize = ReadInt( fp );
	InfoHeader.biWidth = ReadInt( fp );
	InfoHeader.biHeight = ReadInt( fp );

	int nums = InfoHeader.biWidth;
	int numt = InfoHeader.biHeight;

	InfoHeader.biPlanes = ReadShort( fp );
	InfoHeader.biBitCount = ReadShort( fp );
	InfoHeader.biCompression = ReadInt( fp );
	InfoHeader.biSizeImage = ReadInt( fp );
	InfoHeader.biXPelsPerMeter = ReadInt( fp );
	InfoHeader.biYPelsPerMeter = ReadInt( fp );
	InfoHeader.biClrUsed = ReadInt( fp );
	InfoHeader.biClrImportant = ReadInt( fp );

	fprintf( stderr, "Image size in file '%s' is: %d x %d\n", filename, nums, numt );

	unsigned char * texture = new unsigned char[ 3 * nums * numt ];
	if( texture == NULL )
	{
		fprintf( stderr, "Cannot allocate the texture array!\b" );
		return NULL;
	}

	// extra padding bytes:
	int numextra =  4*(( (3*InfoHeader.biWidth)+3)/4) - 3*InfoHeader.biWidth;

	// we do not support compression:
	if( InfoHeader.biCompression != birgb )
	{
		fprintf( stderr, "Image file '%s' has the wrong type of image compression: %d\n", filename, InfoHeader.biCompression );
		fclose( fp );
		return NULL;
	}

	rewind( fp );
	fseek( fp, 14+40, SEEK_SET );

	if( InfoHeader.biBitCount == 24 )
	{
		unsigned char *tp = texture;
		for( int t = 0; t < numt; t++ )
		{
			for( int s = 0; s < nums; s++, tp += 3 )
			{
				*(tp+2) = fgetc( fp );		// b
				*(tp+1) = fgetc( fp );		// g
				*(tp+0) = fgetc( fp );		// r
			}

			for( int e = 0; e < numextra; e++ )
			{
				fgetc( fp );
			}
		}
	}

	fclose( fp );

	*width = nums;
	*height = numt;
	return texture;
}

int
ReadInt( FILE *fp )
{
	unsigned char b3, b2, b1, b0;
	b0 = fgetc( fp );
	b1 = fgetc( fp );
	b2 = fgetc( fp );
	b3 = fgetc( fp );
	return ( b3 << 24 )  |  ( b2 << 16 )  |  ( b1 << 8 )  |  b0;
}

short
ReadShort( FILE *fp )
{
	unsigned char b1, b0;
	b0 = fgetc( fp );
	b1 = fgetc( fp );
	return ( b1 << 8 )  |  b0;
}


// function to convert HSV to RGB
// 0.  <=  s, v, r, g, b  <=  1.
// 0.  <= h  <=  360.
// when this returns, call:
//		glColor3fv( rgb );
void
HsvRgb( float hsv[3], float rgb[3] )
{
	// guarantee valid input:
	float h = hsv[0] / 60.f;
	while( h >= 6. )	h -= 6.;
	while( h <  0. ) 	h += 6.;

	float s = hsv[1];
	if( s < 0. )
		s = 0.;
	if( s > 1. )
		s = 1.;

	float v = hsv[2];
	if( v < 0. )
		v = 0.;
	if( v > 1. )
		v = 1.;

	// if sat==0, then is a gray:
	if( s == 0.0 )
	{
		rgb[0] = rgb[1] = rgb[2] = v;
		return;
	}

	// get an rgb from the hue itself:
	float i = (float)floor( h );
	float f = h - i;
	float p = v * ( 1.f - s );
	float q = v * ( 1.f - s*f );
	float t = v * ( 1.f - ( s * (1.f-f) ) );

	float r=0., g=0., b=0.;			// red, green, blue
	switch( (int) i )
	{
		case 0:
			r = v;	g = t;	b = p;
			break;
	
		case 1:
			r = q;	g = v;	b = p;
			break;
	
		case 2:
			r = p;	g = v;	b = t;
			break;
	
		case 3:
			r = p;	g = q;	b = v;
			break;
	
		case 4:
			r = t;	g = p;	b = v;
			break;
	
		case 5:
			r = v;	g = p;	b = q;
			break;
	}


	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void
Cross(float v1[3], float v2[3], float vout[3])
{
	float tmp[3];
	tmp[0] = v1[1] * v2[2] - v2[1] * v1[2];
	tmp[1] = v2[0] * v1[2] - v1[0] * v2[2];
	tmp[2] = v1[0] * v2[1] - v2[0] * v1[1];
	vout[0] = tmp[0];
	vout[1] = tmp[1];
	vout[2] = tmp[2];
}

float
Dot(float v1[3], float v2[3])
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

float
Unit(float vin[3], float vout[3])
{
	float dist = vin[0] * vin[0] + vin[1] * vin[1] + vin[2] * vin[2];
	if (dist > 0.0)
	{
		dist = sqrtf(dist);
		vout[0] = vin[0] / dist;
		vout[1] = vin[1] / dist;
		vout[2] = vin[2] / dist;
	}
	else
	{
		vout[0] = vin[0];
		vout[1] = vin[1];
		vout[2] = vin[2];
	}
	return dist;
}

float 
SmoothStep(float min, float max, float val)
{
	float t = (val - min) / (max - min);
	if (t < 0.) {
		t = 0.;
	}
	else if (t > 1.) {
		t = 1.;
	}

	return t * t * (3.0 - 2.0 * t);
}

unsigned char * 
ReadTexture2D(char* filename, int* width, int* height)
{
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("Texture Error - FILE NOT FOUND\n");
		return NULL;
	}

	int numS, numT, numP;
	fread(&numS, 4, 1, fp);
	fread(&numT, 4, 1, fp);
//	fread(&numP, 4, 1, fp);

	unsigned char* texture = new unsigned char[4 * numS * numT];

	fread(texture, 4 * numS * numT, 1, fp);
	fclose(fp);
	return texture;
}

void 
CreateBeam(int _levelOfDetail, float _radius, float _length, int _sourceType, int _sourceLength, int _tipType, int _tipLength)
{
	// Setup the Source mesh
	int sourceRows;
	if (_sourceLength < 0) {
		_sourceLength = 0;
		sourceRows = 0;
	}
	else if (_sourceLength > 40) {
		_sourceLength = 40;
		sourceRows = 500 * (1 + _levelOfDetail);
	}
	else {
		sourceRows = 500 * (1 + _levelOfDetail);
	}
	float sourceLength = _length * (float)_sourceLength / 100.;
	float sourceRowLength = sourceLength / (float)sourceRows;

	// Setup the Tip mesh
	int tipRows;
	if (_tipLength < 0) {
		_tipLength = 0;
		tipRows = 0;
	}
	else if (_tipLength > 100 - _sourceLength) {
		_tipLength = 100 - _sourceLength;
		tipRows = 500 * (1 + _levelOfDetail);
	}
	else {
		tipRows = 500 * (1 + _levelOfDetail);
	}
	float tipLength = _length * (float)_tipLength / 100.;
	float tipRowLength = tipLength / (float)tipRows;

	// Setup the Shaft mesh
	float shaftLength = _length - sourceLength - tipLength;
	int shaftRows = 0;
	float shaftRowLength = 0.;
	if (shaftLength > 0.) {
		shaftRows = 500 * (1 + _levelOfDetail);
		shaftRowLength = shaftLength / (float)shaftRows;
	}	 
	
	// Build the mesh array
	int rows = sourceRows + shaftRows + tipRows;
	int cols = 50 * (1 + _levelOfDetail);
	int totalVerts = rows * cols;
	int sourceVerts = sourceRows * cols;
	int shaftVerts  = shaftRows  * cols;
	int tipVerts    = tipRows	 * cols;
	Vert* points = new Vert[totalVerts];
	float curRadius = 0.;
	for (int i = 0; i < totalVerts; i++) {
		// Get the column index
		int   curColumn = i % cols;
		int   curRow	= i / cols;
		
		// Find the Z-Index and Radius of the Vertex
		float curZ;
		// What section are we in?
		if (i < sourceVerts) {
			// Do the source
			curZ = curRow * sourceRowLength;
			float progress = (float)curRow / ((float)sourceRows - 1.);
			curRadius = _radius * SmoothStep(0., _radius, progress);
			
		}
		else if (i < sourceVerts + shaftVerts) {
			// Do the shaft
			curZ = sourceLength + (curRow - sourceRows) * shaftRowLength;
			curRadius = _radius;
		}
		else {
			// Do the tip
			curZ = sourceLength + shaftLength + ( (curRow - sourceRows - shaftRows) * tipRowLength );
			int tipRow = curRow - shaftRows - sourceRows;
			float progress = (float)tipRow / (tipRows - 1);
			curRadius = _radius * sqrt(1. - progress);
		}

		// Set the Vertex values for the current Point
		// Get the theta of the current Point
		float theta = (float)curColumn / (float)cols;
		points[i].ST[0] = theta;
		points[i].ST[1] = curZ / _length;
		points[i].V[0] = curRadius * (float)sin(theta * 2. * M_PI);
		points[i].V[1] = curRadius * (float)cos(theta * 2. * M_PI);
		points[i].V[2] = curZ;

	}

	// Build Normals
	// For each Vertex
	for (int i = 0; i < totalVerts - cols; i++) {
		int col = i % cols;
		int row = i / cols;
		// Get Top
		int top;
		// Check if you're in the top row
		if (row == rows - 1) {
			top = i;
		}
		else {
			top = i + cols;
		}
		// Get TopLeft and Left
		int topLeft = i + cols - 1;
		int left = i - 1;
		// Check in youre in the first column
		if (col == 0){
			topLeft = i + 2 * cols - 1;
			left = i + cols - 1;
		}
		else {
			topLeft = i + cols - 1;
			left = i - 1;
		}
		// Get Bottom, Bottom Right and Right
		int bottom = 0;
		int bottomRight;
		int right;
		// Check if you're in the bottom row
		if (i < cols) {
			bottom = i;
			// Check if you're in the last column
			if (col == cols - 1) {
				bottomRight = 0;
				right = 0;
			}
			else {		// We are not in the last column
				bottomRight = i + 1;
				right = i + 1;
			}
		}
		else {	// We are not in the bottom row
			bottom = i - cols;
			// Check if you're in the last column
			if (col >= cols - 1) {
				bottomRight = i - 2 * cols + 1;
				right = i - cols + 1;
			}
			else {
				bottomRight = bottom + 1;
				right = i + 1;
			}
		}
		
		// Get TopVector
		float topVector[3] = {
			points[top].V[0] - points[i].V[0],
			points[top].V[1] - points[i].V[1],
			points[top].V[2] - points[i].V[2]
		};
		// Get TopLeftVector
		float topLeftVector[3] = {
			points[topLeft].V[0] - points[i].V[0],
			points[topLeft].V[1] - points[i].V[1],
			points[topLeft].V[2] - points[i].V[2]
		};
		// Get LeftVector
		float leftVector[3] = {
			points[left].V[0] - points[i].V[0],
			points[left].V[1] - points[i].V[1],
			points[left].V[2] - points[i].V[2]
		};
		// Get BottomVector
		float bottomVector[3] = {
			points[bottom].V[0] - points[i].V[0],
			points[bottom].V[1] - points[i].V[1],
			points[bottom].V[2] - points[i].V[2]
		};
		// Get BottomRightVector
		float bottomRightVector[3] = {
			points[bottomRight].V[0] - points[i].V[0],
			points[bottomRight].V[1] - points[i].V[1],
			points[bottomRight].V[2] - points[i].V[2]
		};
		// Get RightVector
		float rightVector[3] = {
			points[right].V[0] - points[i].V[0],
			points[right].V[1] - points[i].V[1],
			points[right].V[2] - points[i].V[2]
		};

		//printf("Done building Vectors\n");

		float Normals[3] = { 0., 0., 0. };
		float result[3] = { 0., 0., 0. };
		// Cross Top and TopLeft
		Cross(topVector, topLeftVector, result);
		Normals[0] += result[0];
		Normals[1] += result[1];
		Normals[2] += result[2];
		// Cross TopLeft and Left
		Cross(topLeftVector, leftVector, result);
		Normals[0] += result[0];
		Normals[1] += result[1];
		Normals[2] += result[2];
		// Cross Left and Bottom
		Cross(leftVector, bottomVector, result);
		Normals[0] += result[0];
		Normals[1] += result[1];
		Normals[2] += result[2];
		// Cross Bottom and BottomRight
		Cross(bottomVector, bottomRightVector, result);
		Normals[0] += result[0];
		Normals[1] += result[1];
		Normals[2] += result[2];
		// Cross BottomRight and Right
		Cross(bottomRightVector, rightVector, result);
		Normals[0] += result[0];
		Normals[1] += result[1];
		Normals[2] += result[2];
		// Cross Right and Up
		Cross(rightVector, topVector, result);
		Normals[0] += result[0];
		Normals[1] += result[1];
		Normals[2] += result[2];
		Unit(Normals, Normals);
		points[i].N[0] = -Normals[0];
		points[i].N[1] = -Normals[1];
		points[i].N[2] = -Normals[2];
	}

	// Build the VBO
	// Start at 0, 0
	// Move to 1, 0
	// Move to 0, 1
	int strips = rows - 1;
	// For each strip
	for (int i = 0; i < strips; i++) {
		int startIndex = i * cols;
		int curIndex   = startIndex;
		for (int j = 0; j < cols; j++) {
			int bottomIndex = curIndex + j;
			BeamVBO->glColor3f(1., 0., 1.);
			BeamVBO->glTexCoord2fv(points[bottomIndex].ST);
			BeamVBO->glNormal3fv(points[bottomIndex].N);
			BeamVBO->glVertex3fv(points[bottomIndex].V);
			int topIndex = curIndex + cols + j;
			BeamVBO->glColor3f(1., 0., 1.);
			BeamVBO->glTexCoord2fv(points[topIndex].ST);
			BeamVBO->glNormal3fv(points[topIndex].N);
			BeamVBO->glVertex3fv(points[topIndex].V);
		}
	}
}

void 
SetupParticleBuffer() {
	printf("Starting Perticle Buffer Setup\n");
	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;		// Invalidate makes a big difference when re-writing
	
	// Initialize the Position Buffer
	glGenBuffers(1, &PosSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, PosSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct Pos), NULL, GL_STATIC_DRAW);
	float xRange = 1.;
	float yRange = 1.;
	float zRange = 10.;
	struct Pos* points = (struct Pos*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct Pos), bufMask);
	for (int i = 0; i < NUM_PARTICLES; i++)	{
		points[i].x = xRange / (float)(rand() % 501 - 250);
		points[i].y = yRange / (float)(rand() % 501 - 250);
		points[i].z = zRange / (float)(rand() % 990 + 11);
		points[i].w = 1.;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	// Initialize the Velocity Buffer
	glGenBuffers(1, &VelSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, VelSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct Vel), NULL, GL_STATIC_DRAW);
	struct Vel* vels = (struct Vel*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct Vel), bufMask);
	float xyVelMax = 10.;
	float zVelMax = 100.;
	for (int i = 0; i < NUM_PARTICLES; i++) {
		vels[i].vX = xyVelMax / (float)(rand() % 1001 - 500);
		vels[i].vY = xyVelMax / (float)(rand() % 1001 - 500);
		vels[i].vZ = zVelMax / (float)(rand() % 1001);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	// Initialize the Color Buffer
	glGenBuffers(1, &ColSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ColSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct Col), NULL, GL_STATIC_DRAW);
	struct Col* cols = (struct Col*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct Col), bufMask);
	for (int i = 0; i < NUM_PARTICLES; i++) {
		float dist = sqrt(points[i].x * points[i].x + points[i].y * points[i].y);
		cols[i].r = 230. / dist;
		cols[i].g = 250. / dist;
		cols[i].b = 252. / dist;
		cols[i].a = 1. / (dist + 0.01);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}