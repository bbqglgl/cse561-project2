/****************************************/
/*                                      */
/*                                      */
/*  Game of Life with Pthread and CUDA  */
/*                                      */
/*  CSE561/SCE412 Project #2            */
/*  @ Ajou University                   */
/*                                      */
/*                                      */
/****************************************/

#include "glife.h"
using namespace std;

int gameOfLife(int argc, char *argv[]);
void singleThread(int, int, int);
void *workerThread(void *);
int nprocs;
GameOfLifeGrid *g_GameOfLifeGrid;
const int liveTable[2][10] = {{0,0,0,1,0,0,0,0,0,0}, {0,0,0,1,1,0,0,0,0,0}};

struct arrr
{
  int start;
  int end;
  int gen;
  pthread_barrier_t *barrier;
};

uint64_t dtime_usec(uint64_t start)
{
  timeval tv;
  gettimeofday(&tv, 0);
  return ((tv.tv_sec * USECPSEC) + tv.tv_usec) - start;
}

GameOfLifeGrid::GameOfLifeGrid(int rows, int cols, int gen)
{
  m_Generations = gen;
  m_Rows = rows;
  m_Cols = cols;

  m_Grid = (int **)malloc(sizeof(int *) * rows);
  if (m_Grid == NULL)
    cout << "1 Memory allocation error " << endl;

  m_Temp = (int **)malloc(sizeof(int *) * rows);
  if (m_Temp == NULL)
    cout << "2 Memory allocation error " << endl;

  m_Grid[0] = (int *)malloc(sizeof(int) * (cols * rows));
  if (m_Grid[0] == NULL)
    cout << "3 Memory allocation error " << endl;

  m_Temp[0] = (int *)malloc(sizeof(int) * (cols * rows));
  if (m_Temp[0] == NULL)
    cout << "4 Memory allocation error " << endl;

  for (int i = 1; i < rows; i++)
  {
    m_Grid[i] = m_Grid[i - 1] + cols;
    m_Temp[i] = m_Temp[i - 1] + cols;
  }

  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      m_Grid[i][j] = m_Temp[i][j] = 0;
    }
  }
}

// Entry point
int main(int argc, char *argv[])
{
  if (argc != 7)
  {
    cout << "Usage: " << argv[0] << " <input file> <display> <nprocs>"
                                    " <# of generation> <width> <height>"
         << endl;
    cout << "\n\tnprocs = 0: Enable GPU" << endl;
    cout << "\tnprocs > 0: Run on CPU" << endl;
    cout << "\tdisplay = 1: Dump results" << endl;
    return 1;
  }

  return gameOfLife(argc, argv);
}

int gameOfLife(int argc, char *argv[])
{
  int cols, rows, gen;
  ifstream inputFile;
  int input_row, input_col, display;
  uint64_t difft, cuda_difft;
  pthread_t *threadID;

  inputFile.open(argv[1], ifstream::in);

  if (inputFile.is_open() == false)
  {
    cout << "The " << argv[1] << " file can not be opend" << endl;
    return 1;
  }

  display = atoi(argv[2]);
  nprocs = atoi(argv[3]);
  gen = atoi(argv[4]);
  cols = atoi(argv[5]);
  rows = atoi(argv[6]);

  g_GameOfLifeGrid = new GameOfLifeGrid(rows, cols, gen);

  while (inputFile.good())
  {
    inputFile >> input_row >> input_col;
    if (input_row >= rows || input_col >= cols)
    {
      cout << "Invalid grid number" << endl;
      return 1;
    }
    else
      g_GameOfLifeGrid->setCell(input_row, input_col);
  }

  // Start measuring execution time
  difft = dtime_usec(0);

  // TODO: YOU NEED TO IMPLMENT THE SINGLE THREAD, PTHREAD, AND CUDA
  if (nprocs == 0)
  {
    // Running on GPU
    cuda_difft = runCUDA(rows, cols, gen, g_GameOfLifeGrid, display);
  }
  else if (nprocs == 1)
  {
    // Running a single thread
    singleThread(rows, cols, gen);
  }
  else
  {
    // Running multiple threads (pthread)
    threadID = (pthread_t *)malloc(sizeof(pthread_t) * nprocs);
    pthread_barrier_t barrier;
    int num = (rows * cols / nprocs) + 1;
    pthread_barrier_init(&barrier, NULL, nprocs);
    for (int i = 0; i < nprocs; i++)
    {
      struct arrr *arg = (struct arrr *)malloc(sizeof(struct arrr));
      arg->start = i * num;
      arg->end = (i + 1) * num;
      arg->gen = gen;
      arg->barrier = &barrier;
      pthread_create(threadID + i, NULL, workerThread, (void *)arg);
    }
    for (int i = 0; i < nprocs; i++)
      pthread_join(threadID[i], NULL);

    pthread_barrier_destroy(&barrier);
  }

  difft = dtime_usec(difft);

  // Print indices only for running on CPU(host).
  if (display && nprocs)
  {
    g_GameOfLifeGrid->dump();
    g_GameOfLifeGrid->dumpIndex();
  }

  if (nprocs)
  {
    // Single or multi-thread execution time
    //cout << "Execution time(seconds): " << difft / (float)USECPSEC << endl;
    
    cout << difft / (float)USECPSEC << endl;
  }
  else
  {
    // CUDA execution time
    //cout << "Execution time(seconds): " << cuda_difft / (float)USECPSEC << endl;
    
    cout << cuda_difft / (float)USECPSEC << endl;
  }
  inputFile.close();
  //cout << "Program end... " << endl;
  return 0;
}

