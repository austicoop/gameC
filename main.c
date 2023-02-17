#include <stdio.h>
#include <time.h>
#include <math.h>
#include "src/include/SDL2/SDL.h"
#include "src/include/SDL2/SDL_image.h"
#include "main.h"
#include "status.h"

#define GRAVITY 0.35f

void loadGame(GameState *game) {
    SDL_Surface *surface = NULL;

    //Load images and create rendering textures from them
    surface = IMG_Load("images/star.png");
    if(surface == NULL) {
        printf("Cannot find star.png!\n\n");
        SDL_Quit();
        exit(1);
    }

    game->star = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

//******///*********LOOK UP HOW TO LOAD INDIVIDUAL SPRITE SHEETS /////******/////****************************
    
    for (int i = 0; i < 7; i++) {
        char buf[21];                                   //21 bytes to store string
        snprintf(buf, 21, "images/Walk-%d-0.png", i);   //puts string into buffer
        surface = IMG_Load(buf);
        if(surface == NULL) {
            printf(buf);
            printf("Cant find Walk-%i-0.png!\n\n", i);
            SDL_Quit();
            exit(1);
        }
        game->manFrames[i] = SDL_CreateTextureFromSurface(game->renderer, surface);
        SDL_FreeSurface(surface);
    }
    
//******///*********LOOK UP HOW TO LOAD INDIVIDUAL SPRITE SHEETS /////******/////****************************
    surface = IMG_Load("images/Tile_03.png");
    if(surface == NULL) {
        printf("Cannot find Tile.png!\n\n");
        SDL_Quit();
        exit(1);
    }
    game->brick = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //Loads fonts
    game->font = TTF_OpenFont("fonts/WinterSong-owRGB.ttf", 48);
    if(!game->font) {
        printf("Cannot find font file\n\n");
        SDL_Quit();
        exit(1);
    }

    game->label = NULL;
    
    game->man.x = 320-40;
    game->man.y = 240-40;
    game->man.dx = 0;
    game->man.dy = 0;
    game->man.onLedge = 0;
    game->man.animFrame = 0;
    game->man.facingLeft = 1;
    game->man.slowingDown = 0;
    game->man.lives = 3;
    game->statusState = STATUS_STATE_LIVES;

    init_status_lives(game);
    
    game->time = 0;

    //init stars
    for (int i = 0; i < 100; i++) {
        game->stars[i].x = rand()%640;
        game->stars[i].y = rand()%480;
    }

    //init ledges
    for (int i = 0; i < 100; i++) {
        game->ledges[i].w = 256;
        game->ledges[i].h = 64;
        game->ledges[i].x = i * 256;
        game->ledges[i].y = 400;
    }
    game->ledges[99].x = 350;
    game->ledges[99].y = 200;

    game->ledges[98].x = 350;
    game->ledges[98].y = 350;
}

void process(GameState *game) {
    //add time
    game->time++;

    if(game->time > 120) {
        shutdown_status_lives(game);
        game->statusState = STATUS_STATE_GAME;
    }

    if(game->statusState == STATUS_STATE_GAME) {
        //man movement
        Man *man = &game->man;
        man->x += man->dx;
        man->y += man->dy;
    //change walking animation
        if (man->dx != 0 && man->onLedge && !man->slowingDown) {
            if (game->time % 8 == 0) {
                if(man->animFrame == 6) {
                    man->animFrame = 0;
                }
                else {
                    man->animFrame++;
                }
            }
        }
        //apply gravity to player
        man->dy += GRAVITY;
    }
}

void collisionDetect(GameState *game) {
    //Check for collision with any ledges (brick blocks) 
    for (int i = 0; i < 100; i++) {
        float mw = 48, mh = 48; //previously both 48
        float mx = game->man.x, my = game->man.y;
        float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

        if (my + mh > by && my < by + bh) {
            //rubbing against right edge
            if (mx < bx + bw && mx + mw > bx + bw && game->man.dx < 0) {
                //correct x
                game->man.x = bx + bw;
                mx = bx + bw;

                game->man.dx = 0;
            }
            //rubbing against left edge
            else if (mx + mw > bx && mx < bx && game->man.dx > 0) {
                //correct x
                game->man.x = bx - mw;
                mx = bx - mw;

                game->man.dx = 0;
            }
        }
        if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw) {
            //are we bumping our head?
            if (my < by + bh && my > by && game->man.dy < 0) {
                //correct y
                game->man.y = by + bh;
                my = by + bh;
                //bumped our head, stop any jump velocity
                game->man.dy = 0;
                game->man.onLedge = 1;
            }
        }
        if (mx + mw > bx && mx < bx + bw) {
            //are we landing on the ledge
            if (my + mh > by && my < by && game->man.dy > 0) {
                //correct y
                game->man.y = by - mh;
                my = by - mh;
                //landed on this ledge, stop any jump velocity
                game->man.dy = 0;
                game->man.onLedge = 1;
            }
        }
    }
}

