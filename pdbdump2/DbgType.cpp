#include "stdafx.h"
#include "DbgType.h"
#include "DbgEngine.h"


//-----------------------------------------------------------------
struct CDbgType::impl {
	std::shared_ptr<CDbgEngine> engine;
	ULONG typeindex;
	ULONG64 modbase;
	enum SymTagEnum tag;

	std::shared_ptr<CDbgType> next_type_cache;
	std::shared_ptr<CDbgType::List> children_cache;
	std::shared_ptr<std::wstring> symname_cache;

	HRESULT Init(std::shared_ptr<CDbgEngine> _engine, ULONG64 _modbase, ULONG _typeindex)
	{
		engine = _engine;
		modbase = _modbase;
		typeindex = _typeindex;
		tag = GetTypeTag();
		return S_OK;
	}

	enum SymTagEnum GetTypeTag()
	{
		ULONG tag = 0;

		if (typeindex == 0) {
			return SymTagNull;
		}

		// �^�̃^�O���擾���܂��B
		if (!engine->SymGetTypeInfo(
			modbase, typeindex, TI_GET_SYMTAG, &tag)) {
			//assert(!"Couldn't get the type tag!");
			return SymTagNull;
		}

		return (enum SymTagEnum)tag;
	}

	HRESULT SymGetTypeInfo(IMAGEHLP_SYMBOL_TYPE_INFO GetType, void* pv)
	{
		if (!engine->SymGetTypeInfo(modbase, typeindex, GetType, pv)) {
			return E_NOINTERFACE;
		}

		return S_OK;
	}

	template <class T>
	HRESULT GetTypeInfo(IMAGEHLP_SYMBOL_TYPE_INFO GetType, T& value)
	{
		return SymGetTypeInfo(GetType, &value);
	}

	//void Uninit()
	//{
	//}

	//~impl()
	//{
	//	Uninit();
	//}
};

//-----------------------------------------------------------------
CDbgType::CDbgType(std::shared_ptr<CDbgEngine> engine, ULONG64 modbasep, ULONG typeindex) : m_pimpl(new impl)
{
	HRESULT hr = m_pimpl->Init(engine, modbasep, typeindex);
	if (FAILED(hr))
		throw hr;
}

CDbgType::~CDbgType(void)
{
}

//-----------------------------------------------------------------
/// �f�o�b�O�G���W�����擾���܂��B
std::shared_ptr<CDbgEngine> CDbgType::GetEngine()
{
	return m_pimpl->engine;
}

/// �^�̃C���f�b�N�X���擾���܂��B
ULONG CDbgType::GetTypeIndex()
{
	return m_pimpl->typeindex;
}

/// �^�O���擾���܂��B
enum SymTagEnum CDbgType::GetTypeTag()
{
	return m_pimpl->tag;
}


//-----------------------------------------------------------------
struct SymEnumTypeData {
	std::shared_ptr<CDbgEngine> engine;
	CDbgType::List typelist;
	SymEnumTypeData(std::shared_ptr<CDbgEngine> _engine) : engine(_engine)
	{ }
};

/// �^�I�u�W�F�N�g�����W���܂��B
static BOOL CALLBACK SymEnumTypeCallback(
	PSYMBOL_INFO symbol_info, ULONG /*symbol_size*/, PVOID user_context) {
	// �V�����^�I�u�W�F�N�g��ǉ����܂��B
	SymEnumTypeData *data = static_cast<SymEnumTypeData *>(user_context);
	std::shared_ptr<CDbgType> child;
	HRESULT hr = CreateInsatnce(data->engine, symbol_info->ModBase, symbol_info->TypeIndex, &child);
	assert( SUCCEEDED(hr) );
	if (FAILED(hr))
		return FALSE;
	data->typelist.push_back(child);
	return TRUE;
}

/// ���W���[���ɒ�`���ꂽ�^�����ׂĎ��W���܂��B
CDbgType::List CDbgType::GetModuleTypes(std::shared_ptr<CDbgEngine> engine, DWORD64 modbase)
{
	SymEnumTypeData data(engine);
	if (!engine->SymEnumTypes(modbase, SymEnumTypeCallback, &data)) {
		//assert(!"Failed to call SymEnumTypes");
		return List();
	}
	return data.typelist;
}