// TODO: YOU NEED TO IMPLMENT SINGLE THREAD
void singleThread(int rows, int cols, int gen)
{
  int num;
  while (g_GameOfLifeGrid->decGen())
  {
    for (int i = 0; i < g_GameOfLifeGrid->getRows(); i++)
    {
      for (int j = 0; j < g_GameOfLifeGrid->getCols(); j++)
      {
        num = g_GameOfLifeGrid->getNumOfNeighbors(i, j);
      (*g_GameOfLifeGrid->getTemp(i,j)) = liveTable[*(g_GameOfLifeGrid->getRowAddr(i) + j)][num];
      /*
      if (liveTable[*(g_GameOfLifeGrid->getRowAddr(i) + j)][num])
        g_GameOfLifeGrid->live(i, j);
      else
        g_GameOfLifeGrid->dead(i, j);
        */
      }
    }
    g_GameOfLifeGrid->next();
  }
}

// TODO: YOU NEED TO IMPLMENT PTHREAD
void *workerThread(void *arg)
{
  int start = ((struct arrr *)arg)->start;
  int end = ((struct arrr *)arg)->end;
  int gen = ((struct arrr *)arg)->gen;
  int cols = g_GameOfLifeGrid->getCols();
  int rows = g_GameOfLifeGrid->getRows();
  int num;

  if (end > cols * rows)
    end = cols * rows;

  while (gen > 0)
  {
    for (int index = start; index < end; index++)
    {
      int i = index / cols;
      int j = index % cols;
      num = g_GameOfLifeGrid->getNumOfNeighbors(i, j);
      (*g_GameOfLifeGrid->getTemp(i,j)) = liveTable[*(g_GameOfLifeGrid->getRowAddr(i) + j)][num];
      /*
      if (liveTable[*(g_GameOfLifeGrid->getRowAddr(i) + j)][num])
        g_GameOfLifeGrid->live(i, j);
      else
        g_GameOfLifeGrid->dead(i, j);
        */
    }
    pthread_barrier_wait(((struct arrr *)arg)->barrier);
    g_GameOfLifeGrid->next(start, end);
    pthread_barrier_wait(((struct arrr *)arg)->barrier);
    gen--;
  }
}

// HINT: YOU MAY NEED TO FILL OUT BELOW FUNCTIONS OR CREATE NEW FUNCTIONS
void GameOfLifeGrid::next(const int from, const int to)
{
  memcpy(m_Grid[0]+from, m_Temp[0]+from, sizeof(int) * (to-from));
}

void GameOfLifeGrid::next()
{
  memcpy(m_Grid[0], m_Temp[0], sizeof(int) * (m_Cols * m_Rows));
}

// TODO: YOU MAY NEED TO IMPLMENT IT TO GET NUMBER OF NEIGHBORS
int GameOfLifeGrid::getNumOfNeighbors(int rows, int cols)
{
  int numOfNeighbors = 0;
  int row = rows - 1 < 0 ? 0 : rows - 1;
  int rows_max = rows + 1 == g_GameOfLifeGrid->getRows() ? rows : rows + 1;
  int cols_max = cols + 1 == g_GameOfLifeGrid->getCols() ? cols : cols + 1;

  for (; row <= rows_max; row++)
  {
    int col = cols - 1 < 0 ? 0 : cols - 1;
    for (; col <= cols_max; col++)
      numOfNeighbors += g_GameOfLifeGrid->isLive(row, col);
  }
  return numOfNeighbors;
}

void GameOfLifeGrid::dump()
{
  cout << "===============================" << endl;

  for (int i = 0; i < m_Rows; i++)
  {
    cout << "[" << i << "] ";
    for (int j = 0; j < m_Cols; j++)
    {
      if (m_Grid[i][j] == 1)
        cout << "*";
      else
        cout << "o";
    }
    cout << endl;
  }
  cout << "===============================\n"
       << endl;
}

void GameOfLifeGrid::dumpIndex()
{
  cout << ":: Dump Row Column indices" << endl;
  for (int i = 0; i < m_Rows; i++)
  {
    for (int j = 0; j < m_Cols; j++)
    {
      if (m_Grid[i][j])
        cout << i << " " << j << endl;
    }
  }
}
