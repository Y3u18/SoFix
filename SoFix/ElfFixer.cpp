#include "ElfFixer.h"
#include "linker.h"
#include <QDebug>
#include "Util.h"

#define DEBUG qDebug
#define DL_ERR qDebug

const char ElfFixer::strtab[] =
"\0.dynsym\0.dynstr\0.hash\0.rel.dyn\0.rel.plt\0.plt\0.text\0.ARM.exidx\0"
".fini_array\0.init_array\0.dynamic\0.got\0.data\0.bss\0.shstrtab\0";

Elf32_Word ElfFixer::GetShdrName(int idx)
{
	Elf32_Word ret = 0;
	const char *str = strtab;
	for (int i = 0; i < idx; i++)
	{
		ret += strlen(str) + 1;
		str += strlen(str) + 1;
	}

	return ret;
}

ElfFixer::ElfFixer(soinfo *si, Elf32_Ehdr ehdr, const char *name)
	: si_(si), ehdr_(ehdr), phdr_(nullptr), phnum_(0)
{
	if (name)
	{
		name_ = strdup(name);
		file_.setFileName(name);
	}

	memset(&hash_, 0, sizeof(hash_));
	memset(&dynsym_, 0, sizeof(dynsym_));
	memset(&dynstr_, 0, sizeof(dynstr_));
	memset(&rel_dyn_, 0, sizeof(rel_dyn_));
	memset(&rel_plt_, 0, sizeof(rel_plt_));
	memset(&init_array_, 0, sizeof(init_array_));
	memset(&fini_array_, 0, sizeof(fini_array_));
	memset(&plt_, 0, sizeof(plt_));
	memset(&got_, 0, sizeof(got_));
	memset(&data_, 0, sizeof(data_));
	memset(&bss_, 0, sizeof(bss_));
	memset(&text_, 0, sizeof(text_));
	memset(&shnull_, 0, sizeof(shnull_));
	memset(&shstrtab_, 0, sizeof(shstrtab_));
}

ElfFixer::~ElfFixer()
{
	if (name_)
	{
		free(name_);
	}
	file_.close();
}

bool ElfFixer::fix()
{
	return fixEhdr() &&
		fixPhdr() &&
		fixShdr() &&
		showAll();
}

bool ElfFixer::fixEhdr()
{
	return true;
}

bool ElfFixer::fixPhdr()
{
	if (si_)
	{
		phdr_ = si_->phdr;
		phnum_ = si_->phnum;
		return true;
	}

	return false;
}

bool ElfFixer::fixShdr()
{
	return fixShdrFromPhdr() &&
		fixShdrFromDynamic() &&
		fixShdrFromShdr();
}

#define SHOW_SECTION(s) DEBUG("%s\t%08X\t%08X\t%08X\t%08X\n", \
	strtab + s.sh_name, s.sh_addr, s.sh_offset, s.sh_size, s.sh_entsize);

bool ElfFixer::showAll()
{
	DEBUG("Name\tAddr\tOff\tSize\tEs\n");
	SHOW_SECTION(shnull_);
	SHOW_SECTION(dynsym_);
	SHOW_SECTION(dynstr_);
	SHOW_SECTION(hash_);
	SHOW_SECTION(rel_dyn_);
	SHOW_SECTION(rel_plt_);
	SHOW_SECTION(plt_);
	SHOW_SECTION(text_);
	SHOW_SECTION(arm_exidex);
	SHOW_SECTION(init_array_);
	SHOW_SECTION(fini_array_);
	SHOW_SECTION(dynamic_);
	SHOW_SECTION(got_);
	SHOW_SECTION(data_);
	SHOW_SECTION(bss_);
	SHOW_SECTION(shstrtab_);
	return true;
}

