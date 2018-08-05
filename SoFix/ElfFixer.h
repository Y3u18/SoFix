#pragma once
#include "linker.h"
#include <QFile>

class ElfFixer
{
private:
	enum ShIdx
	{
		SI_NULL = 0,
		SI_DYNSYM,
		SI_DYNSTR,
		SI_HASH,
		SI_RELDYN,
		SI_RELPLT,
		SI_PLT,
		SI_TEXT,
		SI_ARMEXIDX,
		SI_FINI_ARRAY,
		SI_INIT_ARRAY,
		SI_DYNAMIC,
		SI_GOT,
		SI_DATA,
		SI_BSS,
		SI_SHSTRTAB
	};

	static const char strtab[];
	static Elf32_Word GetShdrName(int idx);

	char *name_;	//���޸��ļ�
	soinfo *si_;
	QFile file_;

	Elf32_Ehdr ehdr_;	//ͨ��������so�ļ���ȡ

	//ͨ��soinfo��ȡ
	const Elf32_Phdr *phdr_;
	size_t phnum_;

	//ͨ��soinfo�е�phdr_ֱ�ӻ�ȡ PT_DYNAMIC, PT_ARM_EXIDX
	Elf32_Shdr dynamic_;
	Elf32_Shdr arm_exidex;

	//����dynamic_��ȡ
	Elf32_Shdr hash_;
	Elf32_Shdr dynsym_;
	Elf32_Shdr dynstr_;
	Elf32_Shdr rel_dyn_;
	Elf32_Shdr rel_plt_;
	Elf32_Shdr init_array_;
	Elf32_Shdr fini_array_;

	//ͨ���ڵĹ�ϵ��������ȡ
	Elf32_Shdr plt_;
	Elf32_Shdr got_;
	Elf32_Shdr data_;
	Elf32_Shdr bss_;
	Elf32_Shdr text_;

	//SHT_UNDEF
	Elf32_Shdr shnull_;

	//.shstrtab
	Elf32_Shdr shstrtab_;

public:
	ElfFixer(soinfo *si, Elf32_Ehdr ehdr, const char *name);
	~ElfFixer();
	bool fix();

private:
	//�޸�Ehdr
	bool fixEhdr();

	//�޸�Phdr
	bool fixPhdr();

	//�޸�����Shdr, ����, �ļ�ƫ��
	bool fixShdr();

	bool showAll();

	//��Phdr���޸�����Shdr: .dynamic, .arm.exidx
	bool fixShdrFromPhdr();

	//��.dynamic���޸�����Shdr
	bool fixShdrFromDynamic();

	//����Shdr�Ĺ�ϵ�޸�
	bool fixShdrFromShdr();

	//���ڴ��ַתΪ�ļ�ƫ��, -1��ʾʧ��
	Elf32_Off addrToOff(Elf32_Addr addr);
};

