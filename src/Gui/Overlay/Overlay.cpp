#pragma once
#include <Gui/MenuShortcut.hpp>
#include "Overlay.hpp"
#include <Includes/ImGui/Files/imgui_freetype.h>
#include <Includes/CustomWidgets/Notify.hpp>
#include <Includes/ImGui/Images.hpp>
#include <Includes/ImGui/Fonts.hpp>
#include <Core/Core.hpp>
#include <Gui/Gui.hpp>
#include <Gui/MenuScale.hpp>
#include <dwmapi.h>
#include <tchar.h>
#include <thread>
#include <atomic>

//#define FULLSCREEN_EX

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

namespace Gui
{
	// Low-level keyboard hook para engolir a tecla de toggle do menu
	// antes que chegue no GTA5. Se GTA5 processar INSERT internamente
	// (mapa pra minimize, phone, etc.) o jogo minimiza sem relação com focus.
	static HHOOK   s_hKeyboardHook  = nullptr;
	static int     s_menuVkCode     = VK_INSERT; // atualizado em Render() antes do loop

	// IMPORTANTE: WH_KEYBOARD_LL retornando 1 cancela o input no kernel,
	// inclusive GetAsyncKeyState(key)&0x8000 nunca fica setado para teclas engolidas.
	// Usamos uma flag atômica setada DENTRO do hook para sinalizar o toggle.
	static std::atomic<bool> s_menuKeyTriggered { false };

