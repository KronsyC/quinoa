#pragma once
#include<string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/prctl.h>

void print_trace();
void error(std::string reason, bool trace=false);