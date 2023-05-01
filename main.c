#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>


// Allocates RWX memory of given size and returns a pointer to it. On failure,
// prints out the error and returns NULL.
void* alloc_executable_memory(size_t size) {
  void* ptr = mmap(0, size,
                   PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void*)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

void emit_code_into_memory(unsigned char* m) {
  unsigned char code[] = {
    0x48, 0x89, 0xf8,                   // mov %rdi, %rax
    0x48, 0x83, 0xc0, 0x04,             // add $4, %rax
    0xc3                                // ret
  };
  memcpy(m, code, sizeof(code));
}

const size_t SIZE = 1024;
typedef long (*JittedFunc)(long);

// Allocates RWX memory directly.
void run_from_rwx() {
  void* m = alloc_executable_memory(SIZE);
  emit_code_into_memory(m);

  JittedFunc func = m;
  int result = func(3);
  /* Read eax into i */
  int i;
  asm("\t movl %%eax,%0" : "=r"(i));
  printf("eax = %d\n", i);
  printf("result = %d\n", result);
}

int parse() {
	unsigned int code = 0xD2800000;
	//unsigned int code = 0xD2800001;
	unsigned int mov_opc = 0xD2800000;
	unsigned int mov_rd = 0x1f;
	if ((code & mov_opc) == mov_opc) {
		printf("MOV instruction detected!\n");
		unsigned int rd = code & mov_rd;
		printf("MOV rd = %u\n", rd);
		return 1;
	}
}

int main() {
  //run_from_rwx();
  parse();
}
