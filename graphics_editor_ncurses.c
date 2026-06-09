#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <locale.h>

#define ROWS 22
#define COLS 58
#define MAX_OBJECTS 50

/* Windows */
WINDOW *menu_win;
WINDOW *canvas_win;
WINDOW *status_win;
WINDOW *form_win;

/* Canvas */
char canvas[ROWS][COLS];

/* Object types */
#define OBJ_CIRCLE    1
#define OBJ_RECTANGLE 2
#define OBJ_LINE      3
#define OBJ_TRIANGLE  4

typedef struct {
    int type;
    int active;
    int params[6];
} Object;

Object objects[MAX_OBJECTS];
int obj_count = 0;

/* --- Canvas ------------------------------------------------------------ */

void init_canvas() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            canvas[r][c] = '_';
}

void plot(int r, int c, char ch) {
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = ch;
}

/* --- Drawing algorithms ------------------------------------------------ */

void draw_line_bresenham(int r1, int c1, int r2, int c2, char ch) {
    int dr = abs(r2 - r1), dc = abs(c2 - c1);
    int sr = (r1 < r2) ? 1 : -1;
    int sc = (c1 < c2) ? 1 : -1;
    int err = dr - dc;
    while (1) {
        plot(r1, c1, ch);
        if (r1 == r2 && c1 == c2) break;
        int e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r1 += sr; }
        if (e2 <  dr) { err += dr; c1 += sc; }
    }
}

void draw_circle_midpoint(int cy, int cx, int radius, char ch) {
    int x = 0, y = radius, d = 1 - radius;
    while (x <= y) {
        plot(cy+y, cx+x, ch); plot(cy+y, cx-x, ch);
        plot(cy-y, cx+x, ch); plot(cy-y, cx-x, ch);
        plot(cy+x, cx+y, ch); plot(cy+x, cx-y, ch);
        plot(cy-x, cx+y, ch); plot(cy-x, cx-y, ch);
        if (d < 0) d += 2*x+3;
        else { d += 2*(x-y)+5; y--; }
        x++;
    }
}

void draw_rectangle(int r1, int c1, int r2, int c2, char ch) {
    if (r1 > r2) { int t=r1; r1=r2; r2=t; }
    if (c1 > c2) { int t=c1; c1=c2; c2=t; }
    draw_line_bresenham(r1,c1,r1,c2,ch);
    draw_line_bresenham(r2,c1,r2,c2,ch);
    draw_line_bresenham(r1,c1,r2,c1,ch);
    draw_line_bresenham(r1,c2,r2,c2,ch);
}

void draw_triangle(int r1,int c1,int r2,int c2,int r3,int c3,char ch) {
    draw_line_bresenham(r1,c1,r2,c2,ch);
    draw_line_bresenham(r2,c2,r3,c3,ch);
    draw_line_bresenham(r3,c3,r1,c1,ch);
}

/* --- Render ------------------------------------------------------------ */

void render_all() {
    init_canvas();
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        int *p = objects[i].params;
        switch (objects[i].type) {
            case OBJ_CIRCLE:    draw_circle_midpoint(p[0],p[1],p[2],'*'); break;
            case OBJ_RECTANGLE: draw_rectangle(p[0],p[1],p[2],p[3],'*'); break;
            case OBJ_LINE:      draw_line_bresenham(p[0],p[1],p[2],p[3],'*'); break;
            case OBJ_TRIANGLE:  draw_triangle(p[0],p[1],p[2],p[3],p[4],p[5],'*'); break;
        }
    }
}

/* --- Display canvas window --------------------------------------------- */

void refresh_canvas_win() {
    render_all();
    werase(canvas_win);
    box(canvas_win, 0, 0);
    mvwprintw(canvas_win, 0, 2, " CANVAS ");

    for (int r = 0; r < ROWS; r++) {
        wmove(canvas_win, r + 1, 1);
        for (int c = 0; c < COLS; c++) {
            if (canvas[r][c] == '*') {
                wattron(canvas_win, COLOR_PAIR(3) | A_BOLD);
                waddch(canvas_win, '*');
                wattroff(canvas_win, COLOR_PAIR(3) | A_BOLD);
            } else {
                wattron(canvas_win, COLOR_PAIR(4));
                waddch(canvas_win, canvas[r][c]);
                wattroff(canvas_win, COLOR_PAIR(4));
            }
        }
    }
    wrefresh(canvas_win);
}

