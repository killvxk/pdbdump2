#pragma once

class CDbgEngine;
class CDbgType
{
	struct impl;
	std::unique_ptr<impl> m_pimpl;

public:
	explicit CDbgType(std::shared_ptr<CDbgEngine> engine, ULONG64 modbasep, ULONG typeindex);
	virtual ~CDbgType(void);

	//typedef std::list<std::shared_ptr<CDbgType> > List;
	typedef std::vector<std::shared_ptr<CDbgType> > List;

	/// �f�o�b�O�G���W�����擾���܂��B
	std::shared_ptr<CDbgEngine> GetEngine();

	/// �^�̃C���f�b�N�X���擾���܂��B
	ULONG GetTypeIndex();

	/// �^�O���擾���܂��B
	enum SymTagEnum GetTypeTag();

	/// ���W���[���ɒ�`���ꂽ�^�����ׂĎ��W���܂��B
	static List GetModuleTypes(std::shared_ptr<CDbgEngine> engine, DWORD64 modbase);

	/// �֘A�t���ꂽ'��'�̌^���擾���܂��B
	HRESULT GetNextType(std::shared_ptr<CDbgType>* retval);

	/// ��������΁A�֘A�t����ꂽ�q���̌^���擾���܂��B
	HRESULT GetChildTypes(CDbgType::List* retval);

	/// ��������΃V���{���̖��O���擾���܂��B
	HRESULT GetSymName(std::wstring* retval);

	/// ��������ΐe�N���X���擾���܂��B
	HRESULT GetClassParentType(std::shared_ptr<CDbgType>* retval);

	/// �^�̃T�C�Y���擾���܂��B
	HRESULT GetLength(ULONG64* retval);

	/// Tag==SymTagBase �̏ꍇ�ɂ��̊�{�^���擾���܂��B
	HRESULT GetBaseType(BasicType* retval);

	/// �I�t�Z�b�g�A�h���X���擾���܂��B
	HRESULT GetAddressOffset(ULONG* retval);

	/// �A�h���X���擾���܂��B
	HRESULT GetAddress(ULONG64* retval);

	/// �I�t�Z�b�g���擾���܂��B
	HRESULT GetOffset(ULONG* retval);

	/// �z��̃C���f�b�N�X�^���擾���܂��B
	HRESULT GetArrayIndexType(std::shared_ptr<CDbgType>* retval);

	/// �z��̗v�f�����擾���܂��B
	HRESULT GetArrayCount(ULONG* retval);

	/// �o�[�`�����x�[�X�N���X������΁A������擾���܂��B
	HRESULT GetVirtualBaseClassType(std::shared_ptr<CDbgType>* retval);

	/// ���z�e�[�u���̌^���擾���܂��B
	HRESULT GetVirtualTableShapeType(std::shared_ptr<CDbgType>* retval);

	HRESULT GetVirtualBasePointerOffset(ULONG* retval);

	HRESULT GetVirtualBaseOffset(ULONG* retval);

	/// this�|�C���^�[����̃I�t�Z�b�g���擾���܂��B
	HRESULT GetThisAdjust(ULONG* retval);

	/// UDT(UserDataKind)�̎�ނ��擾���܂��B
	HRESULT GetUdtKind(UdtKind* retval);

	/// Data�̎�ނ��擾���܂��B
	HRESULT GetDataKind(DataKind* retval);

	/// �֐��̌Ăяo���K����擾���܂��B
	HRESULT GetCallingConversion(CV_call_e* retval);
};
