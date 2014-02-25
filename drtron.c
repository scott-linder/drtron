/*
 * drtron.c
 * A small tron/worm clone in C using ncurses.
 * Authors:
 *  Scott Linder
 */

#include <menu.h>
#include <form.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "drtron.h"

int main(int argc, char **argv)
{
    /* We reuse these settings between games. */
    settings_t settings;
    /* We switch on the return of playgame to decide what action to take. */
    enum playgame_ret game_term = NEW;

    /* Initialize curses because we will use it everywhere. */
    initscr();
    start_color();
    cbreak();
    noecho();

    while (game_term != EXIT)
    {
        if (game_term == REPEAT)
        {
            /* Use the same settings for a new game. */
            game_term = play_game(&settings);
        }
        else if (game_term == NEW)
        {
            /* Let user input new settings for a new game. */
            get_new_settings(&settings);
            game_term = play_game(&settings);
        }
        else 
        {
            /* Just exit if we don't understand the response. */
            fputs("Bad return, dieing", stderr);
            break;
        }
    }

    /* End ncurses. */
    endwin();
    /* Clean up dynamic structures in settings. */
    cleanup_settings(&settings);

    return EXIT_SUCCESS;
}

/* Alter settings in place based upon user input through forms. */
void get_new_settings(settings_t *settings)
{
    /* Iterators and key pressed. */
    int i, j, key;
    /* Container dimensions. */
    int height, width;
    /* Enums for first two fields. */
    const char* GM[] = { "Classic", "Worm", NULL };
    const char* NP[] = { "2", "3", "4", NULL };
    WINDOW *container;  /* So we can have a border. */
    WINDOW *form_win;   /* So we can have the form. */
    FORM *main_form;    /* Actual form. */
    const int NUM_FIELDS = 7;
    FIELD *fields[NUM_FIELDS + 1];   /* Null terminated array of form fields. */
    bool done = false;  /* Allow user to break out of input loop. */
    char* buff; /* So we can temporarily hold on to forms field buffers. */

    /* Field colors. */
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    /* Put together our form fields. */
    i = 1;  /* So we can auto-space our fields. */

    fields[0] = new_field(1, 10, i++ * 3, 4, 0, 0);  /* Gamemode. */
        set_field_type(fields[0], TYPE_ENUM, GM, FALSE, FALSE); /* Restrict values of this field to those defined in GAMEMODES. */
        set_field_back(fields[0], COLOR_PAIR(1));                      /* Make it stand out. */
        field_opts_off(fields[0], O_EDIT);                             /* Force user to use our enums only. */
        field_opts_off(fields[0], O_AUTOSKIP);                         /* Don't auto-jump to next field on completion. */

    fields[1] = new_field(1, 1,  i++ * 3, 4, 0, 0);  /* Number of players. */
        set_field_type(fields[1], TYPE_ENUM, NP, FALSE, FALSE);
        set_field_back(fields[1], COLOR_PAIR(1));
        field_opts_off(fields[1], O_EDIT);
        field_opts_off(fields[1], O_AUTOSKIP);

    /* Last four fields are for player names. */
    for (i=2; i < 6; i++)
    {
        fields[i] = new_field(1, 10, (i + 1) * 3, 4, 0, 0);
            set_field_type(fields[i], TYPE_ALPHA, 0); /* strings of alphabetic characters. */
            set_field_back(fields[i], COLOR_PAIR(1));
            field_opts_off(fields[i], O_AUTOSKIP);
    }

    /* Because field buffers update on exit, we must force the user to exit all fields before exiting the dialog. */
    fields[6] = new_field(1, 4, ++i * 3, 7, 0, 0);
        field_opts_off(fields[6], O_EDIT); /* This is basically a button, so don't let the user edit it.... */
        set_field_buffer(fields[6], 0, "DONE"); /* Also set text of the "button". */

    fields[7] = NULL;

    /* Now make our form; scale it and it's container, put it in a sub-window of the container and post it. */
    main_form = new_form(fields);

    scale_form(main_form, &height, &width);

    container = newwin(height + 2, width + 9, (LINES - (height + 2)) / 2, (COLS - (width + 10)) / 2);
    keypad(stdscr, TRUE); /* Was acting funny when assigned to our new window. */

    set_form_win(main_form, container);

    form_win = derwin(container, height, width, 0, 2); /* Subwindow offset 2 left. */
    set_form_sub(main_form, form_win);

    box(container, 0, 0); /* Standard border. */

    post_form(main_form);
    wrefresh(container);

    /* Add some labels. */
    mvwprintw(container, 1, 5, "DrTron Setup");
    mvwprintw(container, 2, 3, "Gamemode: ");
    mvwprintw(container, 5, 3, "Number of Players: ");
    for (i=1; i <= 4; i++)
    {
        mvwprintw(container, 5 + i * 3, 4, "Player %d Name: ", i);
    }
    wrefresh(container);

    /* Load first value for first two fields. */
    form_driver(main_form, REQ_NEXT_CHOICE);
    form_driver(main_form, REQ_NEXT_FIELD);
    form_driver(main_form, REQ_NEXT_CHOICE);
    /* Start at first field. */
    form_driver(main_form, REQ_FIRST_FIELD);
    /* Now let the user take over. */
    while (!done)
    {
        key = getch();
        switch(key)
        {
            /* Tab key. */
            case 9:
            case KEY_DOWN:
                form_driver(main_form, REQ_NEXT_FIELD);
                break;
            case KEY_UP:
                form_driver(main_form, REQ_PREV_FIELD);
                break;
            case KEY_LEFT:
                form_driver(main_form, REQ_PREV_CHOICE);
                break;
            case KEY_RIGHT:
                form_driver(main_form, REQ_NEXT_CHOICE);
                break;
            case KEY_BACKSPACE:
                form_driver(main_form, REQ_DEL_PREV);
                break;
            case 10:
                /* User pressed enter, allow exit if on index 6. */
                if (field_index(current_field(main_form)) == 6)
                {
                    done = true;
                }
                break;
            default:
                form_driver(main_form, key);
                break;
        }
        box(container, 0,0);
        wrefresh(container);
    }

    /* Buffers for the fields should have pre-verified data. */
    if (strncmp(field_buffer(fields[0], 0), GM[0], strlen(GM[0])) == 0)
    {
        settings->gamemode = CLASSIC;
    }
    else
    {
        settings->gamemode = WORM;
    }

    settings->num_pls = atoi(field_buffer(fields[1], 0));

    /* We need to save a copy of each player name into our own buffers as the forms ones are removed along with the fields. */
    for (i=0; i < MAX_PLS; i++)
    {
       buff = field_buffer(fields[i + 2], 0);
       /* This buffer is null terminated, but may contain trailing spaces; we need to fix that. */
       for (j=0; j < strlen(buff); j++)
       {
           if (buff[j] == ' ') buff[j] = '\0';
       }
       /* Make a new buffer, remembering space for the null terminator. */
       settings->pl_names[i] = (char *) malloc(strlen(buff) * sizeof(char) + 1);
       /* Copy the forms buffer into our new one. */
       strcpy(settings->pl_names[i], buff);
    }

    /* TODO: for now we will always assume fullscreen for simplicity. */
    settings->fullscreen = true;

    /* Settings is now populated; cleanup and return. */
    unpost_form(main_form);
    free_form(main_form);
    for (i=0; i < NUM_FIELDS; i++)
    {
        free_field(fields[i]);
    }
    /* Free windows. */
    delwin(form_win);
    delwin(container);
    erase();
    refresh();
}

