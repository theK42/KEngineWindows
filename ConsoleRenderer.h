#pragma once

#include <list>
#include "Transform2D.h"
#include "Renderer2D.h"

namespace KEngineWindows
{	
	struct CharMap
	{
		//CharMap();
		//CharMap(int width, int height, char const * characters);
		int width;
		int height;
		char const * characters;
	};

	class ConsoleRenderer;

	class ConsoleGraphic
	{
	public:
		ConsoleGraphic();
		~ConsoleGraphic();
		void Init(ConsoleRenderer * renderer, CharMap const * characters, KEngine2D::Transform const * transform);
		void Deinit();
		CharMap const * GetCharMap() const;
		void SetCharMap(CharMap const * characters);
		KEngine2D::Transform const * GetTransform() const;

	protected:
		CharMap const *					mCharacters;
		KEngine2D::Transform const *	mTransform;
		ConsoleRenderer *				mRenderer;
	};

	class ConsoleRenderer : public KEngine2D::Renderer
	{
	public:
		ConsoleRenderer();
		virtual ~ConsoleRenderer();
		void Init(int width, int height);
		void Deinit();
		void Render() const;
		void AddToRenderList(ConsoleGraphic * consoleGraphic);
		void RemoveFromRenderList(ConsoleGraphic * consoleGraphic);
		int GetWidth() const;
		int GetHeight() const;
	protected:

		std::list<ConsoleGraphic *>	mRenderList;  
		int							mWidth;
		int							mHeight;
		bool						mInitialized;
		char *						mBuffer;

	};
}