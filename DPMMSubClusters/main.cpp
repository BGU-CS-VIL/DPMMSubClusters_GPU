#include <stdio.h>
#include "module_tests.h"


int main(int argc, char** argv)
{
    printf("Eigen uses %ld threads\n", Eigen::nbThreads());

    module_tests mt;
   // mt.RandomMessHighDim();
    mt.CheckMemoryLeak();

	return EXIT_SUCCESS;
}