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
#define NUMBER_OF_ENEMIES 2
#define DEBUG 1 

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
	int hitpoints;
	int damage;
	int xPosTile;
	int xPosTileLast;
	int yPosTile;
	int yPosTileLast;
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

typedef struct {
	AnimData anim;
	int hitpoints;
	int damage;
	int xPosTile;
	int xPosTileLast;
	int yPosTile;
	int yPosTileLast;
	int xPosC;
	int yPosC;
} Enemy;

typedef struct Tile {
	int image;
	bool coll;
	bool hasEnemy;
	int enemyID;
	bool hasItem;
} Tile;

struct {
	int turn;
	float lastTurnMs;
} gamestate;

Tile map[MAP_WIDTH][MAP_HEIGHT];
GLuint bgTex[16];
GLuint sprites[32];
char shouldExit = 0;
Player player;
Camera camera;
Enemy enemies[NUMBER_OF_ENEMIES];

void animTick(AnimData*, float);
void animSet(AnimData*, AnimDef*);
void animReset(AnimData*);
void animDraw(AnimData*, int,int,int,int);
void bgDraw();
void playerUpdatePixPos();
void playerUpdate(const unsigned char*,Uint32);
void mapUpdateEnemyData();
void playerBoundsCorrection();
void enemiesDraw(int);
void enemiesUpdate();
void enemiesUpdatePixPos();
void turn();
void enemiesDebug();
void enemiesSavePositions();
void enemyPositionReset(Enemy*);
void cameraUpdate();
int combat(Enemy*);

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
	/* Game state stuff */
	gamestate.turn=0;
	gamestate.lastTurnMs=0.0f;

	/* Initialize camera */	
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
	playerAnim.numFrames=1;
	//player.anim.def = &playerAnim;
	animSet(&player.anim ,&playerAnim);
	player.xPosTile=3;
	player.yPosTile=3;
	player.xPosTileLast=3;
	player.yPosTileLast=3;
	player.hitpoints=20;
	player.damage=6;

		
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
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
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
			map[m][n].hasEnemy = false;
			map[m][n].hasItem = false;
			map[m][n].enemyID=-1;
			if(map[m][n].image>0) map[m][n].coll = true;
			else map[m][n].coll = false;		
		}
	}
	//printf("\ncoll val at [1][1]: %d\n", map[1][1].coll);
	
	/* Initialize enemy variables */
	//Enemy enemies[8];
	AnimDef enemyAnim1;
	enemyAnim1.name = "enemy1";
	enemyAnim1.frames[0].frameNum=1;
	enemyAnim1.frames[0].frameTime=1.0;
	enemyAnim1.numFrames=1;
	animSet(&enemies[0].anim, &enemyAnim1);
	animSet(&enemies[1].anim, &enemyAnim1);
	enemies[0].xPosTile=0;
	enemies[0].yPosTile=0;
	enemies[0].xPosTileLast=0;
	enemies[0].yPosTileLast=0;
	enemies[0].hitpoints=12;
	enemies[0].damage=4;
	enemies[1].xPosTile=33;
	enemies[1].xPosTileLast=33;
	enemies[1].yPosTile=28;
	enemies[1].yPosTileLast=28;
	enemies[1].hitpoints=12;
	enemies[1].damage=4;
	enemiesUpdatePixPos();
	mapUpdateEnemyData();

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
		player.xPosTileLast=player.xPosTile;
		player.yPosTileLast=player.yPosTile;
		enemiesSavePositions();
		currentFrameMs = SDL_GetTicks();
		float deltaTime = (currentFrameMs - lastFrameMs)/1000.0f;

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

 	     	/* update positions of player based on input
		 * implicitly handles enemies */
		playerUpdate(kbState, currentFrameMs);
		
		/* Physics */
		int ii;
		do {
			if(map[player.xPosTile][player.yPosTile].coll) {
				playerPositionReset();
			}
			/*for(ii=0;NUMBER_OF_ENEMIES;ii++) {
				if(map[enemies[ii].xPosTile][enemies[ii].yPosTile].coll)
					enemyPositionReset(&enemies[ii]);
			}*/
			//
			lastPhysicsFrameMs+=physicsDeltaMs;
		} while(lastPhysicsFrameMs + physicsDeltaMs<currentFrameMs);
		
		playerUpdatePixPos();

		/* camera adjustment */
		cameraUpdate();

 	     	/* draw backgrounds, handle parallax */
		bgDraw();

 	     	/* draw sprites */
		
		//enemiesUpdate();
		enemiesUpdatePixPos();
		enemiesDraw(NUMBER_OF_ENEMIES);
		animDraw(&player.anim,player.xPosC,player.yPosC,TILE_SIZE,TILE_SIZE);
		//if(!player.anim.isPlaying) animReset(&player.anim);
 	     	/* draw foregrounds, handle parallax */
 	     	/* Game logic goes here */
 	     	
 		SDL_GL_SwapWindow( window );
	}

	SDL_Quit();

	return 0;
}

