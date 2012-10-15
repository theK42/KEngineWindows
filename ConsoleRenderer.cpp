#include <cassert>
#include <windows.h>
#include "ConsoleRenderer.h"

KEngineWindows::ConsoleGraphic::ConsoleGraphic()
{
	mTransform = nullptr;
	mCharacters = nullptr;
}

KEngineWindows::ConsoleGraphic::~ConsoleGraphic()
{
	Deinit();
}

void KEngineWindows::ConsoleGraphic::Init( ConsoleRenderer * renderer, CharMap const * characters, KEngine2D::Transform const * transform )
{
	assert(transform != nullptr);
	assert(renderer != nullptr);
	mRenderer = renderer;
	mCharacters = characters;
	mTransform = transform;
	renderer->AddToRenderList(this);
}

void KEngineWindows::ConsoleGraphic::Deinit()
{
	if (mRenderer != nullptr)
	{
		mRenderer->RemoveFromRenderList(this);
	}
	mTransform = nullptr;
	mCharacters = nullptr;
	mRenderer = nullptr;
}

KEngineWindows::CharMap const * KEngineWindows::ConsoleGraphic::GetCharMap() const
{
	return mCharacters;
}

void KEngineWindows::ConsoleGraphic::SetCharMap( CharMap const * characters )
{
	assert(mTransform != nullptr); /// Initialized
	mCharacters = characters;
}

KEngine2D::Transform const * KEngineWindows::ConsoleGraphic::GetTransform() const
{
	assert(mTransform != nullptr);
	return mTransform;
}

KEngineWindows::ConsoleRenderer::ConsoleRenderer()
{
	mInitialized = false;
	mBuffer = nullptr;
}

KEngineWindows::ConsoleRenderer::~ConsoleRenderer()
{
	Deinit();
}

void KEngineWindows::ConsoleRenderer::Init( int width, int height )
{
	assert(!mInitialized);
	mWidth = width;
	mHeight = height;
	mBuffer = new char [width * height];
	mInitialized = true;
}

void KEngineWindows::ConsoleRenderer::Deinit()
{
	mInitialized = false;
	mRenderList.clear();
}

void KEngineWindows::ConsoleRenderer::Render() const
{	
	assert(mInitialized);
	HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD cursor;
	DWORD writeCount;
	//Uses the buffer to minimize flickering.
	for (int i = 0; i < mHeight; i++)  
	{
		for (int j = 0; j < mWidth; j++)
		{
			mBuffer[(i * mWidth) + j] = ' ';
		}
	}
	for (std::list<ConsoleGraphic *>::const_iterator it = mRenderList.begin(); it != mRenderList.end(); it++)
	{
		KEngine2D::Point graphicPosition =  (*it)->GetTransform()->GetTranslation(); //Need to handle other parts of the transform as well
		CharMap const * charMap = (*it)->GetCharMap();
		///Could implement some sort of camera here
		int x = int(graphicPosition.x + .5f); //Round instead of just truncate
		int y = int(graphicPosition.y + .5f);
		for (int i = 0; i < charMap->width; i++) {
			for (int j = 0; j < charMap->height; j++) {
				char character = charMap->characters[(j * charMap->width) + i];
				if (character != ' ' && x + i >= 0 && x + i < mWidth && y + j >= 0 && y + j < mHeight)
				{
					mBuffer[((y + j) * mWidth) + (x + i)] = character;
				}
			}
		}		
	}
	for (int i = 0; i < mHeight; i++)  
	{
		for (int j = 0; j < mWidth; j++)
		{
			cursor.X = j;
			cursor.Y = i;
			WriteConsoleOutputCharacterA(hConsoleOutput, &mBuffer[(i * mWidth) + j], 1, cursor, &writeCount);
		}
	}
}

void KEngineWindows::ConsoleRenderer::AddToRenderList( ConsoleGraphic * consoleGraphic )
{
	assert(mInitialized);
	mRenderList.push_back(consoleGraphic);
}

void KEngineWindows::ConsoleRenderer::RemoveFromRenderList( ConsoleGraphic * consoleGraphic )
{
	assert(mInitialized);
	mRenderList.remove(consoleGraphic);
}

int KEngineWindows::ConsoleRenderer::GetWidth() const {
	return mWidth;
}

int KEngineWindows::ConsoleRenderer::GetHeight() const {
	return mHeight;
}