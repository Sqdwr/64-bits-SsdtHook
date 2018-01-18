#include "SSDT.h"

VOID PageProtectOff()
{
	ULONG_PTR cr0;
	_disable();											//�����ж�
	cr0 = __readcr0();									//��ȡcr0
	cr0 &= 0xfffffffffffeffff;							//��ҳд�뱣��λ��������
	__writecr0(cr0);									//д��cr0
}

VOID PageProtectOn()
{
	ULONG_PTR cr0;
	cr0 = __readcr0();									//��ȡcr0
	cr0 |= 0x10000;										//��ԭҳ����λ
	__writecr0(cr0);									//д��cr0
	_enable();											//��������ж�����
}

ULONG_PTR GetSsdtBase()
{
	ULONG_PTR SystemCall64;								//��msr�ж�ȡ����SystemCall64�ĵ�ַ
	ULONG_PTR StartAddress;								//��Ѱ����ʼ��ַ����SystemCall64����ʼ��ַ
	ULONG_PTR EndAddress;								//��Ѱ���ս��ַ
	UCHAR *p;											//�����жϵ�������
	ULONG_PTR SsdtBast;									//SSDT��ַ

	SystemCall64 = __readmsr(0xC0000082);
	StartAddress = SystemCall64;
	EndAddress = StartAddress + 0x500;
	while (StartAddress < EndAddress)
	{
		p = (UCHAR*)StartAddress;
		if (MmIsAddressValid(p) && MmIsAddressValid(p + 1) && MmIsAddressValid(p + 2))
		{
			if (*p == 0x4c && *(p + 1) == 0x8d && *(p + 2) == 0x15)
			{
				SsdtBast = (ULONG_PTR)(*(ULONG*)(p + 3)) + (ULONG_PTR)(p + 7);
				break;
			}
		}
		++StartAddress;
	}

	return SsdtBast;
}

ULONG_PTR GetFuncAddress(PWSTR FuncName)
{
	UNICODE_STRING uFunctionName;
	RtlInitUnicodeString(&uFunctionName, FuncName);
	return (ULONG_PTR)MmGetSystemRoutineAddress(&uFunctionName);
}

VOID HookKeBugCheckEx()
{
	ULONG_PTR KeBugCheckExAddress;

	KeBugCheckExAddress = GetFuncAddress(L"KeBugCheckEx");

	*(ULONG_PTR*)(jmp_code + 2) = (ULONG_PTR)MyNtTerminateProcess;

	PageProtectOff();

	RtlCopyMemory(old_code, (PVOID)KeBugCheckExAddress, sizeof(old_code));					//����ԭ����ʮ����ֵ

	RtlCopyMemory((PVOID)KeBugCheckExAddress, jmp_code, sizeof(jmp_code));					//�滻�µ�ֵ��ȥ

	PageProtectOn();
}

VOID UnHookKeBugCheckEx()
{
	ULONG_PTR KeBugCheckExAddress;

	KeBugCheckExAddress = GetFuncAddress(L"KeBugCheckEx");

	PageProtectOff();

	RtlCopyMemory((PVOID)KeBugCheckExAddress, old_code, sizeof(old_code));					//�滻�µ�ֵ��ȥ

	PageProtectOn();
}

VOID StartHook()
{
	PSYSTEM_SERVICE_TABLE SsdtInfo;

	ULONG_PTR KeBugCheckExAddress;

	ULONG Offset;

	SsdtInfo = (PSYSTEM_SERVICE_TABLE)GetSsdtBase();

	old_ValueOnNtTerminateProcess = SsdtInfo->ServiceTableBase[IndexOfNtTerminateProcess];

	old_NtTerminateProcess = (ULONG_PTR)(old_ValueOnNtTerminateProcess >> 4) + (ULONG_PTR)SsdtInfo->ServiceTableBase;

	//KdPrint(("old_ValueOnNtTerminateProcess = %x\n", old_ValueOnNtTerminateProcess));
	//KdPrint(("old_NtTerminateProcess = %llx\n", old_NtTerminateProcess));

	HookKeBugCheckEx();

	KeBugCheckExAddress = GetFuncAddress(L"KeBugCheckEx");

	//KdPrint(("KeBugCheckEx = %llx\n", KeBugCheckExAddress));

	Offset = (ULONG)(KeBugCheckExAddress - (ULONG_PTR)SsdtInfo->ServiceTableBase);

	Offset = Offset << 4;

	PageProtectOff();

	SsdtInfo->ServiceTableBase[IndexOfNtTerminateProcess] = Offset;

	PageProtectOn();
}