/* --- Status bar -------------------------------------------------------- */

void set_status(const char *msg) {
    werase(status_win);
    wattron(status_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(status_win, 0, 1, " %s", msg);
    wattroff(status_win, COLOR_PAIR(2) | A_BOLD);
    wrefresh(status_win);
}

/* --- Form window: read an integer -------------------------------------- */

int form_get_int(const char *prompt) {
    werase(form_win);
    box(form_win, 0, 0);
    mvwprintw(form_win, 0, 2, " INPUT ");
    mvwprintw(form_win, 1, 2, "%s", prompt);
    wattron(form_win, COLOR_PAIR(1));
    mvwprintw(form_win, 2, 2, "> ");
    wattroff(form_win, COLOR_PAIR(1));
    wrefresh(form_win);

    echo();
    curs_set(1);
    int v = 0;
    char buf[16] = {0};
    mvwgetnstr(form_win, 2, 4, buf, 10);
    v = atoi(buf);
    noecho();
    curs_set(0);
    return v;
}

/* --- Object type name -------------------------------------------------- */

const char *type_name(int t) {
    switch(t) {
        case OBJ_CIRCLE:    return "Circle";
        case OBJ_RECTANGLE: return "Rectangle";
        case OBJ_LINE:      return "Line";
        case OBJ_TRIANGLE:  return "Triangle";
        default:            return "Unknown";
    }
}

/* --- Submenu: pick a shape --------------------------------------------- */

int shape_submenu() {
    const char *items[] = { "Circle", "Rectangle", "Line", "Triangle" };
    int n = 4, cur = 0;

    WINDOW *sub = newwin(8, 20, 8, 30);
    keypad(sub, TRUE);

    while (1) {
        werase(sub);
        box(sub, 0, 0);
        mvwprintw(sub, 0, 3, " SHAPE ");
        for (int i = 0; i < n; i++) {
            if (i == cur) {
                wattron(sub, COLOR_PAIR(1) | A_BOLD);
                mvwprintw(sub, i + 2, 3, "> %s", items[i]);
                wattroff(sub, COLOR_PAIR(1) | A_BOLD);
            } else {
                mvwprintw(sub, i + 2, 3, "  %s", items[i]);
            }
        }
        wrefresh(sub);

        int ch = wgetch(sub);
        if (ch == KEY_UP)   cur = (cur - 1 + n) % n;
        if (ch == KEY_DOWN) cur = (cur + 1) % n;
        if (ch == '\n') { delwin(sub); return cur + 1; }
        if (ch == 27)   { delwin(sub); return -1; }
    }
}

/* --- Object list submenu ----------------------------------------------- */

int object_list_submenu(const char *title) {
    /* collect active ids */
    int ids[MAX_OBJECTS], cnt = 0;
    for (int i = 0; i < obj_count; i++)
        if (objects[i].active) ids[cnt++] = i;

    if (cnt == 0) {
        set_status("No objects on canvas.");
        return -1;
    }

    int cur = 0;
    WINDOW *sub = newwin(cnt + 4, 44, 4, 18);
    keypad(sub, TRUE);

    while (1) {
        werase(sub);
        box(sub, 0, 0);
        mvwprintw(sub, 0, 2, " %s ", title);
        for (int i = 0; i < cnt; i++) {
            int id = ids[i];
            int *p = objects[id].params;
            char info[40] = {0};
            switch (objects[id].type) {
                case OBJ_CIRCLE:
                    snprintf(info, sizeof(info), "center(%d,%d) r=%d", p[0],p[1],p[2]);
                    break;
                case OBJ_RECTANGLE:
                    snprintf(info, sizeof(info), "(%d,%d)->(%d,%d)", p[0],p[1],p[2],p[3]);
                    break;
                case OBJ_LINE:
                    snprintf(info, sizeof(info), "(%d,%d)->(%d,%d)", p[0],p[1],p[2],p[3]);
                    break;
                case OBJ_TRIANGLE:
                    snprintf(info, sizeof(info), "(%d,%d)(%d,%d)(%d,%d)",
                             p[0],p[1],p[2],p[3],p[4],p[5]);
                    break;
            }
            if (i == cur) {
                wattron(sub, COLOR_PAIR(1) | A_BOLD);
                mvwprintw(sub, i + 2, 2, "> #%d %-10s %s", id+1,
                          type_name(objects[id].type), info);
                wattroff(sub, COLOR_PAIR(1) | A_BOLD);
            } else {
                mvwprintw(sub, i + 2, 2, "  #%d %-10s %s", id+1,
                          type_name(objects[id].type), info);
            }
        }
        mvwprintw(sub, cnt + 2, 2, "Enter=select  ESC=cancel");
        wrefresh(sub);

        int ch = wgetch(sub);
        if (ch == KEY_UP)   cur = (cur - 1 + cnt) % cnt;
        if (ch == KEY_DOWN) cur = (cur + 1) % cnt;
        if (ch == '\n') { delwin(sub); return ids[cur]; }
        if (ch == 27)   { delwin(sub); return -1; }
    }
}

/* --- Collect params for a shape type ----------------------------------- */

int collect_params(int type, int *p) {
    switch (type) {
        case OBJ_CIRCLE:
            set_status("Circle: enter center row, col, radius");
            p[0] = form_get_int("Center row (0-21):");
            p[1] = form_get_int("Center col (0-57):");
            p[2] = form_get_int("Radius:");
            break;
        case OBJ_RECTANGLE:
            set_status("Rectangle: top-left and bottom-right corners");
            p[0] = form_get_int("Top-left row:");
            p[1] = form_get_int("Top-left col:");
            p[2] = form_get_int("Bottom-right row:");
            p[3] = form_get_int("Bottom-right col:");
            break;
        case OBJ_LINE:
            set_status("Line: start and end point");
            p[0] = form_get_int("Start row:");
            p[1] = form_get_int("Start col:");
            p[2] = form_get_int("End row:");
            p[3] = form_get_int("End col:");
            break;
        case OBJ_TRIANGLE:
            set_status("Triangle: 3 vertices");
            p[0] = form_get_int("Vertex 1 row:");
            p[1] = form_get_int("Vertex 1 col:");
            p[2] = form_get_int("Vertex 2 row:");
            p[3] = form_get_int("Vertex 2 col:");
            p[4] = form_get_int("Vertex 3 row:");
            p[5] = form_get_int("Vertex 3 col:");
            break;
        default: return 0;
    }
    return 1;
}

/* --- Operations -------------------------------------------------------- */

void op_add() {
    if (obj_count >= MAX_OBJECTS) {
        set_status("Canvas full! Max 50 objects."); return;
    }
    int type = shape_submenu();
    if (type < 0) { set_status("Add cancelled."); return; }

    Object obj; memset(&obj, 0, sizeof(obj));
    obj.type = type; obj.active = 1;
    collect_params(type, obj.params);
    objects[obj_count++] = obj;

    char msg[64];
    snprintf(msg, sizeof(msg), "Added %s as object #%d.", type_name(type), obj_count);
    refresh_canvas_win();
    set_status(msg);
}

void op_delete() {
    int id = object_list_submenu("SELECT TO DELETE");
    if (id < 0) { set_status("Delete cancelled."); return; }
    objects[id].active = 0;
    char msg[64];
    snprintf(msg, sizeof(msg), "Deleted object #%d.", id + 1);
    refresh_canvas_win();
    set_status(msg);
}

void op_modify() {
    int id = object_list_submenu("SELECT TO MODIFY");
    if (id < 0) { set_status("Modify cancelled."); return; }
    collect_params(objects[id].type, objects[id].params);
    char msg[64];
    snprintf(msg, sizeof(msg), "Modified object #%d.", id + 1);
    refresh_canvas_win();
    set_status(msg);
}

void op_list() {
    int cnt = 0;
    for (int i = 0; i < obj_count; i++)
        if (objects[i].active) cnt++;

    if (cnt == 0) { set_status("No objects on canvas."); return; }

    WINDOW *lw = newwin(cnt + 5, 56, 3, 10);
    box(lw, 0, 0);
    mvwprintw(lw, 0, 2, " ALL OBJECTS ");
    int row = 2;
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        int *p = objects[i].params;
        switch (objects[i].type) {
            case OBJ_CIRCLE:
                mvwprintw(lw, row, 2, "#%d Circle     center(%d,%d) r=%d",
                          i+1, p[0],p[1],p[2]);
                break;
            case OBJ_RECTANGLE:
                mvwprintw(lw, row, 2, "#%d Rectangle  (%d,%d)->(%d,%d)",
                          i+1, p[0],p[1],p[2],p[3]);
                break;
            case OBJ_LINE:
                mvwprintw(lw, row, 2, "#%d Line       (%d,%d)->(%d,%d)",
                          i+1, p[0],p[1],p[2],p[3]);
                break;
            case OBJ_TRIANGLE:
                mvwprintw(lw, row, 2, "#%d Triangle   (%d,%d)(%d,%d)(%d,%d)",
                          i+1, p[0],p[1],p[2],p[3],p[4],p[5]);
                break;
        }
        row++;
    }
    mvwprintw(lw, row + 1, 2, "Press any key to close...");
    wrefresh(lw);
    wgetch(lw);
    delwin(lw);
    touchwin(stdscr);
    refresh_canvas_win();
    set_status("Ready.");
}

