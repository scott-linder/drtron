/*=============================================================================
#     FileName: drtron.h
#         Desc:
#       Author: Scott Linder
#        Email: scott <at> hamstercafe.com
#     HomePage: www.hamstercafe.com
#      Version: 0.0.1
#   LastChange: 2013-01-26 17:30:44
#      History:
=============================================================================*/

// CONSTANTS //
//Player count bounds
#define MIN_PLS 2
#define MAX_PLS 4
//Directions
#define UP -map.width
#define DOWN map.width
#define LEFT -1
#define RIGHT 1
//Map tile markers
#define FLOOR ' '
#define WALL '#'
#define ADDONE '%'
#define DEF_PL_TEX '+'

// ENUMS //
//Gamemodes
enum gm {
    CLASSIC,
    WORM,
};
//Return values of play_game()
enum playgame_ret {
    RESUME, //Used intra-game to exit a menu withuot exiting game
    REPEAT, //Play again with same settings
    NEW,      //Play again with new settings
    EXIT,     //Exit program entirely
};

// STRUCTS //
//Organize data relevant to instantiation of a game
typedef struct {
    //CLASSIC or WORM
    int gamemode;
    //Number of players in range 2-4
    int num_pls;
    //Array of names for the players
    char* pl_names[MAX_PLS];
    //Use fullscreen?
    bool fullscreen;
    //Map dimensions (ignored if fullscreen)
    int width, height;
} settings_t;

//Hold maps and dimensions thereof
typedef struct {
    //Dimensions
    int width, height;
    /*Map is represented as array in the following manner:
    * Ex. 3x3 map -> tiles[9]
    *       [0][1][2]
    *       [3][4][5]
    *       [6][7][8]
    * Base is map without any players and is used for drawing only
    * Pl_col is map of collisions: it "overlays" base and each position contains true(colliding tile) or false (non-colliding tile)
    */
    char *base;
    bool *pl_col;
} map_t;

//Represents one "node" of a player's "worm"
struct player_node{
    //Postition of node in map
    int pos;
    //Display character
    char tex;
    //Pointer to next node
    struct player_node *next;
};

//Represents actual player and organizes relevant data
typedef struct {
    //Name of the player (to be displayed at head of 'worm')
    char *name;
    //We will make this a pseudo constant later to avoid calls to strlen()
    int name_len;
    //To remember how far into the name we are so far
    int name_index;

    //Direction of player: UP,DOWN,LEFT,RIGHT
    int dir;
    //I wonder what this one is
    int score;
    //Number of struct player_nodes queued to be added to the player's chain
    int nodes_pending;
    //Linked list of player nodes containing positions and display characters
    //Root is always pointing to the 'head' of the 'worm' of the player
    struct player_node *root;

    //Is this player out of play (stopped)?
    bool is_out;
} player_t;

// PROTOTYPES //
void get_new_settings(settings_t*);
void cleanup(settings_t*);
enum playgame_ret play_game(settings_t*);
void draw_map(map_t, player_t*, int);
enum playgame_ret ingame_menu(void);
void cleanup_game(map_t*, player_t[], int);
