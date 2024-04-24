#pragma once
#include <iostream>
using namespace std;

#define i_DEFAULT_NAME "DefaultTable"
#define i_DEFAULT_SIZE 10
#define DEBUG 0

template<typename T> class CTable {
public:
	CTable();
	CTable(string sName, int iTableLen);
	CTable(const CTable& pcOther);
	~CTable();

	int getLength();
	void vSetName(string sName);
	bool bSetNewSize(int iTableLen);
	CTable* pcClone();

	void printTab();

	CTable& operator= (CTable&& pcNewVal) noexcept;

	CTable operator--(int);

	CTable operator+ (const CTable& pcNewVal);

	void vSetValueAt(int iOffset, T iNewVal);
	CTable cCreateTab();

	T getValueAt(int iOffset);
	void v_copy(const CTable& cOther);

	CTable operator=(const CTable& cOther);


	CTable(CTable&& cOther) noexcept;
	

private:
	string s_name;
	T* tab;
	int length;
	void transportData(CTable& cOther);
};

template<typename T>
CTable<T>::CTable() {
	s_name = i_DEFAULT_NAME;
	tab = new T[i_DEFAULT_SIZE];
	length = i_DEFAULT_SIZE;

	if (DEBUG)cout << "bezp: '" + s_name << "'" << endl;
}

template<typename T>
CTable<T>::CTable(string sName, int iTableLen) {
	s_name = sName;
	tab = new T[iTableLen];
	length = iTableLen;

	if (DEBUG) cout << "parametr: '" + s_name << "'" << endl;
}

template<typename T>
CTable<T>::CTable(const CTable& pcOther) {
	s_name = pcOther.s_name + "_copy";
	length = pcOther.length;
	tab = new T[length];

	for (int i = 0; i < length; i++) {
		tab[i] = pcOther.tab[i];
	}
	if (DEBUG)cout << "kopiuj: '" + s_name << "'" << endl;
}

template<typename T>
CTable<T>::CTable(CTable&& cOther) noexcept
{
	transportData(cOther);

	if (DEBUG)cout << "move: '" + s_name << "'" << endl;
}

template<typename T>
CTable<T>::~CTable() {
	delete[] tab;
	if (DEBUG)cout << "usuwam: '" + s_name << "'" << endl;
}

template<typename T>
int CTable<T>::getLength() {
	return length;
}

template<typename T>
void CTable<T>::vSetName(string sName) {
	if (DEBUG)cout << "Changing name from: '" << s_name << "' to: '" << sName << "'" << endl;
	s_name = sName;
}

template<typename T>
bool CTable<T>::bSetNewSize(int iTableLen) {
	if (iTableLen < 0) {
		return false;
	}
	else if (iTableLen == length) {
		return false;
	}
	else {
		if (DEBUG)cout << "Changing size from: '" << length << "' to: '" << iTableLen << "'" << endl;
		T* newTab = new T[iTableLen];

		if (iTableLen > length) {

			for (int i = 0; i < length; i++) {
				newTab[i] = tab[i];
			}
		}
		else for (int i = 0; i < iTableLen; i++) {
			newTab[i] = tab[i];
		}

		delete[] tab;
		tab = newTab;
		length = iTableLen;

		return true;
	}
}

template<typename T>
CTable<T>* CTable<T>::pcClone() {
	return new CTable(*this);
}

template <typename T>
void CTable<T>::printTab() {
	cout << "Name: " << s_name << ", length: " << length << endl;
	for (int i = 0; i < length; i++) {
		cout << "tab[" << i << "] = " << tab[i] << endl;
	}
	cout << endl;
}

template<typename T>
CTable<T> CTable<T>::cCreateTab()
{
	CTable c_result;
	c_result.bSetNewSize(5);
	return(move(c_result));
}

template <typename T>
void CTable<T>::v_copy(const CTable& cOther)
{
	if (tab != NULL) delete this;

	tab = new T[cOther.length];
	length = cOther.length;
	for (int ii = 0; ii < cOther.length; ii++)
		tab[ii] = cOther.tab[ii];
}

template<typename T>
CTable<T> CTable<T>::operator=(const CTable& cOther)
{
	if (tab != NULL) delete tab;
	v_copy(cOther);
	if (DEBUG)cout << "op=\n";
	return(*this);
}

template<typename T>
CTable<T>& CTable<T>::operator=(CTable&& cOther) noexcept {
	if (DEBUG)cout << "op= MS\n";

	if (this != &cOther) {
		if (tab != NULL) delete tab;

		transportData(cOther);
	}

	return *this;
}

template<typename T>
void CTable<T>::transportData(CTable& cOther) {
	tab = cOther.tab;
	s_name = cOther.s_name;
	length = cOther.length;

	cOther.tab = NULL;
	cOther.length = 0;
	cOther.s_name = "hollow_tab";
}

template<typename T>
CTable<T> CTable<T>::operator+ (const CTable& pcNewVal) {
	if (DEBUG) cout << "op+\n";

	CTable result;
	result.bSetNewSize(length + pcNewVal.length);
	result.vSetName("op+_tab");

	for (int i = 0; i < result.length; i++) {
		if (i < length) result.vSetValueAt(i, tab[i]);
		else result.vSetValueAt(i, pcNewVal.tab[i - length]);
	}

	return move(result);
}

template<typename T>
void CTable<T>::vSetValueAt(int iOffset, T iNewVal) {
	if (iOffset < length && iOffset >= 0) tab[iOffset] = iNewVal;
}

template<typename T>
T CTable<T>::getValueAt(int iOffset) {
	if (iOffset < length && iOffset >= 0) return tab[iOffset];;
}

template<typename T>
CTable<T> CTable<T>::operator--(int) {
	bSetNewSize(length - 1);
	return move(*this);
}