void enemiesDraw(int numEnemies) {
	int i;
	for(i=0;i<numEnemies;i++) {
		if(enemies[i].hitpoints>0) animDraw(&enemies[i].anim, 
				enemies[i].xPosC, enemies[i].yPosC,
				TILE_SIZE, TILE_SIZE);
	}
}

int playerPositionReset() {
	player.xPosTile=player.xPosTileLast;
	player.yPosTile=player.yPosTileLast;

}
int debugPosition() {
	printf("   LAST:%d %d\n", player.xPosTileLast,player.yPosTileLast);
	printf("   CURR:%d %d\n", player.xPosTile, player.yPosTile);
}

int combat(Enemy* enemy) {
	player.hitpoints-=enemy->damage;
	enemy->hitpoints-=player.damage;
	if(enemy->hitpoints<=0) {
		map[enemy->xPosTile][enemy->yPosTile].hasEnemy=false;
		map[enemy->xPosTile][enemy->yPosTile].enemyID=-1;
	}
	return 0;
}

int turnLegal(Uint32 currentTime) {
	if(currentTime-gamestate.lastTurnMs>75) {
		gamestate.turn++;
		gamestate.lastTurnMs=currentTime;
		printf("TURN: %d\n", gamestate.turn);
		return 1;
	}
	else return 0;
}
/* No params cos global everything */
void cameraUpdate() {
	if(player.xPosC+TILE_SIZE>camera.right) {
			if(camera.xPos+WINDOW_WIDTH<MAP_WIDTH*TILE_SIZE)
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
			if(camera.yPos+WINDOW_HEIGHT<MAP_HEIGHT*TILE_SIZE)
				camera.yPos+=camera.scroll;
		}
}
void bgDraw() {
	int k,l;
	for(k=0;k<40;k++) {
     		for(l=0;l<40;l++) {
     			glDrawSprite( bgTex[map[l][k].image],
     				TILE_SIZE*l-camera.xPos , 
     				TILE_SIZE*k-camera.yPos, TILE_SIZE , TILE_SIZE );
     		}
     	}
}

/* Tests player position */
void playerBoundsCorrection() {
	if(player.xPosTile<0 )
		player.xPosTile=0;
	if(player.xPosTile>MAP_WIDTH-1)
		player.xPosTile=MAP_WIDTH-1;
	if(player.yPosTile<0)
		player.yPosTile=0;
	if(player.yPosTile>MAP_HEIGHT-1)
		player.yPosTile=MAP_HEIGHT-1;
}

/* Since player input controls player's character, they are
 * coupled. */
void playerUpdate(const unsigned char* kbState, Uint32 currentFrameMs) {
	if(kbState[SDL_SCANCODE_RIGHT]) {
		if(turnLegal(currentFrameMs)) {
			player.xPosTile++;
			playerBoundsCorrection();
			turn();
			if(DEBUG) debugPosition();
			if(DEBUG) enemiesDebug();
		}
	}
	else if(kbState[SDL_SCANCODE_LEFT]) {
		if(turnLegal(currentFrameMs)) {
			player.xPosTile--;
			playerBoundsCorrection();
			turn();
			if(DEBUG) debugPosition();
			if(DEBUG) enemiesDebug();
		}
	}
	else if(kbState[SDL_SCANCODE_UP]) {
		if(turnLegal(currentFrameMs)) {
			player.yPosTile--;
			playerBoundsCorrection();
			turn();
			if(DEBUG) debugPosition();
			
			if(DEBUG) enemiesDebug();
		}
	} 
	else if(kbState[SDL_SCANCODE_DOWN]) {
		if(turnLegal(currentFrameMs)) {
			player.yPosTile++;
			playerBoundsCorrection();
			turn();
			if(DEBUG) debugPosition();

			if(DEBUG) enemiesDebug();
		}
	}
}

