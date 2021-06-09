#include<iostream>
#include<cmath>
#include<unistd.h>
#include<string>
#include<stdlib.h>

using namespace std;

int main(int argc, char* argv[])
{
    int nTable[7] = {0, 1, 2, 4, 12, 48, 96};
    int genTable[6] = {10,100,1000,5000,10000,25000};
	char path[1001];
	string input_path;
    int cols, rows;
    double time;
    double avgTime;

	if(argc < 4)
        return 0;
    input_path = argv[1];
    cols = atoi(argv[2]);
    rows = atoi(argv[3]);
    FILE *fo;

    for(int i=0;i<4;i++)
    {
        int nprocs = nTable[i];
        int gen = genTable[i];
        nprocs = 0;
        //gen = 100;
        avgTime = 0;
        for (int i = 0; i < 6; i++)
        {
            sprintf(path, "~/aca/cse561-project2/glife %s 0 %d %d %d %d > ./output.txt", input_path.c_str(), nprocs, gen, cols, rows);
            system(path);

            fo = fopen("./output.txt", "r");

            fscanf(fo, "%lf", &time);
            avgTime += time;

            fclose(fo);
        }
        printf("%d %d %lf\n", nprocs, gen, avgTime / 10.0);
    }
    return 0;
}