/* Cleanup pl_names in settings. */
void cleanup_settings(settings_t *settings)
{
    int i;
    for (i=0; i < MAX_PLS; i++)
    {
        free(settings->pl_names[i]);
    }
}

/* Run through one game based upon settings. */
/* RETURN:. */
enum playgame_ret play_game(settings_t *settings)
{
    int i;

    /* Ingame menu return status. */
    enum playgame_ret menu_ret;

    /* Map struct. */
    map_t map;

    /* Array of player structs. */
    player_t *players;
    /* Number of players. */
    int num_players = settings->num_pls;
    /* Default name (N replaced by player number). */
    const char* def_name = "PlayerN";

    /* Randomized to assign tiles. */
    int tile_rand;

    /* Value of key pressed during play. */
    int key;
    /* Number of players out of play. */
    int num_out = 0;
    /* Pointer to next node while traversing player body. */
    struct player_node *next_node;
    /* Variable to copy next_node into while processing. */
    struct player_node *cur_node;
    /* Position of last node while traversing player body. */
    int last_pos;
    /* Temp variable for swapping positions of nodes. */
    int temp;

    /* Setup map. */
    if (settings->fullscreen)
    {
        map.height = LINES - 1;
        map.width = COLS - 1;
    }
    else
    {
        map.height = settings->height;
        map.width = settings->width;
    }


