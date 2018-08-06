#include "Util.h"
#include <windows.h>

void * Util::mmap(void *addr, uint32_t size, QFile &file, uint32_t offset)
{
	void *ret = nullptr;
	size = PAGE_END(size);
	addr = (void *)PAGE_END((size_t)addr);
	if (file.seek(offset))
	{
		ret = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
		assert(((uint32_t)ret % 0x1000 == 0));
		qint64 rc = file.read((char *)ret, size);
		if (rc < 0)
		{
			VirtualFree(ret, size, MEM_DECOMMIT);
			return nullptr;
		}

		if (addr)
		{
			memcpy(addr, ret, size);
			VirtualFree(ret, size, MEM_DECOMMIT);
			ret = addr;
		}
	}

	return ret;
}

void * Util::mmap(void *addr, uint32_t size)
{
	size = PAGE_END(size);
	addr = (void *)PAGE_END((size_t)addr);
	if (addr)
	{
		memset(addr, 0, size);
		return addr;
	}
	else
	{
		return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
	}
}

int Util::munmap(void *addr, uint32_t size)
{
	size = PAGE_END(size);
	addr = (void *)PAGE_END((size_t)addr);
	if (addr)
	{
		VirtualFree(addr, size, MEM_DECOMMIT);
	}
	
	return 0;
}

void Util::kmpGetNext(const char *p, int pSize, int next[])
{
	int pLen = pSize;
	next[0] = -1;
	int k = -1;
	int j = 0;
	while (j < pLen - 1)
	{
		//p[k]��ʾǰ׺��p[j]��ʾ��׺    
		if (k == -1 || p[j] == p[k])
		{
			++j;
			++k;
			//��֮ǰnext�����󷨣��Ķ�������4��  
			if (p[j] != p[k])
				next[j] = k;   //֮ǰֻ����һ��  
			else
				//��Ϊ���ܳ���p[j] = p[ next[j ]]�����Ե�����ʱ��Ҫ�����ݹ飬k = next[k] = next[next[k]]  
				next[j] = next[k];
		}
		else
		{
			k = next[k];
		}
	}
}

int Util::kmpSearch(const char *s, int sSize,const char *p, int pSize)
{
	int i = 0;
	int j = 0;
	int sLen = sSize;
	int pLen = pSize;
	int *next = new int[pSize];

	memset(next, 0, pSize * sizeof(int));
	kmpGetNext(p, pSize, next);

	while (i < sLen && j < pLen)
	{
		//1. ���j = -1, ���ߵ�ǰ�ַ�ƥ��ɹ�����S[i] == P[j], ����i++, j++      
		if (j == -1 || s[i] == p[j])
		{
			i++;
			j++;
		}
		else
		{
			//2. ���j != -1, �ҵ�ǰ�ַ�ƥ��ʧ�ܣ���S[i] != P[j], ���� i ���䣬j = next[j]      
			//next[j]��Ϊj����Ӧ��nextֵ        
			j = next[j];
		}
	}

	delete next;
	if (j == pLen)
	{
		return i - j;
	}
	else
	{
		return -1;
	}
}
