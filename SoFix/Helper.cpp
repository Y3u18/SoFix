#include "Helper.h"
#include "ElfReader.h"
#include "QTextStream"
#include "ElfFixer.h"

#define QSTR8BIT(s) (QString::fromLocal8Bit(s))

const Helper::Command Helper::cmdSo =
{
	QSTR8BIT("��ѡ���޸�����:\n1.����So�ļ�\n2.Dump So�ļ�"),
	2,
	{ elfFixNormalSo , elfFixDumpSo }
};

void Helper::elfFixNormalSo()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString sopath;

	qout << QSTR8BIT("����������so�ļ�·��:") << endl;
	qin >> sopath;

	elfFixSo(sopath.toLocal8Bit(), nullptr);
}

void Helper::elfFixDumpSo()
{
	
}

bool Helper::elfFixSo(const char *sopath, const char *dumppath)
{
	const char *name = dumppath ? dumppath : sopath;
	bool dump = dumppath ? true : false;
	ElfReader elf_reader(name, dump);
	QTextStream qout(stdout);

	if (elf_reader.Load())
	{
		QString loadedpath = QSTR8BIT(name) + ".loaded";
		QFile loadedFile(loadedpath);
		if (loadedFile.open(QIODevice::ReadWrite))
		{
			loadedFile.write((char *)elf_reader.load_start(), elf_reader.load_size());
			qout << QSTR8BIT("���سɹ�!���غ��ļ�·��: ") + loadedpath << endl;

			const char* bname = strrchr(name, '\\');//�������һ�γ���"/"֮����ַ���, ����ȡ����·�����ļ���
			soinfo* si = soinfo_alloc(bname ? bname + 1 : name);//����soinfo�ڴ�, ���ڴ��ʼ��Ϊ0, name����
			if (si == NULL) 
			{
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

			QFile sofile(QSTR8BIT(sopath));
			Elf32_Ehdr ehdr;
			if (sofile.open(QIODevice::ReadOnly | QIODevice::ExistingOnly))
			{
				qint64 rc = sofile.read((char *)&ehdr, sizeof(Elf32_Ehdr));
				if (rc != sizeof(Elf32_Ehdr))
				{
					return false;
				}
			}

			QString fixedpath = QSTR8BIT(name) + ".fixed";
			
			ElfFixer elf_fixer(si, ehdr, fixedpath.toLocal8Bit());
			if (elf_fixer.fix() && elf_fixer.write())
			{
				qout << QSTR8BIT("�޸��ɹ�!�޸����ļ�·��: ") + fixedpath << endl;
			}
		}
	}

	return true;
}