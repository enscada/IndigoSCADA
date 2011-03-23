// specification file for an unlimited queue for storing bytes

#ifndef CRYPTOPP_QUEUE_H
#define CRYPTOPP_QUEUE_H

#include "cryptlib.h"
#include <algorithm>

NAMESPACE_BEGIN(CryptoPP)

/** The queue is implemented as a linked list of arrays, but you don't need to
    know about that.  So just ignore this next line. :) */
class ByteQueueNode;

//! Byte Queue
class ByteQueue : public BufferedTransformation
{
public:
	ByteQueue(unsigned int m_nodeSize=256);
	ByteQueue(const ByteQueue &copy);
	~ByteQueue();

	unsigned long MaxRetrievable() const
		{return CurrentSize();}
	bool AnyRetrievable() const
		{return !IsEmpty();}

	void Put(byte inByte);
	void Put(const byte *inString, unsigned int length);

	unsigned int Get(byte &outByte);
	unsigned int Get(byte *outString, unsigned int getMax);

	unsigned int Peek(byte &outByte) const;
	unsigned int Peek(byte *outString, unsigned int peekMax) const;

	unsigned long Skip(unsigned long skipMax=ULONG_MAX);
	unsigned long TransferTo(BufferedTransformation &target, unsigned long transferMax=ULONG_MAX);
	unsigned long CopyTo(BufferedTransformation &target, unsigned long copyMax=ULONG_MAX) const;

	// these member functions are not inherited
	unsigned long CurrentSize() const;
	bool IsEmpty() const;

	void Clear();

	void Unget(byte inByte);
	void Unget(const byte *inString, unsigned int length);

	const byte * Spy(unsigned int &contiguousSize) const;

	byte * MakeNewSpace(unsigned int &contiguousSize);
	void OccupyNewSpace(unsigned int size);

	void LazyPut(const byte *inString, unsigned int size);
	void FinalizeLazyPut();

	ByteQueue & operator=(const ByteQueue &rhs);
	bool operator==(const ByteQueue &rhs) const;
	byte operator[](unsigned long i) const;
	void swap(ByteQueue &rhs);

	class Walker : public BufferedTransformation
	{
	public:
		Walker(const ByteQueue &queue)
			: m_queue(queue), m_node(queue.m_head), m_position(0), m_offset(0)
			, m_lazyString(queue.m_lazyString), m_lazyLength(queue.m_lazyLength) {}

		unsigned long MaxRetrievable() const
			{return m_queue.CurrentSize() - m_position;}

		void Put(byte inByte) {}
		void Put(const byte *inString, unsigned int length) {}

		unsigned int Get(byte &outByte);
		unsigned int Get(byte *outString, unsigned int getMax);

		unsigned int Peek(byte &outByte) const;
		unsigned int Peek(byte *outString, unsigned int peekMax) const;

		unsigned long Skip(unsigned long skipMax=ULONG_MAX);
		unsigned long TransferTo(BufferedTransformation &target, unsigned long transferMax=ULONG_MAX);
		unsigned long CopyTo(BufferedTransformation &target, unsigned long copyMax=ULONG_MAX) const;

	private:
		const ByteQueue &m_queue;
		const ByteQueueNode *m_node;
		unsigned int m_position, m_offset;
		const byte *m_lazyString;
		unsigned int m_lazyLength;
	};

	friend class Walker;

private:
	void CleanupUsedNodes();
	void CopyFrom(const ByteQueue &copy);
	void Destroy();

	unsigned int m_nodeSize;
	ByteQueueNode *m_head, *m_tail;
	const byte *m_lazyString;
	unsigned int m_lazyLength;
};

//! use this to make sure LazyPut is finalized in event of exception
class LazyPutter
{
public:
	LazyPutter(ByteQueue &bq, const byte *inString, unsigned int size)
		: m_bq(bq) {bq.LazyPut(inString, size);}
	~LazyPutter()
		{try {m_bq.FinalizeLazyPut();} catch(...) {}}
private:
	ByteQueue &m_bq;
};

NAMESPACE_END

NAMESPACE_BEGIN(std)
template<> inline void swap(CryptoPP::ByteQueue &a, CryptoPP::ByteQueue &b)
{
	a.swap(b);
}
NAMESPACE_END

#endif
