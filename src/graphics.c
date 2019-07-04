#include <stdio.h>
#include <SDL/SDL.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include "graphics.h"
#include "time_utils.h"

#define SCREEN_W 640 //tamanho da janela que sera criada
#define SCREEN_H 640
#define PADDING_W 100
#define PADDING_H 20

//#define BPP 8
//typedef Uint8 PixelType;
//#define BPP 16
//typedef Uint16 PixelType;
#define BPP 32
typedef Uint32 PixelType;

typedef struct canvas
{
  SDL_Surface *canvas;
  int Height;  // canvas height
  int Width;   // canvas width
  int Xoffset; // X off set, in canvas pixels
  int Yoffset; // Y off set, in canvas pixels
  int Xext;    // X extra width
  int Yext;    // Y extra height
  double Xmax;
  double Ymax;
  double Xstep; // half a distance between X pixels in 'Xmax' scale

  PixelType *zpixel;

} Tcanvas;

typedef struct dataholder
{
  Tcanvas *canvas;
  double Tcurrent;
  double Lcurrent;
  PixelType Lcolor;
  double INcurrent;
  PixelType INcolor;
  double OUTcurrent;
  PixelType OUTcolor;

} Tdataholder;

static void c_pixeldraw(Tcanvas *canvas, int x, int y, PixelType color)
{
  *(((PixelType *)canvas->canvas->pixels) + ((-y + canvas->Yoffset) * canvas->canvas->w + x + canvas->Xoffset)) = color;
}

static void c_hlinedraw(Tcanvas *canvas, int xstep, int y, PixelType color)
{
  int offset = (-y + canvas->Yoffset) * canvas->canvas->w;
  int x;

  for (x = 0; x < canvas->Width + canvas->Xoffset; x += xstep)
  {
    *(((PixelType *)canvas->canvas->pixels) + (offset + x)) = color;
  }
}

static void c_vlinedraw(Tcanvas *canvas, int x, int ystep, PixelType color)
{
  int offset = x + canvas->Xoffset;
  int y;
  int Ystep = ystep * canvas->canvas->w;

  for (y = 0; y < canvas->Height + canvas->Yext; y += ystep)
  {
    *(((PixelType *)canvas->canvas->pixels) + (offset + y * canvas->canvas->w)) = color;
  }
}

void c_linedraw(Tcanvas *canvas, double x0, double y0, double x1, double y1, PixelType color)
{
  double x;

  for (x = x0; x <= x1; x += canvas->Xstep)
  {
    c_pixeldraw(canvas, (int)(x * canvas->Width / canvas->Xmax + 0.5), (int)((double)canvas->Height / canvas->Ymax * (y1 * (x1 - x) + y1 * (x - x0)) / (x1 - x0) + 0.5), color);
  }
}

static Tcanvas *c_open(int Width, int Height, double Xmax, double Ymax)
{
  int x, y;
  Tcanvas *canvas;
  canvas = malloc(sizeof(Tcanvas));

  canvas->Xoffset = PADDING_W;
  canvas->Yoffset = Height;

  canvas->Xext = PADDING_W;
  canvas->Yext = PADDING_H;

  canvas->Height = Height;
  canvas->Width = Width;
  canvas->Xmax = Xmax;
  canvas->Ymax = Ymax;

  canvas->Xstep = Xmax / (double)Width / 2;

  //  canvas->zpixel = (PixelType *)canvas->canvas->pixels +(Height-1)*canvas->canvas->w;

  SDL_Init(SDL_INIT_VIDEO); //SDL init
  canvas->canvas = SDL_SetVideoMode(canvas->Width + canvas->Xext, canvas->Height + canvas->Yext, BPP, SDL_SWSURFACE);

  c_hlinedraw(canvas, 1, 0, (PixelType)SDL_MapRGB(canvas->canvas->format, 255, 255, 255));
  for (y = 10; y < Ymax; y += 10)
  {
    c_hlinedraw(canvas, 3, y * Height / Ymax, (PixelType)SDL_MapRGB(canvas->canvas->format, 220, 220, 220));
  }
  c_vlinedraw(canvas, 0, 1, (PixelType)SDL_MapRGB(canvas->canvas->format, 255, 255, 255));
  for (x = 10; x < Xmax; x += 10)
  {
    c_vlinedraw(canvas, x * Width / Xmax, 3, (PixelType)SDL_MapRGB(canvas->canvas->format, 220, 220, 220));
  }

  return canvas;
}

