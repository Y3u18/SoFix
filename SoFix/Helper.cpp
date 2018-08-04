#include "Helper.h"
#include "ElfReader.h"
#include "QTextStream"

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

	elfFixSo(sopath, false);
}

void Helper::elfFixDumpSo()
{
	
}

void Helper::elfFixSo(QString &sopath, bool dump)
{
	ElfReader elfReader(sopath.toStdString().c_str(), dump);
	QTextStream qout(stdout);

	if (elfReader.Load() && !dump)
	{
		QString loadedpath = sopath + ".loaded";
		QFile loadedFile(loadedpath);
		if (loadedFile.open(QIODevice::ReadWrite))
		{
			loadedFile.write((char *)elfReader.load_start(), elfReader.load_size());
			qout << QSTR8BIT("���سɹ�!���غ��ļ�·��: ") + loadedpath << endl;
		}
	}
}