#pragma once

#include "exec_elf.h"

#define SOINFO_NAME_LEN 128
typedef void(*linker_function_t)();

struct link_map_t
{
	uintptr_t l_addr;
	char*  l_name;
	uintptr_t l_ld;
	link_map_t* l_next;
	link_map_t* l_prev;
};

struct soinfo
{
public:
	char name[SOINFO_NAME_LEN];
	const Elf32_Phdr* phdr; //�Ѽ��ص�PT_PHDR��, ���һ���ɼ��صĶ�PT_LOAD�����ļ�ƫ��p_offsetΪ0�Ķ�
	size_t phnum;
	Elf32_Addr entry;
	Elf32_Addr base;	//�ɼ��صĶεĻ�ַ
	unsigned size;		//�ɼ��صĶεĴ�С

	uint32_t unused1;  // DO NOT USE, maintained for compatibility.

	Elf32_Dyn* dynamic; //PT_DYNAMIC

	uint32_t unused2; // DO NOT USE, maintained for compatibility
	uint32_t unused3; // DO NOT USE, maintained for compatibility

	soinfo* next;    //�������һ��so
	unsigned flags;

	//DT_STRTAB
	const char* strtab;

	//DT_SYMTAB
	Elf32_Sym* symtab;

	//DT_HASH (�����ĸ��ֶε�ַ��������), ���Ź�ϣ��
	//�ȸ��� bucket[hash % nbucket]�õ����ű�������chain������, �ٸ���chain���ѯ
	size_t nbucket;
	size_t nchain;
	unsigned* bucket;	//���������˷��ű��������chain�������
	unsigned* chain;	//ÿ������Ϊ���ű����������һ��chain��

						//DT_PLTGOT
	unsigned* plt_got;

	//DT_JMPREL, DT_PLTRELSZ, �ض�λ
	Elf32_Rel* plt_rel;
	size_t plt_rel_count;

	//DT_REL, DT_RELSZ, �ض�λ
	Elf32_Rel* rel;
	size_t rel_count;

	//DT_PREINIT_ARRAY, DT_PREINIT_ARRAYSZ
	linker_function_t* preinit_array;
	size_t preinit_array_count;

	//DT_INIT_ARRAY, DT_INIT_ARRAYSZ ������ɺ����øú�������
	linker_function_t* init_array;
	size_t init_array_count;

	//DT_FINI_ARRAY, DT_FINI_ARRAYSZ
	linker_function_t* fini_array;
	size_t fini_array_count;

	//DT_INIT ������ɺ����øú���
	linker_function_t init_func;

	//DT_FINI
	linker_function_t fini_func;

	//ARM EABI section used for stack unwinding. PT_ARM_EXIDX����ջչ��
	unsigned* ARM_exidx;
	size_t ARM_exidx_count;

	size_t ref_count;		//���ü���
	link_map_t link_map;

	bool constructors_called; //�������Ƿ��ѵ���, ȷ��ֻ����һ��

							  // When you read a virtual address from the ELF file, add this
							  // value to get the corresponding address in the process' address space.
	Elf32_Addr load_bias; //ʵ�ʶμ��ػ�ַ�������μ��ػ�ַ��ƫ��

						  //�Ƿ���DT_TEXTREL, DT_SYMBOLIC
	bool has_text_relocations;
	bool has_DT_SYMBOLIC;
};

size_t strlcpy(char *dst, const char *src, size_t siz);
soinfo* soinfo_alloc(const char* name);
void phdr_table_get_dynamic_section(const Elf32_Phdr* phdr_table,
	int               phdr_count,
	Elf32_Addr        load_bias,
	Elf32_Dyn**       dynamic,
	size_t*           dynamic_count,
	Elf32_Word*       dynamic_flags);

int phdr_table_get_arm_exidx(const Elf32_Phdr* phdr_table,
	int               phdr_count,
	Elf32_Addr        load_bias,
	Elf32_Addr**      arm_exidx,
	unsigned*         arm_exidx_count);
