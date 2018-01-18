#pragma once
#ifndef SSDTHOOK_H
#define SSDTHOOK_H
#include <ntddk.h>

#define IndexOfNtTerminateProcess 41

typedef struct _SYSTEM_SERVICE_TABLE
{
	PUINT32 ServiceTableBase;
	PUINT32 ServiceCounterTableBase;
	UINT64 NumberOfServices;
	PUCHAR ParamTableBase;
}SYSTEM_SERVICE_TABLE, *PSYSTEM_SERVICE_TABLE;

extern UCHAR *PsGetProcessImageFileName(PEPROCESS Process);

extern unsigned __int64 __readmsr(int register);				//��ȡmsr�Ĵ���

extern unsigned __int64 __readcr0(void);			//��ȡcr0��ֵ

extern void __writecr0(unsigned __int64 Data);		//д��cr0

extern void __debugbreak();							//�ϵ㣬����int 3

extern void _disable(void);							//�����ж�

extern void _enable(void);							//�����ж�

VOID PageProtectOff();

VOID PageProtectOn();

ULONG_PTR GetSsdtBase();							//��ȡSSDT��ַ

ULONG_PTR GetFuncAddress(PWSTR FuncName);			//���ݺ������ֻ�ȡ������ַ��������ntoskrnl�����ģ�

VOID UnHookKeBugCheckEx();

VOID HookKeBugCheckEx();

VOID StartHook();									//��ʼSSDT HOOK

VOID StopHook();									//�ر�SSDT HOOK

typedef NTSTATUS(__fastcall *NTTERMINATEPROCESS)(IN HANDLE ProcessHandle, IN NTSTATUS ExitStatus);

NTSTATUS __fastcall MyNtTerminateProcess(IN HANDLE ProcessHandle, IN NTSTATUS ExitStatus);

ULONG_PTR old_NtTerminateProcess;

ULONG old_ValueOnNtTerminateProcess;

//\x48\xB8+8�ֽڴ���mov rax,*****  \xFF\xE0������jmp rax��ģ����ת
UCHAR jmp_code[12] = { '\x48','\xB8','\xFF','\xFF','\xFF','\xFF','\xFF','\xFF','\xFF','\xFF','\xFF','\xE0' };	
UCHAR old_code[12];
#endif

