// AtlDemo.cpp : Implementation of WinMain


#include "stdafx.h"
#include "resource.h"
#include "AtlDemo_i.h"
#include <Wtsapi32.h>
#include <Userenv.h>
#pragma comment(lib,"wtsapi32.lib")
#pragma comment(lib,"userenv.lib")
#include <stdio.h>
#include "Utility.h"
#include "TimeUtility.h"
#include "Runlog.h"
#include "Markup.h"

class CAppStartModule : public ATL::CAtlServiceModuleT< CAppStartModule, IDS_SERVICENAME >
{
public :
	DECLARE_LIBID(LIBID_AtlDemoLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_ATLDEMO, "{AF1066F3-E0B6-4EA4-9D88-AC35F3B6166B}")
		HRESULT InitializeSecurity() throw()
	{
// 		A_SHAFinal;
// 		A_SHAInit;
// 		A_SHAUpdate;
		// ���� - PKT ����������֤��  
		// RPC_C_IMP_LEVEL_IDENTIFY ��ģ�⼶��  
		// �Լ��ʵ��ķ� null ��ȫ˵������  

		//return S_OK;  
		return CoInitializeSecurity(NULL,-1,NULL,NULL,  
			RPC_C_AUTHN_LEVEL_NONE,  
			RPC_C_IMP_LEVEL_IDENTIFY,  
			NULL,EOAC_NONE,NULL);//������ȫ˵���� 
	};
	void OnPause() throw(); //��ͣ  
	void OnStop() throw();//ֹͣ  
	void Handler(DWORD dwOpcode) throw();//����ͬ�ķ��������Ϣ  
	void OnContinue() throw();//��������  
	HRESULT PreMessageLoop(int nShowCmd) throw();//��Ϣ��Ӧ  
	HRESULT RegisterAppId(bool bService = false) throw();//����ע��  
	shared_ptr<CRunlog> pRunlog;
	int nDelay = -1;
	wstring strAppPath ;
	wstring strServiceExePath;
	bool GetConfiguration()
	{
		TCHAR szPath[MAX_PATH] = { 0 };
		GetAppPath(szPath, MAX_PATH);
		strServiceExePath = szPath;
		_tcscat_s(szPath, MAX_PATH, _T("\\Configuration.xml"));
		if (!PathFileExists(szPath))
		{
			return false;
		}
		CMarkup xml;
		if (!xml.Load(szPath))
			return false;
		/*
		<?xml version="1.0" encoding="utf-8"?>
		<Configuration>
		<App Path = "C:\Windows\System32\Notepad.exe" Delay="5000"/>
		</Configuration>
		*/
		
		if (xml.FindElem(L"App"))
		{
			strAppPath = xml.GetAttrib(L"Path");
			nDelay = (int)_tcstolong(xml.GetAttrib(L"Delay").c_str());
			return true;
		}
		else
			return false;
		
	}
	void RunApp()
	{
		pRunlog->Runlog(_T("%s Try to run Application %s.\n"), __FUNCTIONW__, strAppPath.c_str());
		if (!PathFileExists(strAppPath.c_str()))
		{
			pRunlog->Runlog(_T("%s The specified app:%s not exists.\n"), __FUNCTIONW__, strAppPath.c_str());
			return;
		}
		DWORD dwError = Session0CreateUserProcess(strAppPath.c_str(), NULL, strServiceExePath.c_str());
		if (!dwError)
		{
			pRunlog->Runlog(_T("%s Succeed.\n"), __FUNCTIONW__);
		}
		else
		{
			DWORD dwError = GetLastError();
			LPVOID lpMsgBuf = NULL;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL
				);
			pRunlog->Runlog(_T("%s Failed,Code:%d,Message:%s.\n"), __FUNCTIONW__, dwError, (LPCTSTR)lpMsgBuf);
			if (lpMsgBuf != NULL)
				LocalFree(lpMsgBuf);
		}
	}
	void RunMessageLoop() throw()
	{
		MSG msg;
		if (!GetConfiguration())
		{
			pRunlog->Runlog(_T("%s Failed in Read configuration.\n"), __FUNCTIONW__);
			return;
		}
		
		double dfT1 = GetExactTime();
		double dfDelay = nDelay / 1000;
		do 
		{
			if (TimeSpanEx(dfT1) >= dfDelay)
			{
				RunApp();
				break;
			}
			Sleep(100);
		} while (true);
	
		while (GetMessage(&msg, 0, 0, 0) > 0)
		{
			
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DWORD _stdcall CAppStartModule::Session0CreateUserProcess(LPCTSTR lpApplicationName,
															LPCTSTR lpCommandLine,
															LPCTSTR lpCurrentDirectory )  
	{  		
		DWORD dwRet = 0;  
		PROCESS_INFORMATION pi;  
		STARTUPINFO si;  
		DWORD dwSessionId;//��ǰ�Ự��ID  
		HANDLE hUserToken = NULL;//��ǰ��¼�û�������  
		HANDLE hUserTokenDup = NULL;//���Ƶ��û�����  
		HANDLE hPToken = NULL;//��������  
		DWORD dwCreationFlags;  
		SECURITY_ATTRIBUTES sa;
		__try
		{
			__try
			{
				//�õ���ǰ��ĻỰID������¼�û��ĻỰID  				
				dwSessionId = WTSGetActiveConsoleSessionId();  				
				WTSQueryUserToken(dwSessionId,&hUserToken);//��ȡ��ǰ��¼�û���������Ϣ  
				dwCreationFlags = NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE;//��������  

				ZeroMemory(&si,sizeof(STARTUPINFO));  
				ZeroMemory(&pi,sizeof(pi));  
				ZeroMemory(&sa,sizeof(SECURITY_ATTRIBUTES));

				si.cb = sizeof(STARTUPINFO);  
				si.lpDesktop = L"winsta0\\default";//ָ���������̵Ĵ���վ��Windows��Ψһ�ɽ����Ĵ���վ����WinSta0\Default  

				sa.nLength = sizeof(SECURITY_ATTRIBUTES);

				TOKEN_PRIVILEGES tp;  
				LUID luid;  				
				//�򿪽�������  
				if (!OpenProcessToken(GetCurrentProcess(),  
					TOKEN_ADJUST_PRIVILEGES|  
					TOKEN_QUERY|  
					TOKEN_DUPLICATE|  
					TOKEN_ASSIGN_PRIMARY|  
					TOKEN_ADJUST_SESSIONID|  
					TOKEN_READ|  
					TOKEN_WRITE,&hPToken))  
				{  
					dwRet = GetLastError();  
					pRunlog->Runlog(_T("OpenProcessToken Failed ,Errorcode = %d."),dwRet);	
					__leave;
				}  
				//����DEBUGȨ�޵�UID  
				if (!LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&luid))  
				{  
					dwRet = GetLastError();  
					pRunlog->Runlog(_T("LookupPrivilegeValue Failed ,Errorcode = %d."), dwRet);
					__leave;  
				}  
				//����������Ϣ  
				tp.PrivilegeCount = 1;  
				tp.Privileges[0].Luid = luid;  
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;  
				
				//���Ƶ�ǰ�û�������  
				//GENERIC_ALL_ACCESS
				if (!DuplicateTokenEx(hPToken,MAXIMUM_ALLOWED,&sa,SecurityIdentification,  
					TokenPrimary,&hUserTokenDup))  
				{  
					dwRet  = GetLastError();  
					pRunlog->Runlog(_T("DuplicateTokenEx Failed ,Errorcode = %d."), dwRet);
					__leave;  
				}  
				//���õ�ǰ���̵�������Ϣ  
				if (!SetTokenInformation(hUserTokenDup,TokenSessionId,(void*)&dwSessionId,sizeof(DWORD)))  
				{  
					dwRet = GetLastError();  
					pRunlog->Runlog(_T("SetTokenInformation Failed ,Errorcode = %d."), dwRet);
					__leave;  
				}  
				//Ӧ������Ȩ��  
				if (!AdjustTokenPrivileges(hUserTokenDup,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),  
					(PTOKEN_PRIVILEGES)NULL,NULL))  
				{  
					dwRet = GetLastError(); 
					pRunlog->Runlog(_T("AdjustTokenPrivileges Failed ,Errorcode = %d."), dwRet);
					__leave;  
				}  				
				//�������̻����飬��֤�����������û�����Ļ�����  
				LPVOID pEnv = NULL;  
				if (CreateEnvironmentBlock(&pEnv,hUserTokenDup,TRUE))  
				{  
					dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;  
				}  
				else  
				{  
					pEnv = NULL;  
				}  
				//�����û�����  
				if (!CreateProcessAsUser(hUserTokenDup,lpApplicationName,(LPWSTR)lpCommandLine,&sa,&sa,FALSE,  
					dwCreationFlags,pEnv,lpCurrentDirectory,&si,&pi))  
				{  
					dwRet = GetLastError();  
					pRunlog->Runlog(_T("CreateProcessAsUser Failed ,Errorcode = %d."), dwRet);
					__leave;  
				}
				_TraceMsg(_T("LaunchWin7SessionProcess Suceed."));
			}
			__finally
			{				
				//�رվ��  
				if (NULL != hUserToken)  
				{  
					CloseHandle(hUserToken);  
				}  
				if (NULL != hUserTokenDup)  
				{  
					CloseHandle(hUserTokenDup);  
				}  
				if (NULL != hPToken)  
				{  
					CloseHandle(hPToken);  
				}
			}
		}
		__except(1)
		{
			pRunlog->Runlog(_T("LaunchWin7SessionProcess Exception."));
		}
		return dwRet;  
	}  
};

