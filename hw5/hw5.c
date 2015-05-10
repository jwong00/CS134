/*vim: set tabstop=8:softtabstop=8:shiftwidth=8:noexpandtab*/
#include <stdio.h>
#include <stdbool.h>
#include <GL/glew.h>
#include "DrawUtils.h"
#include "DrawUtils.c"
#include <SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define TILE_SIZE 32
#define MAP_HEIGHT 40
#define MAP_WIDTH 40

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
	/* pixel position in world*/
	int xPosW;
	int xPosWLast;
	int yPosW;
	int yPosWLast;
	int xPosC;
	int yPosC;
} Player;

typedef struct {
	int xPos;
	int yPos;
	/* defines the boundaries of the "inner box" the player
	 * character pushes against before camera begins to move */
	int left;
	int right;
	int top;
	int bottom;
	int scroll;
} Camera;

typedef struct enemy {
	AnimData anim;
	int xPosW;
	int yPosW;
	int xPosC;
	int yPosC;
	int w;
	int h;
} Enemy;

typedef struct Tile {
	int image;
	bool coll;
} Tile;

Tile map[MAP_WIDTH][MAP_HEIGHT];
GLuint bgTex[16];
GLuint sprites[32];
char shouldExit = 0;
Player player;

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
	glViewport( 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT );
	glMatrixMode( GL_PROJECTION );
	glOrtho( 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 100 );
	glEnable( GL_TEXTURE_2D );
	
	/* Art to draw */
	bgTex[0]=glTexImageTGAFile("tile1.tga",NULL,NULL);
	bgTex[2]=glTexImageTGAFile("tile2.tga",NULL,NULL);
	bgTex[1]=glTexImageTGAFile("tile3.tga",NULL,NULL);
	bgTex[3]=glTexImageTGAFile("tile4.tga",NULL,NULL);
	bgTex[4]=glTexImageTGAFile("tile5.tga",NULL,NULL);
	
	sprites[0]=glTexImageTGAFile("character1.tga",NULL,NULL);
	sprites[1]=glTexImageTGAFile("character2.tga",NULL,NULL);

	/* Initialize camera */	
	Camera camera;
	camera.xPos=0;
	camera.yPos=0;
	camera.left = 4*TILE_SIZE;
	camera.right = WINDOW_WIDTH-4*TILE_SIZE;
	camera.top = 4*TILE_SIZE;
	camera.bottom = WINDOW_HEIGHT-4*TILE_SIZE;
	camera.scroll=1;

	//camera.w=WINDOW_WIDTH;
	//camera.h=WINDOW_HEIGHT;

	/* Initalize player variables */
	//Player player;
	AnimDef playerAnim;
	playerAnim.name = "player";
	playerAnim.frames[0].frameNum=0;
	playerAnim.frames[0].frameTime=1.0;
	playerAnim.frames[1].frameNum=1;
	playerAnim.frames[1].frameTime=1.0;
	playerAnim.numFrames=2;
	//player.anim.def = &playerAnim;
	animSet(&player.anim ,&playerAnim);
	player.xPosW=100;
	player.xPosWLast=player.xPosW;
	player.yPosW=120;
	player.yPosWLast=player.yPosWLast;

	player.xPosC=player.xPosW-camera.xPos;
	player.xPosC=player.xPosW-camera.yPos;

	/* Initialize enemy variables */
	/*Enemy enemies[8];
	AnimDef enemyAnim1;
	enemyAnim1.name = "enemy1";
	enemyAnim1.frames[0].frameNum=0;
	enemyAnim1.frames[0].frameTime=0.5;
	enemyAnim1.frames[1].frameNum=1;
	enemyAnim1.frames[1].frameTime=0.5;
	enemyAnim1.numFrames=2;
	animSet(&enemies[0].anim, &enemyAnim1);
	animSet(&enemies[1].anim, &enemyAnim1);
	enemies[0].xPos=100;
	enemies[0].yPos=140;
	enemies[1].xPos=396;
	enemies[1].yPos=400;*/

	
	/* Map for level, a 2d array of pointers to GLuint's */
	int imageMap[MAP_WIDTH][MAP_HEIGHT] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,4,4,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,0,1,1,1,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,1,1,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	//Tile map[40][40];
	int m,n;
	/*for(n=0;n<40;n++) {
		for(m=0;m<40;m++) {
			map[n][m].image=imageMap[n][m];
			if(map[n][m].image>0) map[n][m].coll = true;
			else map[n][m].coll = false;		
		}
	}*/
	for(n=0;n<40;n++) {
		for(m=0;m<40;m++) {
			map[m][n].image=imageMap[m][n];
			if(map[m][n].image>0) map[m][n].coll = true;
			else map[m][n].coll = false;		
		}
	}
	//printf("\ncoll val at [1][1]: %d\n", map[1][1].coll);
	
	/*Previous frame's keyboard state*/
	unsigned char kbPrevState[SDL_NUM_SCANCODES]={0};
	
	/*Current frame's keyboard state*/
	const unsigned char* kbState = NULL;
	/* Keep a pointer to SDL's internal keyboard state */
	kbState = SDL_GetKeyboardState(NULL);
	
	/* Timing */
	Uint32 lastFrameMs;
	Uint32 currentFrameMs = SDL_GetTicks();
	float physicsDeltaTime = 1/100.0f;
	int physicsDeltaMs = 10;
	Uint32 lastPhysicsFrameMs;

	/* The game loop */
	while( !shouldExit ) {
 	     	/* Preserve some information from previous frame */
 	     	lastFrameMs = currentFrameMs;
		memcpy(kbPrevState, kbState,sizeof(kbPrevState));
		player.xPosWLast=player.xPosW;
		player.yPosWLast=player.yPosW;

 	     	// Handle OS message pump
 	     	SDL_Event event;
 	     	while( SDL_PollEvent( &event )) {
 	     		switch( event.type ) {
				case SDL_QUIT:
					shouldExit = 1;
 	     	  	}
 	     	}
		currentFrameMs = SDL_GetTicks();
		float deltaTime = (currentFrameMs - lastFrameMs)/1000.0f;

 	     	glClearColor( 0, 0, 0, 1 );
 	     	glClear( GL_COLOR_BUFFER_BIT );

 	     	/* calls to glDraw go here */

 	     	/* update positions, animations and sprites here */

		animTick(&player.anim, 0.016);
		//if(!&player.anim.isPlaying) 
		//	animReset(&player.anim);

 	     	/* update positions of player here */
 	     	if(kbState[SDL_SCANCODE_RIGHT]) {
			player.xPosW+=1;	
			if(kbState[SDL_SCANCODE_UP]) {
 	     			player.yPosW-=1;
			}
 	     		else if(kbState[SDL_SCANCODE_DOWN]) {
 	     			player.yPosW+=1;
 	     		}

 	     	}
 	     	else if(kbState[SDL_SCANCODE_LEFT]) {
 	     		player.xPosW-=1;
			if(kbState[SDL_SCANCODE_UP]) {
 	     			player.yPosW-=1;
			}
 	     		else if(kbState[SDL_SCANCODE_DOWN]) {
 	     			player.yPosW+=1;
 	     		}

 	     	}
 	     	else if(kbState[SDL_SCANCODE_UP]) {
 	     		player.yPosW-=1;
			if(kbState[SDL_SCANCODE_LEFT]) {
 	     			player.xPosW-=1;
			}
 	     		else if(kbState[SDL_SCANCODE_RIGHT]) {
 	     			player.xPosW+=1;
 	     		}
		}
 	     	else if(kbState[SDL_SCANCODE_DOWN]) {
 	     		player.yPosW+=1;
			if(kbState[SDL_SCANCODE_LEFT]) {
 	     			player.xPosW-=1;
			}
 	     		else if(kbState[SDL_SCANCODE_RIGHT]) {
 	     			player.xPosW+=1;
 	     		}
 	     	}
		//printf("%d %d\n", player.xPos, player.yPos);
		//

		/* Physics */
		int xCollStart = player.xPosW/TILE_SIZE;
		int xCollEnd = (player.xPosW+TILE_SIZE-1)/TILE_SIZE;
		int yCollStart = player.yPosW/TILE_SIZE;
		int yCollEnd = (player.yPosW+TILE_SIZE-1)/TILE_SIZE;
		int r,t;
		do{
			for(r=yCollStart;r<=yCollEnd;r++) {
				for(t=xCollStart;t<=xCollEnd;t++) {
					if(playerOutOfBounds()) {
						player.xPosW=player.xPosWLast;
						player.yPosW=player.yPosWLast;

					}
					if(map[t][r].coll==true) {
						printf("map[%d][%d].coll==%d\n",t,r,map[t][r].coll);
						//player.xPosW+=playerLeftOf(t,r);
						//player.xPosW-=playerRightOf(t,r);
						//player.yPosW-=playerBelow(t,r);
						//player.yPosW-=playerAbove(t,r);
						player.xPosW=player.xPosWLast;
						player.yPosW=player.yPosWLast;
						//collision resolution goes here?
						//player.xPosW-=t*TILE_SIZE;
						//player.yPosW-=r*TILE_SIZE;
					}
				}
			}
			lastPhysicsFrameMs+=physicsDeltaMs;	
		} while(lastPhysicsFrameMs + physicsDeltaMs <currentFrameMs);


		/* camera adjustment */
		if(player.xPosC+TILE_SIZE>camera.right) {
			if(camera.xPos+WINDOW_WIDTH<40*TILE_SIZE)
				camera.xPos+=camera.scroll;	
		}
		else if(player.xPosC<camera.left) {
			if(camera.xPos>0)
				camera.xPos-=camera.scroll;
		}
		if(player.yPosC<camera.top) {
			if(camera.yPos>0)
				camera.yPos-=camera.scroll;
		}
		else if(player.yPosC+TILE_SIZE>camera.bottom) {
			if(camera.yPos+WINDOW_WIDTH<40*TILE_SIZE)
				camera.yPos+=camera.scroll;
		}
		/* compute player position relative to camera */
		player.xPosC = player.xPosW-camera.xPos;
		player.yPosC = player.yPosW-camera.yPos;

 	     	/* draw backgrounds, handle parallax */
 	     	int xStart=camera.xPos-1/TILE_SIZE;
 	     	int yStart=camera.yPos-1/TILE_SIZE;
 	     	int xFinish=(camera.xPos+WINDOW_WIDTH)+1/TILE_SIZE;
 	     	int yFinish=(camera.yPos+WINDOW_HEIGHT)+1/TILE_SIZE;

		/* For safety. Ensures no out-of-bounds errors */
 	     	if(xStart<0) xStart=0;
 	     	if(yStart<0) yStart=0;
 	     	if(xFinish>40) xFinish=40;
 	     	if(yFinish>40) yFinish=40;
		
		int k,l;
 	     			
 	     	

 	     	/*for(l=xStart;l<xFinish;l++) {
 	     		 for(k=yStart;k<yFinish;k++) {
 	     			glDrawSprite( bgTex[map[l][k].image],
 	     				TILE_SIZE*l-camera.xPos , 
 	     				TILE_SIZE*k+camera.yPos, TILE_SIZE , TILE_SIZE );
 	     		}
 	     	}*/
		
		for(k=0;k<40;k++) {
 	     		for(l=0;l<40;l++) {
 	     			glDrawSprite( bgTex[map[l][k].image],
 	     				TILE_SIZE*l-camera.xPos , 
 	     				TILE_SIZE*k-camera.yPos, TILE_SIZE , TILE_SIZE );
 	     		}
 	     	}
 	     	/* draw sprites */
		
 	     	//glDrawSprite(sprites[0],player.xPos,player.yPos,50,50);
		animDraw(&player.anim,player.xPosC,player.yPosC,TILE_SIZE,TILE_SIZE);
		if(!player.anim.isPlaying) animReset(&player.anim);
 	     	/* draw foregrounds, handle parallax */
 	     	/* Game logic goes here */
 	     	
 		SDL_GL_SwapWindow( window );
	}

	SDL_Quit();

	return 0;
}

