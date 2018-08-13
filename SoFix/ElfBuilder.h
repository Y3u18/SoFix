#pragma once
#include "exec_elf.h"
#include <QVector>
#include <QJsonDocument>
#include <QFile>

class ElfBuilder
{
private:
	QString sopath_;
	QFile sofile_;

	QJsonDocument json_doc;
	QFile json_file_;
	QString json_;	//json�ļ�����

	Elf32_Addr load_bias_;
	QVector<Elf32_Phdr> phdrs_;
	QStringList phdr_datapaths;
	QVector<Elf32_Dyn> dyns_;

	Elf32_Ehdr ehdr_;
	Elf32_Phdr ph_myload_;
	Elf32_Phdr ph_phdr_;
	Elf32_Phdr ph_dynamic_;

public:
	ElfBuilder(QString json);
	~ElfBuilder();

	//��json�ļ��ж�ȡso�ı�Ҫ��Ϣ
	bool ReadJson();

	//����json�е���Ϣ����so�ļ�
	bool BuildInfo();

	//������д��, �õ�so�ļ�
	bool Write();

	bool Build();
};