    /* Now allocate our data-structures based on our settings. */
    map.base = (char *) malloc(map.width * map.height);
    map.pl_col = (bool *) malloc(map.width * map.height);

    players = (player_t *) malloc(num_players * sizeof(player_t));
    for (i=0; i < num_players; i++)
    {
        players[i].name = settings->pl_names[i];
        players[i].name_len = strlen(players[i].name);
        players[i].name_index = 0;
        /* Default empty names. */
        if (players[i].name_len == 0)
        {
            /* We don't need to hold on to the empty buffer, just free it and make new. */
            free(players[i].name);
            /* Add an extra char for null terminator. */
            players[i].name = (char *) malloc((strlen(def_name) + 1) * sizeof(char));
            strcpy(players[i].name, def_name);
            /* Reset name length. */
            players[i].name_len = strlen(players[i].name);
            /* Add the players number to the final printed character. */
            players[i].name[ players[i].name_len - 1 ] = '0' + i + 1;
        }
        /* Player starts with only one node. */
        players[i].nodes_pending = 0;
        /* Create root node of linked list for each player. */
        players[i].root = malloc(sizeof(struct player_node));
        /* Since there are no more struct player_nodes, root has a null pointer. */
        players[i].root->next = NULL;
        /* Set the character to be displayed for this root node. */
        players[i].root->tex = players[i].name[players[i].name_index++];
        /* All players begin in play. */
        players[i].is_out = false;
        /* Initialize player's score. */
        players[i].score = 0;
    }