bool ElfFixer::fixShdrFromPhdr()
{
	shstrtab_.sh_name = GetShdrName(SI_SHSTRTAB);
	shstrtab_.sh_type = SHT_STRTAB;
	shstrtab_.sh_flags = 0;
	shstrtab_.sh_addr = 0;
	shstrtab_.sh_offset = 0;	//����, ��������ͷ�����ַ������������
	shstrtab_.sh_size = sizeof(strtab);
	shstrtab_.sh_link = 0;
	shstrtab_.sh_info = 0;
	shstrtab_.sh_addralign = 1;
	shstrtab_.sh_entsize = 0;

	phdr_table_get_dynamic_section(phdr_, phnum_, si_->load_bias, &si_->dynamic, NULL, NULL);
	dynamic_.sh_name = GetShdrName(SI_DYNAMIC);
	dynamic_.sh_type = SHT_DYNAMIC;
	dynamic_.sh_flags = SHF_WRITE | SHF_ALLOC;
	dynamic_.sh_addr = (Elf32_Addr)si_->dynamic - si_->load_bias;
	dynamic_.sh_offset = addrToOff(dynamic_.sh_addr);
	//dynamic_.sh_size = 0;		//����, ������DT_NULL����ȷ����С
	dynamic_.sh_link = SI_DYNSTR;
	dynamic_.sh_info = 0;
	dynamic_.sh_addralign = 4;
	dynamic_.sh_entsize = sizeof(Elf32_Dyn);

	(void)phdr_table_get_arm_exidx(phdr_, phnum_, si_->load_bias, &si_->ARM_exidx, &si_->ARM_exidx_count);
	arm_exidex.sh_name = GetShdrName(SI_ARMEXIDX);
	arm_exidex.sh_type = SHT_AMMEXIDX;
	arm_exidex.sh_flags = SHF_ALLOC | SHF_LINK_ORDER;
	arm_exidex.sh_addr = (Elf32_Addr)si_->ARM_exidx - si_->load_bias;
	arm_exidex.sh_offset = addrToOff(arm_exidex.sh_addr);
	arm_exidex.sh_size = si_->ARM_exidx_count * 8;
	arm_exidex.sh_link = SI_TEXT;
	arm_exidex.sh_info = 0;
	arm_exidex.sh_addralign = 4;
	arm_exidex.sh_entsize = 8;
	
	return true;
}

