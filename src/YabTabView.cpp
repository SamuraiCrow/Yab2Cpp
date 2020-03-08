//------------------------------------------------------------------------------
//      Copyright (c) 2001-2005, Haiku
//
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the "Software"),
//      to deal in the Software without restriction, including without limitation
//      the rights to use, copy, modify, merge, publish, distribute, sublicense,
//      and/or sell copies of the Software, and to permit persons to whom the
//      Software is furnished to do so, subject to the following conditions:
//
//      The above copyright notice and this permission notice shall be included in
//      all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//      FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//      DEALINGS IN THE SOFTWARE.
//
//      File Name:              YabTabView.cpp
//      Author:                 Marc Flerackers (mflerackers@androme.be)
//                  Modified by Jan Bungeroth (jan@be-logos.org)
//                  Modified by Kacper Kasper (kacperkasper@gmail.com)
//      Description:  YabTabView provides the framework for containing and
//                  managing groups of BView objects. Modified for *sane*
//                  view handling (they stay connected to the window).
//------------------------------------------------------------------------------   

#include <List.h>
#include <Rect.h>  
#include <String.h>
#include <View.h>  
#include <stdio.h>
#include "YabTabView.h"


YabTabView::YabTabView(BRect frame, const char* name, button_width width, uint32 resizingMode, uint32 flags)
	: BTabView(frame, name, width, resizingMode, flags)
{
	fTabNames = new BList;

	FocusChanged = 1;
	OldTabView = 1;
}

YabTabView::~YabTabView()
{
	for(int i=0; i<CountTabs(); i++)
	{
		delete (BString*)fTabNames->RemoveItem(i);
	}

	delete fTabNames;
}

void YabTabView::AddTab(BView *tabView, const char* label)
{
	if(tabView)
	{
		BTab *tab = new BTab();
		BTabView::AddTab(tabView, tab);
		tab->SetLabel(label);
		// HACK
		// YAB assumes all views have a window, but that's not true for tabs
		if(CountTabs() > 1)
			AddChild(tabView);
		tabView->Hide();

		fTabNames->AddItem(new BString(label));
		
	}
}


void YabTabView::Select(int32 index)
{
	if (index < 0 || index >= CountTabs())
		index = Selection();

	BTab* tab = TabAt(index);
	if (tab) 
	{
		FocusChanged = index+1;
	}

	// HACK
	// YAB assumes all views have a window, but that's not true for tabs
	int32 prevSelected = Selection();
	RemoveChild(tab->View());
	tab->View()->Show();
	BTabView::Select(index);
	if(prevSelected > -1) {
		BTab* prevTab = TabAt(prevSelected);
		prevTab->View()->Hide();
		AddChild(prevTab->View());
	}

}

void YabTabView::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);

	SetFocusTab(Selection(), focused);
}

void YabTabView::SetFocusTab(int32 tab, bool focused)
{
	FocusChanged = (FocusTab() != tab) ? 1 : 0;
	
	BTabView::SetFocusTab(tab, focused);
}

void YabTabView::RemovingTab(int32 index)
{
	 BTabView::RemoveTab(index);
}
/*
void YabTabView::RemovingTab(int32 index, bool focused)
{
	int oldindex=index;
	int index_a;
	int tab;
	if (index < 0 || index >= CountTabs())
		return NULL;
		BTab* tab = (BTab*)fTabNames->RemoveItem(index);
		if (tab==NULL)
		return NULL;
		
		tab->Deselect();
		BTab::Private(tab).SetTabView(Null);
		if (fContainerView->GetLayout())
			fContainerView->GetLayout()->RemoveItem(index);
			if (CountTabs()==0)
				fFocus = -1;
			else if (index <= fSelection)
				Select (fSelection-1);
			if (fFocus >=0) {
				if(fFocus == CountTabs() -1 || CountTabs() == 0)
					BTabView::Select(f.Focus, false);
				else
					BTabView::Select(f.Focus, true);	
		}
		return tab;
		BTabView::RemoveTab(oldindex);
		BTabView::Select(1);
	
	
	BTab* tab = TabAt(index);
	if (tab) 
	{
		FocusChanged = index;
	}

	int32 prevSelected = 1; //Selection();
	RemoveChild(tab->View());
	tab->View()->Show();
	BTabView::Select(index);
	if(prevSelected > -1) {
		BTab* prevTab = TabAt(prevSelected);  //prevSelected);
		prevTab->View()->Hide();
		AddChild(prevTab->View());
	}
}	
	*/


const char* YabTabView::GetTabName(int32 index) const
{
	if(index < 0 || index >= CountTabs())
		return NULL; 

	return ((BString*)fTabNames->ItemAt(index))->String();
}
void YabTabView::PrintOut()
{
	//printf("\n %d",fTabNames->CountItems());
	if (fTabNames->CountItems()==0)
		{
		}
	else if(fTabNames->CountItems()>0)
	{
		printf("\n");
		for(int i=0; i<fTabNames->CountItems(); i++)
			printf("\t View %s and the id %d\n", ((BString*)(fTabNames->ItemAt(i)))->String() , fTabNames->ItemAt(i));
		printf("\n");
	}
}
void YabTabView::FindTabName(const char* tabname )
{
	BString test = NULL;
	//printf("\n %d",fTabNames->CountItems());
	if (fTabNames->CountItems()<=0)
		{
			//return NULL; 
		}
	else if(fTabNames->CountItems()>0)
	{
		//printf("%s \n", tabname);
		for(int i=0; i<fTabNames->CountItems(); i++)
		{
			printf("%s\n", ((BString*)(fTabNames->ItemAt(i)))->String());
			test=((BString*)(fTabNames->ItemAt(i)))->String();
			
			if (test == tabname)			
			{
				//printf("stimmt"); 
				printf("%s %d",test,i );
				printf("\n");
				//return tabname;
				//return true;
			}
		}
		//printf("\n");		
	}	
}