    /* TODO: Allow player to customize their color pair. */
    /* Players 1-4. */
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    /* Other tiles. */
    init_pair(FLOOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(WALL, COLOR_WHITE, COLOR_BLACK);
    init_pair(ADDONE, COLOR_MAGENTA, COLOR_BLACK);

    /* Initialize with a default map. */
    for (i = 0; i < map.width * map.height; i++)
    {
        /* Surround the map in walls. */
        if ((i < map.width) || (i > map.width * (map.height-1)) || ((i+1) % map.width == 0) || ((i) % map.width == 0))
        {
            map.base[i] = WALL;
            /* Players collide with walls. */
            map.pl_col[i] = true;
        }
        /* Fill it with floor or more. */
        else
        {
            tile_rand = rand() % 100;
            if (tile_rand > 5 || settings->gamemode == CLASSIC)
            {
                map.base[i] = FLOOR;
                map.pl_col[i] = false;
            }
            else
            {
                map.base[i] = ADDONE;
                map.pl_col[i] = false;
            }
        }
    }

    /* Place the players on the map and give them an initial direction. */
    switch(num_players)
    {
        case 4:
            /* Lower left. */
            players[3].root->pos = (map.width * map.height) - (map.width * (map.height/4)) + (map.width/4);
            players[3].dir = RIGHT;
        case 3:
            /* Upper right. */
            players[2].root->pos = (map.width * (map.height/4)) + (3*map.width/4);
            players[2].dir = LEFT;
        case 2:
            /* Lower right. */
            players[1].root->pos = (map.width * map.height) - (map.width * (map.height/4)) + (3*map.width/4);
            players[1].dir = UP;
            /* Upper left. */
            players[0].root->pos = (map.width * (map.height/4)) + (map.width/4);
            players[0].dir = DOWN;
            break;
        default:
            puts("Inproper number of players");
            cleanup_game(&map, players, num_players);
            /* Something wrong has occured if an improper number of players reaches this point. */
            return EXIT;
    }

    /* Non-blocking getch(). */
    nodelay(stdscr, TRUE);
    /* No cursor. */
    curs_set(0);

    /* Start game loop. */
    while (true)
    {
        /* Non blocking read of stdin; returns ERR if no key available. */
        key = getch();
        /* Process all queued user input since last update. */
        while (key != ERR)
        {
            /* Exit on <esc>. */
            if (key == 0x1B)
            {
                /* Prompt user for input. */
                menu_ret = ingame_menu();
                if (menu_ret == RESUME)
                {
                    /* User wants to keep playing this game. */
                    key = getch();
                    continue;
                }
                else
                {
                    /* User wants to do something else. */
                    cleanup_game(&map, players, num_players);
                    return menu_ret;
                }
            }

            /* Player one keybinds. */
            else if (key == 'w' && players[0].dir != DOWN  ) players[0].dir = UP;
            else if (key == 'a' && players[0].dir != RIGHT ) players[0].dir = LEFT;
            else if (key == 's' && players[0].dir != UP    ) players[0].dir = DOWN;
            else if (key == 'd' && players[0].dir != LEFT  ) players[0].dir = RIGHT;

            /* Player two keybinds. */
            else if (key == KEY_UP    && players[1].dir != DOWN) players[1].dir = UP;
            else if (key == KEY_LEFT  && players[1].dir != RIGHT) players[1].dir = LEFT;
            else if (key == KEY_DOWN  && players[1].dir != UP  ) players[1].dir = DOWN;
            else if (key == KEY_RIGHT && players[1].dir != LEFT) players[1].dir = RIGHT;
            /* To avoid processing of player 3/4 keybinds when they don't exist. */
            else if (num_players == 2) { key = getch(); continue; }

            /* Player three keybinds. */
            else if (key == 'y' && players[2].dir != DOWN) players[2].dir = UP;
            else if (key == 'g' && players[2].dir != RIGHT) players[2].dir = LEFT;
            else if (key == 'h' && players[2].dir != UP  ) players[2].dir = DOWN;
            else if (key == 'j' && players[2].dir != LEFT) players[2].dir = RIGHT;
            else if (num_players == 3) { key = getch(); continue; }

            /* Player four keybinds. */
            else if (key == 'p' && players[3].dir != DOWN) players[3].dir = UP;
            else if (key == 'l' && players[3].dir != RIGHT) players[3].dir = LEFT;
            else if (key == ';' && players[3].dir != UP  ) players[3].dir = DOWN;
            else if (key == '\'' && players[3].dir != LEFT ) players[3].dir = RIGHT;

            key = getch();
        }

        for (i=0; i < num_players; i++)
        {
            /* We are only concerned with players in play. */
            if (!players[i].is_out)
            {
                /* If the player can move to the requested tile. */
                if (map.pl_col[ players[i].root->pos + players[i].dir ] != true)
                {
                    /* Start out at root node. */
                    cur_node = players[i].root;
                    /* Save position of head so we can move the rest of the nodes along. */
                    last_pos = cur_node->pos;
                    /* Actually move root. */
                    cur_node->pos += players[i].dir;
                    /* Remember this tile now collides. */
                    map.pl_col[ cur_node->pos ] = true;

                    /* Check if square should add another player_node. */
                    if (map.base[ cur_node->pos ] == ADDONE || settings->gamemode == CLASSIC)
                    {
                        /* Replace more tile with floor. */
                        map.base[ cur_node->pos ] = FLOOR;
                        /* Make the player longer. */
                        players[i].nodes_pending++;
                    }

                    /* Now prime the loop with the next node. */
                    next_node = cur_node->next;

                    while (next_node != NULL)
                    {
                        /* For ease of reading. */
                        cur_node = next_node;

                        /* We need to swap position of current node with last_pos. */
                        temp = cur_node->pos;
                        cur_node->pos = last_pos;
                        last_pos = temp;

                        /* Now move on to the next node (will be null if we are at the end). */
                        next_node = cur_node->next;
                    }

                    /* last_pos is now empty so we don't want players colliding with it. */
                    map.pl_col[ last_pos ] = false;

                    /* Now cur_node contains the last node and last_pos holds its previous position. */
                    /* If there are nodes_pending to be added, we can add one to the position at temp. */
                    if (players[i].nodes_pending > 0)
                    {
                        /* We need to allocate a new struct player_node and link it into the list. */
                        cur_node->next = malloc(sizeof(struct player_node));
                        /* We are only concerned with this new node. */
                        next_node = cur_node->next;
                        cur_node = next_node;
                        /* Set it's position. */
                        cur_node->pos = last_pos;
                        /* Allow players to collide with it. */
                        map.pl_col[ cur_node->pos ] = true;
                        /* Remember it is the terminal node. */
                        cur_node->next = NULL;
                        /* Set its display character; next index of name or DEF_PL_TEX. */
                        if (players[i].name_len > players[i].name_index)
                        {
                            cur_node->tex = players[i].name[players[i].name_index++];
                        }
                        else
                        {
                            cur_node->tex = DEF_PL_TEX;
                        }
                        /* Remember that we have added another node. */
                        players[i].nodes_pending--;
                        /* And give the player a point. */
                        players[i].score++;
                    }
                }
                /* Player is not out of play, but is unable to move. */
                else
                {
                    /* So we make him out of play. */
                    players[i].is_out = true;
                    num_out++;
                }
            }
        }

        /* Check if only one remains. */
        if (num_out >= (num_players - 1))
        {
            if (settings->gamemode == CLASSIC)
            {
                for (i=0; i < num_players; i++)
                {
                    if (!players[i].is_out)
                    {
                        attron(COLOR_PAIR(i + 1));
                        mvprintw(2, 2, "%s doesn't suck!", players[i].name);
                        attroff(COLOR_PAIR(i + 1));
                        break;
                    }
                }
                i = 1; /* Set i to how many lines we printed. */
            }
            else if (settings->gamemode == WORM)
            {
                for (i=0; i < num_players; i++)
                {
                    attron(COLOR_PAIR(i + 1));
                    mvprintw(2 + i, 2, "%s scored %d points!", players[i].name, players[i].score);
                    attroff(COLOR_PAIR(i + 1));
                }
            }
            refresh();
            cleanup_game(&map, players, num_players);
            nodelay(stdscr, FALSE);
            mvprintw(2 + i, 2, "Press any key to continue...");
            getch();
            return REPEAT;
        }

        /* Advance frame. */
        draw_map(map, players, num_players);
        usleep(60000);
    }
}

void draw_map(map_t map, player_t *players, int num_players)
{
    int i;
    /*  To traverse player lists. */
    /* Pointer to next node while traversing player body. */
    struct player_node *next_node;
    /* Variable to dereference next_node into while processing. */
    struct player_node *cur_node;

    /* Move cursor back to top left. */
    move(0,0);
    /* Print the base map. */
    for (i=0; i < map.width * map.height; i++)
    {
        /* Drawing. */
        addch(map.base[i] | COLOR_PAIR(map.base[i]));
        if ((i + 1) % map.width == 0)
        {
            addch('\n');
        }
    }

    /* Print players on top of base. */
    for (i=0; i < num_players; i++)
    {
        /* Make colors of each player unique. */
        /* Ncurses makes us start at index 1 for color pairs.... */
        attron(COLOR_PAIR(i+1));

        /* Prime loop with root node. */
        next_node = players[i].root;

        while (next_node != NULL)
        {
            cur_node = next_node;
            /* Draw character at player's position. */
            mvaddch((cur_node->pos / map.width), (cur_node->pos % map.width), cur_node->tex);
            /* Move on to the next node; rinse and repeat. */
            next_node = cur_node->next;
        }
        attroff(COLOR_PAIR(i+1));
    }
    /* Put it onto the screen. */
    refresh();
}

/* Display simple ingame menu. */
enum playgame_ret ingame_menu()
{
    int i, key, ret_index; /* Iterator and current key pressed and index return. */
    int width, height;  /* Dimensions of container. */
    const char* TITLE = "Ingame Menu"; /* Container title. */
    WINDOW *container;  /* So we can have a border. */
    MENU *ingame_menu;    /* Actual menu. */
    const int NUM_ITEMS = 4;
    ITEM *items[NUM_ITEMS + 1]; /* Null terminated array of items for menu. */
    bool complete = false; /* Is user done?. */
    /* Map item index to return value. */
    const enum playgame_ret USER_OPT[] = { RESUME, REPEAT, NEW, EXIT };

