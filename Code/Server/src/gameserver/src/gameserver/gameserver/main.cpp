#include <stdio.h>
#include <signal.h>
#include <locale.h>

//gameServer *g_server;

#if (defined(WIN32) ||defined(_WIN32) || defined(__WIN32__))
#define X_OS_WIN32
#endif

#if (defined(WIN64) ||defined(_WIN64) || defined(__WIN64__))
#define X_OS_WIN64
#endif

#if (defined(__linux) || defined(__linux__))
#define X_OS_LINUX
#endif

#if (defined(X_OS_WIN32) || defined(X_OS_WIN64))
#define X_OS_WIN
#endif

#define FAIL_HANDLE(name) \
	if(result < 0)\
	{\
		goto fail_handle_##name;\
	}

#define DEF_FAIL_HANDLE(name) fail_handle_##name:

#ifdef X_OS_WIN
//#include <windump.h>
#include <windows.h>
BOOL WINAPI ctrlHandle(DWORD code)
{
	switch(code)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		//g_server->setStopFlag();
		break;
	}
	return TRUE;
}
#else
void sigusr2_handle(int)
{
	//g_server->setStopFlag();
	signal(SIGUSR2, sigusr2_handle);
}
#endif

int core_init(int argc, char** argv)
{
#ifdef X_OS_WIN
//	g_server = new gameServer();
#else
	g_server = new(std::nothrow) gameServer();
#endif

//	if(!g_server)
//	{
//		printf("can't allocate memory for g_server, exit\n");
//		exit(-1);
//	}

// return g_server->init(argc, argv);

	return 0;
}

static int game_run()
{
//	return g_server->Run();
	return 0;
}

int main(int argc, char** argv)
{
#ifdef X_OS_WIN
	//AddWinDump("gameserver");
	SetConsoleCtrlHandler(ctrlHandle, TRUE);
#else
	signal(SIGUSR2, sigusr2_handle);
#endif

	::setlocale(LC_CTYPE, "");

#ifdef X_OS_WIN
	_set_error_mode(_OUT_TO_MSGBOX);
#endif

	//todo:读取配置
	//todo:Log初始化
	//todo:网络层初始化
	//todo:DbLog初始化
	//todo:服务器链接
	int result = 0;
//	result = app_3rd_init(argc, argv);FAIL_HANDLE(main);
	result = core_init(argc, argv);FAIL_HANDLE(main);
	result = game_run();FAIL_HANDLE(main);

	//todo:服务器链接结束
	//todo:DbLog结束
	//todo:网络层结束
	//todo:Log结束
	
	return result;

DEF_FAIL_HANDLE(main);
	exit(result);

	return 0;
}

