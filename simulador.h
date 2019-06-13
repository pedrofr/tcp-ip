
typedef struct parallel_argument
{
  parscomm* pcomm;
  int* modificado;
} pararg;

void *simulate(void *args);
