#include <stdio.h>
#include <SDL/SDL.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include "graphics.h"
#include "time_utils.h"

#define SCREEN_W 640 //tamanho da janela que sera criada
#define SCREEN_H 640

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

  canvas->Xoffset = 10;
  canvas->Yoffset = Height;

  canvas->Xext = 10;
  canvas->Yext = 10;

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

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int _leave;
static volatile double _time;
static volatile double _var1;
static volatile double _var2;
static volatile double _var3;

void *graphics()
{
  // struct timespec time_initial, time_last, time_current;

  // clock_gettime(CLOCK_MONOTONIC_RAW, &time_initial);
  // time_current = time_initial;
  // time_last = time_current;

  // char buffer[26];
  // time_t timer;
  // time(&timer);
  // struct tm* tm_info = localtime(&timer);
  // strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
  // printf("\nStarting graphics at %s!\n", buffer);

  Tdataholder *data = datainit(640, 480, 55, 110, 45, 0, 0);

  timestamp_printf("Starting graphics!");

  struct timespec sleepTime = {0, 50000000L};

  while (1)
  {
    pthread_mutex_lock(&mutex);
    int leave = _leave;
    double time = _time;
    double var1 = _var1; //level
    double var2 = _var2; //inangle
    double var3 = _var3; //outangle
    pthread_mutex_unlock(&mutex);

    if (leave)
      break;

    datadraw(data, time, var1, var2, var3);

    // clock_gettime(CLOCK_MONOTONIC_RAW, &time_current);
    // double T = (time_current.tv_sec - time_initial.tv_sec) * 1000. + (time_current.tv_nsec - time_initial.tv_nsec) / 1000000.;
    // double dT = (time_current.tv_sec - time_last.tv_sec) * 1000. + (time_current.tv_nsec - time_last.tv_nsec) / 1000000.;
    // time_last = time_current;

    if (leave)
      break;

    nanosleep(&sleepTime, NULL);
  }

  // time(&timer);
  // tm_info = localtime(&timer);
  // strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
  // printf("\nClosing plant at %s!\n", buffer);

  timestamp_printf("Closing graphics!");

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
  pthread_mutex_lock(&mutex);
  _leave = 1;
  pthread_mutex_unlock(&mutex);
}
