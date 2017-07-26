#pragma once
#include <crtdbg.h>
#include <string.h>
#include "Spinlock.h"

struct Buffer
{
	char * buffer_;
	unsigned int size_;
	unsigned int offset_;
	unsigned int head_;
	core::Spinlock lock_;

	~Buffer()
	{
		delete buffer_;
	}

	void Init(unsigned int size)
	{
		buffer_ = new char[size];
		size_ = size;
		offset_ = 0;
		head_ = 0;
		SecureZeroMemory(buffer_, size_);
	}

	void SetHead(unsigned int offset)
	{
		head_ = offset;
	}

	char * Read()
	{
		return buffer_ + head_;
	}

	void Consume(unsigned int length)
	{
		memmove(buffer_, buffer_ + offset_, size_ - length);
		offset_ -= length;
		head_ = 0;
		_ASSERT(offset_ >= 0);
		lock_.Unlock();
	}

	void Produce(unsigned int length)
	{
		lock_.Lock();
		offset_ += length;
		_ASSERT(offset_ <= size_);
	}

	unsigned int GetRemained()
	{
		return size_ - offset_;
	}

	char * GetBuffer()
	{
		return buffer_ + offset_;
	}
};