#include <unistd.h>

int main(){
	char message[] = "Hello, World!\n";
	syscall(1, 1, message, sizeof(message)-1);
	return 0;
}