CAppStartModule _AtlModule;

//
extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
								LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	_AtlModule.pRunlog = make_shared<CRunlog>(_T("AppInitiator"));
	return _AtlModule.WinMain(nShowCmd);
}

HRESULT CAppStartModule::RegisterAppId( bool bService /*= false*/ ) throw()  
{  
	HRESULT hr = S_OK;  
	BOOL res = __super::RegisterAppId(bService);  
	if (bService)  
	{  
		if (IsInstalled())//�����Ѿ���װ  
		{  
			SC_HANDLE hSCM = ::OpenSCManager(NULL,NULL,SERVICE_CHANGE_CONFIG);//�򿪷��������  
			SC_HANDLE hService = NULL;  

			if (hSCM == NULL)  
			{  
				hr = ATL::AtlHresultFromLastError();  
			}  
			else  
			{  
				//�򿪷���m_szServiceNameΪ�����Ա����������ǰ��������  
				//��������Դ�ļ��б��String Table���޸�  
				hService = OpenService(hSCM,m_szServiceName,SERVICE_CHANGE_CONFIG);  
				if (hService != NULL)  
				{  
					//�޸ķ�������  
					ChangeServiceConfig(hService,  
						SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,//�������̡�������  
						SERVICE_AUTO_START,//�����Զ�����  
						NULL,NULL,NULL,NULL,NULL,NULL,NULL,  
						m_szServiceName);  

					//����������Ϣ  
					SERVICE_DESCRIPTION sDescription;  
					TCHAR szDescription[1024];  
					ZeroMemory(szDescription,1024);  
					ZeroMemory(&sDescription,sizeof(SERVICE_DESCRIPTION));  

					//��������  
					lstrcpy(szDescription,L"Initiator for desktop application in System Service!");  
					sDescription.lpDescription = szDescription;  

					//�޸ķ���������Ϣ  
					ChangeServiceConfig2(hService,SERVICE_CONFIG_DESCRIPTION,&sDescription);  

					//�رշ�����  
					CloseServiceHandle(hService);  

				}  
				else  
				{  
					hr = ATL::AtlHresultFromLastError();  
				}  
			}  

			//�رշ�����������  
			::CloseServiceHandle(hSCM);  
		}  
	}  
	return hr;  
}

