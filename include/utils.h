#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <unistd.h>
#include <stdio.h>
#include "pnetcdf.h"
#include <DataReceiver.h>
using namespace std;
void getCoords(int rank, int size, int *coords, int *dims, MPI_Comm comm_compute);

void writeBinaryFile(int rank, char filename[], int global[3], int start[3], int count[3], int *buf);

void readBinaryFile(char filename[]);

void random_sleep(int min_seconds, int max_seconds, int world_rank);
void printStartCount(int start[], int count[]);
