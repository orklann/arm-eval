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

int parse_mov() {
	unsigned int code = 0xD2800000;
	//unsigned int code = 0xD2800020;
	unsigned int mov_opc = 0xD2800000;
	unsigned int mov_rd = 0x1f;
	unsigned int mov_imm = 0x1FFFE0;
	if ((code & mov_opc) == mov_opc) {
		printf("MOV instruction detected!\n");
		unsigned int rd = code & mov_rd;
		unsigned int imm12 = (code & mov_imm) >> 5;
		printf("MOV rd = %u\n", rd);
		printf("MOV imm12 = %u\n", imm12);
		return 1;
	}
}

int parse_add() {
	unsigned int code = 0x91000421;
	unsigned int add_opc = 0x91000000;
	unsigned int add_rn = 0x3e0;
	unsigned int add_rd = 0xf;
	unsigned int add_imm12 = 0x3ffc00;
	if ((code & add_opc) == add_opc) {
		printf("Add instruction detected!\n");
		unsigned int rn = (code & add_rn) >> 5;
		printf("Add rn = %d\n", rn);
		unsigned int rd = code & add_rd;
		printf("Add rd = %d\n", rd);
		unsigned int imm12 = (code & add_imm12) >> 10;
		printf("Add imm12 = %d\n", imm12);
		return 1;
	}
}

int main() {
  //run_from_rwx();
  parse_mov();
  parse_add();
}