int processEvents(SDL_Window *window, GameState *game) {
    int done = 0;
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_WINDOWEVENT_CLOSE: {
                if(window) {
                    SDL_DestroyWindow(window);
                    window = NULL;
                    done = 1;
                }
            } break;
            case SDL_KEYDOWN: {
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE: 
                        done = 1;
                    break;
                    case SDLK_UP:
                        if (game->man.onLedge) {
                            game->man.dy = -8;
                            game->man.onLedge = 0;
                        }
                    break;
                }
            } 
            break;
            case SDL_QUIT:
                done = 1;
            break;
        }
    }  

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    //more jumping - more vertical if jump is held down
    if(state[SDL_SCANCODE_UP]) {
        game->man.dy -= 0.2f;
    }
    //walking
    if(state[SDL_SCANCODE_LEFT]) {
        game->man.dx -= 0.25;
        if (game->man.dx < -3) {
            game->man.dx = -3;
        }
        game->man.facingLeft = 1;
        game->man.slowingDown = 0;
    }
    else if(state[SDL_SCANCODE_RIGHT]) {
        game->man.dx += 0.25;
        if(game->man.dx > 3) {
            game->man.dx = 3;
        }
        game->man.facingLeft = 0;
        game->man.slowingDown = 0;
    }
    else {
        game->man.animFrame = 0;
        game->man.dx *= 0.8f;
        game->man.slowingDown = 1;
        if(fabsf(game->man.dx) < 0.1f) {
            game->man.dx = 0;
        }
    }
    // if(state[SDL_SCANCODE_DOWN]) game->man.y += 10;
    // if(state[SDL_SCANCODE_UP]) game->man.y -= 10;

    return done;  
}

void doRender(SDL_Renderer *renderer, GameState *game) {
    
    if(game->statusState == STATUS_STATE_LIVES) {
        draw_status_lives(game);
    }
    else if(game->statusState == STATUS_STATE_GAME) {

        //set the drawing color to blue
        SDL_SetRenderDrawColor(renderer, 128, 128, 255, 255);

        //Clear the screen (to blue)
        SDL_RenderClear(renderer);

        //set drawing color to white
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        //draw ledges
        for (int i = 0; i < 100; i++) {
            SDL_Rect ledgeRect = { game->ledges[i].x, game->ledges[i].y, game->ledges[i].w, game->ledges[i].h };
            SDL_RenderCopy(renderer, game->brick, NULL, &ledgeRect);
        }
        
        //draw rectangle at mans position
        SDL_Rect rect = { game->man.x, game->man.y, 48, 48 }; //***** Possibly make character bigger
        SDL_RenderCopyEx(renderer, game->manFrames[game->man.animFrame], NULL, &rect, 0, NULL, (SDL_RendererFlip)(game->man.facingLeft == 1));

        // //draw star image
        // for (int i = 0; i < 100; i++) {
        //     SDL_Rect starRect = {game->stars[i].x, game->stars[i].y, 64, 64};
        //     SDL_RenderCopy(renderer, game->star, NULL, &starRect);
        // }
    }
    //Show the screen what has been drawn
    SDL_RenderPresent(renderer);   
}

int main(int argc, char** argv) {
    GameState gameState;
    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Init(SDL_INIT_VIDEO);

    //initializes a random number generator
    srand((int)time(NULL));

    window = SDL_CreateWindow("Game Window",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            640,
                            480,
                            0
                            );
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    gameState.renderer = renderer;

    TTF_Init(); //Required! Initialize TTF

    loadGame(&gameState);

    // Window is open: enter program loop (see SDL_PollEvent)
    int done = 0;

    while(!done) {
        //Check for events
        done = processEvents(window, &gameState);

        process(&gameState);
        collisionDetect(&gameState);

        //Render display
        doRender(renderer, &gameState);

        //don't burn up the CPU. 100 milliseconds per frame
        //SDL_Delay(10);
    }

    //Shutdown game and unload all memory
    SDL_DestroyTexture(gameState.star);
    for (int i = 0; i < 7; i++) {
        SDL_DestroyTexture(gameState.manFrames[i]);
    }
    SDL_DestroyTexture(gameState.brick);
    if(gameState.label != NULL) {
        SDL_DestroyTexture(gameState.label);
    }
    TTF_CloseFont(gameState.font);

    //Close and destroy the window
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);

    TTF_Quit();

    //Clean up
    SDL_Quit();
    return 0;
}