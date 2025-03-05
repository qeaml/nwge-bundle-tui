#include <SDL2/SDL_error.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_stdinc.h>
#include <ncurses.h>
#include <nwge/bndl/reader.h>
#include <stdio.h>

WINDOW *win;
u32 selection;
BndlReader reader;

static void putFileName(const BndlFile *file)
{
  for(s32 i = 0; i < BNDL_FILE_NAME_LEN; ++i) {
    if(file->name[i] == '\0') {
      break;
    }
    waddch(win, file->name[i]);
  }
  if(file->ext[0] == '\0') {
    return;
  }
  waddch(win, '.');
  for(s32 i = 0; i < BNDL_FILE_EXT_LEN; ++i) {
    if(file->ext[i] == '\0') {
      break;
    }
    waddch(win, file->ext[i]);
  }
}

static void updateSelection(u32 newSelection)
{
  wmove(win, 3+selection, 1);
  putFileName(&reader.tree.files[selection]);
  selection = newSelection;
  wmove(win, 3+selection, 1);
  wattron(win, A_REVERSE);
  putFileName(&reader.tree.files[selection]);
  wattroff(win, A_REVERSE);
}

#define BILLION 1000000000
#define MILLION 1000000
#define THOUSAND 1000

static void humanizedSize(s32 num)
{
  if(num > BILLION) {
    wprintw(win, "%.01fGB", (f32)num/BILLION);
  } else if(num > MILLION) {
    wprintw(win, "%.01fMB", (f32)num/MILLION);
  } else if(num > THOUSAND) {
    wprintw(win, "%.01fkB", (f32)num/THOUSAND);
  } else {
    wprintw(win, "%d bytes", num);
  }
}

s32 main(s32 argc, CStr argv[])
{
  if(argc != 2) {
    fprintf(stderr, "Usage: %s <bundle file>\n", argv[0]);
    return 1;
  }

  CStr path = argv[1];
  SDL_RWops *file = SDL_RWFromFile(path, "rb");
  if(file == NULL) {
    fprintf(stderr, "%s\n", SDL_GetError());
    return 1;
  }

  BndlErr err = bndlInitReader(&reader, file);
  if(err != BndlErrOK) {
    fprintf(stderr, "Could not initialize reader:\n%s\n", bndlErrorMsg(err));
    bndlFreeReader(&reader);
    return 1;
  }

  initscr();
  noecho();
  cbreak();
  keypad(stdscr, TRUE);
  curs_set(0);
  refresh();
  s32 pathLen = strlen(path);
  s32 filenameStart = 0;
  for(s32 i = pathLen; i >= 0; --i) {
    if(path[i] == '/') {
      filenameStart = i + 1;
      break;
    }
  }
  s32 width = SDL_min(2+16+3+10+3+10, pathLen+2);
  s32 height = reader.tree.count+4;
  s32 y, x;
  getmaxyx(stdscr, y, x);
  y = (y - height) / 2;
  x = (x - width) / 2;
  win = newwin(height, width, y, x);

  box(win, 0, 0);
  wmove(win, 0, 18);
  waddch(win, ACS_TTEE);
  wmove(win, 0, 31);
  waddch(win, ACS_TTEE);
  wmove(win, height-1, 18);
  waddch(win, ACS_BTEE);
  wmove(win, height-1, 31);
  waddch(win, ACS_BTEE);
  wmove(win, 0, 1);
  waddstr(win, &path[filenameStart]);
  wmove(win, 1, 1);
  waddstr(win, " Name            ");
  waddch(win, ACS_VLINE);
  waddstr(win, " Size       ");
  waddch(win, ACS_VLINE);
  waddstr(win, " Offset");
  wmove(win, 2, 0);
  waddch(win, ACS_LTEE);
  whline(win, ACS_HLINE, 17);
  wmove(win, 2, 18);
  waddch(win, ACS_PLUS);
  whline(win, ACS_HLINE, 12);
  wmove(win, 2, 31);
  waddch(win, ACS_PLUS);
  whline(win, ACS_HLINE, 11);
  wmove(win, 2, 43);
  waddch(win, ACS_RTEE);
  for(u32 i = 0; i < reader.tree.count; ++i) {
    BndlFile *file = &reader.tree.files[i];
    wmove(win, 3+i, 1);
    if(selection == i) {
      wattron(win, A_REVERSE);
    }
    putFileName(file);
    wattroff(win, A_REVERSE);
    wmove(win, 3+i, 18);
    waddch(win, ACS_VLINE);
    waddch(win, ' ');
    humanizedSize(file->size);
    wmove(win, 3+i, 31);
    waddch(win, ACS_VLINE);
    wprintw(win, " %-10d", file->offset);
  }

  wrefresh(win);

  do {
    int cmd = getch();
    if(cmd == 'q') {
      break;
    } else if(cmd == KEY_UP) {
      if(selection == 0) {
        updateSelection(reader.tree.count - 1);
      } else {
        updateSelection(selection - 1);
      }
    } else if(cmd == KEY_DOWN) {
      if(selection >= reader.tree.count - 1) {
        updateSelection(0);
      } else {
        updateSelection(selection + 1);
      }
    }
    wrefresh(win);
  } while(1);

  delwin(win);
  refresh();
  endwin();
  bndlFreeReader(&reader);

  return 0;
}