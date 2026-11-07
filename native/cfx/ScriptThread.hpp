/*
 * This file is part of the Cfx project - https://cfx.re/
 *
 * See LICENSE in the root of the source tree for information
 * regarding licensing.
 */

// This file contains code adapted from the original GTA IV script hook, the
// copyright notice for which follows below.

/*****************************************************************************\

Copyright (C) 2009, Aru <oneforaru at gmail dot com>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

\*****************************************************************************/

// Adapted SDK declaration: only the exported CfxThread interface is retained.
// Source: citizenfx/fivem code/components/rage-scripting-five/include/scrThread.h
#pragma once
class GtaThread;
class __declspec(dllimport) CfxThread {
public:
 CfxThread();
 virtual ~CfxThread();
 virtual void Reset() {}
 virtual void DoRun()=0;
 virtual void Kill() {}
 void AttachScriptHandler();
 void DetachScriptHandler();
 GtaThread* GetThread(){return Thread;}
private:
 GtaThread* Thread;
};

