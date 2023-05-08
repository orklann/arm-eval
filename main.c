#include <stdio.h>
#include <stdint.h>
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

int parse_mov(unsigned int code) {
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

int parse_add(unsigned int code) {
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

int parse_add_register(unsigned int code) {
	unsigned int add_opc = 0x8B000000;
	unsigned int add_rd = 0x1f;
	unsigned int add_rn = 0x3e0;
	unsigned int add_rm = 0x1F0000;
	if ((code & add_opc) == add_opc) {
		printf("Add register instruction detected!\n");
		unsigned int rd = code & add_rd;
		printf("Add rd = %d\n", rd);
		unsigned int rn = (code & add_rn) >> 5;
		printf("Add rn = %d\n", rn);
		unsigned int rm = (code & add_rm) >> 16;
		printf("Add rm = %d\n", rm);
	}
	return 1;
}

int parse_cmp(unsigned int code) {
	unsigned int cmp_opc = 0xF1000000;
	unsigned int cmp_rn = 0x3e0;
	unsigned int cmp_imm12 = 0x3FFC00;
	if ((code & cmp_opc) == cmp_opc) {
		printf("CMP instruction detected!\n");
		unsigned int rn = (code & cmp_rn) >> 5;
		printf("CMP rn = %d\n", rn);
		unsigned int imm12 = (code & cmp_imm12) >> 10;
		printf("CMP imm12 = %d\n", imm12);
	}
	return 1;
}

int parse_bne(unsigned int code) {
	unsigned int bne_opc = 0x54000000;
	if ((code & bne_opc) == bne_opc) {
		printf("BNE instruction detected!\n");
	}
	return 1;
}

int eval() {
	int codes[] = {
    		0xD2800000,
		0xD2800001,
		0xD2800042,
		0x91000421,
		0x8B020000,
		0xF101903F,
		0x54FFFFA1
	};
	uint64_t x[5];
	int pc = 0;
	int len = 7;
	unsigned int mov_opc = 0xD2800000;
	unsigned int add_opc = 0x91000000;
	unsigned int add_register_opc = 0x8B000000;
	unsigned int cmp_opc = 0xF1000000;
	unsigned int bne_opc = 0x54000000;
	while (pc <= len -1) {
		unsigned int code = codes[pc];
		if ((code & mov_opc) == mov_opc) {
			parse_mov(code);
		} 

		if ((code & add_opc) == add_opc) {
			parse_add(code);
		}
		if ((code & add_register_opc) == add_register_opc) {
			parse_add_register(code);
		}
		if ((code & cmp_opc) == cmp_opc) {

			parse_cmp(code);
		}
		if ((code & bne_opc) == bne_opc) {
			parse_bne(code);
		}
		pc++;
	}
	return 0;
}

int main() {
	//run_from_rwx();
	eval();
}