static Tdataholder *datainit(int Width, int Height, double Xmax, double Ymax, double Lcurrent, double INcurrent, double OUTcurrent)
{
  Tdataholder *data = malloc(sizeof(Tdataholder));

  data->canvas = c_open(Width, Height, Xmax, Ymax);
  data->Tcurrent = 0;
  data->Lcurrent = Lcurrent;
  data->Lcolor = (PixelType)SDL_MapRGB(data->canvas->canvas->format, 255, 180, 0);
  data->INcurrent = INcurrent;
  data->INcolor = (PixelType)SDL_MapRGB(data->canvas->canvas->format, 180, 255, 0);
  data->OUTcurrent = OUTcurrent;
  data->OUTcolor = (PixelType)SDL_MapRGB(data->canvas->canvas->format, 0, 180, 255);

  return data;
}

static void setdatacolors(Tdataholder *data, PixelType Lcolor, PixelType INcolor, PixelType OUTcolor)
{
  data->Lcolor = Lcolor;
  data->INcolor = INcolor;
  data->OUTcolor = OUTcolor;
}

static void datadraw(Tdataholder *data, double time, double level, double inangle, double outangle)
{
  c_linedraw(data->canvas, data->Tcurrent, data->Lcurrent, time, level, data->Lcolor);
  c_linedraw(data->canvas, data->Tcurrent, data->INcurrent, time, inangle, data->INcolor);
  c_linedraw(data->canvas, data->Tcurrent, data->OUTcurrent, time, outangle, data->OUTcolor);
  data->Tcurrent = time;
  data->Lcurrent = level;
  data->INcurrent = inangle;
  data->OUTcurrent = outangle;

  SDL_Flip(data->canvas->canvas);
}

static void quitevent()
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_QUIT)
    {
      // close files, etc...

      SDL_Quit();
      exit(1); // this will terminate all threads !
    }
  }
}

#define GRAPHICS_PERIOD \
  {                     \
    0, 50000000L        \
  }
#define EPOCH_DURATION 60

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static volatile double _time;
static volatile double _var1;
static volatile double _var2;
static volatile double _var3;

static volatile unsigned char load = 1;
static volatile unsigned char quit;

void *graphics()
{
  timestamp_printf("Starting graphics!\n");

  int epoch = 0, last_epoch = 0;

  SDL_Rect future = {PADDING_W, 0, SCREEN_W, SCREEN_H + PADDING_H};
  SDL_Rect past = {SCREEN_W, 0, PADDING_W, SCREEN_H + PADDING_H};
  Tdataholder *data = datainit(SCREEN_W, SCREEN_H, EPOCH_DURATION, 110, 0, 0, 0);
  SDL_Surface *clean = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA,
                                            data->canvas->Width + data->canvas->Xext,
                                            data->canvas->Height + data->canvas->Yext,
                                            32,
                                            0xFF000000,
                                            0x00FF0000,
                                            0x0000FF00,
                                            0x000000FF);

  SDL_BlitSurface(data->canvas->canvas, NULL, clean, NULL);

  struct timespec time_start;

  now(&time_start);
  perspec pspec = {time_start, GRAPHICS_PERIOD};

  load = 0;

  while (!quit)
  {
    pthread_mutex_lock(&mutex);
    double time = _time;
    double var1 = _var1; //level
    double var2 = _var2; //inangle
    double var3 = _var3; //outangle
    pthread_mutex_unlock(&mutex);

    if (load)
    {
      pthread_mutex_lock(&mutex);
      timestamp_printf("Restarting graphics!\n");

      epoch = last_epoch = 0;
      time = _time = 0;
      var1 = _var1 = 0; //level
      var2 = _var2 = 0; //inangle
      var3 = _var3 = 0; //outangle

      now(&time_start);
      pspec.time_next = time_start;

      SDL_BlitSurface(clean, NULL, data->canvas->canvas, NULL);

      timestamp_printf("Done restarting graphics!\n");

      pthread_mutex_unlock(&mutex);
      load = 0;
    }

    if (last_epoch != (epoch = time / EPOCH_DURATION))
    {
      last_epoch = epoch;

      SDL_BlitSurface(data->canvas->canvas, &past, data->canvas->canvas, NULL);
      SDL_BlitSurface(clean, &future, data->canvas->canvas, &future);

      data->Tcurrent = fmod(time, EPOCH_DURATION);
    }

    datadraw(data, fmod(time, EPOCH_DURATION), var1, var2, var3);

    ensure_period(&pspec);
  }

  // SDL_Quit();

  timestamp_printf("Closing graphics!\n");

  pthread_exit(NULL);
}

void update_graphics(double time, double var1, double var2, double var3)
{
  pthread_mutex_lock(&mutex);
  _time = time;
  _var1 = var1; //level
  _var2 = var2; //inangle
  _var3 = var3; //outangle
  pthread_mutex_unlock(&mutex);
}

void quit_graphics()
{
  quit = 1;
}

void restart_graphics()
{
  load = 1;
}

unsigned char loading_graphics()
{
  return load;
}