/// �֘A�t���ꂽ'��'�̌^���擾���܂��B
HRESULT CDbgType::GetNextType(std::shared_ptr<CDbgType>* retval)
{
	if (m_pimpl->next_type_cache != NULL) {
		*retval = m_pimpl->next_type_cache;
		return S_OK;
	}

	// Get the next type down the chain and decode that one.
	ULONG index = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_TYPEID, index);
	if (FAILED(hr))
		return hr;

	std::shared_ptr<CDbgType> next_type;
	hr = CreateInsatnce(m_pimpl->engine, m_pimpl->modbase, index, &next_type);
	if (FAILED(hr))
		return hr;
	m_pimpl->next_type_cache = next_type;
	*retval = next_type;
	return S_OK;
}

#define CHILDREN_MAX 512

/**
 * @brief �q�^�C�v��񋓂���Ƃ��Ɏg���܂��B
 */
struct FINDCHILDREN : public TI_FINDCHILDREN_PARAMS {
	FINDCHILDREN() {
		memset(this, 0, sizeof(TI_FINDCHILDREN_PARAMS));
		this->Count = CHILDREN_MAX;
		this->Start = 0;
	}

private:
	ULONG children_id[CHILDREN_MAX];
};

/// ��������΁A�֘A�t����ꂽ�q���̌^���擾���܂��B
HRESULT CDbgType::GetChildTypes(CDbgType::List* retval)
{
	if (m_pimpl->children_cache != nullptr) {
		*retval = *m_pimpl->children_cache;
		return S_OK;
	}

	// �q���̐����擾���܂��B
	ULONG children_count = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_CHILDRENCOUNT, children_count);
	if (FAILED(hr))
		return hr;

	if (children_count == 0) {
		return E_NOT_SET;
	}

	// �q�^�C�v�����W���A�R���e�i�Ɏ��߂܂��B
	TI_FINDCHILDREN_PARAMS *find = (TI_FINDCHILDREN_PARAMS *)HeapAlloc(
		GetProcessHeap(), 0, sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * children_count);
	if (find == NULL) {
		return E_OUTOFMEMORY;
	}
	find->Count = children_count;
	find->Start = 0;

	hr = m_pimpl->SymGetTypeInfo(TI_FINDCHILDREN, find);
	if (FAILED(hr)) {
		HeapFree(GetProcessHeap(), 0, find);
		return hr;
	}

	List result;
	for (ULONG i = 0; i < find->Count; ++i) {
		std::shared_ptr<CDbgType> child;
		hr = CreateInsatnce(m_pimpl->engine, m_pimpl->modbase, find->ChildId[i], &child);
		if (FAILED(hr))
			break;
		result.push_back(child);
	}

	HeapFree(GetProcessHeap(), 0, find);
//	m_pimpl->children_cache = std::make_shared<List>(result);
	*retval = result;
	return S_OK;
}

/// ��������΃V���{���̖��O���擾���܂��B
HRESULT CDbgType::GetSymName(std::wstring* retval)
{
	if (m_pimpl->symname_cache != nullptr) {
		*retval = *m_pimpl->symname_cache;
		return S_OK;
	}

	WCHAR *wname = NULL;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_SYMNAME, wname);
	if (FAILED(hr)) {
		return hr;
	}

	m_pimpl->symname_cache = std::make_shared<std::wstring>(wname);
	LocalFree(wname);
	*retval = *m_pimpl->symname_cache;
	return S_OK;
}

/// ��������ΐe�N���X���擾���܂��B
HRESULT CDbgType::GetClassParentType(std::shared_ptr<CDbgType>* retval)
{
	ULONG index = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_CLASSPARENTID, index);
	if (FAILED(hr)) {
		return hr;
	}
	std::shared_ptr<CDbgType> type;
	hr = CreateInsatnce(m_pimpl->engine, m_pimpl->modbase, index, &type);
	if (FAILED(hr)) {
		return hr;
	}
	*retval = type;
	return S_OK;
}

/// �^�̃T�C�Y���擾���܂��B
HRESULT CDbgType::GetLength(ULONG64* retval)
{
	ULONG64 value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_LENGTH, value);
	if (FAILED(hr))
		return hr;
	*retval = value;
	return S_OK;
}