bool ElfFixer::fixShdrFromDynamic()
{
	/* "base" might wrap around UINT32_MAX. */
	Elf32_Addr base = si_->load_bias;
	dynamic_.sh_size = 0;

	// Extract useful information from dynamic section. ��ȡ��̬���е�������Ϣ������DT_
	// ��ȡ����entry������(��ַ��ֵ), DT_NULLΪ�ýڵĽ�����־

	for (Elf32_Dyn* d = si_->dynamic; d->d_tag != DT_NULL; ++d)
	{
		dynamic_.sh_size += sizeof(Elf32_Dyn);	//����DT�õ�dynamic_��׼ȷ��С
		//DEBUG("d = %p, d[0](tag) = 0x%08x d[1](val) = 0x%08x", d, d->d_tag, d->d_un.d_val);
		switch (d->d_tag)
		{
		case DT_HASH:
			si_->nbucket = ((unsigned *)(base + d->d_un.d_ptr))[0];
			si_->nchain = ((unsigned *)(base + d->d_un.d_ptr))[1];
			si_->bucket = (unsigned *)(base + d->d_un.d_ptr + 8);
			si_->chain = (unsigned *)(base + d->d_un.d_ptr + 8 + si_->nbucket * 4);

			hash_.sh_name = GetShdrName(SI_HASH);
			hash_.sh_type = SHT_HASH;
			hash_.sh_flags = SHF_ALLOC;
			hash_.sh_addr = d->d_un.d_ptr;
			hash_.sh_offset = addrToOff(hash_.sh_addr);
			hash_.sh_size = (2 + si_->nbucket + si_->nchain) * sizeof(Elf32_Word);
			hash_.sh_link = ShIdx::SI_DYNSYM;	//.dynsym�Ľ�����
			hash_.sh_info = 0;
			hash_.sh_addralign = 4;
			hash_.sh_entsize = sizeof(Elf32_Word);
			break;
		case DT_STRTAB:
			si_->strtab = (const char *)(base + d->d_un.d_ptr);

			dynstr_.sh_name = GetShdrName(SI_DYNSTR);
			dynstr_.sh_type = SHT_STRTAB;
			dynstr_.sh_flags = SHF_ALLOC;
			dynstr_.sh_addr = d->d_un.d_ptr;
			dynstr_.sh_offset = addrToOff(dynstr_.sh_addr);
			//dynstr_.sh_size = 0;	//����, ��������ֶβ��Ǳ����, ��DT_STRSZ��ȡ��С���ܲ�׼ȷ
			dynstr_.sh_link = 0;
			dynstr_.sh_info = 0;
			dynstr_.sh_addralign = 1;
			dynstr_.sh_entsize = 0;
			break;
		case DT_STRSZ:
			dynstr_.sh_size = d->d_un.d_val;	//��DT_STRSZ��ȡ��С���ܲ�׼ȷ
			break;
		case DT_SYMTAB:
			si_->symtab = (Elf32_Sym *)(base + d->d_un.d_ptr);

			dynsym_.sh_name = GetShdrName(SI_DYNSYM);
			dynsym_.sh_type = SHT_DYNSYM;
			dynsym_.sh_flags = SHF_ALLOC;
			dynsym_.sh_addr = d->d_un.d_ptr;
			dynsym_.sh_offset = addrToOff(dynsym_.sh_addr);
			//dynsym_.sh_size = 0;		//����, �������ݷ���HASH�����
			dynsym_.sh_link = ShIdx::SI_DYNSTR;	//.dynstr�Ľ�����
			dynsym_.sh_info = 1;	//���һ���ֲ����ŵķ��ű�����ֵ��һ, ��ʱ��1, ����
			dynsym_.sh_addralign = 4;
			dynsym_.sh_entsize = sizeof(Elf32_Sym);
			break;
		case DT_JMPREL:
			si_->plt_rel = (Elf32_Rel*)(base + d->d_un.d_ptr);

			rel_plt_.sh_name = GetShdrName(SI_RELPLT);
			rel_plt_.sh_type = SHT_REL;
			rel_plt_.sh_flags = SHF_ALLOC | SHF_INFO_LINK;
			rel_plt_.sh_addr = d->d_un.d_ptr;
			rel_plt_.sh_offset = addrToOff(rel_plt_.sh_addr);
			//rel_plt_.sh_size = 0;		//����, ��������DT_PLTRELSZ��ȡ׼ȷ��С
			rel_plt_.sh_link = ShIdx::SI_DYNSYM;	//.dynsym�Ľ�����
			rel_plt_.sh_info = ShIdx::SI_GOT;		//.got�Ľ�����
			rel_plt_.sh_addralign = 4;
			rel_plt_.sh_entsize = sizeof(Elf32_Rel);
			break;
		case DT_PLTRELSZ:
			si_->plt_rel_count = d->d_un.d_val / sizeof(Elf32_Rel);

			rel_plt_.sh_size = d->d_un.d_val;	//����DT_PLTRELSZ��ȡ.rel.plt׼ȷ��С
			break;
		case DT_REL:
			si_->rel = (Elf32_Rel*)(base + d->d_un.d_ptr);

			rel_dyn_.sh_name = GetShdrName(SI_RELDYN);
			rel_dyn_.sh_type = SHT_REL;
			rel_dyn_.sh_flags = SHF_ALLOC;
			rel_dyn_.sh_addr = d->d_un.d_ptr;
			rel_dyn_.sh_offset = addrToOff(rel_dyn_.sh_addr);
			//rel_dyn_.sh_size = 0;	//����, ������DT_RELSZȷ��׼ȷ��С
			rel_dyn_.sh_link = ShIdx::SI_DYNSYM;	//.dynsym�Ľ�����
			rel_dyn_.sh_info = 0;
			rel_dyn_.sh_addralign = 4;
			rel_dyn_.sh_entsize = sizeof(Elf32_Rel);
			break;
		case DT_RELSZ:
			si_->rel_count = d->d_un.d_val / sizeof(Elf32_Rel); 

			rel_dyn_.sh_size = d->d_un.d_val;	//��ȡ.rel.dyn׼ȷ��С
			break;
		case DT_PLTGOT:
			/* Save this in case we decide to do lazy binding. We don't yet. */
			si_->plt_got = (unsigned *)(base + d->d_un.d_ptr);	//_global_offset_table_�������ַ, ���Ǳ�����Ϣ, ���ֻ��������׼ȷ�޸�
			break;
		case DT_INIT_ARRAY:
			si_->init_array = reinterpret_cast<linker_function_t*>(base + d->d_un.d_ptr);
			DEBUG("%s constructors (DT_INIT_ARRAY) found at %p", si_->name, si_->init_array);
			
			init_array_.sh_name = GetShdrName(SI_INIT_ARRAY);	//����
			init_array_.sh_type = SHT_INIT_ARRAY;
			init_array_.sh_flags = SHF_WRITE | SHF_ALLOC;
			init_array_.sh_addr = d->d_un.d_ptr;
			init_array_.sh_offset = addrToOff(init_array_.sh_addr);
			//init_array_.sh_size = 0;	//����, ���Ը���DT_INIT_ARRAYSZ��ȡ׼ȷ��С
			init_array_.sh_link = 0;
			init_array_.sh_info = 0;
			init_array_.sh_addralign = 4;
			init_array_.sh_entsize = 4;
			break;
		case DT_INIT_ARRAYSZ:
			si_->init_array_count = ((unsigned)d->d_un.d_val) / sizeof(Elf32_Addr);
			
			init_array_.sh_size = d->d_un.d_val;	//��ȡ.init_array��׼ȷ��С
			break;
		case DT_FINI_ARRAY:
			si_->fini_array = reinterpret_cast<linker_function_t*>(base + d->d_un.d_ptr);
			DEBUG("%s destructors (DT_FINI_ARRAY) found at %p", si_->name, si_->fini_array);
			
			fini_array_.sh_name = GetShdrName(SI_FINI_ARRAY);	//����
			fini_array_.sh_type = SHT_FINI_ARRAY;
			fini_array_.sh_flags = SHF_WRITE | SHF_ALLOC;
			fini_array_.sh_addr = d->d_un.d_ptr;
			fini_array_.sh_offset = addrToOff(fini_array_.sh_addr);
			//fini_array_.sh_size = 0;	//����, ���Ը���DT_FINI_ARRAYSZ��ȡ׼ȷ��С
			fini_array_.sh_link = 0;
			fini_array_.sh_info = 0;
			fini_array_.sh_addralign = 4;
			fini_array_.sh_entsize = 4;
			break;
		case DT_FINI_ARRAYSZ:
			si_->fini_array_count = ((unsigned)d->d_un.d_val) / sizeof(Elf32_Addr);
			
			fini_array_.sh_size = d->d_un.d_val;	//��ȡ.fini_array��׼ȷ��С
			break;
		}

	}

	//DEBUG("si->base = 0x%08x, si->strtab = %p, si->symtab = %p", si_->base, si_->strtab, si_->symtab);
	return true;
}

