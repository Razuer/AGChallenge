#pragma once
class CRefCounter
{
public:
	CRefCounter() { i_count = 0; }
	int iAdd() { return(++i_count); }
	int iDec() { return(--i_count); };
	int iGet() { return(i_count); }
private:
	int i_count;
};

template <typename T>
class SmartPtr
{
public:
	SmartPtr()
	{
		pc_pointer = NULL;
		pc_counter = NULL;
	}

	SmartPtr(T* pcPointer)
	{
		pc_pointer = pcPointer;
		pc_counter = new CRefCounter();
		pc_counter->iAdd();
	}

	SmartPtr(const SmartPtr& pcOther)
	{
		pc_pointer = pcOther.pc_pointer;
		pc_counter = pcOther.pc_counter;
		pc_counter->iAdd();
	}

	~SmartPtr()
	{
		if (pc_counter->iDec() == 0)
		{
			delete pc_pointer;
			delete pc_counter;
		}
	}
	T& operator*() { return(*pc_pointer); }
	T* operator->() { return(pc_pointer); }

	SmartPtr& operator=(SmartPtr&& pcOther) noexcept;

	SmartPtr cDuplicate();

	void operator<<=(SmartPtr<T>& other) {
		swap(pc_pointer, other.pc_pointer);
		swap(pc_counter, other.pc_counter);
	}

private:
	CRefCounter* pc_counter;
	T* pc_pointer;
};

template <typename T>
SmartPtr<T>& SmartPtr<T>::operator=(SmartPtr&& pcOther) noexcept {
	if (this != &pcOther) {
		if (pc_pointer != NULL) {
			delete this;
		}
		pc_pointer = pcOther.pc_pointer;
		pc_counter = pcOther.pc_counter;
		pc_counter->iAdd();
	}

	return *this;
}

template <typename T>
SmartPtr<T> SmartPtr<T>::cDuplicate() {

	return (SmartPtr<T>(*this));
}