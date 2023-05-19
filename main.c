#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>


// Allocates RWX memory of given size and returns a pointer to it. On failure,
// prints out the error and returns NULL.
void* alloc_executable_memory(size_t size) { void* ptr = mmap(0, size,
                   PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void*)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

void emit_code_into_memory(unsigned char* m, unsigned char *code, int size) {
  memcpy(m, code, size);
}

const size_t SIZE = 1024;
typedef long (*JittedFunc)(long);

uint64_t x[5] = {0 , 0, 0, 0, 0};
int pc = 0;
unsigned int flag = 0;

// Allocates RWX memory directly.
void run_from_rwx(unsigned char *code) {
  void* m = alloc_executable_memory(1024);
  /*
  unsigned char code[] = {
    0x48, 0xc7, 0xc3, 0x01, 0x00, 0x00, 0x00,
    0xc3                                // ret
  };
  */
  emit_code_into_memory(m, code, 1);

  JittedFunc func = m;
  int result = func(3);
}

/*
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
		x[rd] = imm12;
		printf("x[rd] = %lu\n", x[rd]);
		if (rd == 0) {
			unsigned char code[] = {
				0x48, 0xc7, 0xc0, 
				(imm12 & 0x000000FF), 
				(imm12 & 0x0000FF00) >> 8, 
				(imm12 & 0x00FF0000) >> 16, 
				(imm12 & 0xFF000000) >> 24,
				0xc3                // ret
			};
			run_from_rwx(code);
			int i;
			asm("\t movl %%eax,%0" : "=r"(i));
			printf("rax = %d\n", i);
		} else if (rd == 1) {
			unsigned char code[] = {
				0x48, 0xc7, 0xc3, 
				(imm12 & 0x000000FF), 
				(imm12 & 0x0000FF00) >> 8, 
				(imm12 & 0x00FF0000) >> 16, 
				(imm12 & 0xFF000000) >> 24,
				0xc3                // ret
			};
			run_from_rwx(code);
			int i;
			asm("\t movl %%ebx,%0" : "=r"(i));
			printf("rbx = %d\n", i);
		} else if (rd == 2) {
			unsigned char code[] = {
				0x48, 0xc7, 0xc1, 
				(imm12 & 0x000000FF), 
				(imm12 & 0x0000FF00) >> 8, 
				(imm12 & 0x00FF0000) >> 16, 
				(imm12 & 0xFF000000) >> 24,
				0xc3                // ret
			};
			
			printf("imm12=%d\n", imm12);
			run_from_rwx(code);
			int i;
			asm("\t movl %%ecx,%0" : "=r"(i));
			printf("rcx = %d\n", i);
		}
		return 1;
	}
}

int parse_add(unsigned int code) {
	unsigned int add_opc = 0x91000000;
	unsigned int add_rn = 0x3e0;
	unsigned int add_rd = 0x1F;
	unsigned int add_imm12 = 0x3ffc00;
	if ((code & add_opc) == add_opc) {
		printf("Add instruction detected!\n");
		printf("Code = 0x%x\n", code);
		unsigned int rn = (code & add_rn) >> 5;
		printf("Add rn = %d\n", rn);
		unsigned int rd = code & add_rd;
		printf("Add rd = %d\n", rd);
		unsigned int imm12 = (code & add_imm12) >> 10;
		printf("Add imm12 = %d\n", imm12);
		x[rd] = x[rn] + imm12;
		if (rd == 1) {
			// mov rbx, 0
			// mov rdx, imm
			unsigned char code[] = {
				0x48, 0xc7, 0xc3, 0x00, 0x00, 0x00, 0x00,
				0x48, 0xc7, 0xc1, 0x01, 0x00, 0x00, 0x00,
				0x48, 0x01, 0xcb,
				0xc3                // ret
			};
			
			run_from_rwx(code);
			int i;
			asm("\t movl %%ebx,%0" : "=r"(i));
			printf("rbx = %d\n", i);
		}
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
		x[rd] = x[rn] + x[rm];
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
		printf("x[1] = %lu\n", x[1]);
		if (x[rn] == imm12) {
			flag = 1;
		}
	}
	return 1;
}

int parse_bne(unsigned int code) {
	unsigned int bne_opc = 0x54000000;
	if ((code & bne_opc) == bne_opc) {
		printf("BNE instruction detected!\n");
		if (flag == 0) {
			pc -= 4;
		}
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
		0xF13E803F,
		0x54FFFFA1
	};
	int len = 7;
	unsigned int mov_opc = 0xD2800000;
	unsigned int add_opc = 0x91000000;
	unsigned int add_register_opc = 0x8B000000;
	unsigned int cmp_opc = 0xF100001F;
	unsigned int bne_opc = 0x54000000;
	while (pc <= len -1) {
		unsigned int code = codes[pc];
		if ((code & cmp_opc) == cmp_opc) {
			parse_cmp(code);
		} else if ((code & mov_opc) == mov_opc) {
			parse_mov(code);
		} else if ((code & add_opc) == add_opc && code != 0xF101903F) {
			parse_add(code);
		} else if ((code & add_register_opc) == add_register_opc) {
			parse_add_register(code);
		} else if ((code & bne_opc) == bne_opc) {
			break;
			parse_bne(code);
		}
		printf("x0 = %lu\n", x[0]);
		printf("x1 = %lu\n", x[1]);
		printf("x2 = %lu\n", x[2]);
		pc++;
	}
	return 0;
}

*/
void run_code() {
  void* m = alloc_executable_memory(1024);
  
  unsigned char code[] = {
	0x48, 0xc7, 0xc3, 0x00, 0x00, 0x00, 0x00,
	0x48, 0xc7, 0xc1, 0x01, 0x00, 0x00, 0x00,
	0x48, 0x01, 0xcb,
	0xc3                // ret
  };

  emit_code_into_memory(m, code, sizeof(code) / sizeof(code[0]));

  JittedFunc func = m;
  int result = func(3);

  int i;
  asm("\t movl %%ebx,%0" : "=r"(i));
  printf("rbx = %d\n", i);
}


int main() {
	//run_from_rwx();
	//eval();
	run_code();
	/*printf("flag = %d\n", flag);
	printf("x0 = %lu\n", x[0]);
	*/
}