HRESULT CAppStartModule::PreMessageLoop( int nShowCmd ) throw()  
{  
	//�÷���������ͣ�ͼ�������  
	m_status.dwControlsAccepted = m_status.dwControlsAccepted|SERVICE_ACCEPT_PAUSE_CONTINUE;  
	HRESULT hr = __super::PreMessageLoop(nShowCmd);  
	if (hr == S_FALSE)  
	{  
		hr = S_OK;//������Bug,��������д��������ܼ���  
	}  

	//������״̬����Ϊ����  
	SetServiceStatus(SERVICE_RUNNING);  

	//д��ϵͳ��־  
	LogEvent(L"App Initiator Service Start Successfully~!");  

	return hr;  
}

void CAppStartModule::Handler( DWORD dwOpcode ) throw()  
{  
	switch(dwOpcode)  
	{  
	case SERVICE_CONTROL_PAUSE://��ͣ  
		{  
			OnPause();  
			break;  
		}  
	case SERVICE_CONTROL_CONTINUE://����  
		{  
			OnContinue();  
			break;  
		}  
	default:  
		break;  
	}  

	__super::Handler(dwOpcode);  
}

void CAppStartModule::OnPause() throw()  
{  
	//���÷���״̬Ϊ��ͣ  
	SetServiceStatus(SERVICE_PAUSED);  

	__super::OnPause();  
}

void CAppStartModule::OnStop() throw()  
{  
	//���÷���״̬Ϊֹͣ  
	SetServiceStatus(SERVICE_STOPPED);  

	__super::OnStop();  
} 

void CAppStartModule::OnContinue() throw()  
{  
	//���÷���״̬Ϊ����  
	SetServiceStatus(SERVICE_RUNNING); 
	__super::OnContinue();  
}  