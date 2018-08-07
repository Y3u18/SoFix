#pragma once
#include "linker.h"
#include <QFile>

class ElfFixer
{
private:

	//��ͷ, �����������������Ӧ��
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
		SI_RODATA,
		SI_FINI_ARRAY,
		SI_INIT_ARRAY,
		SI_DATA_REL_RO,
		SI_DYNAMIC,
		SI_GOT,
		SI_DATA,
		SI_BSS,
		SI_SHSTRTAB,
		SI_MAX
	};

	Elf32_Shdr shdrs_[SI_MAX];
	static const char strtab[];

	static Elf32_Word GetShdrName(int idx);

	char *name_;	//�޸����ļ�����
	soinfo *si_;	//���޸�dump so����ElfReader��������so�ļ��õ���
	QFile file_;

	Elf32_Ehdr ehdr_;	//ͨ��������so�ļ���ȡ

	//ͨ��soinfo��ȡ
	const Elf32_Phdr *phdr_;
	size_t phnum_;

public:
	ElfFixer(soinfo *si, Elf32_Ehdr ehdr, const char *name);
	~ElfFixer();
	bool fix();
	bool write();

private:
	//�޸�Ehdr
	bool fixEhdr();

	//�޸�Phdr
	bool fixPhdr();

	//�޸�����Shdr, ����, �ļ�ƫ��
	bool fixShdr();

	//��Phdr���޸�����Shdr: .dynamic, .arm.exidx
	bool fixShdrFromPhdr();

	//��.dynamic���޸�����Shdr:  .hash, .dynsym, .dynstr, .rel.dyn, .rel.plt, .init_array, fini_array
	bool fixShdrFromDynamic();

	//����.dynamic��.dynsym���õ��ַ�����ȷ��.dynstr�ڵĴ�С
	bool fixDynstr();

	//����.hash,.rel.plt,.rel.dyn���õķ�����Ϣ��ȷ��.dynsym, .dynstr�ڵĴ�С
	bool fixDynsym();

	//����Shdr�Ĺ�ϵ�޸� .plt, .text, .got, .data, .bss
	bool fixShdrFromShdr();

	//���ļ��м�¼���ڴ��ַתΪ�ļ�ƫ��, -1��ʾʧ��
	Elf32_Off addrToOff(Elf32_Addr addr);

	Elf32_Addr offToAddr(Elf32_Off off);

	//�����ļ��м�¼���ڴ��ַ���ڵĽ�, -1��ʾû�ҵ�
	int findShIdx(Elf32_Addr addr);
};

