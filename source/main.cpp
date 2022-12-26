#include "pch.h"

#if defined(_DEBUG)
#include "vld.h"
#endif

#undef main
#include "Renderer.h"

using namespace dae;

bool gDisplayFPS = false;


void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

void ToggleDisplayFPS()
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (gDisplayFPS == true)
	{
		gDisplayFPS = false;

		SetConsoleTextAttribute(h, 6);
		std::cout << "**(SHARED) Print FPS OFF\n";
		SetConsoleTextAttribute(h, 7);
	}
	else if (gDisplayFPS == false)
	{
		gDisplayFPS = true;

		SetConsoleTextAttribute(h, 6);
		std::cout << "**(SHARED) Print FPS ON\n";
		SetConsoleTextAttribute(h, 7);
	}

}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"DirectX - ***Dewachtere Michiel/2DAE08***",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Display info in console
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(h, 6);
	std::cout << "[Key Bindings - SHARED]\n"
		<< "  [F1]  Toggle Rasterizer Mode (HARDWARE/SOFTWARE)\n"
		<< "  [F2]  Toggle Vehicle Rotation (ON/OFF)\n"
		<< "  [F9]  Cycle CullMode (BACK/FRONT/NONE)\n"
		<< "  [F10] Toggle Uniform ClearColor (ON/OFF)\n"
		<< "  [F11] Toggle Print FPS (ON/OFF)\n\n";
	SetConsoleTextAttribute(h, 2);
	std::cout << "[Key Bindings - HARDWARE]\n"
		<< "  [F3]  Toggle FireFX (ON/OFF)\n"
		<< "  [F4]  Cycle Sampler State (POINT/LINEAR/ANISOTROPIC)\n\n";
	SetConsoleTextAttribute(h, 5);
	std::cout << "[Key Bindings - SOFTWARE]\n"
		<< "  [F5]  Cycle Shading Mode (COMBINED/OBSERVED AREA/DIFFUSE/SPECULAR)\n"
		<< "  [F6]  Toggle NormalMap (ON/OFF)\n"
		<< "  [F7]  Toggle DepthBuffer Visualization (ON/OFF)\n"
		<< "  [F8]  Toggle BoundingBox Visualization (ON/OFF)\n";
	SetConsoleTextAttribute(h, 7);

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				if (e.key.keysym.scancode == SDL_SCANCODE_F1) {}
				if (e.key.keysym.scancode == SDL_SCANCODE_F2) { pRenderer->ToggleRotation(); }
				if (e.key.keysym.scancode == SDL_SCANCODE_F3) { pRenderer->ToggleFireFx(); }
				if (e.key.keysym.scancode == SDL_SCANCODE_F4) { pRenderer->ToggleSamplerState(); }
				if (e.key.keysym.scancode == SDL_SCANCODE_F5) {}
				if (e.key.keysym.scancode == SDL_SCANCODE_F6) {}
				if (e.key.keysym.scancode == SDL_SCANCODE_F7) {}
				if (e.key.keysym.scancode == SDL_SCANCODE_F8) {}
				if (e.key.keysym.scancode == SDL_SCANCODE_F9) { pRenderer->ToggleCullMode(); }
				if (e.key.keysym.scancode == SDL_SCANCODE_F10) { pRenderer->ToggleUniformClearColor(); }
				if (e.key.keysym.scancode == SDL_SCANCODE_F11) { ToggleDisplayFPS(); }
				break;
			default: ;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f && gDisplayFPS)
		{
			printTimer = 0.f;
			SetConsoleTextAttribute(h, 8);
			std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
			SetConsoleTextAttribute(h, 7);
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete pTimer;

	ShutDown(pWindow);
	return 0;
}