bool ElfFixer::fixShdrFromShdr()
{
	return true;
}

Elf32_Off ElfFixer::addrToOff(Elf32_Addr addr)
{
	const Elf32_Phdr* phdr = si_->phdr;
	const Elf32_Phdr* phdr_limit = phdr + si_->phnum;
	Elf32_Off off = -1;

	for (phdr = si_->phdr; phdr < phdr_limit; phdr++)
	{
		if (phdr->p_type != PT_LOAD)
		{
			continue;
		}

		Elf32_Addr seg_start = phdr->p_vaddr;	//�ڴ�ӳ��ʵ����ʼ��ַ
		Elf32_Addr seg_end = seg_start + phdr->p_memsz;	//�ڴ�ӳ��ʵ����ֹ��ַ

		Elf32_Addr seg_page_start = PAGE_START(seg_start); //�ڴ�ӳ��ҳ�׵�ַ
		Elf32_Addr seg_page_end = PAGE_END(seg_end);	   //�ڴ�ӳ����ֹҳ

		Elf32_Addr seg_file_end = seg_start + phdr->p_filesz; //�ļ�ӳ����ֹҳ

		// File offsets.
		Elf32_Addr file_start = phdr->p_offset;
		Elf32_Addr file_end = file_start + phdr->p_filesz;

		Elf32_Addr file_page_start = PAGE_START(file_start); //�ļ�ӳ��ҳ�׵�ַ
		Elf32_Addr file_length = file_end - file_page_start; //�ļ�ӳ���С

		//���û�п�дȨ��, �����ļ���ʵ��ӳ���С
		if ((phdr->p_flags & PF_W) == 0)
		{
			seg_file_end = PAGE_END(seg_file_end);
		}

		//�ж�addr�Ƿ���LOAD����
		if (addr >= seg_page_start && addr < seg_file_end)
		{
			off = file_page_start + addr - seg_page_start;
			break;
		}
	}

	return off;
}

