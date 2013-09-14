#include "stdafx.h"
#include "DbgModule.h"
#include "DbgEngine.h"

// �֐��J�X�^���f���[�^
void closeFile(HANDLE handle) {
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
}
void closeHandle(HANDLE handle) {
    if (handle != NULL) {
        CloseHandle(handle);
    }
}

//-----------------------------------------------------------------
struct CDbgModule::impl {
	std::shared_ptr<CDbgEngine> engine;
	HANDLE file;
	HANDLE mapfile;
	DWORD64 modbase;
	IMAGE_NT_HEADERS* ntheader;
	IMAGEHLP_MODULE64 module_info;

	impl() : file(INVALID_HANDLE_VALUE), mapfile(NULL), modbase(0), ntheader(NULL)
	{ }

	~impl()
	{
		Uninit();
	}

	HRESULT Init(std::shared_ptr<CDbgEngine> _engine, LPCTSTR modname)
	{
		engine = _engine;

		// �܂��C���[�W���J���܂��B
		file = CreateFile(
			modname, GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, 0, NULL);
		if (file == INVALID_HANDLE_VALUE) {
			assert(!"Couldn't open the image file.");
			return HRESULT_FROM_WIN32(::GetLastError());
		}

		mapfile = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
		if (mapfile == NULL) {
			//CloseHandle(file);
			return HRESULT_FROM_WIN32(::GetLastError());
		}

		LPBYTE pbase = (LPBYTE)MapViewOfFile(mapfile, FILE_MAP_READ, 0, 0, 0);
		if (pbase == NULL) {
			//CloseHandle(mapfile);
			//CloseHandle(file);
			return HRESULT_FROM_WIN32(::GetLastError());
		}

		// PE�w�b�_���擾���܂��B
		ntheader = engine->ImageNtHeader(pbase);
		if (ntheader == NULL
			|| ntheader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386
			|| ntheader->FileHeader.SizeOfOptionalHeader == 0
			|| ntheader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC) {
			return E_INVALIDARG;
		}

		// ���W���[�����f�o�b�O��񍞂݂œǂݍ��݂܂��B
		std::string modnameA = to_string(modname);
		modbase = engine->SymLoadModule64(
			file, modnameA.c_str(), modnameA.c_str(), 0, 0);
		if (modbase == 0) {
			return HRESULT_FROM_WIN32(::GetLastError());
		}

		// ���W���[���̏����擾���܂��B
		memset(&module_info, 0, sizeof(module_info));
		module_info.SizeOfStruct = sizeof(module_info);
		if (!engine->SymGetModuleInfo64(modbase, &module_info)) {
			engine->SymUnloadModule64(modbase);
			return HRESULT_FROM_WIN32(::GetLastError());
		}

		if (modbase != ntheader->OptionalHeader.ImageBase) {
			engine->SymUnloadModule64(modbase);
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	void Uninit()
	{
		if (modbase != 0) {
			engine->SymUnloadModule64(modbase);
		}
		if (mapfile != NULL) {
			::CloseHandle(mapfile);
		}
		if (file != INVALID_HANDLE_VALUE) {
			::CloseHandle(file);
		}
	}
};

//-----------------------------------------------------------------
CDbgModule::CDbgModule(std::shared_ptr<CDbgEngine> engine, LPCTSTR modname) : m_pimpl(new impl)
{
	HRESULT hr = m_pimpl->Init(engine, modname);
	if (FAILED(hr))
		throw hr;
}

CDbgModule::~CDbgModule(void)
{
}

//-----------------------------------------------------------------
/// ���W���[���̃w�b�_���擾���܂��B
IMAGE_NT_HEADERS* CDbgModule::GetHeader()
{
	assert(m_pimpl->ntheader != NULL);
	return m_pimpl->ntheader;
}

/// ���̃��W���[���̏����擾���܂��B
IMAGEHLP_MODULE64* CDbgModule::GetModuleInfo()
{
	assert(m_pimpl->ntheader != NULL);
	return &m_pimpl->module_info;
}

/// ���W���[���̃x�[�X�A�h���X���擾���܂��B
DWORD64 CDbgModule::GetImageBase()
{
	assert(m_pimpl->ntheader != NULL);
	return m_pimpl->modbase;
}

/// �Z�N�V�������擾���܂��B
IMAGE_SECTION_HEADER* CDbgModule::GetSections()
{
	assert(m_pimpl->ntheader != NULL);
	return IMAGE_FIRST_SECTION(m_pimpl->ntheader);
}

/// �Z�N�V�����̐����擾���܂��B
DWORD CDbgModule::GetSectionNum()
{
	assert(m_pimpl->ntheader != NULL);
	return m_pimpl->ntheader->FileHeader.NumberOfSections;
}

