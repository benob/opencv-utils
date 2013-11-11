#include <ncurses.h>

int main(int argc, char** argv) {
    initscr();
    start_color();          /* Start color          */
    init_color(COLOR_RED, 700, 700, 700);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1));
    mvwprintw(stdscr, 0, 0, "Hello");
    fprintf(stdout, "world");
    attroff(COLOR_PAIR(1));

    getch();
    endwin();

}
