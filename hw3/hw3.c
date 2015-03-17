/*vim: set tabstop=8:softtabstop=8:shiftwidt=8:noexpandtab*/
#include <stdio.h>
#include <stdbool.h>
#include <GL/glew.h>
#include "DrawUtils.h"
#include "DrawUtils.c"
#include <SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define TILE_SIZE 16

GLuint bgTex[16];
GLuint sprites[32];
char shouldExit = 0;

/* Defines an animation frame. */
typedef struct AnimFrameDef {
	int frameNum; 
	float frameTime;
} AnimFrameDef;

/* Defines an animation. Contains animation frames. */ 
typedef struct AnimDef {
	const char* name;
	AnimFrameDef frames[16];
 	int numFrames;
} AnimDef;

/* Stores the run-time state of an animation. */
typedef struct AnimData {
	AnimDef* def;
	int curFrame;
	float timeToNextFrame;
	bool isPlaying;
} AnimData;

typedef struct {
	AnimData anim;
	int xPos;
	int yPos;
} PLAYER;

typedef struct {
	int xPos;
	int yPos;
	int w;
	int h;
} CAMERA;



void animTick(AnimData*, float);
void animSet(AnimData*, AnimDef*);
void animReset(AnimData*);
void animDraw(AnimData*, int,int,int,int);


int main( void ) {
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
	bgTex[0]=glTexImageTGAFile("tile1.tga",NULL,NULL);
	bgTex[1]=glTexImageTGAFile("tile2.tga",NULL,NULL);
	bgTex[2]=glTexImageTGAFile("tile3.tga",NULL,NULL);
	bgTex[3]=glTexImageTGAFile("tile4.tga",NULL,NULL);
	bgTex[4]=glTexImageTGAFile("tile5.tga",NULL,NULL);
	
	
	sprites[0]=glTexImageTGAFile("character1.tga",NULL,NULL);
	sprites[1]=glTexImageTGAFile("character2.tga",NULL,NULL);

	/* Initalize player variables */
	PLAYER player;
	AnimDef playerAnim;
	playerAnim.name = "player";
	playerAnim.frames[0].frameNum=0;
	playerAnim.frames[0].frameTime=0.16;
	playerAnim.frames[1].frameNum=1;
	playerAnim.frames[1].frameTime=0.16;
	playerAnim.numFrames=2;
	//player.anim.def = &playerAnim;
	animSet(&player.anim ,&playerAnim);
	player.xPos=320;
	player.yPos=240;

	/* Initialize camera */	
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

		animTick(&player.anim, 0.016);
		//if(!&player.anim.isPlaying) 
		//	animReset(&player.anim);

 	     	/* update positions of camera here */
 	     	if(kbState[SDL_SCANCODE_RIGHT]) {
 	     		camera.xPos+=2;
 	     	}
 	     	else if(kbState[SDL_SCANCODE_LEFT]) {
 	     		camera.xPos-=2;
 	     	}
 	     	else if(kbState[SDL_SCANCODE_UP]) {
 	     		camera.yPos+=2;
 	     	}
 	     	else if(kbState[SDL_SCANCODE_DOWN]) {
 	     		camera.yPos-=2;
 	     	}
 	     	/* draw backgrounds, handle parallax */
 	     	int xStart=camera.xPos/TILE_SIZE;
 	     	int yStart=camera.yPos/TILE_SIZE;
 	     	int xFinish=(camera.xPos+WINDOW_WIDTH)/TILE_SIZE;
 	     	int yFinish=(camera.yPos+WINDOW_HEIGHT)/TILE_SIZE;

		/* For safety. Ensures no out-of-bounds errors */
 	     	if(xStart<0) xStart=0;
 	     	if(yStart<0) yStart=0;
 	     	if(xFinish>100) xFinish=100;
 	     	if(yFinish>100) yFinish=100;

 	     	//
 	     	int k,l;

 	     	for(k=yStart;k<yFinish;k++) {
 	     		for(l=xStart;l<xFinish;l++) {
 	     			glDrawSprite( bgTex[ map[l][k] ],
 	     				16*l-camera.xPos , 
 	     				16*k+camera.yPos,16,16 );
 	     		}
 	     	}
 	     	/* draw sprites */
		
 	     	//glDrawSprite(sprites[0],player.xPos,player.yPos,50,50);
		animDraw(&player.anim,player.xPos,player.yPos,50,50);
		if(!player.anim.isPlaying) animReset(&player.anim);
 	     	/* draw foregrounds, handle parallax */
 	     	/* Game logic goes here */
 	     	
 		SDL_GL_SwapWindow( window );
	}

	SDL_Quit();

	return 0;
}

void animTick(AnimData* data, float dt) {
	if(!data->isPlaying) {
		return;
	}

	int numFrames = data->def->numFrames;
	data->timeToNextFrame -= dt;
	if(data->timeToNextFrame < 0) {
		++data->curFrame;
		if(data->curFrame >= numFrames) { //stops anim
			data->curFrame = numFrames-1;
			//data->curFrame = 0;
			data->timeToNextFrame = 0;
			data->isPlaying=false;
		}
		else {
			AnimFrameDef* curFrame
			 	= &data->def->frames[data->curFrame];
			data->timeToNextFrame
				+= curFrame->frameTime;
		}
	}
}

void animSet(AnimData* anim, AnimDef* toPlay) {
	anim->def = toPlay;
	anim->curFrame = 0;
	anim->timeToNextFrame
		= toPlay->frames[0].frameTime;
	anim->isPlaying = true;
}

void animReset(AnimData* anim) {
	animSet(anim, anim->def);
}

void animDraw(AnimData* anim, int x, int y, int w, int h) {
	AnimDef* def = anim->def;
	int curFrameNum = def->frames[anim->curFrame].frameNum;
	GLuint frameToDraw = sprites[curFrameNum];
	glDrawSprite(frameToDraw,x,y,w,h);
}