    /* Define our item labels and descriptions. */
    items[0] = new_item("Resume", "Continue playing current game");
    items[1] = new_item("Repeat", "Start a new game with same settings");
    items[2] = new_item("New", "Start a new game with new settings");
    items[3] = new_item("Exit", "A despicable course of action");
    items[4] = NULL;

    ingame_menu = new_menu(items);

    height = 10, width = 50;
    container = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    /* Assign items to container and a sub-window thereof. */
    set_menu_win(ingame_menu, container);
    set_menu_sub(ingame_menu, derwin(container, 6, 48, 3, 1));

    set_menu_mark(ingame_menu, " > ");

    box(container, 0, 0); /* default border. */

    post_menu(ingame_menu);
    mvwprintw(container, 1, (width - strlen(TITLE)) / 2, TITLE);
    wrefresh(container);

    /* Now let the user decide how to proceed. */
    nodelay(stdscr, FALSE);
    while (!complete)
    {
        key = getch();
        switch(key)
        {
            /* Tab key. */
            case 9:
            case KEY_DOWN:
                menu_driver(ingame_menu, REQ_DOWN_ITEM);
                break;
            case KEY_UP:
                menu_driver(ingame_menu, REQ_UP_ITEM);
                break;
            /* Enter key. */
            case 10:
                /* end loop. */
                complete = true;
        }
        wrefresh(container);
    }
    nodelay(stdscr, TRUE);

    /* Get our option's index. */
    ret_index = item_index(current_item(ingame_menu));

    /* Cleanup. */
    unpost_menu(ingame_menu);
    free_menu(ingame_menu);
    for (i=0; i < NUM_ITEMS; i++)
        free(items[i]);
    erase();
    refresh();

    /* Return user option. */
    return USER_OPT[ret_index];
}

void cleanup_game(map_t *map, player_t players[], int num_players)
{
    int i;
    /* To traverse player linked lists. */
    struct player_node *next_node;
    struct player_node *temp_node;

    /* Free data structures. */
    free(map->base);
    free(map->pl_col);
    /* free(map);. */
    for (i=0; i < num_players; i++)
    {
        /* Prime loop with root. */
        next_node = players[i].root->next;
        /* Free up root and then the rest of the nodes. */
        free(players[i].root);
        while (next_node != NULL)
        {
            /* Save reference to next node. */
            temp_node = next_node->next;
            /* Free current node. */
            free(next_node);
            /* Restore reference to next node. */
            next_node = temp_node;
        }
    }
    free(players);

    /* Make getch blocking again. */
    nodelay(stdscr, FALSE);
    /* Show the cursor again. */
    curs_set(1);
}
