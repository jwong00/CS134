#include <stdio.h>
#include <GL/glew.h>
#include "DrawUtils.h"
#include "DrawUtils.c"
#include <SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define TILE_WIDTH 16
#define TILE_HEIGHT 16

char shouldExit = 0;

typedef struct {
		int xPos;
		int yPos;
	} PLAYER;

typedef struct {
		int xPos;
		int yPos;
		int w;
		int h;
	} CAMERA; 

int main( void )
{
    /* Initialize SDL */
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
       return 1;
    }

    /* Create the window, OpenGL context */
    SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 32 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_Window* window = SDL_CreateWindow(
            "TestSDL",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            SDL_WINDOW_OPENGL );
	int x = 5;
	
   if( !window ) {
      fprintf( stderr, "Could not create window.  ErrorCode=%s\n", SDL_GetError() );
      SDL_Quit();
      return 1;
   }
   SDL_GL_CreateContext( window );
 
   /* Make sure we have a recent version of OpenGL */
   GLenum glewError = glewInit();
   if( glewError != GLEW_OK ) {
       fprintf( stderr, "Colud not initialize glew.  ErrorCode=%s\n", glewGetErrorString( glewError ));
       SDL_Quit();
       return 1;
   }
   if( !GLEW_VERSION_3_0 ) {
       fprintf( stderr, "OpenGL max supported version is too low.\n" );
       SDL_Quit();
       return 1;
   }

   /* Setup OpenGL state */
   glViewport( 0, 0, 640, 480 );
   glMatrixMode( GL_PROJECTION );
   glOrtho( 0, 640, 480, 0, 0, 100 );
   glEnable( GL_TEXTURE_2D );

	/* Art to draw */
	GLuint tex[16];
	tex[0]=glTexImageTGAFile("character1.tga",NULL,NULL);
	tex[1]=glTexImageTGAFile("tile1.tga",NULL,NULL);
	tex[2]=glTexImageTGAFile("tile2.tga",NULL,NULL);
	tex[3]=glTexImageTGAFile("tile3.tga",NULL,NULL);
	tex[4]=glTexImageTGAFile("tile4.tga",NULL,NULL);
	tex[5]=glTexImageTGAFile("tile5.tga",NULL,NULL);

	/* Initalize some variables */
	PLAYER player;
	player.xPos=320;
	player.yPos=240;

	CAMERA camera;
	camera.xPos=0;
	camera.yPos=0;
	//camera.w=WINDOW_WIDTH;
	//camera.h=WINDOW_HEIGHT;

	/* Map for level, a 2d array of pointers to GLuint's */
	int map[100][100];

	int j,i;
	for(j=0;j<100;j++) {
		for(i=0;i<100;i++) {
			map[i][j]=1;
		}
	}

	/*Previous frame's keyboard state*/
	unsigned char kbPrevState[SDL_NUM_SCANCODES]={0};

	/*Current frame's keyboard state*/
	const unsigned char* kbState = NULL;
	/* Keep a pointer to SDL's internal keyboard state */
	kbState = SDL_GetKeyboardState(NULL);

	/* The game loop */
   while( !shouldExit ) {
		/*kbState is updated by the message pump. Copy old state before the pump.*/
		memcpy(kbPrevState, kbState,sizeof(kbPrevState));

		// Handle OS message pump
		SDL_Event event;
		while( SDL_PollEvent( &event )) {
			switch( event.type ) {
		   	case SDL_QUIT:
		      shouldExit = 1;
		  }
		}
		glClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT );

 		/* calls to glDraw go here */

		/* update positions, animations and sprites here */

		/* update positions of camera here */
		if(kbState[SDL_SCANCODE_RIGHT]) {
			camera.xPos+=10;
		}
		else if(kbState[SDL_SCANCODE_LEFT]) {
			camera.xPos-=10;
		}
		else if(kbState[SDL_SCANCODE_UP]) {
			camera.yPos+=10;
		}
		else if(kbState[SDL_SCANCODE_DOWN]) {
			camera.yPos-=10;
		}
		/* draw backgrounds, handle parallax */
		int k,l;
		//find the region on the map to render
		//
		for(k=0;k<100;k++) {
			for(l=0;l<100;l++) {
				glDrawSprite( tex[ map[l][k] ],16*l-camera.xPos , 16*k+camera.yPos,16,16 );
			}
		}
		/* draw sprites */

 		glDrawSprite(tex[0],player.xPos,camera.yPos,50,50);
		/* draw foregrounds, handle parallax */
		/* Game logic goes here */
		
   	SDL_GL_SwapWindow( window );
   }
 
   SDL_Quit();
 
   return 0;
}
