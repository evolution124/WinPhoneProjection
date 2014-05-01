#pragma once

#include "stdafx.h"
#include <list>

template<typename ComInterface>
class IComInterfaceQueue : protected std::list<ComInterface*>,public IUnknown
{
	typedef std::list<ComInterface*> supper;
private:
	ULONG RefCount = 1;
	CRITICAL_SECTION cs;
public: //IUnknown
	IFACEMETHODIMP QueryInterface(REFIID iid,void** ppvObject)
	{
		return ppvObject == NULL ? E_POINTER:E_NOINTERFACE;
	}
	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return (ULONG)InterlockedIncrement(&RefCount);
	}
	IFACEMETHODIMP_(ULONG) Release()
	{
		ULONG _RefCount = (ULONG)InterlockedDecrement(&RefCount);
		if (_RefCount == 0)
			delete this;
		return _RefCount;
	}
public:
	IComInterfaceQueue()
	{
			InitializeCriticalSectionEx(&cs,0,CRITICAL_SECTION_NO_DEBUG_INFO);
	}
	~IComInterfaceQueue()
	{
		DeleteCriticalSection(&cs);
	}
public:
	void PushIUnknown(ComInterface* p)
	{
		if (p == NULL)
			return;
		p->AddRef();
		supper::push_back(p);
	}

	ComInterface* PopIUnknown()
	{
		if (empty())
			return NULL;
		ComInterface* p = NULL;
		p = supper::front();
		supper::pop_front();
		return p;
	}

	DECLSPEC_NOINLINE void PushIUnknownSafe(ComInterface* p)
	{
		EnterCriticalSection(&cs);
		PushIUnknown(p);
		LeaveCriticalSection(&cs);
	}

	DECLSPEC_NOINLINE ComInterface* PopIUnknownSafe()
	{
		EnterCriticalSection(&cs);
		ComInterface* p = PopIUnknown();
		LeaveCriticalSection(&cs);
		return p;
	}

	bool empty() const
	{
		return supper::empty();
	}

	size_t size() const
	{
		return supper::size();
	}

	void clear()
	{
		for (auto i = supper::begin();i != supper::end();++i)
		{
			auto p = *i;
			if (p)
				p->Release();
		}
		supper::clear();
	}
};