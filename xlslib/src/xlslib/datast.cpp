/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * xlslib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * xlslib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with xlslib.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * Copyright 2004 Yeico S. A. de C. V.
 * Copyright 2008 David Hoerl
 *  
 * $Source: /cvsroot/xlslib/xlslib/src/xlslib/datast.cpp,v $
 * $Revision: 1.4 $
 * $Author: dhoerl $
 * $Date: 2009/03/02 04:08:43 $
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * File description:
 *
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <xlsys.h>

#include <datast.h>
#include <systype.h>  // XTRACE2()      [i_a]

// for factory:
#include <record.h>
#include <row.h>
#include <font.h>
#include <format.h>
#include <number.h>
#include <boolean.h>
#include <err.h>
#include <note.h>
#include <formula.h>
#include <merged.h>
#include <label.h>
#include <index.h>
#include <extformat.h>
#include <continue.h>
#include <colinfo.h>
#include <blank.h>
#include <recdef.h>


namespace xlslib_core
{

/* 
***********************************
CDataStorage class Implementation
***********************************
*/

CDataStorage::CDataStorage() :
#if defined(LEIGHTWEIGHT_UNIT_FEATURE)
    store(),
	m_FlushStack(),
	m_FlushLastEndLevel(0),
#else
	data(),
#endif
	m_DataSize(0)
{
#if defined(LEIGHTWEIGHT_UNIT_FEATURE)
	store.reserve(300);
	m_FlushLastEndPos = 0; // .begin();
#else
	data.reserve(100);
#endif
}
	CDataStorage::CDataStorage(size_t blobs) :
#if defined(LEIGHTWEIGHT_UNIT_FEATURE)
    store(),
	m_FlushStack(),
	m_FlushLastEndLevel(0),
#else
	data(), 
#endif
	m_DataSize(0)
{
#if defined(LEIGHTWEIGHT_UNIT_FEATURE)
	store.reserve(blobs);
	m_FlushLastEndPos = 0; // m_FlushStack.begin();
#else
	data.reserve(blobs);
#endif
}

CDataStorage::~CDataStorage()
{
  // Delete all the data. (Only if it exists)
#if defined(LEIGHTWEIGHT_UNIT_FEATURE)
	// flush all lingering units BEFORE we discard the associated UnitStore entities or we'll get a nasty assertion failure.
  FlushEm((unsigned16_t)(-1));

  if (!store.empty())
  {
	StoreList_Itor_t x0, x1;
	size_t cnt = 0;

	x0 = store.begin();
	x1 = store.end();
	for(StoreList_Itor_t di = x0; di != x1; ++di)
	 {
		 di->Reset();
		 cnt++;
	 }
	XTRACE2("ACTUAL: total storage unit count: ", cnt);
#if OLE_DEBUG
	std::cerr << "ACTUAL: total unit count: " << cnt << std::endl;
#endif
	store.resize(0);
  }

#else
  if(!data.empty())
  {
	DataList_Itor_t x0, x1;

	x0 = data.begin();
	x1 = data.end();
	for(DataList_Itor_t di = x0; di != x1; ++di)
	 {
		delete (*di);
	 }
	 data.resize(0);
  }
#endif
}

#if defined(LEIGHTWEIGHT_UNIT_FEATURE)

void CDataStorage::operator+=(CUnit* from)
{
	/*
	constructor already positioned the CUnit; make sure our assumption sticks:

	we assume a usage style like this, i.e. += add in order AND BEFORE the next
	unit is requested/constructed:

		p1 = new CUnit(store, ...);
		...
		store += p1;
		p2 = new CUnit(store, ...);
		...
		store += p2;

	and we currently FAIL for any other 'mixed' usage pattern, e.g. this will b0rk:

		p1 = new CUnit(store, ...);
		p2 = new CUnit(store, ...);
		...
		store += p1;
		store += p2;
	*/
	XL_ASSERT(from->m_Index == store.size() - 1);

	m_DataSize += from->GetDataSize();

	// and 'persist' the data associated with the CUnit from here-on after...
	// (that way, we can safely 'delete CUnit' and still have the data generated by the CUnit intact)
	store[from->m_Index].MakeSticky();
	// and signal that we've made our data 'sticky':
	XL_ASSERT(from->m_Index >= 0);
	from->m_Index = -1 ^ from->m_Index;
	XL_ASSERT(from->m_Index < 0);
}

size_t CDataStorage::GetDataSize() const
{
  return m_DataSize;
}

StoreList_Itor_t CDataStorage::begin()
{
  return store.begin();
}
StoreList_Itor_t CDataStorage::end()
{
  return store.end();
}

#else

void CDataStorage::operator+=(CUnit* from)
{
  data.push_back(from);
  m_DataSize += from->GetDataSize();
}

size_t CDataStorage::GetDataSize() const
{
  return m_DataSize;
}

DataList_Itor_t CDataStorage::begin()
{
  return data.begin();
}
DataList_Itor_t CDataStorage::end()
{
  return data.end();
}

#endif // defined(LEIGHTWEIGHT_UNIT_FEATURE)



#if defined(LEIGHTWEIGHT_UNIT_FEATURE)

signed32_t CDataStorage::RequestIndex(size_t minimum_size)
{
	signed32_t idx = static_cast<signed32_t>(store.size());
	CUnitStore &unit = *store.insert(store.end(), CUnitStore());

	unit.Prepare(minimum_size);

	return idx;
}

CUnitStore& CDataStorage::operator[](signed32_t index)
{
	XL_ASSERT(index != INVALID_STORE_INDEX);
	XL_ASSERT(index >= 0 ? index < store.size() : 1);
	XL_ASSERT(index < 0 ? (-1 ^ index) < store.size() : 1);

	return (index >= 0 ? store[index] : store[-1 ^ index]);
}

void CDataStorage::Push(CUnit* unit)
{
	m_FlushStack.push_back(unit);
}

void CDataStorage::FlushEm(unsigned16_t backpatch_level)
{
	/*
	delete units which don't need to live any longer.

	In the same loop, we shrink the 'stack' for 
	future speed and to keep storage requirements in check.
	*/
	UnitList_Itor_t start = m_FlushStack.begin();
	if (m_FlushLastEndLevel == backpatch_level 
		&& backpatch_level != (unsigned16_t)(-1)	// do not use cached position when 'flushing all'
		//&& m_FlushLastEndPos != m_FlushStack.begin()
		&& m_FlushLastEndPos != m_FlushStack.size()) //.end())
	{
		start = m_FlushStack.begin() + m_FlushLastEndPos;
		start++;
	}

	UnitList_Itor_t j = start;
	size_t cnt = 0;
	size_t cntleft = 0;
	for (UnitList_Itor_t i = j; i != m_FlushStack.end(); i++)
	{
		if ((*i)->m_Backpatching_Level <= backpatch_level)
		{
			XL_ASSERT((*i) != NULL);
			delete (*i);
			(*i) = NULL;
			cnt++;
			continue;
		}

		XL_ASSERT((*i)->m_Backpatching_Level <= 4);

		// do we need to move-copy the unit reference down as part of a shrink operation?
		if (i != j)
		{
			(*j) = (*i);
		}
		j++;
		cntleft++;
	}
	
	size_t count = j - m_FlushStack.begin();

#if OLE_DEBUG
	std::cerr << "number of records deleted: " << cnt << ", left: " << cntleft << ", new.size: " << count << std::endl;
#endif

	m_FlushStack.resize(count);
	XL_ASSERT(m_FlushStack.size() == count);

	// remember for the next time around
	m_FlushLastEndLevel = backpatch_level;
	j = m_FlushStack.end();
	if (m_FlushStack.size() > 0)
	{
		j--;
	}
	else
	{
#if OLE_DEBUG
		std::cerr << "empty!" << std::endl;
#endif
	}
	m_FlushLastEndPos = j - m_FlushStack.begin();
}

void CDataStorage::FlushLowerLevelUnits(const CUnit *unit)
{
	 if (unit && unit->m_Backpatching_Level > 0)
	 {
		FlushEm(unit->m_Backpatching_Level - 1);
	 }
}

CUnit* CDataStorage::MakeCUnit()
{
	return new CUnit(*this);
}

CRecord* CDataStorage::MakeCRecord()
{
	return new CRecord(*this);
}

CRow* CDataStorage::MakeCRow(unsigned32_t rownum,
           unsigned32_t firstcol,
           unsigned32_t lastcol,
           unsigned16_t rowheight,
		   const xf_t* xformat)
{
	return new CRow(*this, rownum, firstcol, lastcol, rowheight, xformat);
}

CBof* CDataStorage::MakeCBof(unsigned16_t boftype)
{
	return new CBof(*this, boftype);
}

CEof* CDataStorage::MakeCEof()
{
	return new CEof(*this);
}

CDimension* CDataStorage::MakeCDimension(unsigned32_t minRow, 
		  unsigned32_t maxRow, 
		  unsigned32_t minCol, 
		  unsigned32_t maxCol)
{
	return new CDimension(*this, minRow, maxRow, minCol, maxCol);
}

CWindow1* CDataStorage::MakeCWindow1(const window1& wind1)
{
	return new CWindow1(*this, wind1);
}

CWindow2* CDataStorage::MakeCWindow2(bool isActive)
{
	return new CWindow2(*this, isActive);
}

CDateMode* CDataStorage::MakeCDateMode()
{
	return new CDateMode(*this);
}

CStyle* CDataStorage::MakeCStyle(const style_t* styledef)
{
	return new CStyle(*this, styledef);
}

CBSheet* CDataStorage::MakeCBSheet(const boundsheet_t* bsheetdef)
{
	return new CBSheet(*this, bsheetdef);
}

CFormat* CDataStorage::MakeCFormat(const format_t* formatdef)
{
	return new CFormat(*this, formatdef);
}

CFont* CDataStorage::MakeCFont(const font_t* fontdef)
{
	return new CFont(*this, fontdef);
}

CNumber* CDataStorage::MakeCNumber(const number_t& numdef)
{
	return new CNumber(*this, numdef);
}

CBoolean* CDataStorage::MakeCBoolean(const boolean_t& booldef)
{
	return new CBoolean(*this, booldef);
}

CErr* CDataStorage::MakeCErr(const err_t& errdef)
{
	return new CErr(*this, errdef);
}

CNote* CDataStorage::MakeCNote(const note_t& notedef)
{
	return new CNote(*this, notedef);
}

CFormula* CDataStorage::MakeCFormula(const formula_t& fdef)
{
	return new CFormula(*this, fdef);
}

CMergedCells* CDataStorage::MakeCMergedCells()
{
	return new CMergedCells(*this);
}

CLabel* CDataStorage::MakeCLabel(const label_t& labeldef)
{
	return new CLabel(*this, labeldef);
}

CIndex* CDataStorage::MakeCIndex(unsigned32_t firstrow, unsigned32_t lastrow)
{
	return new CIndex(*this, firstrow, lastrow);
}

CExtFormat* CDataStorage::MakeCExtFormat(const xf_t* xfdef)
{
	return new CExtFormat(*this, xfdef);
}

CContinue* CDataStorage::MakeCContinue(const unsigned8_t* data, size_t size)
{
	return new CContinue(*this, data, size);
}

CPalette* CDataStorage::MakeCPalette(const color_entry_t *colors)
{
	return new CPalette(*this, colors);
}

CColInfo* CDataStorage::MakeCColInfo(const colinfo_t* newci)
{
	return new CColInfo(*this, newci);
}

CBlank* CDataStorage::MakeCBlank(const blank_t& blankdef)
{
	return new CBlank(*this, blankdef);
}

CCodePage* CDataStorage::MakeCCodePage(unsigned16_t boftype)
{
	return new CCodePage(*this, boftype);
}

CDBCell* CDataStorage::MakeCDBCell(size_t startblock)
{
	return new CDBCell(*this, startblock);
}

HPSFdoc* CDataStorage::MakeHPSFdoc(docType_t dt)
{
	return new HPSFdoc(*this, dt);
}






CUnitStore::CUnitStore():
	m_varying_width(0),
	m_is_in_use(0),
	m_is_sticky(0),
	m_nDataSize(0)
{
	memset(&s, 0, sizeof(s));
	XL_ASSERT(s.vary.m_pData == NULL);
}

CUnitStore::~CUnitStore()
{
	Reset();
	XL_ASSERT(s.vary.m_pData == NULL);
}

/*
This copy constructor is required as otherwise you'd get nuked by CDataStore
when it has to redimension its vector store when more units than
anticipated are requested: internally, STL detroys each unit during this
vector resize operation, so we'll need to copy the data to new space, especially
when we're m_varying_width !!!
*/
CUnitStore::CUnitStore(const CUnitStore &src)
{
	if (&src == this)
	{
		return;
	}

	m_varying_width = src.m_varying_width;
	m_is_in_use = src.m_is_in_use;
	m_is_sticky = src.m_is_sticky;
	m_nDataSize = src.m_nDataSize;
	if (!m_varying_width)
	{
		XL_ASSERT(m_nDataSize <= FIXEDWIDTH_STORAGEUNIT_SIZE);
		memcpy(&s, &src.s, sizeof(s));
	}
	else
	{
		XL_ASSERT(m_is_in_use); 
		XL_ASSERT(src.s.vary.m_nSize > 0);
		s.vary.m_pData = (unsigned8_t *)malloc(src.s.vary.m_nSize);
		if (!s.vary.m_pData)
		{
			// ret = ERR_UNABLE_TOALLOCATE_MEMORY;
			m_nDataSize = s.vary.m_nSize = 0;
		}
		else
		{
			memcpy(s.vary.m_pData, src.s.vary.m_pData, m_nDataSize);
			s.vary.m_nSize = src.s.vary.m_nSize;
		}
	}
}


signed8_t CUnitStore::Prepare(size_t minimum_size)
{
	signed8_t ret = NO_ERRORS;

	// allocate space in the 'variable sized store' if we cannot fit in a fixed-width unit:
	if (minimum_size <= FIXEDWIDTH_STORAGEUNIT_SIZE)
	{
		m_varying_width = 0;
		m_is_in_use = 1;
		m_is_sticky = 0;
		m_nDataSize = 0;
		memset(&s, 0, sizeof(s));

		// range: 0 ... +oo
	}
	else
	{
		m_varying_width = 1;
		m_is_in_use = 1;
		m_is_sticky = 0;
		m_nDataSize = 0;
		memset(&s, 0, sizeof(s));
		XL_ASSERT(s.vary.m_pData == NULL);
		if (minimum_size > 0)
		{
			s.vary.m_pData = (unsigned8_t *)malloc(minimum_size);
			if (!s.vary.m_pData)
			{
				ret = ERR_UNABLE_TOALLOCATE_MEMORY;
				minimum_size = 0;
			}
			s.vary.m_nSize = minimum_size;
		}
	}

	return ret;
}

void CUnitStore::Reset()
{
	 if (m_varying_width && s.vary.m_pData)
	 {
		 XL_ASSERT(m_is_in_use);
		 free((void *)s.vary.m_pData);
	 }
	m_varying_width = 0;
	m_is_in_use = 0;
	m_is_sticky = 0;
	m_nDataSize = 0;
	memset(&s, 0, sizeof(s));
	XL_ASSERT(s.vary.m_pData == NULL);
}

signed8_t CUnitStore::Resize(size_t newlen)
{
	signed8_t ret = NO_ERRORS;

	XL_ASSERT(m_is_in_use);
	XL_ASSERT(newlen > 0);
	XL_ASSERT(newlen >= m_nDataSize);

	if (!m_varying_width)
	{
		if (newlen > FIXEDWIDTH_STORAGEUNIT_SIZE)
		{
			// turn this node into a varying-width unit store:
			unsigned8_t *p = (unsigned8_t *)malloc(newlen);
			if (!p)
			{
				ret = ERR_UNABLE_TOALLOCATE_MEMORY;
				newlen = 0;
			}
			else
			{
				memcpy(p, s.fixed.m_pData, m_nDataSize);
			}
			s.vary.m_pData = p;
			s.vary.m_nSize = newlen;
			m_varying_width = 1;
		}
	}	
	else 
	{
		if (newlen != s.vary.m_nSize)
		{
			if (!s.vary.m_pData)
			{
				XL_ASSERT(m_nDataSize == 0);
				s.vary.m_pData = (unsigned8_t *)malloc(newlen);
			}
			else
			{
				s.vary.m_pData = (unsigned8_t *)realloc((void *)s.vary.m_pData, newlen);
			}
			if (!s.vary.m_pData)
			{
				ret = ERR_UNABLE_TOALLOCATE_MEMORY;
				newlen = 0;
			}
			s.vary.m_nSize = newlen;
		}
	}

	return ret;
}

signed8_t CUnitStore::Init(const unsigned8_t *data, size_t size, size_t datasize)
{
	signed8_t ret;

	XL_ASSERT(m_is_in_use);
	XL_ASSERT(size > 0);
	XL_ASSERT(datasize <= size);
	ret = Resize(size);
	memcpy(GetBuffer(), data, datasize);
	SetDataSize(datasize);
	return ret;
}

signed8_t CUnitStore::InitWithValue(unsigned8_t value, size_t size)
{
	signed8_t ret;

	XL_ASSERT(m_is_in_use);
	XL_ASSERT(size > 0);
	ret = Resize(size);
	memset(GetBuffer(), value, size);
	SetDataSize(size);
	return ret;
}

#endif // defined(LEIGHTWEIGHT_UNIT_FEATURE)

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Log: datast.cpp,v $
 * Revision 1.4  2009/03/02 04:08:43  dhoerl
 * Code is now compliant to gcc  -Weffc++
 *
 * Revision 1.3  2009/01/08 02:53:15  dhoerl
 * December Rework
 *
 * Revision 1.2  2008/10/25 18:39:54  dhoerl
 * 2008
 *
 * Revision 1.1.1.1  2004/08/27 16:31:46  darioglz
 * Initial Import.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

