#include <ncurses.h>
#include <nwge/bndl/reader.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_stdinc.h>
#include <stdarg.h>
#include <stdio.h>

#define NAME_BUF_SZ (BNDL_FILE_NAME_LEN+1+BNDL_FILE_EXT_LEN+1)

static void copyFileNameToBuf(const BndlFile *file, char *buf)
{
  for(s32 i = 0; i < BNDL_FILE_NAME_LEN; ++i) {
    if(file->name[i] == '\0') {
      break;
    }
    *buf++ = file->name[i];
  }
  if(file->ext[0] == '\0') {
    *buf = '\0';
    return;
  }
  *buf++ = '.';
  for(s32 i = 0; i < BNDL_FILE_EXT_LEN; ++i) {
    if(file->ext[i] == '\0') {
      break;
    }
    *buf++ = file->ext[i];
  }
  *buf = '\0';
}

static void showDialog(const char *title, int lines, ...)
{
  s32 maxX, maxY;
  getmaxyx(stdscr, maxY, maxX);
  s32 x = 3;
  s32 width = maxX - (2*x);
  s32 height = 2+lines;
  s32 y = (maxY - height) - 2;
  WINDOW *dialog = newwin(height, width, y, x);
  wattron(dialog, A_REVERSE);
  box(dialog, 0, 0);
  wmove(dialog, 0, 1);
  waddstr(dialog, title);
  wattroff(dialog, A_REVERSE);
  va_list ap;
  va_start(ap, lines);
  for(int i = 0; i < lines; ++i) {
    wmove(dialog, 1+i, 1);
    waddstr(dialog, va_arg(ap, const char*));
  }
  va_end(ap);
  wrefresh(dialog);
  refresh();
  getch();
  wclear(dialog);
  wrefresh(dialog);
  refresh();
  delwin(dialog);
}

WINDOW *win;
u32 selection;
SDL_RWops *bndlRW;
BndlReader reader;

static void extractFile(const BndlFile *file)
{
  char fileName[NAME_BUF_SZ];
  copyFileNameToBuf(file, fileName);

  SDL_RWops *outRW = SDL_RWFromFile(fileName, "wb");
  if(outRW == NULL) {
    showDialog("Error", 2, "Could not extract file:", SDL_GetError());
    return;
  }
  char *fileData = malloc(file->size);
  SDL_RWseek(bndlRW, file->offset, RW_SEEK_SET);
  SDL_RWread(bndlRW, fileData, sizeof(char), file->size);

  SDL_RWwrite(outRW, fileData, sizeof(char), file->size);
  SDL_RWclose(outRW);
  free(fileData);

  showDialog("Success", 2, "Successfully extracted file:", fileName);
}

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
  bndlRW = SDL_RWFromFile(path, "rb");
  if(bndlRW == NULL) {
    fprintf(stderr, "%s\n", SDL_GetError());
    return 1;
  }

  BndlErr err = bndlInitReader(&reader, bndlRW);
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
  s32 width = SDL_max(1+16+3+10+3+10+1, pathLen-filenameStart+2);
  s32 height = reader.tree.count+4;
  s32 maxY, maxX;
  getmaxyx(stdscr, maxY, maxX);
  mvaddstr(maxY-1, 0, "Use <Up> and <Down> to navigate. Press <X> to extract file. Press <Q> to quit.");
  s32 y = (maxY - height) / 2;
  s32 x = (maxX - width) / 2;
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
    } else if(cmd == 'x') {
      extractFile(&reader.tree.files[selection]);
    }
    wrefresh(win);
  } while(1);

  delwin(win);
  refresh();
  endwin();
  bndlFreeReader(&reader);

  return 0;
}