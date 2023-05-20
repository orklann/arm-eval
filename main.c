#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

unsigned char cache_code[1024];
unsigned int code_length = 0;

uint64_t R8= 0, R9 = 0, R10 = 0;
uint64_t a, b, c;

#define save_registers \
  	asm("movq %%r8, %0" : "=r"(R8)); \
  	asm("movq %%r9, %0" : "=r"(R9)); \
  	asm("movq %%r10, %0" : "=r"(R10)); \

#define load_registers \
	unsigned char code1[] = { \
		0x49, 0xc7, 0xc0, \
		(R8 & 0x000000FF), \
		(R8 & 0x0000FF00) >> 8, \
		(R8 & 0x00FF0000) >> 16,\
		(R8 & 0xFF000000) >> 24,\
					\
		0x49, 0xc7, 0xc1,	\
		(R9 & 0x000000FF), 	\
		(R9 & 0x0000FF00) >> 8, \
		(R9 & 0x00FF0000) >> 16,\
		(R9 & 0xFF000000) >> 24,\
					\
		0x49, 0xc7, 0xc2,	\
					\
		(R10 & 0x000000FF), 	\
		(R10 & 0x0000FF00) >> 8,\
		(R10 & 0x00FF0000) >> 16,\
		(R10 & 0xFF000000) >> 24,\
	};				\
	emit_code(code1, sizeof(code1) / sizeof(code1[0]));\

void emit_code(unsigned char* user_code, int code_size) {
	memcpy(cache_code + code_length, user_code, code_size); 
	code_length += code_size;
}


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
void run_from_rwx(unsigned char *code, int size) {
  void* m = alloc_executable_memory(1024);
  emit_code_into_memory(m, code, size);

  JittedFunc func = m;

  int result = func(3);
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
		x[rd] = imm12;
		printf("x[rd] = %lu\n", x[rd]);
		if (rd == 0) {
			unsigned char code[] = {
				// mov r8, imm12
				0x49, 0xc7, 0xc0, 
				(imm12 & 0x000000FF), 
				(imm12 & 0x0000FF00) >> 8, 
				(imm12 & 0x00FF0000) >> 16, 
				(imm12 & 0xFF000000) >> 24,
			};

			emit_code(code, sizeof(code) / sizeof(code[0]));
		} else if (rd == 1) {
			unsigned char code[] = {
				// mov r9, imm12
				0x49, 0xc7, 0xc1, 
				(imm12 & 0x000000FF), 
				(imm12 & 0x0000FF00) >> 8, 
				(imm12 & 0x00FF0000) >> 16, 
				(imm12 & 0xFF000000) >> 24,
			};

			emit_code(code, sizeof(code) / sizeof(code[0]));
		} else if (rd == 2) {
			unsigned char code[] = {
				// mov r10, imm12
				0x49, 0xc7, 0xc2, 
				(imm12 & 0x000000FF), 
				(imm12 & 0x0000FF00) >> 8, 
				(imm12 & 0x00FF0000) >> 16, 
				(imm12 & 0xFF000000) >> 24,
			};
			
			emit_code(code, sizeof(code) / sizeof(code[0]));
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
			unsigned char code[] = {
				// mov r11, imm12
				0x49, 0xc7, 0xc3, 
				(imm12 & 0x000000FF), 
				(imm12 & 0x0000FF00) >> 8, 
				(imm12 & 0x00FF0000) >> 16, 
				(imm12 & 0xFF000000) >> 24,
				0x4d, 0x01, 0xd9,
			};
			
			emit_code(code, sizeof(code) / sizeof(code[0]));
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

		if (rd == 0) {
			// mov r8, r9
			unsigned char code[] = {
				0x4d, 0x01, 0xd0,
			};
			
			emit_code(code, sizeof(code) / sizeof(code[0]));
		}
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
		unsigned char code[] = {
			0x4d, 0x89, 0xcf,
			0x49, 0x83, 0xef, imm12,
		};
		
		emit_code(code, sizeof(code) / sizeof(code[0]));

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
		0xF101903F,
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
			printf("cache code size: %d\n", code_length);
			for (int i = 0; i < code_length; i++) {
				printf("%x ", cache_code[i]);
			}
			unsigned char ret[] = {0xc3};
			memcpy(cache_code + code_length, ret, 1);
			code_length += 1;
			run_from_rwx(cache_code, code_length);
			save_registers;
			int64_t r15;
  			asm("\t movq %%r15,%0" : "=r"(r15));
			printf("r15 = %ld\n", r15);
			printf("===============================\n");
			printf("Saved registers r8 = %ld, r9= %ld, r10 = %ld\n", R8, R9, R10);
			if (r15 < 0) {
				parse_bne(code);
				code_length = 0;
				load_registers;
			}
		}
		pc++;
	}
	return 0;
}

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
	eval();
	//run_code();
	/*printf("flag = %d\n", flag);
	printf("x0 = %lu\n", x[0]);
	*/
}

