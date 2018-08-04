#pragma once
#include <QFile>
#include <stdint.h>

#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE-1))
#define PAGE_START(x)  ((x) & PAGE_MASK)
#define PAGE_END(x)    (PAGE_START((x) + (PAGE_SIZE-1)))
#define PAGE_OFFSET(x) ((x) & ~PAGE_MASK)

typedef struct 
{
	void *aligned;
} AlignedMemory;

class Util
{
public:
	Util() = delete;
	~Util() = delete;

	//��ȡ�ļ����ݵ��ڴ���, ��addrΪNULL���·����ڴ�, ʹ��VirtualAlloc��֤��ҳ����, addr, size����ǿ��ҳ����
	static void *mmap(void *addr, uint32_t size, QFile &file, uint32_t offset);

	//���addrΪNULL���·����ڴ�, �����ڴ���Ϊ0
	static void *mmap(void *addr, uint32_t size);

	//�ͷ���mmap������ڴ�
	static int munmap(void *addr, uint32_t size);
};