void op_clear() {
    for (int i = 0; i < obj_count; i++)
        objects[i].active = 0;
    refresh_canvas_win();
    set_status("Canvas cleared.");
}

/* --- Main menu --------------------------------------------------------- */

void draw_menu_win(int cur) {
    const char *items[] = {
        "Add Object",
        "Delete Object",
        "Modify Object",
        "List Objects",
        "Clear Canvas",
        "Exit"
    };
    int n = 6;

    werase(menu_win);
    box(menu_win, 0, 0);
    wattron(menu_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(menu_win, 1, 2, "2D GRAPHICS");
    mvwprintw(menu_win, 2, 2, "   EDITOR  ");
    wattroff(menu_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(menu_win, 3, 1, "-----------");

    for (int i = 0; i < n; i++) {
        if (i == cur) {
            wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
            mvwprintw(menu_win, i + 5, 2, "> %s", items[i]);
            wattroff(menu_win, COLOR_PAIR(1) | A_BOLD);
        } else {
            mvwprintw(menu_win, i + 5, 3, "%s", items[i]);
        }
    }

    mvwprintw(menu_win, 12, 1, "-----------");
    mvwprintw(menu_win, 13, 2, "^ navigate");
    mvwprintw(menu_win, 14, 2, "v select");

    /* object counter */
    int cnt = 0;
    for (int i = 0; i < obj_count; i++)
        if (objects[i].active) cnt++;
    mvwprintw(menu_win, 16, 2, "Objects: %d", cnt);

    wrefresh(menu_win);
}

/* --- Layout setup ------------------------------------------------------ */

void setup_windows() {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    /* menu: 18 wide, left side */
    menu_win   = newwin(max_y - 2, 15, 0, 0);
    /* canvas: rest of screen */
    canvas_win = newwin(max_y - 2, max_x - 15, 0, 15);
    /* status bar: bottom */
    status_win = newwin(1, max_x, max_y - 2, 0);
    /* form: floating center-ish */
    form_win   = newwin(5, 34, max_y/2 - 2, max_x/2 - 17);

    keypad(menu_win, TRUE);
    keypad(stdscr, TRUE);
}

/* --- Main -------------------------------------------------------------- */

int main() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    if (!has_colors()) {
        endwin();
        printf("Terminal does not support colors.\n");
        return 1;
    }
    start_color();
    /* 1=highlight, 2=title/status, 3=draw char, 4=bg char */
    init_pair(1, COLOR_BLACK,  COLOR_CYAN);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN,  COLOR_BLACK);
    init_pair(4, COLOR_WHITE,  COLOR_BLACK);

    setup_windows();
    init_canvas();
    refresh_canvas_win();
    set_status("Welcome! Use arrow keys to navigate, Enter to select.");

    int cur = 0, n = 6;
    draw_menu_win(cur);

    int running = 1;
    while (running) {
        int ch = wgetch(menu_win);
        switch (ch) {
            case KEY_UP:   cur = (cur - 1 + n) % n; break;
            case KEY_DOWN: cur = (cur + 1) % n;     break;
            case '\n':
                switch (cur) {
                    case 0: op_add();    break;
                    case 1: op_delete(); break;
                    case 2: op_modify(); break;
                    case 3: op_list();   break;
                    case 4: op_clear();  break;
                    case 5: running = 0; break;
                }
                break;
        }
        draw_menu_win(cur);
    }

    delwin(menu_win);
    delwin(canvas_win);
    delwin(status_win);
    delwin(form_win);
    endwin();
    printf("Thanks for using 2D Graphics Editor!\n");
    return 0;
}
