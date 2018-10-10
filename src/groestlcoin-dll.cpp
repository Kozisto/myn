

#define main myntd_main
#include "myntd.cpp"

extern "C" __declspec(dllexport) void myntdEntry() {
	exit(myntd_main(__argc, __argv));
}

#undef main
#define main mynt_cli_main
#include "mynt-cli.cpp"

extern "C" __declspec(dllexport) void GroestlcoinCliEntry() {
	exit(mynt_cli_main(__argc, __argv));
}

#undef main
#define main mynt_tx_main
#include "mynt-tx.cpp"

extern "C" __declspec(dllexport) void GroestlcoinTxEntry() {
	exit(mynt_tx_main(__argc, __argv));
}

#undef main
#include "qt/bitcoin.cpp"

extern "C" WORD __cdecl __scrt_get_show_window_mode();

extern "C" __declspec(dllexport) void GroestlcoinQtEntry() {
	ExitProcess(wWinMain(GetModuleHandle(0), nullptr, _get_wide_winmain_command_line(), __scrt_get_show_window_mode()));
}