/* Tests player position */
int playerOutOfBounds() {
	int playerOutOfBounds=0;
	if(player.xPosW<0 || player.xPosW+TILE_SIZE>MAP_WIDTH*TILE_SIZE)
		playerOutOfBounds=1;
	if(player.yPosW<0 || player.yPosW+TILE_SIZE>MAP_HEIGHT*TILE_SIZE)
		playerOutOfBounds=1;
	return playerOutOfBounds;
}

int playerLeftOf(int m, int n) {
	int overlap=(player.xPosW+TILE_SIZE)-m*TILE_SIZE;
	printf("LEFT OF: %d\n", overlap);
	if(overlap>0)
		return overlap;
	else return 0;
}

int playerRightOf(int m, int n) {
	int overlap=player.xPosW-(m*TILE_SIZE+TILE_SIZE);
	printf("RIGHT OF: %d\n", overlap);
	if(overlap<0)
		return overlap;
	else return 0;
}

int playerAbove(int m, int n) {
	int overlap=(player.yPosW+TILE_SIZE)-n*TILE_SIZE;
	printf("ABOVE: %d\n", overlap);
	if(overlap>0)
		return overlap;
	return 0;
}

int playerBelow(int m, int n) {
	int overlap=player.yPosW-(n*TILE_SIZE+TILE_SIZE);
	printf("BELOW: %d\n", overlap);
	if(overlap>0)
		return overlap;
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