/* Updates the player's character's pixel position
 * based on its tile position */
void playerUpdatePixPos() {
	player.xPosC=player.xPosTile*TILE_SIZE-camera.xPos;
	player.yPosC=player.yPosTile*TILE_SIZE-camera.yPos;
}

/* Updates every enemy's pixel position
 * based on its tile position */
void enemiesUpdatePixPos() {
	int i;
	Enemy* curEnemy;
	for(i=0;i<NUMBER_OF_ENEMIES;i++) {
		curEnemy=&enemies[i];
		curEnemy->xPosC=curEnemy->xPosTile*TILE_SIZE-camera.xPos;
		curEnemy->yPosC=curEnemy->yPosTile*TILE_SIZE-camera.yPos;
		//enemies[i].xPosC=enemies[i].xPosTile*TILE_SIZE-camera.xPos;
	}
}

/* Most changes to game state occurs during a turn. This should
 * only execute after some collision checks have happened.
 * Game should not execute this if player is colliding into a wall,
 * for example. */
void turn() {
	int x=player.xPosTile;
	int y=player.yPosTile;
	Enemy* enemy;
	//first take care of player initiated combat
	if(map[x][y].hasEnemy==true) {
		if(DEBUG) printf("COMBAT!!!!");
		enemy=&(enemies[map[x][y].enemyID]);
		playerPositionReset();
		combat(enemy);
	}
	//then allow all monsters to move
	enemiesUpdate();
}
void enemiesSavePositions() {
	int i;
	for(i=0;i<NUMBER_OF_ENEMIES;i++) {
		enemies[i].xPosTileLast=enemies[i].xPosTile;
		enemies[i].yPosTileLast=enemies[i].yPosTile;
	}
}
void enemyPositionReset(Enemy* enemy) {
	//int x=enemy->xPosTile;
	//int y=enemy->yPosTile;
	enemy->xPosTile=enemy->xPosTileLast;
	enemy->yPosTile=enemy->yPosTileLast;
}
/* Contains enemy logic. Every time this is executed,
 * all enemies should do something. They can:
 * 	1. Do nothing
 * 	2. Move randomly
 * 	3. Move a long a path(?)
 */
void enemiesUpdate() {
	int i;
	float r;
	Enemy* curEnemy;
	for(i=0;i<NUMBER_OF_ENEMIES;i++) {
		curEnemy=&enemies[i];
		r=rand()/(float)RAND_MAX;
		if(r<0.8) {
			if(r<0.4) { //move along x-axis
			       	if(curEnemy->xPosTile<player.xPosTile)
					curEnemy->xPosTile++;
				else curEnemy->xPosTile--;
			}
			else { //move along y-axis
				if(curEnemy->yPosTile<player.yPosTile)
					curEnemy->yPosTile++;
				else curEnemy->yPosTile--;
			}
		} //else do nothing

	}
	//mapUpdateEnemyData();
}

void enemiesDebug() {
	int i;
	for(i=0;i<NUMBER_OF_ENEMIES;i++) {
		printf("\tenemy[%d] tilepos: %d %d\n", i,enemies[i].xPosTile,enemies[i].yPosTile);
		printf("\tenemy[%d] cPos: %d %d\n", i,enemies[i].xPosC,enemies[i].yPosC);
		printf("\tPLAYER POS: %d %d\n", player.xPosC,player.yPosC);
		printf("\tCAMERA POS: %d %d\n", camera.xPos,camera.yPos);
	}
}

/* This keeps the map aware of enemy locations. */
void mapUpdateEnemyData() {
	int i;
	int x;
	int y;
	Enemy curEnemy;
	for(i=0;i<NUMBER_OF_ENEMIES;i++) {
		curEnemy=enemies[i];
		x=curEnemy.xPosTile;
		y=curEnemy.yPosTile;
		map[curEnemy.xPosTileLast][curEnemy.yPosTileLast].hasEnemy=false;
		map[curEnemy.xPosTileLast][curEnemy.yPosTileLast].enemyID=-1;
		map[x][y].hasEnemy=true;
		map[x][y].enemyID=i;
	}
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