	static LRESULT CALLBACK LLKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam ) {
		if ( nCode >= 0 && ( wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN ) ) {
			const KBDLLHOOKSTRUCT * kb = reinterpret_cast< KBDLLHOOKSTRUCT * >( lParam );
			if ( static_cast< int >( kb->vkCode ) == s_menuVkCode ) {
				// Sinaliza toggle antes de engolir, pois após retornar 1
				// o kernel NÃO atualiza o key state table — GetAsyncKeyState
				// nunca veria essa tecla. A flag é o único canal seguro.
				s_menuKeyTriggered.store( true, std::memory_order_release );
				return 1; // Engole: não passa pro GTA5
			}
		}
		return CallNextHookEx( s_hKeyboardHook, nCode, wParam, lParam );
	}

	// Restaurar foco para o jogo de forma confiável cross-process via AttachThreadInput.
	// SetForegroundWindow simples é bloqueado pelo Windows quando chamado de um
	// processo que não é foreground — AttachThreadInput contorna essa restrição.
	static void RestoreGameFocus( ) {
		if ( !g_Variables.g_hGameWindow ) return;
		DWORD fgTid  = GetWindowThreadProcessId( g_Variables.g_hGameWindow, nullptr );
		DWORD myTid  = GetCurrentThreadId( );
		if ( fgTid && fgTid != myTid ) {
			AttachThreadInput( myTid, fgTid, TRUE );
			SetForegroundWindow( g_Variables.g_hGameWindow );
			BringWindowToTop( g_Variables.g_hGameWindow );
			AttachThreadInput( myTid, fgTid, FALSE );
		} else {
			SetForegroundWindow( g_Variables.g_hGameWindow );
		}
	}
	typedef HWND( WINAPI * CreateWindowInBand )( _In_ DWORD dwExStyle, _In_opt_ ATOM atom, _In_opt_ LPCWSTR lpWindowName, _In_ DWORD dwStyle, _In_ int X, _In_ int Y, _In_ int nWidth, _In_ int nHeight, _In_opt_ HWND hWndParent, _In_opt_ HMENU hMenu, _In_opt_ HINSTANCE hInstance, _In_opt_ LPVOID lpParam, DWORD band );


	void Overlay::Render( )
	{

#ifdef FULLSCREEN_EX
		CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
		CreateWindowInBand pCreateWindowInBand = reinterpret_cast< CreateWindowInBand >( GetProcAddress( LoadLibraryA( xorstr( "user32.dll" ) ), xorstr( "CreateWindowInBand" ) ) );
		PrepareForUIAccess( );
#endif // FULLSCREEN_EX

		WNDCLASSEXW wc = { sizeof( wc ), CS_CLASSDC, Overlay::WndProc, 0L, 0L, GetModuleHandle( NULL ), NULL, NULL, NULL, NULL, L" ", NULL };
		ATOM RegClass = RegisterClassExW( &wc );

#ifdef FULLSCREEN_EX
		g_Variables.g_hCheatWindow = pCreateWindowInBand( WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, RegClass, L" ", WS_POPUP, static_cast< int >( g_Variables.g_vGameWindowPos.x ), static_cast< int >( g_Variables.g_vGameWindowPos.y ), static_cast< int >( g_Variables.g_vGameWindowSize.x ), static_cast< int >( g_Variables.g_vGameWindowSize.y ), NULL, NULL, wc.hInstance, NULL, 2/*ZBID_UIACCESS*/ );
#else
		// WS_EX_TRANSPARENT SEMPRE presente — sem ele, DXGI/DWM trata o overlay como
		// uma janela competidora do fullscreen exclusivo e minimiza o jogo no startup.
		// Click-through controlado por WM_NCHITTEST no WndProc:
		//   WS_EX_TRANSPARENT age via DefWindowProc (retorna HTTRANSPARENT). O WndProc
		//   AINDA recebe WM_NCHITTEST antes de DefWindowProc e pode retornar HTCLIENT
		//   para abrir mouse input pro ImGui — sem remover o flag, sem SetWindowLong.
		g_Variables.g_hCheatWindow = CreateWindowExW( WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, wc.lpszClassName, L" ", WS_POPUP, g_Variables.g_vGameWindowPos.x, g_Variables.g_vGameWindowPos.y, g_Variables.g_vGameWindowSize.x, g_Variables.g_vGameWindowSize.y, NULL, NULL, wc.hInstance, NULL );
#endif // FULLSCREEN_EX		
		
		SetLayeredWindowAttributes( g_Variables.g_hCheatWindow, RGB( 0, 0, 0 ), 255, LWA_ALPHA );
		MARGINS Margin = { -1, -1, -1, -1 };
		DwmExtendFrameIntoClientArea( g_Variables.g_hCheatWindow, &Margin );

		if ( !CreateDeviceD3D( g_Variables.g_hCheatWindow ) )
		{
			CleanupDeviceD3D( );
			UnregisterClassW( wc.lpszClassName, wc.hInstance );
			return;
		}


		SetWindowDisplayAffinity( g_Variables.g_hCheatWindow, WDA_EXCLUDEFROMCAPTURE );

		// SW_SHOWNOACTIVATE: mostra sem ativar a janela.
		// SW_SHOWDEFAULT ativa a janela e rouba o foco do GTA5 imediatamente
		// na injeção, causando minimize em fullscreen exclusivo.
		ShowWindow( g_Variables.g_hCheatWindow, SW_SHOWNOACTIVATE );
		UpdateWindow( g_Variables.g_hCheatWindow );

		// Coloca o overlay na frente sem roubar foco do jogo (evita alt-tab em fullscreen)
		SetWindowPos( g_Variables.g_hCheatWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE );
		// NÃO chamar BringWindowToTop nem SetForegroundWindow(overlay) aqui.

		// Devolve o foco ao GTA5 após a janela overlay aparecer.
		// Isso evita que o GTA5 perca foreground durante a criação do overlay e minimize.
		// O Sleep(150) dá tempo ao DWM/DXGI de estabilizar antes de transferir o foco.
		Sleep( 150 );
		if ( g_Variables.g_hGameWindow )
			SetForegroundWindow( g_Variables.g_hGameWindow );


		ImGui::CreateContext( );

		Style( );
		Fonts( );

		ImGui_ImplWin32_Init( g_Variables.g_hCheatWindow );
		ImGui_ImplDX11_Init( g_pd3dDevice, g_pd3dDeviceContext );



		Core::StartThreads( );
		if ( !Core::ThreadsStarted ) return;

		std::thread( [ & ] ( ) { NotifyManager::Send( xorstr( "We are here!" ), 4000 ); } ).detach( );

		// Instala hook de teclado low-level para engolir o INSERT (ou tecla customizada)
		// ANTES de chegar no GTA5. Se GTA5 tem INSERT mapeado para algo (minimize,
		// phone, pause), ele minimizaria independente do nosso foco. O hook previne isso.
		{
			const int customKey = g_Config.General->MenuKey;
			s_menuVkCode = ( customKey > 0 && customKey < 255 ) ? customKey : VK_INSERT;
		}
		s_hKeyboardHook = SetWindowsHookExW( WH_KEYBOARD_LL, LLKeyboardProc, GetModuleHandle( nullptr ), 0 );

		static RECT old_rc;
		ZeroMemory( &Message, sizeof( MSG ) );

		while ( Message.message != WM_QUIT )
		{
			while ( PeekMessage( &Message, g_Variables.g_hCheatWindow, 0, 0, PM_REMOVE ) ) {
				TranslateMessage( &Message );
				DispatchMessage( &Message );
			}

			HWND ActiveWindow = GetForegroundWindow( );

            // Em fullscreen exclusivo GetForegroundWindow() pode retornar uma janela DX
            // interna do GTA5 (não "grcWindow"). Verificamos o PID da janela foreground
            // contra o PID do processo do GTA5 (não do nosso processo) para cobrir isso.
            // Também aceitamos nossa janela overlay em foreground (menu aberto).
            DWORD fgPid   = 0;
            DWORD gamePid = 0;
            if ( ActiveWindow ) GetWindowThreadProcessId( ActiveWindow, &fgPid );
            if ( g_Variables.g_hGameWindow ) GetWindowThreadProcessId( g_Variables.g_hGameWindow, &gamePid );
            const bool gameHasFocus =
                ActiveWindow == g_Variables.g_hGameWindow  ||
                ActiveWindow == g_Variables.g_hCheatWindow ||
                ( fgPid != 0 && gamePid != 0 && fgPid == gamePid ) ||
                ( fgPid != 0 && fgPid == GetCurrentProcessId( ) );

            // Toggle once per key press, including while the overlay has focus.
            // Atualiza s_menuVkCode para o hook de teclado ficar sincronizado.
            // NOTA: teclas engolidas pelo WH_KEYBOARD_LL não atualizam o key state table,
            // então GetAsyncKeyState & 0x8000 nunca fica setado para elas.
            // Usamos s_menuKeyTriggered (setada no hook) para INSERT/customKey,
            // e GetAsyncKeyState apenas para 'P' (que não é engolida pelo hook).
            const int customKey = g_Config.General->MenuKey;
            s_menuVkCode = ( customKey > 0 && customKey < 255 ) ? customKey : VK_INSERT;

            // Lê e limpa a flag atômica — disparada uma vez por keydown no hook
            const bool hookFired = s_menuKeyTriggered.exchange( false, std::memory_order_acq_rel );

            // 'P' não é engolida pelo hook, usa edge-detection normal
            static bool s_prevP = false;
            const bool pDown   = ( GetAsyncKeyState( 'P' ) & 0x8000 ) != 0;
            const bool pEdge   = pDown && !s_prevP;
            s_prevP = pDown;

            // hookFired cobre INSERT (default) OU customKey (ambos são engolidos pelo hook)
            // pEdge cobre 'P' somente quando não há customKey configurado
            const bool pressed = hookFired || ( !( customKey > 0 && customKey < 255 ) && pEdge );
            const bool editingShortcut = MenuShortcut::Poll(g_Config.General->MenuKey,
                gameHasFocus);
            // hookFired: INSERT/customKey engolida globalmente — se o hook disparou,
            // o usuário pressionou a tecla. Não exige gameHasFocus pois o hook global
            // só é relevante quando o mod está rodando. pEdge (tecla P) ainda exige
            // gameHasFocus para não toggle acidental com outro app em foreground.
            const bool hookToggle = !editingShortcut && hookFired;
            const bool keyToggle  = !editingShortcut && ( !( customKey > 0 && customKey < 255 ) && pEdge ) && gameHasFocus;
            if ( hookToggle || keyToggle ) {
                g_MenuInfo.IsOpen = !g_MenuInfo.IsOpen;
                // Ao fechar o menu, devolve o foco ao jogo se o overlay ficou em foreground.
                // Não usa SetWindowLong — isso triggera DWM reactivation e minimiza o GTA5.
                // Click-through é controlado por WM_NCHITTEST no WndProc.
                if ( !g_MenuInfo.IsOpen && ActiveWindow == g_Variables.g_hCheatWindow )
                    RestoreGameFocus( );
            }


			static float BgAlpha = 0.f;
			BgAlpha = ImLerp( BgAlpha, g_MenuInfo.IsOpen ? 1.f : 0.f, ImGui::GetIO( ).DeltaTime * 8 );

			ImGui_ImplDX11_NewFrame( );
			ImGui_ImplWin32_NewFrame( );
			
			MenuScale::PrepareFrame();
            ImGui::NewFrame( );
            MenuScale::BeginFrame();
			{
				

				ImGui::PushStyleVar( ImGuiStyleVar_Alpha, BgAlpha );

				ImGui::GetBackgroundDrawList( )->AddRectFilled( ImVec2( 0, 0 ), ImVec2( g_Variables.g_vGameWindowSize.x, g_Variables.g_vGameWindowSize.y ), ImColor( 0.f, 0.f, 0.f, BgAlpha >= 0.4f ? 0.4f : BgAlpha ) );
				Gui::Rendering( );

				ImGui::PopStyleVar( );
			}
			MenuScale::EndFrame();
            ImGui::EndFrame( );

			const float ClearColor[ 4 ] = { 0 };
			g_pd3dDeviceContext->OMSetRenderTargets( 1U, &g_mainRenderTargetView, NULL );
			g_pd3dDeviceContext->ClearRenderTargetView( g_mainRenderTargetView, ClearColor );

			ImGui::Render( );
            MenuScale::PrepareDrawData(ImGui::GetDrawData());
			ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );

			g_pSwapChain->Present( g_Config.General->VSync, 0U ); //VSync

            // Re-assert HWND_TOPMOST every ~60 frames so the overlay survives
            // fullscreen transitions (flip-model exclusive fullscreen on W10/11
            // still composites TOPMOST layered windows over the game).
            static int s_topmostCounter = 0;
            if ( ++s_topmostCounter >= 60 ) {
                s_topmostCounter = 0;
                SetWindowPos( g_Variables.g_hCheatWindow, HWND_TOPMOST, 0, 0, 0, 0,
                              SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
            }
		}

		if ( s_hKeyboardHook ) { UnhookWindowsHookEx( s_hKeyboardHook ); s_hKeyboardHook = nullptr; }

		ImGui_ImplDX11_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );

		CleanupDeviceD3D( );
		DestroyWindow( g_Variables.g_hCheatWindow );
		UnregisterClassW( wc.lpszClassName, wc.hInstance );


		return;
	}

	void Overlay::Fonts( )
	{
		ImGuiIO & io = ImGui::GetIO( ); ( void ) io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.IniFilename = nullptr;

		ImFontConfig Cfg;
		Cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_LoadColor;
		Cfg.FontDataOwnedByAtlas = false;

		ImFontConfig DrawCfg;
		DrawCfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_Bitmap;
		DrawCfg.FontDataOwnedByAtlas = false;


		g_Variables.m_FontBig = io.Fonts->AddFontFromMemoryTTF( InterSemiBold, sizeof( InterSemiBold ), 20.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic( ) );
		g_Variables.m_FontBigSmall = io.Fonts->AddFontFromMemoryTTF( InterSemiBold, sizeof( InterSemiBold ), 18.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic( ) );
		g_Variables.m_FontNormal = io.Fonts->AddFontFromMemoryTTF( InterSemiBold, sizeof( InterSemiBold ), 18.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic( ) );
		g_Variables.m_FontSecundary = io.Fonts->AddFontFromMemoryTTF( InterSemiBold, sizeof( InterSemiBold ), 16.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic( ) );
		g_Variables.m_FontSmaller = io.Fonts->AddFontFromMemoryTTF( InterSemiBold, sizeof( InterSemiBold ), 14.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic( ) );

		g_Variables.m_DrawFont = io.Fonts->AddFontFromMemoryTTF( InterSemiBold, sizeof( InterSemiBold ), 12.f, &DrawCfg, io.Fonts->GetGlyphRangesCyrillic( ) );

		static const ImWchar FontAwesomeRanges[ ] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		static const ImWchar FontAwesomeRangesBrands[ ] = { ICON_MIN_FAB, ICON_MAX_FAB, 0 };

		ImFontConfig FontAwesomeConfig;
		FontAwesomeConfig.PixelSnapH = true;
		FontAwesomeConfig.GlyphMinAdvanceX = 19;

		g_Variables.FontAwesomeSolid = io.Fonts->AddFontFromMemoryCompressedTTF( FontAwesome6Solid_compressed_data, sizeof( FontAwesome6Solid_compressed_size ), 19, &FontAwesomeConfig, FontAwesomeRanges );
		g_Variables.FontAwesomeSolidSmall = io.Fonts->AddFontFromMemoryCompressedTTF( FontAwesome6Solid_compressed_data, sizeof( FontAwesome6Solid_compressed_size ), 16, &FontAwesomeConfig, FontAwesomeRanges );
		g_Variables.FontAwesomeRegular = io.Fonts->AddFontFromMemoryCompressedTTF( FontAwesome6Regular_compressed_data, sizeof( FontAwesome6Regular_compressed_size ), 19, &FontAwesomeConfig, FontAwesomeRanges );

		g_Variables.m_Expand = io.Fonts->AddFontFromMemoryTTF( expand_binary, sizeof( expand_binary ), 11.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic( ) );
	}

	void Overlay::Style( )
	{
		ImGui::StyleColorsDark( );

		ImGuiStyle * style = &ImGui::GetStyle( );
		{
			style->WindowPadding = ImVec2( 0, 0 );
			style->WindowBorderSize = 0.f;
			style->ItemSpacing = ImVec2( 12, 12 );
			style->ScrollbarSize = 4.f;
		}

		if ( g_Variables.Logo == nullptr )
		{
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, logo_binary, sizeof( logo_binary ), &Info, Pump, &g_Variables.Logo, 0 );
			//WeaponsIcons
			Overlay::LoadWeaponIcons( );
		}
	}

	bool Overlay::CreateDeviceD3D( HWND hWnd )
	{
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory( &sd, sizeof( sd ) );
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		// ALLOW_MODE_SWITCH removed: essa flag permite que o DXGI troque o modo de display,
		// o que interrompe o fullscreen exclusivo do GTA5 e causa minimize automático.
		// Overlay não precisa de mode switching — deve ficar windowed sempre.
		sd.Flags = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		UINT createDeviceFlags = 0;
		//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[ 2 ] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
		if ( D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext ) != S_OK )
			return false;

		CreateRenderTarget( );
		return true;
	}

	void Overlay::CleanupDeviceD3D( )
	{
		CleanupRenderTarget( );

		if ( g_pSwapChain ) { g_pSwapChain->Release( ); g_pSwapChain = NULL; }
		if ( g_pd3dDeviceContext ) { g_pd3dDeviceContext->Release( ); g_pd3dDeviceContext = NULL; }
		if ( g_pd3dDevice ) { g_pd3dDevice->Release( ); g_pd3dDevice = NULL; }
	}

	void Overlay::CreateRenderTarget( )
	{
		ID3D11Texture2D * pBackBuffer;
		g_pSwapChain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );
		g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_mainRenderTargetView );
		pBackBuffer->Release( );
	}

	void Overlay::CleanupRenderTarget( )
	{
		if ( g_mainRenderTargetView ) { g_mainRenderTargetView->Release( ); g_mainRenderTargetView = NULL; }
	}

	LRESULT CALLBACK Overlay::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {

		// Bloquear TODA ativação do overlay ANTES do ImGui processar qualquer mensagem.
		// WS_EX_NOACTIVATE previne ativação por clique mas não impede o Windows de
		// enviar WM_ACTIVATE em resposta a SetWindowLong/ShowWindow/SetWindowPos.
		// Se ImGui_ImplWin32_WndProcHandler processar WM_ACTIVATE antes de nós,
		// ele pode retornar true e o overlay vira foreground → GTA5 minimiza.
		switch ( msg ) {
		case WM_NCHITTEST:
			// Click-through controlado por WM_NCHITTEST — SEM remover WS_EX_TRANSPARENT.
			// WS_EX_TRANSPARENT fica presente SEMPRE (evita minimize no startup com DXGI).
			// DefWindowProc retornaria HTTRANSPARENT por causa do WS_EX_TRANSPARENT flag,
			// mas o WndProc intercepta ANTES e pode retornar o valor que quiser:
			//   - Menu fechado → HTTRANSPARENT: todo mouse passa pro GTA5 abaixo.
			//   - Menu aberto  → HTCLIENT: ImGui recebe mouse (override do WS_EX_TRANSPARENT).
			// NUNCA usar SetWindowLong(GWL_EXSTYLE) para trocar WS_EX_TRANSPARENT —
			// isso triggera DWM reactivation que minimiza o jogo em fullscreen exclusivo.
			return g_MenuInfo.IsOpen ? HTCLIENT : HTTRANSPARENT;

		case WM_MOUSEACTIVATE:
			// Não ativar o overlay quando o mouse clicar nele.
			// MA_NOACTIVATE: não ativa a janela MAS processa o clique normalmente (ImGui recebe).
			return MA_NOACTIVATE;

		case WM_NCACTIVATE:
			// Não desenhar o nonclient area como "ativo" (evita flicker de título).
			return FALSE;

		case WM_ACTIVATE:
			// Se o Windows tentar ativar nossa janela, devolver o foco ao GTA5.
			// Usa AttachThreadInput para garantir que SetForegroundWindow funcione
			// cross-process (SetForegroundWindow simples é bloqueado quando o
			// processo chamador não é foreground).
			if ( LOWORD( wParam ) != WA_INACTIVE ) {
				RestoreGameFocus( );
				return 0;
			}
			break;

		case WM_SETFOCUS:
			// Se o foco vier para o overlay por qualquer razão, dar de volta ao jogo.
			RestoreGameFocus( );
			return 0;
		}

		if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
			return true;

		switch ( msg ) {
		case WM_SIZE:
			if ( g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED ) {
				g_pd3dDeviceContext->OMSetRenderTargets( 0, nullptr, nullptr );
				Overlay::CleanupRenderTarget( );
				g_pSwapChain->ResizeBuffers( 0, ( UINT ) LOWORD( lParam ), ( UINT ) HIWORD( lParam ), DXGI_FORMAT_UNKNOWN, 0 );
				Overlay::CreateRenderTarget( );
			}
			return 0;
		case WM_SYSCOMMAND:
			if ( ( wParam & 0xfff0 ) == SC_KEYMENU ) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage( 0 );
			return 0;
			break;
		}

		return DefWindowProc( hWnd, msg, wParam, lParam );
	}

	DWORD Overlay::GetWinLogonToken( DWORD dwSessionId, DWORD dwDesiredAccess, PHANDLE phToken )
	{
		DWORD dwErr;
		PRIVILEGE_SET ps;

		ps.PrivilegeCount = 1;
		ps.Control = PRIVILEGE_SET_ALL_NECESSARY;

		if ( LookupPrivilegeValue( NULL, SE_TCB_NAME, &ps.Privilege[ 0 ].Luid ) ) {
			HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
			if ( INVALID_HANDLE_VALUE != hSnapshot ) {
				BOOL bCont, bFound = FALSE;
				PROCESSENTRY32 pe;

				pe.dwSize = sizeof( pe );
				dwErr = ERROR_NOT_FOUND;

				for ( bCont = Process32First( hSnapshot, &pe ); bCont; bCont = Process32Next( hSnapshot, &pe ) ) {
					HANDLE hProcess;

					if ( 0 != _tcsicmp( pe.szExeFile, xorstr( "winlogon.exe" ) ) ) {
						continue;
					}

					hProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID );
					if ( hProcess ) {
						HANDLE hToken;
						DWORD dwRetLen, sid;

						if ( OpenProcessToken( hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken ) ) {
							BOOL fTcb;

							if ( PrivilegeCheck( hToken, &ps, &fTcb ) && fTcb ) {
								if ( GetTokenInformation( hToken, TokenSessionId, &sid, sizeof( sid ), &dwRetLen ) && sid == dwSessionId ) {
									bFound = TRUE;
									if ( DuplicateTokenEx( hToken, dwDesiredAccess, NULL, SecurityImpersonation, TokenImpersonation, phToken ) ) {
										dwErr = ERROR_SUCCESS;
									}
									else {
										dwErr = GetLastError( );
									}
								}
							}
							CloseHandle( hToken );
						}
						CloseHandle( hProcess );
					}

					if ( bFound ) break;
				}

				CloseHandle( hSnapshot );
			}
			else {
				dwErr = GetLastError( );
			}
		}
		else {
			dwErr = GetLastError( );
		}


		return dwErr;
	}

	DWORD Overlay::CreateUIAccessToken( PHANDLE phToken )
	{
		DWORD dwErr;
		HANDLE hTokenSelf;

		if ( OpenProcessToken( GetCurrentProcess( ), TOKEN_QUERY | TOKEN_DUPLICATE, &hTokenSelf ) ) {
			DWORD dwSessionId, dwRetLen;

			if ( GetTokenInformation( hTokenSelf, TokenSessionId, &dwSessionId, sizeof( dwSessionId ), &dwRetLen ) ) {
				HANDLE hTokenSystem;

				dwErr = GetWinLogonToken( dwSessionId, TOKEN_IMPERSONATE, &hTokenSystem );
				if ( ERROR_SUCCESS == dwErr ) {
					if ( SetThreadToken( NULL, hTokenSystem ) ) {
						if ( DuplicateTokenEx( hTokenSelf, TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT, NULL, SecurityAnonymous, TokenPrimary, phToken ) ) {
							BOOL bUIAccess = TRUE;

							if ( !SetTokenInformation( *phToken, TokenUIAccess, &bUIAccess, sizeof( bUIAccess ) ) ) {
								dwErr = GetLastError( );
								CloseHandle( *phToken );
							}
						}
						else {
							dwErr = GetLastError( );
						}
						RevertToSelf( );
					}
					else {
						dwErr = GetLastError( );
					}
					CloseHandle( hTokenSystem );
				}
			}
			else {
				dwErr = GetLastError( );
			}

			CloseHandle( hTokenSelf );
		}
		else {
			dwErr = GetLastError( );
		}

		return dwErr;
	}

	BOOL Overlay::CheckForUIAccess( DWORD * pdwErr, DWORD * pfUIAccess )
	{
		BOOL result = FALSE;
		HANDLE hToken;

		if ( OpenProcessToken( GetCurrentProcess( ), TOKEN_QUERY, &hToken ) ) {
			DWORD dwRetLen;

			if ( GetTokenInformation( hToken, TokenUIAccess, pfUIAccess, sizeof( *pfUIAccess ), &dwRetLen ) ) {
				result = TRUE;
			}
			else {
				*pdwErr = GetLastError( );
			}
			CloseHandle( hToken );
		}
		else {
			*pdwErr = GetLastError( );
		}

		return result;
	}

	DWORD Overlay::PrepareForUIAccess( )
	{
		DWORD dwErr;
		HANDLE hTokenUIAccess;
		DWORD fUIAccess;

		if ( CheckForUIAccess( &dwErr, &fUIAccess ) ) {
			if ( fUIAccess ) {
				dwErr = ERROR_SUCCESS;
			}
			else {
				dwErr = CreateUIAccessToken( &hTokenUIAccess );
				if ( ERROR_SUCCESS == dwErr ) {
					STARTUPINFO si;
					PROCESS_INFORMATION pi;

					GetStartupInfo( &si );
					if ( CreateProcessAsUser( hTokenUIAccess, NULL, GetCommandLine( ), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) ) {
						CloseHandle( pi.hProcess ), CloseHandle( pi.hThread );
						ExitProcess( 0 );
					}
					else {
						dwErr = GetLastError( );
					}

					CloseHandle( hTokenUIAccess );
				}
			}
		}

		return dwErr;
	}

	void Overlay::LoadWeaponIcons( ) {
		{
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::gadget_parachute, sizeof( FiveM_Weapons::gadget_parachute ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.gadget_parachute, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_advancedrifle, sizeof( FiveM_Weapons::weapon_advancedrifle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_advancedrifle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_appistol, sizeof( FiveM_Weapons::weapon_appistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_appistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_assaultrifle, sizeof( FiveM_Weapons::weapon_assaultrifle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_assaultrifle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_assaultrifle_mk2, sizeof( FiveM_Weapons::weapon_assaultrifle_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_assaultrifle_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_assaultshotgun, sizeof( FiveM_Weapons::weapon_assaultshotgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_assaultshotgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_assaultsmg, sizeof( FiveM_Weapons::weapon_assaultsmg ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_assaultsmg, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_autoshotgun, sizeof( FiveM_Weapons::weapon_autoshotgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_autoshotgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_ball, sizeof( FiveM_Weapons::weapon_ball ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_ball, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_bat, sizeof( FiveM_Weapons::weapon_bat ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_bat, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_battleaxe, sizeof( FiveM_Weapons::weapon_battleaxe ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_battleaxe, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_bottle, sizeof( FiveM_Weapons::weapon_bottle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_bottle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_bullpuprifle, sizeof( FiveM_Weapons::weapon_bullpuprifle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_bullpuprifle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_bullpuprifle_mk2, sizeof( FiveM_Weapons::weapon_bullpuprifle_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_bullpuprifle_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_bullpupshotgun, sizeof( FiveM_Weapons::weapon_bullpupshotgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_bullpupshotgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_bzgas, sizeof( FiveM_Weapons::weapon_bzgas ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_bzgas, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_carbinerifle, sizeof( FiveM_Weapons::weapon_carbinerifle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_carbinerifle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_carbinerifle_mk2, sizeof( FiveM_Weapons::weapon_carbinerifle_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_carbinerifle_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_ceramicpistol, sizeof( FiveM_Weapons::weapon_ceramicpistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_ceramicpistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_combatmg, sizeof( FiveM_Weapons::weapon_combatmg ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_combatmg, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_combatmg_mk2, sizeof( FiveM_Weapons::weapon_combatmg_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_combatmg_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_combatpdw, sizeof( FiveM_Weapons::weapon_combatpdw ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_combatpdw, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_combatpistol, sizeof( FiveM_Weapons::weapon_combatpistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_combatpistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_combatshotgun, sizeof( FiveM_Weapons::weapon_combatshotgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_combatshotgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_compactlauncher, sizeof( FiveM_Weapons::weapon_compactlauncher ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_compactlauncher, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_compactrifle, sizeof( FiveM_Weapons::weapon_compactrifle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_compactrifle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_crowbar, sizeof( FiveM_Weapons::weapon_crowbar ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_crowbar, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_dagger, sizeof( FiveM_Weapons::weapon_dagger ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_dagger, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_dbshotgun, sizeof( FiveM_Weapons::weapon_dbshotgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_dbshotgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_doubleaction, sizeof( FiveM_Weapons::weapon_doubleaction ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_doubleaction, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_fireextinguisher, sizeof( FiveM_Weapons::weapon_fireextinguisher ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_fireextinguisher, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_firework, sizeof( FiveM_Weapons::weapon_firework ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_firework, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_flare, sizeof( FiveM_Weapons::weapon_flare ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_flare, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_flaregun, sizeof( FiveM_Weapons::weapon_flaregun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_flaregun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_flashlight, sizeof( FiveM_Weapons::weapon_flashlight ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_flashlight, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_gadgetpistol, sizeof( FiveM_Weapons::weapon_gadgetpistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_gadgetpistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_golfclub, sizeof( FiveM_Weapons::weapon_golfclub ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_golfclub, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_grenade, sizeof( FiveM_Weapons::weapon_grenade ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_grenade, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_grenadelauncher, sizeof( FiveM_Weapons::weapon_grenadelauncher ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_grenadelauncher, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_grenadelauncher_smoke, sizeof( FiveM_Weapons::weapon_grenadelauncher_smoke ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_grenadelauncher_smoke, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_gusenberg, sizeof( FiveM_Weapons::weapon_gusenberg ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_gusenberg, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_hammer, sizeof( FiveM_Weapons::weapon_hammer ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_hammer, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_hatchet, sizeof( FiveM_Weapons::weapon_hatchet ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_hatchet, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_hazardcan, sizeof( FiveM_Weapons::weapon_hazardcan ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_hazardcan, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_heavypistol, sizeof( FiveM_Weapons::weapon_heavypistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_heavypistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_heavyshotgun, sizeof( FiveM_Weapons::weapon_heavyshotgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_heavyshotgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_heavysniper, sizeof( FiveM_Weapons::weapon_heavysniper ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_heavysniper, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_hominglauncher, sizeof( FiveM_Weapons::weapon_hominglauncher ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_hominglauncher, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_knife, sizeof( FiveM_Weapons::weapon_knife ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_knife, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_knuckle, sizeof( FiveM_Weapons::weapon_knuckle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_knuckle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_machete, sizeof( FiveM_Weapons::weapon_machete ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_machete, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_machinepistol, sizeof( FiveM_Weapons::weapon_machinepistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_machinepistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_marksmanpistol, sizeof( FiveM_Weapons::weapon_marksmanpistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_marksmanpistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_marksmanrifle, sizeof( FiveM_Weapons::weapon_marksmanrifle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_marksmanrifle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_marksmanrifle_mk2, sizeof( FiveM_Weapons::weapon_marksmanrifle_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_marksmanrifle_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_mg, sizeof( FiveM_Weapons::weapon_mg ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_mg, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_microsmg, sizeof( FiveM_Weapons::weapon_microsmg ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_microsmg, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_militaryrifle, sizeof( FiveM_Weapons::weapon_militaryrifle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_militaryrifle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_minigun, sizeof( FiveM_Weapons::weapon_minigun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_minigun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_minismg, sizeof( FiveM_Weapons::weapon_minismg ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_minismg, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_molotov, sizeof( FiveM_Weapons::weapon_molotov ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_molotov, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_musket, sizeof( FiveM_Weapons::weapon_musket ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_musket, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_navyrevolver, sizeof( FiveM_Weapons::weapon_navyrevolver ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_navyrevolver, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_nightstick, sizeof( FiveM_Weapons::weapon_nightstick ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_nightstick, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_petrolcan, sizeof( FiveM_Weapons::weapon_petrolcan ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_petrolcan, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_pipebomb, sizeof( FiveM_Weapons::weapon_pipebomb ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_pipebomb, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_pistol, sizeof( FiveM_Weapons::weapon_pistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_pistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_pistol50, sizeof( FiveM_Weapons::weapon_pistol50 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_pistol50, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_pistol_mk2, sizeof( FiveM_Weapons::weapon_pistol_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_pistol_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_poolcue, sizeof( FiveM_Weapons::weapon_poolcue ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_poolcue, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_proxmine, sizeof( FiveM_Weapons::weapon_proxmine ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_proxmine, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_pumpshotgun, sizeof( FiveM_Weapons::weapon_pumpshotgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_pumpshotgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_pumpshotgun_mk2, sizeof( FiveM_Weapons::weapon_pumpshotgun_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_pumpshotgun_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_railgun, sizeof( FiveM_Weapons::weapon_railgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_railgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_raycarbine, sizeof( FiveM_Weapons::weapon_raycarbine ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_raycarbine, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_rayminigun, sizeof( FiveM_Weapons::weapon_rayminigun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_rayminigun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_raypistol, sizeof( FiveM_Weapons::weapon_raypistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_raypistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_revolver, sizeof( FiveM_Weapons::weapon_revolver ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_revolver, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_revolver_mk2, sizeof( FiveM_Weapons::weapon_revolver_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_revolver_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_rpg, sizeof( FiveM_Weapons::weapon_rpg ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_rpg, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_sawnoffshotgun, sizeof( FiveM_Weapons::weapon_sawnoffshotgun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_sawnoffshotgun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_smg, sizeof( FiveM_Weapons::weapon_smg ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_smg, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_smg_mk2, sizeof( FiveM_Weapons::weapon_smg_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_smg_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_smokegrenade, sizeof( FiveM_Weapons::weapon_smokegrenade ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_smokegrenade, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_sniperrifle, sizeof( FiveM_Weapons::weapon_sniperrifle ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_sniperrifle, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_snowball, sizeof( FiveM_Weapons::weapon_snowball ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_snowball, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_snspistol, sizeof( FiveM_Weapons::weapon_snspistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_snspistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_snspistol_mk2, sizeof( FiveM_Weapons::weapon_snspistol_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_snspistol_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_specialcarbine, sizeof( FiveM_Weapons::weapon_specialcarbine ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_specialcarbine, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_specialcarbine_mk2, sizeof( FiveM_Weapons::weapon_specialcarbine_mk2 ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_specialcarbine_mk2, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_stickybomb, sizeof( FiveM_Weapons::weapon_stickybomb ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_stickybomb, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_stone_hatchet, sizeof( FiveM_Weapons::weapon_stone_hatchet ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_stone_hatchet, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_stungun, sizeof( FiveM_Weapons::weapon_stungun ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_stungun, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_switchblade, sizeof( FiveM_Weapons::weapon_switchblade ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_switchblade, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_unarmed, sizeof( FiveM_Weapons::weapon_unarmed ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_unarmed, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_vintagepistol, sizeof( FiveM_Weapons::weapon_vintagepistol ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_vintagepistol, 0 );
			D3DX11CreateShaderResourceViewFromMemory( g_pd3dDevice, FiveM_Weapons::weapon_wrench, sizeof( FiveM_Weapons::weapon_wrench ), &Info, Pump, &g_Variables.FiveM_WeaponsPic.weapon_wrench, 0 );
		}
	}
}