/// Tag==SymTagBase �̏ꍇ�ɂ��̊�{�^���擾���܂��B
HRESULT CDbgType::GetBaseType(BasicType* retval)
{
	ULONG value = btNoType;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_BASETYPE, value);
	if (FAILED(hr))
		return hr;
	*retval = (BasicType)value;
	return S_OK;
}

/// �I�t�Z�b�g�A�h���X���擾���܂��B
HRESULT CDbgType::GetAddressOffset(ULONG* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_ADDRESSOFFSET, value);
	if (FAILED(hr))
		return hr;
	*retval = value;
	return S_OK;
}

/// �A�h���X���擾���܂��B
HRESULT CDbgType::GetAddress(ULONG64* retval)
{
	ULONG64 value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_ADDRESS, value);
	if (FAILED(hr))
		return hr;
	*retval = value;
	return S_OK;
}

/// �I�t�Z�b�g���擾���܂��B
HRESULT CDbgType::GetOffset(ULONG* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_OFFSET, value);
	if (FAILED(hr))
		return hr;
	*retval = value;
	return S_OK;
}

/// �z��̃C���f�b�N�X�^���擾���܂��B
HRESULT CDbgType::GetArrayIndexType(std::shared_ptr<CDbgType>* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_ARRAYINDEXTYPEID, value);
	if (FAILED(hr))
		return hr;
	std::shared_ptr<CDbgType> type;
	hr = CreateInsatnce(m_pimpl->engine, m_pimpl->modbase, value, &type);
	if (FAILED(hr)) {
		return hr;
	}
	*retval = type;
	return S_OK;
}

/// �z��̗v�f�����擾���܂��B
HRESULT CDbgType::GetArrayCount(ULONG* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_COUNT, value);
	if (FAILED(hr))
		return hr;
	*retval = value;
	return S_OK;
}

/// �o�[�`�����x�[�X�N���X������΁A������擾���܂��B
HRESULT CDbgType::GetVirtualBaseClassType(std::shared_ptr<CDbgType>* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_VIRTUALBASECLASS, value);
	if (FAILED(hr))
		return hr;
	std::shared_ptr<CDbgType> type;
	hr = CreateInsatnce(m_pimpl->engine, m_pimpl->modbase, value, &type);
	if (FAILED(hr)) {
		return hr;
	}
	*retval = type;
	return S_OK;
}

/// ���z�e�[�u���̌^���擾���܂��B
HRESULT CDbgType::GetVirtualTableShapeType(std::shared_ptr<CDbgType>* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_VIRTUALTABLESHAPEID, value);
	if (FAILED(hr))
		return hr;
	std::shared_ptr<CDbgType> type;
	hr = CreateInsatnce(m_pimpl->engine, m_pimpl->modbase, value, &type);
	if (FAILED(hr)) {
		return hr;
	}
	*retval = type;
	return S_OK;
}

HRESULT CDbgType::GetVirtualBasePointerOffset(ULONG* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_VIRTUALBASEPOINTEROFFSET, value);
	if (FAILED(hr))
		return hr;
	*retval = value;
	return S_OK;
}

HRESULT CDbgType::GetVirtualBaseOffset(ULONG* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_VIRTUALBASEOFFSET, value);
	if (FAILED(hr))
		return hr;
	*retval = value;
	return S_OK;
}

/// this�|�C���^�[����̃I�t�Z�b�g���擾���܂��B
HRESULT CDbgType::GetThisAdjust(ULONG* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_THISADJUST, value);
	if (FAILED(hr))
		return hr;
	*retval = value;
	return S_OK;
}

/// UDT(UserDataKind)�̎�ނ��擾���܂��B
HRESULT CDbgType::GetUdtKind(UdtKind* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_UDTKIND, value);
	if (FAILED(hr))
		return hr;
	*retval = (UdtKind)value;
	return S_OK;
}

/// Data�̎�ނ��擾���܂��B
HRESULT CDbgType::GetDataKind(DataKind* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_DATAKIND, value);
	if (FAILED(hr))
		return hr;
	*retval = (DataKind)value;
	return S_OK;
}

/// �֐��̌Ăяo���K����擾���܂��B
HRESULT CDbgType::GetCallingConversion(CV_call_e* retval)
{
	ULONG value = 0;
	HRESULT hr = m_pimpl->GetTypeInfo(TI_GET_CALLING_CONVENTION, value);
	if (FAILED(hr))
		return hr;
	*retval = (CV_call_e)value;
	return S_OK;
}

