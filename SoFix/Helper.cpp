#include "Helper.h"
#include "ElfReader.h"
#include "QTextStream"
#include "ElfFixer.h"
#include <QSettings>
#include "Util.h"

#define QSTR8BIT(s) (QString::fromLocal8Bit(s))

const Helper::Command Helper::cmdSo =
{
	QSTR8BIT("------------Android Arm so Fix Tool, By Youlor, Reference ThomasKing------------\n"
	"��ѡ���޸��ļ�����:\n1.����So�ļ�\n2.Dump So�ļ�(������so�ļ������޸�)"
	"\n3.Dump So�ļ�(ֱ���޸�)"),
	3,
	{ elfFixNormalSo , elfFixDumpSoFromNormal, elfFixDumpSo }
};

void Helper::elfFixNormalSo()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString sopath;

	qout << QSTR8BIT("��������޸�������so�ļ�·��:") << endl;
	qin >> sopath;

	elfFixSo(sopath.toLocal8Bit(), nullptr);
}

void Helper::elfFixDumpSoFromNormal()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString sopath;
	QString dumppath;

	qout << QSTR8BIT("���������ڸ����޸�������so�ļ�·��:") << endl;
	qin >> sopath;

	qout << QSTR8BIT("��������޸���dump so�ļ�·��:") << endl;
	qin >> dumppath;

	elfFixSo(sopath.toLocal8Bit(), dumppath.toLocal8Bit());
}

bool Helper::elfDumpSoToNormal(QString &dumppath)
{
	QString normalpath;
	QTextStream qout(stdout);

	normalpath = dumppath + ".normal";
	QFile normalFile(normalpath);

	ElfReader elf_reader(nullptr, dumppath.toLocal8Bit());
	if (elf_reader.Load() && normalFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		const Elf32_Phdr* phdr = elf_reader.loaded_phdr();
		const Elf32_Phdr* phdr_limit = phdr + elf_reader.phdr_count();
		Elf32_Addr load_bias = (Elf32_Addr)elf_reader.load_bias();

		for (phdr = elf_reader.loaded_phdr(); phdr < phdr_limit; phdr++)
		{
			if (phdr->p_type != PT_LOAD)
			{
				continue;
			}

			Elf32_Addr seg_start = phdr->p_vaddr + load_bias;	//�ڴ�ӳ��ʵ����ʼ��ַ
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
				file_end = PAGE_END(file_end);
				file_length = PAGE_END(file_length);
			}

			if (file_length != 0) //���ڴ�д���ļ�
			{
				normalFile.seek(file_page_start);
				normalFile.write((char *)seg_page_start, file_length);
			}
		}
	}

	normalFile.seek(0);
	normalFile.write((char *)&elf_reader.header(), sizeof(Elf32_Ehdr));
	normalFile.close();
	qout << QSTR8BIT("��ԭΪ�ļ�so�ɹ�: ") + normalpath << endl;
	return true;
}

void Helper::elfFixDumpSo()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString dumppath;
	QString normalpath;

	qout << QSTR8BIT("��������޸���dump so�ļ�·��:") << endl;
	qin >> dumppath;
	normalpath = dumppath + ".normal";

	//��dump soתΪ�����ļ�so, �ٵ���elfFixNormalSo�޸���-_-
	//��...Խ��Խ�� ����-_- �ļ�so���ܿ�~_~
	if (elfDumpSoToNormal(dumppath))
	{
		//elfFixSo(normalpath.toLocal8Bit(), nullptr);
	}
}

//���dumppathΪ����ֱ���޸�����so�ļ�, ������ݸ�������so�ļ��޸�dump�ļ�
bool Helper::elfFixSo(const char *sopath, const char *dumppath)
{
	const char *name = dumppath ? dumppath : sopath;
	ElfReader elf_reader(sopath, dumppath);
	QTextStream qout(stdout);

	if (elf_reader.Load())
	{
		QString loadedpath = QSTR8BIT(name) + ".loaded";
		QFile loadedFile(loadedpath);
		if (loadedFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			loadedFile.write((char *)elf_reader.load_start(), elf_reader.load_size());
			qout << QSTR8BIT("���سɹ�!���غ��ļ�·��: ") + loadedpath << endl;

			const char* bname = strrchr(name, '\\');//�������һ�γ���"/"֮����ַ���, ����ȡ����·�����ļ���
			soinfo* si = soinfo_alloc(bname ? bname + 1 : name);//����soinfo�ڴ�, ���ڴ��ʼ��Ϊ0, name����
			if (si == NULL)
			{
				qout << QSTR8BIT("��Ǹ, �ļ������ܳ���128���ַ�!") << endl;
				return NULL;
			}

			//��ʼ��soinfo�������ֶ�
			si->base = elf_reader.load_start();
			si->size = elf_reader.load_size();
			si->load_bias = elf_reader.load_bias();
			si->flags = 0;
			si->entry = 0;
			si->dynamic = NULL;
			si->phnum = elf_reader.phdr_count();
			si->phdr = elf_reader.loaded_phdr();

			QString fixedpath = QSTR8BIT(name) + ".fixed";
			
			ElfFixer elf_fixer(si, sopath, fixedpath.toLocal8Bit());
			if (elf_fixer.fix() && elf_fixer.write())
			{
				qout << QSTR8BIT("�޸��ɹ�!�޸����ļ�·��: ") + fixedpath << endl;
			}
			else
			{
				qout << QSTR8BIT("so�޸�ʧ��, ���ܲ�����Ч��so�ļ�(������DT_DYNAMIC, DT_HASH, DT_STRTAB, DT_SYMTAB)") + fixedpath << endl;
			}
		}
	}
	else
	{
		qout << QSTR8BIT("so����ʧ��, ���ܲ�����Ч��so�ļ�") << endl;
	}
	return true;
}
