#pragma once

class CDbgEngine;
class CDbgModule
{
	struct impl;
	std::unique_ptr<impl> m_pimpl;

public:
	explicit CDbgModule(std::shared_ptr<CDbgEngine> engine, LPCTSTR modname);
	virtual ~CDbgModule(void);

	/// ���W���[���̃w�b�_���擾���܂��B
	IMAGE_NT_HEADERS *GetHeader();

	/// ���̃��W���[���̏����擾���܂��B
	IMAGEHLP_MODULE64 *GetModuleInfo();

	/// ���W���[���̃x�[�X�A�h���X���擾���܂��B
	DWORD64 GetImageBase();

	/// �Z�N�V�������擾���܂��B
	IMAGE_SECTION_HEADER *GetSections();

	/// �Z�N�V�����̐����擾���܂��B
	DWORD GetSectionNum();
};

