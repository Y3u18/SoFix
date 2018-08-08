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
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString sopath;
	QString dumppath;

	qout << QSTR8BIT("����������so�ļ�·��:") << endl;
	qin >> sopath;

	qout << QSTR8BIT("������dump so�ļ�·��:") << endl;
	qin >> dumppath;

	elfFixSo(sopath.toLocal8Bit(), dumppath.toLocal8Bit());
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
		if (loadedFile.open(QIODevice::ReadWrite))
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
		}
	}
	else
	{
		qout << QSTR8BIT("so����ʧ��, ���ܲ�����Ч��so�ļ�") << endl;
	}
	return true;
}