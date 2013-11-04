#include "main.h"
#include "runPandora.h"
int main(int argc, char **argv) {
	runPandora *myPandora;
	myPandora->init();
	myPandora->loopEvent();
	myPandora->finalize();

}

