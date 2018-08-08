#pragma once
#include "exec_elf.h"
#include <QFile>


class ElfReader 
{
public:
	ElfReader(const char* sopath, const char *dumppath);
	~ElfReader();

	bool Load();

	size_t phdr_count() { return phdr_num_; }
	Elf32_Addr load_start() { return reinterpret_cast<Elf32_Addr>(load_start_); }
	Elf32_Addr load_size() { return load_size_; }
	Elf32_Addr load_bias() { return load_bias_; }
	const Elf32_Phdr* loaded_phdr() { return loaded_phdr_; }

private:
	bool OpenElf();
	bool ReadElfHeader();
	bool VerifyElfHeader();
	bool ReadProgramHeader();
	static size_t phdr_table_get_load_size(const Elf32_Phdr* phdr_table, 
		size_t phdr_count, Elf32_Addr* out_min_vaddr, Elf32_Addr* out_max_vaddr = 0);
	bool ReserveAddressSpace();
	bool LoadSegments();
	bool FindPhdr();
	bool CheckPhdr(Elf32_Addr);

	char* sopath_;		//����so
	char* dumppath_;	//���޸�dump so, ���Ϊ��˵���޸�����so

	QFile sofile_;
	QFile dumpfile_;

	Elf32_Ehdr header_;			//elf�ļ�ͷ��
	size_t phdr_num_;			//����ͷ��������

	void* phdr_mmap_;			//����ͷ���ڴ�ӳ���ҳ��ʼ��ַ
	Elf32_Phdr* phdr_table_;	//����ͷ����ڴ��ַ
	Elf32_Addr phdr_size_;		//����ͷ����ռҳ��С

	// First page of reserved address space. ���������ڶμ��صĵ�ַ�ռ����ʼҳ���ڴ�ӳ����ʼҳ
	void* load_start_;
	// Size in bytes of reserved address space. ������ַ�ռ�Ĵ�С���ڴ�ӳ���С
	Elf32_Addr load_size_;
	// Load bias. ʵ���ڴ�ӳ���������ڴ�ӳ���ƫ��
	Elf32_Addr load_bias_;

	// Loaded phdr. �Ѽ��ص�PT_PHDR��, ���һ���ɼ��صĶ�PT_LOAD�����ļ�ƫ��p_offsetΪ0�Ķ�
	const Elf32_Phdr* loaded_phdr_;
};