VOID StopHook()
{
	PSYSTEM_SERVICE_TABLE SsdtInfo;

	SsdtInfo = (PSYSTEM_SERVICE_TABLE)GetSsdtBase();

	PageProtectOff();

	SsdtInfo->ServiceTableBase[IndexOfNtTerminateProcess] = old_ValueOnNtTerminateProcess;

	PageProtectOn();

	UnHookKeBugCheckEx();
}

NTSTATUS __fastcall MyNtTerminateProcess(IN HANDLE ProcessHandle, IN NTSTATUS ExitStatus)
{
	PEPROCESS CurrentProcess;

	NTSTATUS status;

	BOOLEAN Flag;							//��־λ�������TRUE������Ҫ�Ծ�����н�����ã�������Ҫ

	if (ProcessHandle != NULL)
	{
		status = ObReferenceObjectByHandle(
			ProcessHandle,
			0,
			*PsProcessType,
			KernelMode,
			&CurrentProcess,
			NULL);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("��ȡ���̶���ʧ�ܣ�status = %x\n", status));
			return STATUS_UNSUCCESSFUL;
		}
		Flag = TRUE;
	}
	else
	{
		Flag = FALSE;
		CurrentProcess = PsGetCurrentProcess();
	}
	KdPrint(("��Ҫ�رյĽ��̵������ǣ�%s\n", PsGetProcessImageFileName(CurrentProcess)));

	if (strstr(PsGetProcessImageFileName(CurrentProcess), "calc"))
	{
		KdPrint(("�ܾ��رռ�������\n"));

		if (Flag)
			ObDereferenceObject(CurrentProcess);

		return STATUS_ACCESS_DENIED;
	}

	if (Flag)
		ObDereferenceObject(CurrentProcess);

	return ((NTTERMINATEPROCESS)(old_NtTerminateProcess))(ProcessHandle, ExitStatus);
}

/*NTSTATUS __fastcall MyNtTerminateProcess(IN HANDLE ProcessHandle, IN NTSTATUS ExitStatus)
{
	PEPROCESS CurrentProcess;

	NTSTATUS status;

	BOOLEAN Flag;							//��־λ�������TRUE������Ҫ�Ծ�����н�����ã�������Ҫ

	status = ObReferenceObjectByHandle(
		ProcessHandle,
		0,
		*PsProcessType,
		KernelMode,
		&CurrentProcess,
		NULL);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("��ȡ���̶���ʧ�ܣ�status = %x\n", status));
		return STATUS_ACCESS_DENIED;
	}
	Flag = TRUE;

	KdPrint(("��Ҫ�رյĽ��̵������ǣ�%s\n", PsGetProcessImageFileName(CurrentProcess)));

	if (strstr(PsGetProcessImageFileName(CurrentProcess), "calc"))
	{
		KdPrint(("�ܾ��رռ�������\n"));

		if (Flag)
			ObDereferenceObject(CurrentProcess);

		return STATUS_ACCESS_DENIED;
	}

	if (Flag)
		ObDereferenceObject(CurrentProcess);

	return ((NTTERMINATEPROCESS)(old_NtTerminateProcess))(ProcessHandle, ExitStatus);
}*/

VOID Unload(PDRIVER_OBJECT DriverObject)
{
	KdPrint(("Unload Success!\n"));

	StopHook();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegString)
{
	KdPrint(("Entry Driver!\n"));
	StartHook();
	DriverObject->DriverUnload = Unload;
	return STATUS_SUCCESS;
}